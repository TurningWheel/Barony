#include "SmokeTestHooks.hpp"

#include "../game.hpp"
#include "../mod_tools.hpp"
#include "../net.hpp"
#include "../player.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>

namespace
{
	bool envHasValue(const char* key)
	{
		const char* raw = std::getenv(key);
		return raw && raw[0] != '\0';
	}

	std::string toLowerCopy(const char* value)
	{
		std::string result = value ? value : "";
		for ( auto& ch : result )
		{
			ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
		}
		return result;
	}

	bool parseEnvBool(const char* key, const bool fallback)
	{
		const char* raw = std::getenv(key);
		if ( !raw || !raw[0] )
		{
			return fallback;
		}
		const std::string value = toLowerCopy(raw);
		if ( value == "1" || value == "true" || value == "yes" || value == "on" )
		{
			return true;
		}
		if ( value == "0" || value == "false" || value == "no" || value == "off" )
		{
			return false;
		}
		return fallback;
	}

	int parseEnvInt(const char* key, const int fallback, const int minValue, const int maxValue)
	{
		const char* raw = std::getenv(key);
		if ( !raw || !raw[0] )
		{
			return fallback;
		}
		char* end = nullptr;
		const long parsed = std::strtol(raw, &end, 10);
		if ( end == raw || (end && *end != '\0') )
		{
			return fallback;
		}
		return std::max(minValue, std::min(maxValue, static_cast<int>(parsed)));
	}

	std::string parseEnvString(const char* key, const std::string& fallback)
	{
		const char* raw = std::getenv(key);
		if ( !raw || !raw[0] )
		{
			return fallback;
		}
		return std::string(raw);
	}

	enum class SmokeAutopilotRole : Uint8
	{
		DISABLED = 0,
		ROLE_HOST,
		ROLE_CLIENT
	};

	struct SmokeAutopilotConfig
	{
		bool enabled = false;
		SmokeAutopilotRole role = SmokeAutopilotRole::DISABLED;
		std::string connectAddress = "";
		int connectDelayTicks = 0;
		int retryDelayTicks = 0;
		int expectedPlayers = 2;
		bool autoStartLobby = false;
		int autoStartDelayTicks = 0;
		std::string seedString = "";
		bool autoReady = false;
	};

	struct SmokeAutopilotRuntime
	{
		bool initialized = false;
		SmokeAutopilotConfig config;
		Uint32 nextActionTick = 0;
		bool hostLaunchAttempted = false;
		bool joinAttempted = false;
		bool startIssued = false;
		Uint32 expectedPlayersMetTick = 0;
		bool seedApplied = false;
		bool readyIssued = false;
	};

	static SmokeAutopilotRuntime g_smokeAutopilot;

	SmokeAutopilotConfig& smokeAutopilotConfig()
	{
		if ( g_smokeAutopilot.initialized )
		{
			return g_smokeAutopilot.config;
		}
		g_smokeAutopilot.initialized = true;

		auto& cfg = g_smokeAutopilot.config;
		const std::string role = toLowerCopy(std::getenv("BARONY_SMOKE_ROLE"));
		if ( role == "host" )
		{
			cfg.role = SmokeAutopilotRole::ROLE_HOST;
		}
		else if ( role == "client" )
		{
			cfg.role = SmokeAutopilotRole::ROLE_CLIENT;
		}

		cfg.enabled = parseEnvBool("BARONY_SMOKE_AUTOPILOT", cfg.role != SmokeAutopilotRole::DISABLED);
		if ( !cfg.enabled || cfg.role == SmokeAutopilotRole::DISABLED )
		{
			cfg.enabled = false;
			return cfg;
		}

		char defaultAddress[64];
		const Uint16 serverPort = ::portnumber ? ::portnumber : DEFAULT_PORT;
		snprintf(defaultAddress, sizeof(defaultAddress), "127.0.0.1:%u", static_cast<unsigned>(serverPort));
		cfg.connectAddress = parseEnvString("BARONY_SMOKE_CONNECT_ADDRESS", defaultAddress);
		cfg.connectDelayTicks = parseEnvInt("BARONY_SMOKE_CONNECT_DELAY_SECS", 2, 0, 60) * TICKS_PER_SECOND;
		cfg.retryDelayTicks = parseEnvInt("BARONY_SMOKE_RETRY_DELAY_SECS", 3, 1, 120) * TICKS_PER_SECOND;
		cfg.expectedPlayers = parseEnvInt("BARONY_SMOKE_EXPECTED_PLAYERS", 2, 1, MAXPLAYERS);
		cfg.autoStartLobby = parseEnvBool("BARONY_SMOKE_AUTO_START", false);
		cfg.autoStartDelayTicks = parseEnvInt("BARONY_SMOKE_AUTO_START_DELAY_SECS", 2, 0, 120) * TICKS_PER_SECOND;
		cfg.seedString = parseEnvString("BARONY_SMOKE_SEED", "");
		cfg.autoReady = parseEnvBool("BARONY_SMOKE_AUTO_READY", false);
		g_smokeAutopilot.nextActionTick = ticks + static_cast<Uint32>(cfg.connectDelayTicks);

		const char* roleName = cfg.role == SmokeAutopilotRole::ROLE_HOST ? "host" : "client";
		printlog("[SMOKE]: enabled role=%s addr=%s expected=%d autoStart=%d",
			roleName, cfg.connectAddress.c_str(), cfg.expectedPlayers, cfg.autoStartLobby ? 1 : 0);

		return cfg;
	}

