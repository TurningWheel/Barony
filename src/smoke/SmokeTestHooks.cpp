#include "SmokeTestHooks.hpp"

#include "../game.hpp"
#include "../mod_tools.hpp"
#include "../net.hpp"
#include "../player.hpp"
#include "../scores.hpp"
#include "../status_effect_owner_encoding.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <sstream>

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
		int autoKickTargetSlot = 0;
		int autoKickDelayTicks = 0;
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
		bool autoKickIssued = false;
		Uint32 autoKickArmedTick = 0;
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
		cfg.autoKickTargetSlot = parseEnvInt("BARONY_SMOKE_AUTO_KICK_TARGET_SLOT", 0, 0, MAXPLAYERS - 1);
		cfg.autoKickDelayTicks = parseEnvInt("BARONY_SMOKE_AUTO_KICK_DELAY_SECS", 2, 0, 120) * TICKS_PER_SECOND;
		cfg.seedString = parseEnvString("BARONY_SMOKE_SEED", "");
		cfg.autoReady = parseEnvBool("BARONY_SMOKE_AUTO_READY", false);
		g_smokeAutopilot.nextActionTick = ticks + static_cast<Uint32>(cfg.connectDelayTicks);

		const char* roleName = cfg.role == SmokeAutopilotRole::ROLE_HOST ? "host" : "client";
		printlog("[SMOKE]: enabled role=%s addr=%s expected=%d autoStart=%d autoKickTarget=%d",
			roleName, cfg.connectAddress.c_str(), cfg.expectedPlayers, cfg.autoStartLobby ? 1 : 0, cfg.autoKickTargetSlot);

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

	void maybeAutoKickLobbyPlayer(const SmokeTestHooks::MainMenu::AutopilotCallbacks& callbacks,
		SmokeAutopilotConfig& cfg, SmokeAutopilotRuntime& runtime)
	{
		if ( cfg.role != SmokeAutopilotRole::ROLE_HOST )
		{
			return;
		}
		if ( cfg.autoKickTargetSlot <= 0 || cfg.autoKickTargetSlot >= MAXPLAYERS )
		{
			return;
		}
		if ( runtime.autoKickIssued )
		{
			return;
		}
		if ( !callbacks.kickPlayer )
		{
			runtime.autoKickIssued = true;
			printlog("[SMOKE]: auto-kick callback unavailable");
			return;
		}
		if ( cfg.autoKickTargetSlot >= cfg.expectedPlayers )
		{
			runtime.autoKickIssued = true;
			printlog("[SMOKE]: auto-kick target=%d out of expected player range (expected=%d)",
				cfg.autoKickTargetSlot, cfg.expectedPlayers);
			return;
		}

		const int connected = connectedLobbyPlayers();
		if ( connected < cfg.expectedPlayers )
		{
			runtime.autoKickArmedTick = 0;
			return;
		}

		if ( runtime.autoKickArmedTick == 0 )
		{
			runtime.autoKickArmedTick = ticks;
			printlog("[SMOKE]: auto-kick armed target=%d delay=%u sec",
				cfg.autoKickTargetSlot, static_cast<unsigned>(cfg.autoKickDelayTicks / TICKS_PER_SECOND));
		}
		if ( ticks - runtime.autoKickArmedTick < static_cast<Uint32>(cfg.autoKickDelayTicks) )
		{
			return;
		}

		const int targetSlot = cfg.autoKickTargetSlot;
		const int beforeConnected = connectedLobbyPlayers();
		if ( client_disconnected[targetSlot] )
		{
			runtime.autoKickIssued = true;
			printlog("[SMOKE]: auto-kick skipped target=%d reason=already_disconnected", targetSlot);
			return;
		}

		callbacks.kickPlayer(targetSlot);
		runtime.autoKickIssued = true;

		const int afterConnected = connectedLobbyPlayers();
		const int expectedAfter = std::max(1, cfg.expectedPlayers - 1);
		const bool targetDisconnected = client_disconnected[targetSlot];
		int unexpectedDisconnected = 0;
		for ( int slot = 1; slot < cfg.expectedPlayers; ++slot )
		{
			if ( slot == targetSlot )
			{
				continue;
			}
			if ( client_disconnected[slot] )
			{
				++unexpectedDisconnected;
			}
		}
		const bool statusOk = targetDisconnected
			&& afterConnected == expectedAfter
			&& unexpectedDisconnected == 0;
		printlog("[SMOKE]: auto-kick result target=%d before_connected=%d after_connected=%d expected_after=%d target_disconnected=%d unexpected_disconnected=%d status=%s",
			targetSlot,
			beforeConnected,
			afterConnected,
			expectedAfter,
			targetDisconnected ? 1 : 0,
			unexpectedDisconnected,
			statusOk ? "ok" : "fail");
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
		GameUI::flushStatusEffectQueueInitTrace();

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
			maybeAutoKickLobbyPlayer(callbacks, cfg, runtime);
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

namespace GameUI
{
	bool isStatusEffectQueueTraceEnabled()
	{
		static const bool enabled = parseEnvBool("BARONY_SMOKE_TRACE_STATUS_EFFECT_QUEUE", false);
		return enabled;
	}

	namespace
	{
		bool g_statusEffectQueueInitRecorded[MAXPLAYERS] = {};
		int g_statusEffectQueueInitOwners[MAXPLAYERS] = {};
		bool g_statusEffectQueueInitLoggedOk[MAXPLAYERS] = {};
		bool g_statusEffectQueueInitLoggedMismatch[MAXPLAYERS] = {};
	}

	void traceStatusEffectQueueLane(const char* lane, const int slot, const int owner,
		bool (&loggedOk)[MAXPLAYERS], bool (&loggedMismatch)[MAXPLAYERS])
	{
		if ( !isStatusEffectQueueTraceEnabled() )
		{
			return;
		}
		if ( slot < 0 || slot >= MAXPLAYERS )
		{
			return;
		}

		const bool ok = owner == slot;
		if ( ok )
		{
			if ( loggedOk[slot] )
			{
				return;
			}
			loggedOk[slot] = true;
			printlog("[SMOKE]: statusfx queue %s slot=%d owner=%d status=ok", lane, slot, owner);
			return;
		}

		if ( loggedMismatch[slot] )
		{
			return;
		}
		loggedMismatch[slot] = true;
		printlog("[SMOKE]: statusfx queue %s slot=%d owner=%d status=mismatch", lane, slot, owner);
	}

	void recordStatusEffectQueueInit(const int slot, const int owner)
	{
		if ( slot < 0 || slot >= MAXPLAYERS )
		{
			return;
		}
		g_statusEffectQueueInitRecorded[slot] = true;
		g_statusEffectQueueInitOwners[slot] = owner;
	}

	void flushStatusEffectQueueInitTrace()
	{
		if ( !isStatusEffectQueueTraceEnabled() )
		{
			return;
		}

		for ( int slot = 0; slot < MAXPLAYERS; ++slot )
		{
			if ( !g_statusEffectQueueInitRecorded[slot] )
			{
				continue;
			}
			if ( client_disconnected[slot] )
			{
				continue;
			}
			traceStatusEffectQueueLane("init", slot, g_statusEffectQueueInitOwners[slot],
				g_statusEffectQueueInitLoggedOk, g_statusEffectQueueInitLoggedMismatch);
		}
	}

	void traceStatusEffectQueueCreate(const int slot, const int owner)
	{
		recordStatusEffectQueueInit(slot, owner);
		flushStatusEffectQueueInitTrace();
		static bool loggedOk[MAXPLAYERS] = {};
		static bool loggedMismatch[MAXPLAYERS] = {};
		traceStatusEffectQueueLane("create", slot, owner, loggedOk, loggedMismatch);
	}

	void traceStatusEffectQueueUpdate(const int slot, const int owner)
	{
		static bool loggedOk[MAXPLAYERS] = {};
		static bool loggedMismatch[MAXPLAYERS] = {};
		traceStatusEffectQueueLane("update", slot, owner, loggedOk, loggedMismatch);
	}
}

namespace Net
{
	bool isForceHeloChunkEnabled()
	{
		static bool initialized = false;
		static bool enabled = false;
		if ( !initialized )
		{
			initialized = true;
			enabled = parseEnvBool("BARONY_SMOKE_FORCE_HELO_CHUNK", false);
			if ( enabled )
			{
				printlog("[SMOKE]: BARONY_SMOKE_FORCE_HELO_CHUNK is enabled");
			}
		}
		return enabled;
	}

	int heloChunkPayloadMaxOverride(const int defaultPayloadMax, const int minPayloadMax)
	{
		return MainMenu::heloChunkPayloadMaxOverride(defaultPayloadMax, minPayloadMax);
	}

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

namespace SaveReload
{
	namespace
	{
		enum class OwnerEncodingKind : Uint8
		{
			FAST_COMPAT = 0,
			PACKED_NIBBLE,
			FULL_BYTE
		};

		struct OwnerEncodingEffectSpec
		{
			int effect = 0;
			const char* name = "";
			OwnerEncodingKind encoding = OwnerEncodingKind::PACKED_NIBBLE;
			bool forceNonPlayerSentinel = false;
		};

		struct EffectExpectation
		{
			Uint8 rawValue = 0;
			int owner = -1;
			int timer = 0;
			int accretion = 0;
		};

		static const std::array<OwnerEncodingEffectSpec, 15> kOwnerEncodingEffects = {
			OwnerEncodingEffectSpec{EFF_FAST, "EFF_FAST", OwnerEncodingKind::FAST_COMPAT, false},
			OwnerEncodingEffectSpec{EFF_DIVINE_FIRE, "EFF_DIVINE_FIRE", OwnerEncodingKind::PACKED_NIBBLE, false},
			OwnerEncodingEffectSpec{EFF_SIGIL, "EFF_SIGIL", OwnerEncodingKind::PACKED_NIBBLE, false},
			OwnerEncodingEffectSpec{EFF_SANCTUARY, "EFF_SANCTUARY", OwnerEncodingKind::PACKED_NIBBLE, true},
			OwnerEncodingEffectSpec{EFF_NIMBLENESS, "EFF_NIMBLENESS", OwnerEncodingKind::PACKED_NIBBLE, false},
			OwnerEncodingEffectSpec{EFF_GREATER_MIGHT, "EFF_GREATER_MIGHT", OwnerEncodingKind::PACKED_NIBBLE, false},
			OwnerEncodingEffectSpec{EFF_COUNSEL, "EFF_COUNSEL", OwnerEncodingKind::PACKED_NIBBLE, false},
			OwnerEncodingEffectSpec{EFF_STURDINESS, "EFF_STURDINESS", OwnerEncodingKind::PACKED_NIBBLE, false},
			OwnerEncodingEffectSpec{EFF_MAXIMISE, "EFF_MAXIMISE", OwnerEncodingKind::PACKED_NIBBLE, false},
			OwnerEncodingEffectSpec{EFF_MINIMISE, "EFF_MINIMISE", OwnerEncodingKind::PACKED_NIBBLE, false},
			OwnerEncodingEffectSpec{EFF_CONFUSED, "EFF_CONFUSED", OwnerEncodingKind::FULL_BYTE, false},
			OwnerEncodingEffectSpec{EFF_TABOO, "EFF_TABOO", OwnerEncodingKind::FULL_BYTE, false},
			OwnerEncodingEffectSpec{EFF_PINPOINT, "EFF_PINPOINT", OwnerEncodingKind::FULL_BYTE, false},
			OwnerEncodingEffectSpec{EFF_PENANCE, "EFF_PENANCE", OwnerEncodingKind::FULL_BYTE, false},
			OwnerEncodingEffectSpec{EFF_CURSE_FLESH, "EFF_CURSE_FLESH", OwnerEncodingKind::FULL_BYTE, false}
		};

		std::string joinIntVector(const std::vector<int>& values)
		{
			std::ostringstream oss;
			for ( size_t i = 0; i < values.size(); ++i )
			{
				if ( i > 0 )
				{
					oss << ';';
				}
				oss << values[i];
			}
			return oss.str();
		}

		bool writeSaveInfoToSlot(const bool singleplayer, const int saveIndex, SaveGameInfo info)
		{
			char path[PATH_MAX] = "";
			const std::string savefile = setSaveGameFileName(singleplayer, SaveFileType::JSON, saveIndex);
			completePath(path, savefile.c_str(), outputdir);
			return FileHelper::writeObject(path, EFileFormat::Json_Compact, info);
		}

		void ensureEffectVectors(SaveGameInfo::Player::stat_t& statsData)
		{
			if ( static_cast<int>(statsData.EFFECTS.size()) < NUMEFFECTS )
			{
				statsData.EFFECTS.resize(NUMEFFECTS, 0);
			}
			if ( static_cast<int>(statsData.EFFECTS_TIMERS.size()) < NUMEFFECTS )
			{
				statsData.EFFECTS_TIMERS.resize(NUMEFFECTS, 0);
			}
			if ( static_cast<int>(statsData.EFFECTS_ACCRETION_TIME.size()) < NUMEFFECTS )
			{
				statsData.EFFECTS_ACCRETION_TIME.resize(NUMEFFECTS, 0);
			}
		}

		int decodeOwner(const OwnerEncodingEffectSpec& spec, const Uint8 value)
		{
			switch ( spec.encoding )
			{
				case OwnerEncodingKind::FAST_COMPAT:
					return StatusEffectOwnerEncoding::decodeFastCasterCompat(value);
				case OwnerEncodingKind::PACKED_NIBBLE:
					return StatusEffectOwnerEncoding::decodeOwnerNibbleToPlayer(value);
				case OwnerEncodingKind::FULL_BYTE:
				default:
					if ( value >= 1 && value <= MAXPLAYERS )
					{
						return static_cast<int>(value) - 1;
					}
					return -1;
			}
		}

		EffectExpectation buildEffectExpectation(const OwnerEncodingEffectSpec& spec, const int slot, const int connectedPlayers)
		{
			EffectExpectation expected;
			const int clampedPlayers = std::max(1, std::min(MAXPLAYERS, connectedPlayers));
			const int owner = std::max(0, std::min(clampedPlayers - 1, slot % clampedPlayers));
			const int strength = 1 + ((slot + spec.effect) % 5);
			expected.timer = 120 + slot * 11 + (spec.effect % 17);
			expected.accretion = 30 + slot * 5 + (spec.effect % 13);

			if ( spec.forceNonPlayerSentinel )
			{
				expected.owner = -1;
				expected.rawValue = static_cast<Uint8>(strength & StatusEffectOwnerEncoding::kStrengthNibbleMask);
				return expected;
			}

			expected.owner = owner;
			switch ( spec.encoding )
			{
				case OwnerEncodingKind::FAST_COMPAT:
					expected.rawValue = StatusEffectOwnerEncoding::encodeOwnerNibbleFromPlayer(owner);
					break;
				case OwnerEncodingKind::PACKED_NIBBLE:
					expected.rawValue = StatusEffectOwnerEncoding::packStrengthWithOwnerNibble(
						static_cast<Uint8>(strength), owner);
					break;
				case OwnerEncodingKind::FULL_BYTE:
				default:
					expected.rawValue = static_cast<Uint8>(owner + 1);
					break;
			}
			return expected;
		}

		void seedOwnerEncodingEffects(SaveGameInfo::Player::stat_t& statsData, const int slot, const int connectedPlayers)
		{
			ensureEffectVectors(statsData);
			for ( const auto& spec : kOwnerEncodingEffects )
			{
				const EffectExpectation expected = buildEffectExpectation(spec, slot, connectedPlayers);
				statsData.EFFECTS[spec.effect] = expected.rawValue;
				statsData.EFFECTS_TIMERS[spec.effect] = expected.timer;
				statsData.EFFECTS_ACCRETION_TIME[spec.effect] = expected.accretion;
			}
		}

		void simulateOneEffectTick(SaveGameInfo::Player::stat_t& statsData)
		{
			ensureEffectVectors(statsData);
			for ( const auto& spec : kOwnerEncodingEffects )
			{
				int& timer = statsData.EFFECTS_TIMERS[spec.effect];
				if ( timer > 0 )
				{
					--timer;
					if ( timer == 0 )
					{
						statsData.EFFECTS[spec.effect] = 0;
					}
				}
			}
		}

		bool validatePlayersConnected(
			const std::string& laneName,
			const std::vector<int>& expectedConnected,
			const std::vector<int>& actualConnected)
		{
			if ( expectedConnected == actualConnected )
			{
				return true;
			}
			printlog("[SMOKE]: save_reload_owner lane=%s phase=players_connected expected=\"%s\" actual=\"%s\" status=fail",
				laneName.c_str(),
				joinIntVector(expectedConnected).c_str(),
				joinIntVector(actualConnected).c_str());
			return false;
		}

		bool validatePlayerEffects(
			const std::string& laneName,
			const char* phase,
			const int slot,
			const SaveGameInfo::Player::stat_t& statsData,
			const int connectedPlayers,
			const bool expectTicked,
			int& checks)
		{
			if ( static_cast<int>(statsData.EFFECTS.size()) < NUMEFFECTS
				|| static_cast<int>(statsData.EFFECTS_TIMERS.size()) < NUMEFFECTS
				|| static_cast<int>(statsData.EFFECTS_ACCRETION_TIME.size()) < NUMEFFECTS )
			{
				printlog("[SMOKE]: save_reload_owner lane=%s phase=%s slot=%d field=vectors expected=numeffects actual=short status=fail",
					laneName.c_str(), phase, slot);
				return false;
			}

			bool ok = true;
			for ( const auto& spec : kOwnerEncodingEffects )
			{
				++checks;
				const EffectExpectation expected = buildEffectExpectation(spec, slot, connectedPlayers);
				int expectedTimer = expected.timer;
				int expectedRaw = expected.rawValue;
				int expectedOwner = expected.owner;
				if ( expectTicked )
				{
					if ( expectedTimer > 0 )
					{
						--expectedTimer;
					}
					if ( expectedTimer == 0 )
					{
						expectedRaw = 0;
						expectedOwner = -1;
					}
				}

				const int actualRaw = statsData.EFFECTS[spec.effect];
				const int actualTimer = statsData.EFFECTS_TIMERS[spec.effect];
				const int actualAccretion = statsData.EFFECTS_ACCRETION_TIME[spec.effect];
				const int actualOwner = decodeOwner(spec, static_cast<Uint8>(actualRaw));
				if ( actualRaw != expectedRaw )
				{
					printlog("[SMOKE]: save_reload_owner lane=%s phase=%s slot=%d effect=%s field=raw expected=%d actual=%d status=fail",
						laneName.c_str(), phase, slot, spec.name, expectedRaw, actualRaw);
					ok = false;
				}
				if ( actualTimer != expectedTimer )
				{
					printlog("[SMOKE]: save_reload_owner lane=%s phase=%s slot=%d effect=%s field=timer expected=%d actual=%d status=fail",
						laneName.c_str(), phase, slot, spec.name, expectedTimer, actualTimer);
					ok = false;
				}
				if ( actualAccretion != expected.accretion )
				{
					printlog("[SMOKE]: save_reload_owner lane=%s phase=%s slot=%d effect=%s field=accretion expected=%d actual=%d status=fail",
						laneName.c_str(), phase, slot, spec.name, expected.accretion, actualAccretion);
					ok = false;
				}
				if ( actualOwner != expectedOwner )
				{
					printlog("[SMOKE]: save_reload_owner lane=%s phase=%s slot=%d effect=%s field=owner expected=%d actual=%d status=fail",
						laneName.c_str(), phase, slot, spec.name, expectedOwner, actualOwner);
					ok = false;
				}
			}
			return ok;
		}

		bool runOwnerEncodingFixtureLane(
			const std::string& laneName,
			const SaveGameInfo& baseline,
			const bool singleplayer,
			const int saveIndex,
			const int playersCount,
			const std::vector<int>& fixturePlayersConnected,
			const std::vector<int>& expectedPlayersConnected,
			const int multiplayerTypeOverride,
			int& checks)
		{
			SaveGameInfo fixture = baseline;
			fixture.player_num = 0;
			fixture.players.resize(std::max(1, playersCount));
			if ( multiplayerTypeOverride >= 0 )
			{
				fixture.multiplayer_type = multiplayerTypeOverride;
			}
			fixture.players_connected = fixturePlayersConnected;
			for ( int slot = 0; slot < static_cast<int>(fixture.players.size()); ++slot )
			{
				seedOwnerEncodingEffects(fixture.players[slot].stats, slot, std::max(1, playersCount));
			}

			if ( !writeSaveInfoToSlot(singleplayer, saveIndex, fixture) )
			{
				printlog("[SMOKE]: save_reload_owner lane=%s phase=write_fixture status=fail",
					laneName.c_str());
				return false;
			}

			const SaveGameInfo loaded = getSaveGameInfo(singleplayer, saveIndex);
			if ( loaded.magic_cookie != "BARONYJSONSAVE" )
			{
				printlog("[SMOKE]: save_reload_owner lane=%s phase=reload_fixture status=fail",
					laneName.c_str());
				return false;
			}

			bool ok = true;
			if ( !validatePlayersConnected(laneName, expectedPlayersConnected, loaded.players_connected) )
			{
				ok = false;
			}

			const int connectedPlayers = std::max(1, playersCount);
			for ( int slot = 0; slot < static_cast<int>(expectedPlayersConnected.size()); ++slot )
			{
				if ( expectedPlayersConnected[slot] == 0 )
				{
					continue;
				}
				if ( slot >= static_cast<int>(loaded.players.size()) )
				{
					printlog("[SMOKE]: save_reload_owner lane=%s phase=post_load slot=%d field=player expected=present actual=missing status=fail",
						laneName.c_str(), slot);
					ok = false;
					continue;
				}

				const auto& postLoadStats = loaded.players[slot].stats;
				if ( !validatePlayerEffects(laneName, "post_load", slot, postLoadStats, connectedPlayers, false, checks) )
				{
					ok = false;
				}

				auto tickedStats = postLoadStats;
				simulateOneEffectTick(tickedStats);
				if ( !validatePlayerEffects(laneName, "post_tick", slot, tickedStats, connectedPlayers, true, checks) )
				{
					ok = false;
				}
			}

			printlog("[SMOKE]: save_reload_owner lane=%s players_connected=%d result=%s checks=%d",
				laneName.c_str(),
				playersCount,
				ok ? "pass" : "fail",
				checks);
			return ok;
		}
	}

	bool isOwnerEncodingSweepEnabled()
	{
		static const bool enabled = parseEnvBool("BARONY_SMOKE_SAVE_RELOAD_OWNER_SWEEP", false);
		return enabled;
	}

	bool runOwnerEncodingSweep(const bool singleplayer, const int saveIndex)
	{
		static bool initialized = false;
		static bool lastResult = true;
		if ( initialized )
		{
			return lastResult;
		}
		initialized = true;

		if ( !isOwnerEncodingSweepEnabled() )
		{
			lastResult = true;
			return lastResult;
		}

		SaveGameInfo baseline = getSaveGameInfo(singleplayer, saveIndex);
		if ( baseline.magic_cookie != "BARONYJSONSAVE" )
		{
			printlog("[SMOKE]: save_reload_owner sweep result=fail lanes=0 legacy=0 reason=missing-baseline");
			lastResult = false;
			return lastResult;
		}

		bool pass = true;
		int totalChecks = 0;
		const int laneCount = MAXPLAYERS;
		const int legacyCount = 3;

		for ( int playersConnected = 1; playersConnected <= MAXPLAYERS; ++playersConnected )
		{
			std::vector<int> expectedConnected(playersConnected, 1);
			const std::string laneName = "p" + std::to_string(playersConnected);
			if ( !runOwnerEncodingFixtureLane(
				laneName,
				baseline,
				singleplayer,
				saveIndex,
				playersConnected,
				expectedConnected,
				expectedConnected,
				-1,
				totalChecks) )
			{
				pass = false;
			}
		}

		const int legacyPlayers = std::min(MAXPLAYERS, 8);
		{
			std::vector<int> fixtureConnected;
			std::vector<int> expectedConnected(legacyPlayers, 1);
			if ( !runOwnerEncodingFixtureLane(
				"legacy-empty",
				baseline,
				singleplayer,
				saveIndex,
				legacyPlayers,
				fixtureConnected,
				expectedConnected,
				SERVER,
				totalChecks) )
			{
				pass = false;
			}
		}

		{
			std::vector<int> fixtureConnected = { 1, 0, 1 };
			std::vector<int> expectedConnected = fixtureConnected;
			expectedConnected.resize(legacyPlayers, fixtureConnected.back());
			if ( !runOwnerEncodingFixtureLane(
				"legacy-short",
				baseline,
				singleplayer,
				saveIndex,
				legacyPlayers,
				fixtureConnected,
				expectedConnected,
				SERVER,
				totalChecks) )
			{
				pass = false;
			}
		}

		{
			std::vector<int> fixtureConnected;
			for ( int i = 0; i < legacyPlayers + 2; ++i )
			{
				fixtureConnected.push_back(i % 2 == 0 ? 1 : 0);
			}
			std::vector<int> expectedConnected(fixtureConnected.begin(), fixtureConnected.begin() + legacyPlayers);
			if ( !runOwnerEncodingFixtureLane(
				"legacy-long",
				baseline,
				singleplayer,
				saveIndex,
				legacyPlayers,
				fixtureConnected,
				expectedConnected,
				SERVER,
				totalChecks) )
			{
				pass = false;
			}
		}

		if ( !writeSaveInfoToSlot(singleplayer, saveIndex, baseline) )
		{
			printlog("[SMOKE]: save_reload_owner phase=restore_baseline status=fail");
			pass = false;
		}

		printlog("[SMOKE]: save_reload_owner sweep result=%s lanes=%d legacy=%d checks=%d",
			pass ? "pass" : "fail",
			laneCount,
			legacyCount,
			totalChecks);

		lastResult = pass;
		return lastResult;
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