	int connectedLobbyPlayers()
	{
		int connected = 0;
		for ( int c = 0; c < MAXPLAYERS; ++c )
		{
			if ( !client_disconnected[c] )
			{
				++connected;
			}
		}
		return connected;
	}

	void applySmokeSeedIfNeeded()
	{
		auto& runtime = g_smokeAutopilot;
		auto& cfg = smokeAutopilotConfig();
		if ( runtime.seedApplied || cfg.seedString.empty() )
		{
			return;
		}
		gameModeManager.currentSession.seededRun.setup(cfg.seedString);
		runtime.seedApplied = true;
		printlog("[SMOKE]: applied seed '%s'", cfg.seedString.c_str());
	}

	struct SmokeAutoEnterDungeonState
	{
		bool initialized = false;
		bool enabled = false;
		int expectedPlayers = 2;
		Uint32 delayTicks = 0;
		Uint32 readySinceTick = 0;
		bool readyWindowStarted = false;
		int maxTransitions = 1;
		int transitionsIssued = 0;
	};

	static SmokeAutoEnterDungeonState g_smokeAutoEnterDungeon;

	SmokeAutoEnterDungeonState& smokeAutoEnterDungeonState()
	{
		auto& state = g_smokeAutoEnterDungeon;
		if ( state.initialized )
		{
			return state;
		}
		state.initialized = true;

		const bool smokeEnabled = parseEnvBool("BARONY_SMOKE_AUTOPILOT", false);
		const bool autoEnterDungeon = parseEnvBool("BARONY_SMOKE_AUTO_ENTER_DUNGEON", false);
		const std::string smokeRole = toLowerCopy(std::getenv("BARONY_SMOKE_ROLE"));
		const bool smokeHost = smokeRole == "host";

		if ( !smokeEnabled || !autoEnterDungeon || !smokeHost )
		{
			return state;
		}

		state.enabled = true;
		state.expectedPlayers = parseEnvInt("BARONY_SMOKE_EXPECTED_PLAYERS", 2, 1, MAXPLAYERS);
		const int delaySecs = parseEnvInt("BARONY_SMOKE_AUTO_ENTER_DUNGEON_DELAY_SECS", 3, 0, 120);
		state.delayTicks = static_cast<Uint32>(delaySecs * TICKS_PER_SECOND);
		state.maxTransitions = parseEnvInt("BARONY_SMOKE_AUTO_ENTER_DUNGEON_REPEATS", 1, 1, 256);
		printlog("[SMOKE]: gameplay auto-enter enabled expected=%d delay=%d sec repeats=%d",
			state.expectedPlayers, delaySecs, state.maxTransitions);
		return state;
	}

	int smokeConnectedPlayers()
	{
		int connected = 0;
		for ( int i = 0; i < MAXPLAYERS; ++i )
		{
			if ( !client_disconnected[i] )
			{
				++connected;
			}
		}
		return connected;
	}

	bool smokeConnectedPlayersLoaded()
	{
		for ( int i = 0; i < MAXPLAYERS; ++i )
		{
			if ( client_disconnected[i] )
			{
				continue;
			}
			if ( !players[i] || !players[i]->entity )
			{
				return false;
			}
		}
		return true;
	}
}

namespace SmokeTestHooks
{
namespace MainMenu
{
	bool isAutopilotEnvEnabled()
	{
		static const bool enabled = envHasValue("BARONY_SMOKE_AUTOPILOT")
			|| envHasValue("BARONY_SMOKE_ROLE");
		return enabled;
	}

	bool isHeloChunkPayloadOverrideEnvEnabled()
	{
		static const bool enabled = envHasValue("BARONY_SMOKE_HELO_CHUNK_PAYLOAD_MAX");
		return enabled;
	}

	bool isHeloChunkTxModeOverrideEnvEnabled()
	{
		static const bool enabled = envHasValue("BARONY_SMOKE_HELO_CHUNK_TX_MODE");
		return enabled;
	}

	bool isReadyStateSyncTraceEnabled()
	{
		static const bool enabled = parseEnvBool("BARONY_SMOKE_TRACE_READY_SYNC", false);
		return enabled;
	}

	void traceReadyStateSnapshotQueued(const int player, const int attempts, const Uint32 firstSendTick)
	{
		if ( !isReadyStateSyncTraceEnabled() )
		{
			return;
		}
		printlog("[SMOKE]: ready snapshot queued target=%d attempts=%d first_send_tick=%u",
			player, attempts, static_cast<unsigned>(firstSendTick));
	}

	void traceReadyStateSnapshotSent(const int player, const int readyEntries)
	{
		if ( !isReadyStateSyncTraceEnabled() )
		{
			return;
		}
		printlog("[SMOKE]: ready snapshot sent target=%d ready_entries=%d",
			player, readyEntries);
	}

	bool isAccountLabelTraceEnabled()
	{
		static const bool enabled = parseEnvBool("BARONY_SMOKE_TRACE_ACCOUNT_LABELS", false);
		return enabled;
	}

	void traceLobbyAccountLabelResolved(const int slot, const char* accountName)
	{
		if ( !isAccountLabelTraceEnabled() )
		{
			return;
		}
		if ( slot < 0 || slot >= MAXPLAYERS || !accountName || !accountName[0] )
		{
			return;
		}
		if ( accountName[0] == '.' && accountName[1] == '.' && accountName[2] == '.' )
		{
			return;
		}

		static bool loggedSlots[MAXPLAYERS] = {};
		if ( loggedSlots[slot] )
		{
			return;
		}
		loggedSlots[slot] = true;
		printlog("[SMOKE]: lobby account label resolved slot=%d account=\"%s\"",
			slot, accountName);
	}

	bool hasHeloChunkPayloadOverride()
	{
		static bool initialized = false;
		static bool hasOverride = false;
		if ( !initialized )
		{
			initialized = true;
			const char* raw = std::getenv("BARONY_SMOKE_HELO_CHUNK_PAYLOAD_MAX");
			hasOverride = raw && raw[0];
		}
		return hasOverride;
	}

	int heloChunkPayloadMaxOverride(const int defaultPayloadMax, const int minPayloadMax)
	{
		static bool initialized = false;
		static int value = 0;
		if ( !initialized )
		{
			initialized = true;
			value = parseEnvInt("BARONY_SMOKE_HELO_CHUNK_PAYLOAD_MAX",
				defaultPayloadMax, minPayloadMax, defaultPayloadMax);
			if ( value != defaultPayloadMax )
			{
				printlog("[SMOKE]: using HELO chunk payload max override: %d", value);
			}
		}
		return value > 0 ? value : defaultPayloadMax;
	}

	bool hasHeloChunkTxModeOverride()
	{
		static bool initialized = false;
		static bool hasOverride = false;
		if ( !initialized )
		{
			initialized = true;
			const char* raw = std::getenv("BARONY_SMOKE_HELO_CHUNK_TX_MODE");
			hasOverride = raw && raw[0];
		}
		return hasOverride;
	}

	const char* heloChunkTxModeName(const HeloChunkTxMode mode)
	{
		switch ( mode )
		{
			case HeloChunkTxMode::NORMAL: return "normal";
			case HeloChunkTxMode::REVERSE: return "reverse";
			case HeloChunkTxMode::EVEN_ODD: return "even-odd";
			case HeloChunkTxMode::DUPLICATE_FIRST: return "duplicate-first";
			case HeloChunkTxMode::DROP_LAST: return "drop-last";
			case HeloChunkTxMode::DUPLICATE_CONFLICT_FIRST: return "duplicate-conflict-first";
			default: return "normal";
		}
	}

	HeloChunkTxMode heloChunkTxMode()
	{
		static bool initialized = false;
		static HeloChunkTxMode mode = HeloChunkTxMode::NORMAL;
		if ( initialized )
		{
			return mode;
		}
		initialized = true;

		const std::string rawMode = toLowerCopy(std::getenv("BARONY_SMOKE_HELO_CHUNK_TX_MODE"));
		if ( rawMode == "reverse" )
		{
			mode = HeloChunkTxMode::REVERSE;
		}
		else if ( rawMode == "evenodd" || rawMode == "even-odd" || rawMode == "even_odd" )
		{
			mode = HeloChunkTxMode::EVEN_ODD;
		}
		else if ( rawMode == "duplicate-first" || rawMode == "duplicate_first" )
		{
			mode = HeloChunkTxMode::DUPLICATE_FIRST;
		}
		else if ( rawMode == "drop-last" || rawMode == "drop_last" )
		{
			mode = HeloChunkTxMode::DROP_LAST;
		}
		else if ( rawMode == "duplicate-conflict-first" || rawMode == "duplicate_conflict_first" )
		{
			mode = HeloChunkTxMode::DUPLICATE_CONFLICT_FIRST;
		}
		else
		{
			mode = HeloChunkTxMode::NORMAL;
		}

		if ( mode != HeloChunkTxMode::NORMAL )
		{
			printlog("[SMOKE]: using HELO chunk tx mode override: %s", heloChunkTxModeName(mode));
		}
		return mode;
	}

	void applyHeloChunkTxModePlan(std::vector<HeloChunkSendPlanEntry>& sendPlan, const int chunkCount, const Uint16 transferId)
	{
		if ( sendPlan.empty() || chunkCount <= 0 )
		{
			return;
		}
		if ( !(isHeloChunkTxModeOverrideEnvEnabled() && hasHeloChunkTxModeOverride()) )
		{
			return;
		}

		const HeloChunkTxMode txMode = heloChunkTxMode();
		switch ( txMode )
		{
			case HeloChunkTxMode::NORMAL:
				break;
			case HeloChunkTxMode::REVERSE:
				std::reverse(sendPlan.begin(), sendPlan.end());
				break;
			case HeloChunkTxMode::EVEN_ODD:
			{
				std::vector<HeloChunkSendPlanEntry> reordered;
				reordered.reserve(sendPlan.size());
				for ( int i = 1; i < chunkCount; i += 2 )
				{
					reordered.push_back(HeloChunkSendPlanEntry{ i, false });
				}
				for ( int i = 0; i < chunkCount; i += 2 )
				{
					reordered.push_back(HeloChunkSendPlanEntry{ i, false });
				}
				sendPlan = reordered;
				break;
			}
			case HeloChunkTxMode::DUPLICATE_FIRST:
				sendPlan.push_back(sendPlan.front());
				break;
			case HeloChunkTxMode::DROP_LAST:
				sendPlan.pop_back();
				break;
			case HeloChunkTxMode::DUPLICATE_CONFLICT_FIRST:
				sendPlan.insert(sendPlan.begin() + 1, HeloChunkSendPlanEntry{ sendPlan.front().chunkIndex, true });
				break;
		}

		if ( txMode != HeloChunkTxMode::NORMAL )
		{
			printlog("[SMOKE]: HELO chunk tx mode=%s transfer=%u packets=%u chunks=%u",
				heloChunkTxModeName(txMode),
				static_cast<unsigned>(transferId),
				static_cast<unsigned>(sendPlan.size()),
				static_cast<unsigned>(chunkCount));
		}
	}

	bool isAutopilotEnabled()
	{
		return smokeAutopilotConfig().enabled;
	}

	bool isAutopilotHostEnabled()
	{
		auto& cfg = smokeAutopilotConfig();
		return cfg.enabled && cfg.role == SmokeAutopilotRole::ROLE_HOST;
	}

	int expectedHostLobbyPlayerSlots(const int fallbackSlots)
	{
		auto& cfg = smokeAutopilotConfig();
		if ( !cfg.enabled || cfg.role != SmokeAutopilotRole::ROLE_HOST )
		{
			return fallbackSlots;
		}
		return std::max(1, std::min(MAXPLAYERS, cfg.expectedPlayers));
	}

	void tickAutopilot(const AutopilotCallbacks& callbacks)
	{
		auto& runtime = g_smokeAutopilot;
		auto& cfg = smokeAutopilotConfig();
		if ( !cfg.enabled )
		{
			return;
		}

		if ( cfg.role == SmokeAutopilotRole::ROLE_HOST )
		{
			if ( !runtime.hostLaunchAttempted )
			{
				runtime.hostLaunchAttempted = true;
				if ( !callbacks.hostLANLobbyNoSound || !callbacks.hostLANLobbyNoSound() )
				{
					printlog("[SMOKE]: host launch failed, smoke autopilot disabled");
					cfg.enabled = false;
				}
				return;
			}

			if ( multiplayer != SERVER )
			{
				return;
			}

			applySmokeSeedIfNeeded();
			if ( !cfg.autoStartLobby || runtime.startIssued )
			{
				return;
			}

			const int connected = connectedLobbyPlayers();
			if ( connected < cfg.expectedPlayers )
			{
				runtime.expectedPlayersMetTick = 0;
				return;
			}

			if ( runtime.expectedPlayersMetTick == 0 )
			{
				runtime.expectedPlayersMetTick = ticks;
				printlog("[SMOKE]: expected players reached (%d/%d), start in %d sec",
					connected, cfg.expectedPlayers, cfg.autoStartDelayTicks / TICKS_PER_SECOND);
			}
			if ( ticks - runtime.expectedPlayersMetTick < static_cast<Uint32>(cfg.autoStartDelayTicks) )
			{
				return;
			}
			if ( !callbacks.startGame )
			{
				printlog("[SMOKE]: start callback unavailable, smoke autopilot disabled");
				cfg.enabled = false;
				return;
			}

			runtime.startIssued = true;
			printlog("[SMOKE]: auto-starting game");
			callbacks.startGame();
			return;
		}

		// Client autopilot.
		if ( receivedclientnum )
		{
			if ( cfg.autoReady && !runtime.readyIssued && clientnum > 0 && clientnum < MAXPLAYERS )
			{
				if ( callbacks.createReadyStone )
				{
					runtime.readyIssued = true;
					printlog("[SMOKE]: auto-ready client %d", clientnum);
					callbacks.createReadyStone(clientnum, true, true);
				}
			}
			return;
		}
		if ( runtime.joinAttempted && multiplayer != CLIENT )
		{
			runtime.joinAttempted = false;
			runtime.nextActionTick = ticks + cfg.retryDelayTicks;
		}
		if ( runtime.joinAttempted || ticks < runtime.nextActionTick )
		{
			return;
		}

		if ( multiplayer == CLIENT )
		{
			// Connect attempt already in-flight.
			runtime.joinAttempted = true;
			return;
		}
		if ( !callbacks.connectToLanServer )
		{
			printlog("[SMOKE]: connect callback unavailable, smoke autopilot disabled");
			cfg.enabled = false;
			return;
		}

		if ( callbacks.connectToLanServer(cfg.connectAddress.c_str()) )
		{
			runtime.joinAttempted = true;
			printlog("[SMOKE]: join attempt sent to %s", cfg.connectAddress.c_str());
		}
		else
		{
			runtime.nextActionTick = ticks + cfg.retryDelayTicks;
			printlog("[SMOKE]: join attempt failed, retrying in %d sec", cfg.retryDelayTicks / TICKS_PER_SECOND);
		}
	}
}

namespace Gameplay
{
	bool isHooksEnvEnabled()
	{
		static const bool enabled = envHasValue("BARONY_SMOKE_AUTOPILOT")
			|| envHasValue("BARONY_SMOKE_ROLE")
			|| envHasValue("BARONY_SMOKE_AUTO_ENTER_DUNGEON");
		return enabled;
	}

	bool isAutoEnterDungeonEnabled()
	{
		return smokeAutoEnterDungeonState().enabled;
	}

	void tickAutoEnterDungeon()
	{
		auto& smoke = smokeAutoEnterDungeonState();
		if ( !smoke.enabled )
		{
			return;
		}
		if ( multiplayer != SERVER )
		{
			return;
		}
		if ( smoke.transitionsIssued >= smoke.maxTransitions )
		{
			return;
		}
		if ( loadnextlevel )
		{
			return;
		}

		const int connected = smokeConnectedPlayers();
		if ( connected < smoke.expectedPlayers || !smokeConnectedPlayersLoaded() )
		{
			smoke.readySinceTick = 0;
			smoke.readyWindowStarted = false;
			return;
		}

		if ( !smoke.readyWindowStarted )
		{
			smoke.readyWindowStarted = true;
			smoke.readySinceTick = ticks;
			printlog("[SMOKE]: expected players loaded in starting area (%d/%d), entering dungeon in %u sec",
				connected, smoke.expectedPlayers, static_cast<unsigned>(smoke.delayTicks / TICKS_PER_SECOND));
		}
		if ( ticks - smoke.readySinceTick < smoke.delayTicks )
		{
			return;
		}

		smoke.readySinceTick = 0;
		smoke.readyWindowStarted = false;
		++smoke.transitionsIssued;
		loadnextlevel = true;
		Compendium_t::Events_t::previousCurrentLevel = currentlevel;
		printlog("[SMOKE]: auto-entering dungeon transition %d/%d from level %d",
			smoke.transitionsIssued, smoke.maxTransitions, currentlevel);
	}
}

namespace Net
{
	bool isJoinRejectTraceEnabled()
	{
		static const bool enabled = parseEnvBool("BARONY_SMOKE_TRACE_JOIN_REJECTS", false);
		return enabled;
	}

	void traceLobbyJoinReject(const Uint32 result, const Uint8 requestedSlot, const bool lockedSlots[MAXPLAYERS], const bool disconnectedSlots[MAXPLAYERS])
	{
		if ( !isJoinRejectTraceEnabled() )
		{
			return;
		}

		int freeUnlocked = 0;
		int freeLocked = 0;
		int occupied = 0;
		int firstFree = -1;
		char slotStates[MAXPLAYERS + 1] = {};
		int statePos = 0;

		for ( int slot = 1; slot < MAXPLAYERS; ++slot )
		{
			const bool disconnected = disconnectedSlots ? disconnectedSlots[slot] : false;
			const bool locked = lockedSlots ? lockedSlots[slot] : false;
			char state = '?';
			if ( !disconnected )
			{
				state = 'O';
				++occupied;
			}
			else if ( locked )
			{
				state = 'L';
				++freeLocked;
			}
			else
			{
				state = 'F';
				++freeUnlocked;
				if ( firstFree < 0 )
				{
					firstFree = slot;
				}
			}
			if ( statePos < MAXPLAYERS )
			{
				slotStates[statePos++] = state;
			}
		}
		slotStates[statePos] = '\0';

		const bool anySlotRequest = requestedSlot == 0;
		const int requested = anySlotRequest ? -1 : static_cast<int>(requestedSlot);
		printlog("[SMOKE]: lobby join reject code=%u requested_slot=%d any_slot=%d free_unlocked=%d free_locked=%d occupied=%d first_free=%d states=%s",
			static_cast<unsigned>(result),
			requested,
			anySlotRequest ? 1 : 0,
			freeUnlocked,
			freeLocked,
			occupied,
			firstFree,
			slotStates);
	}
}

namespace Mapgen
{
	int connectedPlayersOverride()
	{
		static bool initialized = false;
		static int overridePlayers = 0;
		if ( initialized )
		{
			return overridePlayers;
		}
		initialized = true;

		if ( !envHasValue("BARONY_SMOKE_MAPGEN_CONNECTED_PLAYERS") )
		{
			return 0;
		}
		overridePlayers = parseEnvInt("BARONY_SMOKE_MAPGEN_CONNECTED_PLAYERS", 0, 1, MAXPLAYERS);
		if ( overridePlayers <= 0 )
		{
			printlog("[SMOKE]: ignoring invalid BARONY_SMOKE_MAPGEN_CONNECTED_PLAYERS");
			return 0;
		}
		printlog("[SMOKE]: mapgen connected-player override active: %d", overridePlayers);
		return overridePlayers;
	}
}
}
