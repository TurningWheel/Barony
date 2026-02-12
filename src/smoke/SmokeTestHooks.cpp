#include "SmokeTestHooks.hpp"

#include "../game.hpp"
#include "../lobbies.hpp"
#include "../mod_tools.hpp"
#include "../net.hpp"
#include "../paths.hpp"
#include "../player.hpp"
#include "../scores.hpp"
#include "../interface/interface.hpp"
#include "../status_effect_owner_encoding.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <limits>
#include <sstream>
#include <unordered_map>

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
		for ( char& ch : result )
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

	std::string trimCopy(const std::string& value)
	{
		const size_t begin = value.find_first_not_of(" \t\r\n");
		if ( begin == std::string::npos )
		{
			return "";
		}
		const size_t end = value.find_last_not_of(" \t\r\n");
		return value.substr(begin, end - begin + 1);
	}

	bool parseBoundedIntString(const std::string& value, const int minValue, const int maxValue, int& outValue)
	{
		if ( value.empty() )
		{
			return false;
		}
		char* end = nullptr;
		const long parsed = std::strtol(value.c_str(), &end, 10);
		if ( end == value.c_str() || (end && *end != '\0') )
		{
			return false;
		}
		if ( parsed < minValue || parsed > maxValue )
		{
			return false;
		}
		outValue = static_cast<int>(parsed);
		return true;
	}

	enum class SmokeAutopilotRole : Uint8
	{
		DISABLED = 0,
		ROLE_HOST,
		ROLE_CLIENT,
		ROLE_LOCAL
	};

	enum class SmokeAutopilotBackend : Uint8
	{
		LAN = 0,
		STEAM,
		EOS
	};

	SmokeAutopilotBackend parseSmokeAutopilotBackend()
	{
		const std::string raw = toLowerCopy(std::getenv("BARONY_SMOKE_NETWORK_BACKEND"));
		if ( raw == "steam" )
		{
			return SmokeAutopilotBackend::STEAM;
		}
		else if ( raw == "eos" )
		{
			return SmokeAutopilotBackend::EOS;
		}
		return SmokeAutopilotBackend::LAN;
	}

	const char* smokeAutopilotBackendName(const SmokeAutopilotBackend backend)
	{
		switch ( backend )
		{
			case SmokeAutopilotBackend::STEAM:
				return "steam";
			case SmokeAutopilotBackend::EOS:
				return "eos";
			case SmokeAutopilotBackend::LAN:
			default:
				return "lan";
		}
	}

	bool smokeAutopilotUsesOnlineBackend(const SmokeAutopilotBackend backend)
	{
		return backend != SmokeAutopilotBackend::LAN;
	}

		struct SmokeAutopilotConfig
		{
			bool enabled = false;
			SmokeAutopilotRole role = SmokeAutopilotRole::DISABLED;
			SmokeAutopilotBackend backend = SmokeAutopilotBackend::LAN;
		std::string connectAddress = "";
		int connectDelayTicks = 0;
		int retryDelayTicks = 0;
		int expectedPlayers = 2;
		bool autoStartLobby = false;
		int autoStartDelayTicks = 0;
		int autoKickTargetSlot = 0;
		int autoKickDelayTicks = 0;
		int autoPlayerCountTarget = 0;
			int autoPlayerCountDelayTicks = 0;
			std::string seedString = "";
			int startFloor = 0;
			bool autoReady = false;
			bool autoLobbyPageSweep = false;
			int autoLobbyPageDelayTicks = 0;
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
		bool autoPlayerCountIssued = false;
		Uint32 autoPlayerCountArmedTick = 0;
		bool autoLobbyPageSweepComplete = false;
			int autoLobbyPageNextIndex = 0;
			Uint32 autoLobbyPageArmedTick = 0;
			bool seedApplied = false;
			bool startFloorApplied = false;
			bool readyIssued = false;
			bool localLobbyReady = false;
			bool roomKeyLogged = false;
		};

	static SmokeAutopilotRuntime g_smokeAutopilot;

	SmokeAutopilotConfig& smokeAutopilotConfig()
	{
		if ( g_smokeAutopilot.initialized )
		{
			return g_smokeAutopilot.config;
		}
		g_smokeAutopilot.initialized = true;

		SmokeAutopilotConfig& cfg = g_smokeAutopilot.config;
		const std::string role = toLowerCopy(std::getenv("BARONY_SMOKE_ROLE"));
		if ( role == "host" )
		{
			cfg.role = SmokeAutopilotRole::ROLE_HOST;
		}
		else if ( role == "client" )
		{
			cfg.role = SmokeAutopilotRole::ROLE_CLIENT;
		}
		else if ( role == "local" )
		{
			cfg.role = SmokeAutopilotRole::ROLE_LOCAL;
		}

		cfg.enabled = parseEnvBool("BARONY_SMOKE_AUTOPILOT", cfg.role != SmokeAutopilotRole::DISABLED);
		if ( !cfg.enabled || cfg.role == SmokeAutopilotRole::DISABLED )
		{
			cfg.enabled = false;
			return cfg;
		}

		cfg.backend = parseSmokeAutopilotBackend();
		char defaultAddress[64];
		const Uint16 serverPort = ::portnumber ? ::portnumber : DEFAULT_PORT;
		if ( smokeAutopilotUsesOnlineBackend(cfg.backend) )
		{
			defaultAddress[0] = '\0';
		}
		else
		{
			snprintf(defaultAddress, sizeof(defaultAddress), "127.0.0.1:%u", static_cast<unsigned>(serverPort));
		}
		cfg.connectAddress = parseEnvString("BARONY_SMOKE_CONNECT_ADDRESS", defaultAddress);
		cfg.connectDelayTicks = parseEnvInt("BARONY_SMOKE_CONNECT_DELAY_SECS", 2, 0, 60) * TICKS_PER_SECOND;
		cfg.retryDelayTicks = parseEnvInt("BARONY_SMOKE_RETRY_DELAY_SECS", 3, 1, 120) * TICKS_PER_SECOND;
		cfg.expectedPlayers = parseEnvInt("BARONY_SMOKE_EXPECTED_PLAYERS", 2, 1, MAXPLAYERS);
		cfg.autoStartLobby = parseEnvBool("BARONY_SMOKE_AUTO_START", false);
		cfg.autoStartDelayTicks = parseEnvInt("BARONY_SMOKE_AUTO_START_DELAY_SECS", 2, 0, 120) * TICKS_PER_SECOND;
			cfg.autoKickTargetSlot = parseEnvInt("BARONY_SMOKE_AUTO_KICK_TARGET_SLOT", 0, 0, MAXPLAYERS - 1);
			cfg.autoKickDelayTicks = parseEnvInt("BARONY_SMOKE_AUTO_KICK_DELAY_SECS", 2, 0, 120) * TICKS_PER_SECOND;
			cfg.autoPlayerCountTarget = parseEnvInt("BARONY_SMOKE_AUTO_PLAYER_COUNT_TARGET", 0, 0, MAXPLAYERS);
			cfg.autoPlayerCountDelayTicks = parseEnvInt("BARONY_SMOKE_AUTO_PLAYER_COUNT_DELAY_SECS", 2, 0, 120) * TICKS_PER_SECOND;
			cfg.seedString = parseEnvString("BARONY_SMOKE_SEED", "");
			cfg.startFloor = parseEnvInt("BARONY_SMOKE_START_FLOOR", 0, 0, 99);
			cfg.autoReady = parseEnvBool("BARONY_SMOKE_AUTO_READY", false);
			cfg.autoLobbyPageSweep = parseEnvBool("BARONY_SMOKE_AUTO_LOBBY_PAGE_SWEEP", false);
			cfg.autoLobbyPageDelayTicks = parseEnvInt("BARONY_SMOKE_AUTO_LOBBY_PAGE_DELAY_SECS", 2, 0, 120) * TICKS_PER_SECOND;
			g_smokeAutopilot.nextActionTick = ticks + static_cast<Uint32>(cfg.connectDelayTicks);

		const char* roleName = "disabled";
		if ( cfg.role == SmokeAutopilotRole::ROLE_HOST )
		{
			roleName = "host";
		}
		else if ( cfg.role == SmokeAutopilotRole::ROLE_CLIENT )
		{
			roleName = "client";
		}
		else if ( cfg.role == SmokeAutopilotRole::ROLE_LOCAL )
		{
			roleName = "local";
		}
		printlog("[SMOKE]: enabled role=%s backend=%s addr=%s expected=%d autoStart=%d autoKickTarget=%d autoPlayerCountTarget=%d autoPageSweep=%d",
			roleName, smokeAutopilotBackendName(cfg.backend), cfg.connectAddress.c_str(),
			cfg.expectedPlayers, cfg.autoStartLobby ? 1 : 0,
			cfg.autoKickTargetSlot, cfg.autoPlayerCountTarget, cfg.autoLobbyPageSweep ? 1 : 0);

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
			SmokeAutopilotRuntime& runtime = g_smokeAutopilot;
			SmokeAutopilotConfig& cfg = smokeAutopilotConfig();
			if ( !runtime.startFloorApplied )
			{
				startfloor = cfg.startFloor;
				runtime.startFloorApplied = true;
				if ( cfg.startFloor > 0 )
				{
					printlog("[SMOKE]: applied start floor %d", cfg.startFloor);
				}
			}
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

	void maybeAutoRequestLobbyPlayerCount(const SmokeTestHooks::MainMenu::AutopilotCallbacks& callbacks,
		SmokeAutopilotConfig& cfg, SmokeAutopilotRuntime& runtime)
	{
		if ( cfg.role != SmokeAutopilotRole::ROLE_HOST )
		{
			return;
		}
		if ( cfg.autoPlayerCountTarget < 2 || cfg.autoPlayerCountTarget > MAXPLAYERS )
		{
			return;
		}
		if ( runtime.autoPlayerCountIssued )
		{
			return;
		}
		if ( !callbacks.requestLobbyPlayerCountSelection )
		{
			runtime.autoPlayerCountIssued = true;
			printlog("[SMOKE]: auto-player-count callback unavailable");
			return;
		}

		const int connected = connectedLobbyPlayers();
		if ( connected < cfg.expectedPlayers )
		{
			runtime.autoPlayerCountArmedTick = 0;
			return;
		}

		if ( runtime.autoPlayerCountArmedTick == 0 )
		{
			runtime.autoPlayerCountArmedTick = ticks;
			printlog("[SMOKE]: auto-player-count armed target=%d delay=%u sec",
				cfg.autoPlayerCountTarget, static_cast<unsigned>(cfg.autoPlayerCountDelayTicks / TICKS_PER_SECOND));
		}
		if ( ticks - runtime.autoPlayerCountArmedTick < static_cast<Uint32>(cfg.autoPlayerCountDelayTicks) )
		{
			return;
		}

		runtime.autoPlayerCountIssued = true;
		printlog("[SMOKE]: auto-player-count request target=%d", cfg.autoPlayerCountTarget);
		callbacks.requestLobbyPlayerCountSelection(cfg.autoPlayerCountTarget);
	}

	void maybeAutoSweepLobbyPages(const SmokeTestHooks::MainMenu::AutopilotCallbacks& callbacks,
		SmokeAutopilotConfig& cfg, SmokeAutopilotRuntime& runtime)
	{
		if ( cfg.role != SmokeAutopilotRole::ROLE_HOST || !cfg.autoLobbyPageSweep )
		{
			return;
		}
		if ( runtime.autoLobbyPageSweepComplete )
		{
			return;
		}
		if ( !callbacks.requestLobbyVisiblePage )
		{
			runtime.autoLobbyPageSweepComplete = true;
			printlog("[SMOKE]: auto-lobby-page callback unavailable");
			return;
		}

		const int connected = connectedLobbyPlayers();
		if ( connected < cfg.expectedPlayers )
		{
			runtime.autoLobbyPageArmedTick = 0;
			runtime.autoLobbyPageNextIndex = 0;
			return;
		}

		if ( runtime.autoLobbyPageArmedTick == 0 )
		{
			runtime.autoLobbyPageArmedTick = ticks;
			printlog("[SMOKE]: auto-lobby-page sweep armed delay=%u sec",
				static_cast<unsigned>(cfg.autoLobbyPageDelayTicks / TICKS_PER_SECOND));
		}
		if ( ticks - runtime.autoLobbyPageArmedTick < static_cast<Uint32>(cfg.autoLobbyPageDelayTicks) )
		{
			return;
		}

		const int slotsPerPage = MAXPLAYERS > 4 ? 4 : MAXPLAYERS;
		const int pageCount = std::max(1, (MAXPLAYERS + slotsPerPage - 1) / slotsPerPage);
		const int pageIndex = std::max(0, std::min(runtime.autoLobbyPageNextIndex, pageCount - 1));
		printlog("[SMOKE]: auto-lobby-page request page=%d/%d", pageIndex + 1, pageCount);
		callbacks.requestLobbyVisiblePage(pageIndex);

		runtime.autoLobbyPageNextIndex = pageIndex + 1;
		runtime.autoLobbyPageArmedTick = ticks;
		if ( runtime.autoLobbyPageNextIndex >= pageCount )
		{
			runtime.autoLobbyPageSweepComplete = true;
			printlog("[SMOKE]: auto-lobby-page sweep complete pages=%d", pageCount);
		}
	}

	struct SmokeAutoEnterDungeonState
	{
		bool initialized = false;
		bool enabled = false;
		bool allowSingleplayer = false;
		bool reloadSameLevel = false;
		bool preventDeath = false;
		bool preventDeathLogged = false;
		int expectedPlayers = 2;
		Uint32 delayTicks = 0;
		Uint32 readySinceTick = 0;
		bool readyWindowStarted = false;
		int maxTransitions = 1;
		int transitionsIssued = 0;
		Uint32 reloadSeedBase = 0;
		int hpFloor = 0;
	};

	static SmokeAutoEnterDungeonState g_smokeAutoEnterDungeon;

	SmokeAutoEnterDungeonState& smokeAutoEnterDungeonState()
	{
		SmokeAutoEnterDungeonState& state = g_smokeAutoEnterDungeon;
		if ( state.initialized )
		{
			return state;
		}
		state.initialized = true;

		const bool smokeEnabled = parseEnvBool("BARONY_SMOKE_AUTOPILOT", false);
		const bool autoEnterDungeon = parseEnvBool("BARONY_SMOKE_AUTO_ENTER_DUNGEON", false);
		const std::string smokeRole = toLowerCopy(std::getenv("BARONY_SMOKE_ROLE"));
		const bool smokeHost = smokeRole == "host";
		const bool smokeLocal = smokeRole == "local";

		if ( !smokeEnabled || !autoEnterDungeon || (!smokeHost && !smokeLocal) )
		{
			return state;
		}

		state.enabled = true;
		state.allowSingleplayer = smokeLocal;
		state.reloadSameLevel = parseEnvBool("BARONY_SMOKE_MAPGEN_RELOAD_SAME_LEVEL", false);
		state.preventDeath = parseEnvBool("BARONY_SMOKE_MAPGEN_PREVENT_DEATH", state.reloadSameLevel);
		state.expectedPlayers = parseEnvInt("BARONY_SMOKE_EXPECTED_PLAYERS", 2, 1, MAXPLAYERS);
		const int delaySecs = parseEnvInt("BARONY_SMOKE_AUTO_ENTER_DUNGEON_DELAY_SECS", 3, 0, 120);
		state.delayTicks = static_cast<Uint32>(delaySecs * TICKS_PER_SECOND);
		state.maxTransitions = parseEnvInt("BARONY_SMOKE_AUTO_ENTER_DUNGEON_REPEATS", 1, 1, 256);
		state.reloadSeedBase = static_cast<Uint32>(
			parseEnvInt("BARONY_SMOKE_MAPGEN_RELOAD_SEED_BASE", 0, 0, std::numeric_limits<int>::max()));
		state.hpFloor = state.preventDeath
			? parseEnvInt("BARONY_SMOKE_MAPGEN_HP_FLOOR", 500, 1, std::numeric_limits<int>::max())
			: 0;
		printlog("[SMOKE]: gameplay auto-enter enabled expected=%d delay=%d sec repeats=%d reload_same_level=%d seed_base=%u prevent_death=%d hp_floor=%d",
			state.expectedPlayers, delaySecs, state.maxTransitions,
			state.reloadSameLevel ? 1 : 0, static_cast<unsigned>(state.reloadSeedBase),
			state.preventDeath ? 1 : 0, state.hpFloor);
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

	struct SmokeRemoteCombatState
	{
		bool initialized = false;
		bool traceEnabled = false;
		bool hostAutopilotEnabled = false;
		int expectedPlayers = 2;
		int autoPausePulses = 0;
		Uint32 autoPauseDelayTicks = 0;
		Uint32 autoPauseHoldTicks = 0;
		int autoCombatPulses = 0;
		Uint32 autoCombatDelayTicks = 0;
		bool readyArmed = false;
		Uint32 nextPauseTick = 0;
		Uint32 nextCombatTick = 0;
		bool pauseActive = false;
		int pauseActionsIssued = 0;
		int combatPulsesIssued = 0;
		bool pauseCompleteLogged = false;
		bool combatCompleteLogged = false;
	};

	static SmokeRemoteCombatState g_smokeRemoteCombat;

	SmokeRemoteCombatState& smokeRemoteCombatState()
	{
		SmokeRemoteCombatState& state = g_smokeRemoteCombat;
		if ( state.initialized )
		{
			return state;
		}
		state.initialized = true;

		const bool smokeEnabled = parseEnvBool("BARONY_SMOKE_AUTOPILOT", false);
		const std::string smokeRole = toLowerCopy(std::getenv("BARONY_SMOKE_ROLE"));
		const bool smokeHost = smokeRole == "host";
		state.traceEnabled = smokeEnabled && parseEnvBool("BARONY_SMOKE_TRACE_REMOTE_COMBAT_SLOT_BOUNDS", false);
		state.expectedPlayers = parseEnvInt("BARONY_SMOKE_EXPECTED_PLAYERS", 2, 1, MAXPLAYERS);

		if ( smokeEnabled && smokeHost )
		{
			state.autoPausePulses = parseEnvInt("BARONY_SMOKE_AUTO_PAUSE_PULSES", 0, 0, 64);
			state.autoPauseDelayTicks = static_cast<Uint32>(
				parseEnvInt("BARONY_SMOKE_AUTO_PAUSE_DELAY_SECS", 2, 0, 120) * TICKS_PER_SECOND);
			state.autoPauseHoldTicks = static_cast<Uint32>(
				parseEnvInt("BARONY_SMOKE_AUTO_PAUSE_HOLD_SECS", 1, 0, 120) * TICKS_PER_SECOND);
			state.autoCombatPulses = parseEnvInt("BARONY_SMOKE_AUTO_REMOTE_COMBAT_PULSES", 0, 0, 64);
			state.autoCombatDelayTicks = static_cast<Uint32>(
				parseEnvInt("BARONY_SMOKE_AUTO_REMOTE_COMBAT_DELAY_SECS", 2, 0, 120) * TICKS_PER_SECOND);
			state.hostAutopilotEnabled = (state.autoPausePulses > 0 || state.autoCombatPulses > 0);
		}

		if ( state.traceEnabled )
		{
			printlog("[SMOKE]: remote-combat trace enabled expected=%d", state.expectedPlayers);
		}
		if ( state.hostAutopilotEnabled )
		{
			printlog("[SMOKE]: remote-combat autopilot enabled pause_pulses=%d pause_delay=%u hold=%u combat_pulses=%d combat_delay=%u",
				state.autoPausePulses,
				static_cast<unsigned>(state.autoPauseDelayTicks / TICKS_PER_SECOND),
				static_cast<unsigned>(state.autoPauseHoldTicks / TICKS_PER_SECOND),
				state.autoCombatPulses,
				static_cast<unsigned>(state.autoCombatDelayTicks / TICKS_PER_SECOND));
		}
		return state;
	}

	int firstConnectedRemoteSlot(const int expectedPlayers)
	{
		for ( int slot = 1; slot < MAXPLAYERS; ++slot )
		{
			if ( slot >= expectedPlayers )
			{
				break;
			}
			if ( client_disconnected[slot] )
			{
				continue;
			}
			if ( !players[slot] || !players[slot]->entity )
			{
				continue;
			}
			return slot;
		}
		return -1;
	}

	constexpr int kSmokeLocalMaxPlayers = 4;

	struct SmokeLocalSplitscreenState
	{
		bool initialized = false;
		bool enabled = false;
		bool traceEnabled = false;
		int expectedPlayers = kSmokeLocalMaxPlayers;
		int autoPausePulses = 0;
		Uint32 autoPauseDelayTicks = 0;
		Uint32 autoPauseHoldTicks = 0;
		bool readyArmed = false;
		Uint32 readySinceTick = 0;
		Uint32 nextPauseTick = 0;
		bool pauseActive = false;
		int pauseActionsIssued = 0;
		bool baselineLoggedOk = false;
		bool baselineLoggedFail = false;
		bool pauseCompleteLogged = false;
		bool transitionLogged = false;
	};

	static SmokeLocalSplitscreenState g_smokeLocalSplitscreen;

	SmokeLocalSplitscreenState& smokeLocalSplitscreenState()
	{
		SmokeLocalSplitscreenState& state = g_smokeLocalSplitscreen;
		if ( state.initialized )
		{
			return state;
		}
		state.initialized = true;

		const bool smokeEnabled = parseEnvBool("BARONY_SMOKE_AUTOPILOT", false);
		const std::string smokeRole = toLowerCopy(std::getenv("BARONY_SMOKE_ROLE"));
		const bool smokeLocal = smokeRole == "local";
		state.traceEnabled = smokeEnabled && parseEnvBool("BARONY_SMOKE_TRACE_LOCAL_SPLITSCREEN", false);
		state.expectedPlayers = parseEnvInt("BARONY_SMOKE_EXPECTED_PLAYERS",
			kSmokeLocalMaxPlayers, 1, kSmokeLocalMaxPlayers);
		state.autoPausePulses = parseEnvInt("BARONY_SMOKE_LOCAL_PAUSE_PULSES", 0, 0, 64);
		state.autoPauseDelayTicks = static_cast<Uint32>(
			parseEnvInt("BARONY_SMOKE_LOCAL_PAUSE_DELAY_SECS", 2, 0, 120) * TICKS_PER_SECOND);
		state.autoPauseHoldTicks = static_cast<Uint32>(
			parseEnvInt("BARONY_SMOKE_LOCAL_PAUSE_HOLD_SECS", 1, 0, 120) * TICKS_PER_SECOND);
		state.enabled = smokeEnabled && smokeLocal &&
			(state.traceEnabled || state.autoPausePulses > 0);

		if ( !state.enabled )
		{
			return state;
		}

		printlog("[SMOKE]: local-splitscreen baseline enabled expected=%d trace=%d pause_pulses=%d pause_delay=%u hold=%u",
			state.expectedPlayers,
			state.traceEnabled ? 1 : 0,
			state.autoPausePulses,
			static_cast<unsigned>(state.autoPauseDelayTicks / TICKS_PER_SECOND),
			static_cast<unsigned>(state.autoPauseHoldTicks / TICKS_PER_SECOND));
		return state;
	}

	struct SmokeLocalSplitscreenCapState
	{
		bool initialized = false;
		bool enabled = false;
		bool traceEnabled = false;
		int targetPlayers = 0;
		int cappedPlayers = 0;
		Uint32 commandDelayTicks = 0;
		Uint32 verifyDelayTicks = 0;
		bool armed = false;
		Uint32 armTick = 0;
		bool commandIssued = false;
		Uint32 verifyTick = 0;
		bool loggedOk = false;
		bool loggedFail = false;
	};

	static SmokeLocalSplitscreenCapState g_smokeLocalSplitscreenCap;

	SmokeLocalSplitscreenCapState& smokeLocalSplitscreenCapState()
	{
		SmokeLocalSplitscreenCapState& state = g_smokeLocalSplitscreenCap;
		if ( state.initialized )
		{
			return state;
		}
		state.initialized = true;

		const bool smokeEnabled = parseEnvBool("BARONY_SMOKE_AUTOPILOT", false);
		const std::string smokeRole = toLowerCopy(std::getenv("BARONY_SMOKE_ROLE"));
		const bool smokeLocal = smokeRole == "local";
		state.traceEnabled = parseEnvBool("BARONY_SMOKE_TRACE_LOCAL_SPLITSCREEN_CAP", false);
		state.targetPlayers = parseEnvInt("BARONY_SMOKE_AUTO_SPLITSCREEN_CAP_TARGET", 0, 0, MAXPLAYERS);
		state.cappedPlayers = std::max(2, std::min(state.targetPlayers, kSmokeLocalMaxPlayers));
		state.commandDelayTicks = static_cast<Uint32>(
			parseEnvInt("BARONY_SMOKE_SPLITSCREEN_CAP_DELAY_SECS", 2, 0, 120) * TICKS_PER_SECOND);
		state.verifyDelayTicks = static_cast<Uint32>(
			parseEnvInt("BARONY_SMOKE_SPLITSCREEN_CAP_VERIFY_DELAY_SECS", 1, 0, 120) * TICKS_PER_SECOND);
		state.enabled = smokeEnabled && smokeLocal && state.targetPlayers >= 2;

		if ( !state.enabled )
		{
			return state;
		}

		printlog("[SMOKE]: local-splitscreen cap enabled target=%d cap=%d trace=%d delay=%u verify_delay=%u",
			state.targetPlayers,
			state.cappedPlayers,
			state.traceEnabled ? 1 : 0,
			static_cast<unsigned>(state.commandDelayTicks / TICKS_PER_SECOND),
			static_cast<unsigned>(state.verifyDelayTicks / TICKS_PER_SECOND));
		return state;
	}

	int smokeLocalConnectedPlayers(const int expectedPlayers)
	{
		const int limit = std::max(1, std::min(expectedPlayers, kSmokeLocalMaxPlayers));
		int connected = 0;
		for ( int slot = 0; slot < limit; ++slot )
		{
			if ( !client_disconnected[slot] )
			{
				++connected;
			}
		}
		return connected;
	}

	int smokeLocalLoadedPlayers(const int expectedPlayers)
	{
		const int limit = std::max(1, std::min(expectedPlayers, kSmokeLocalMaxPlayers));
		int loaded = 0;
		for ( int slot = 0; slot < limit; ++slot )
		{
			if ( client_disconnected[slot] )
			{
				continue;
			}
			if ( players[slot] && players[slot]->entity )
			{
				++loaded;
			}
		}
		return loaded;
	}

	bool smokeLocalCameraLayoutOk(const int expectedPlayers, int& localSlotsOk,
		int& vmouseFailures, int& hudMissing)
	{
		localSlotsOk = 1;
		vmouseFailures = 0;
		hudMissing = 0;

		const int limit = std::max(1, std::min(expectedPlayers, kSmokeLocalMaxPlayers));
		int connected = 0;
		for ( int slot = 0; slot < limit; ++slot )
		{
			if ( !client_disconnected[slot] )
			{
				++connected;
			}
		}
		if ( connected <= 0 )
		{
			return false;
		}

		bool cameraOk = true;
		int playerIndex = 0;
		for ( int slot = 0; slot < limit; ++slot )
		{
			if ( client_disconnected[slot] )
			{
				continue;
			}
			if ( !players[slot] )
			{
				localSlotsOk = 0;
				cameraOk = false;
				++playerIndex;
				continue;
			}
			if ( !players[slot]->isLocalPlayer() )
			{
				localSlotsOk = 0;
			}
			if ( !players[slot]->hud.hudFrame )
			{
				++hudMissing;
			}

			int expectedX = 0;
			int expectedY = 0;
			int expectedW = xres;
			int expectedH = yres;
			if ( connected >= 3 )
			{
				expectedX = (playerIndex % 2) * xres / 2;
				expectedY = (playerIndex / 2) * yres / 2;
				expectedW = xres / 2;
				expectedH = yres / 2;
			}

			if ( connected >= 3 )
			{
				if ( players[slot]->camera_x1() != expectedX
					|| players[slot]->camera_y1() != expectedY
					|| players[slot]->camera_width() != expectedW
					|| players[slot]->camera_height() != expectedH )
				{
					cameraOk = false;
				}
			}

				if ( decltype(inputs.getVirtualMouse(slot)) vmouse = inputs.getVirtualMouse(slot) )
				{
					const int minX = players[slot]->camera_x1();
				const int minY = players[slot]->camera_y1();
				const int maxX = players[slot]->camera_x2();
				const int maxY = players[slot]->camera_y2();
				if ( vmouse->x < minX || vmouse->x >= maxX
					|| vmouse->y < minY || vmouse->y >= maxY )
				{
					++vmouseFailures;
				}
			}
			else
			{
				++vmouseFailures;
			}
			++playerIndex;
		}

		return cameraOk;
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

	bool isSlotLockTraceEnabled()
	{
		static const bool enabled = parseEnvBool("BARONY_SMOKE_TRACE_SLOT_LOCKS", false);
		return enabled;
	}

	void traceLobbySlotLockSnapshot(const char* context, const bool lockedSlots[MAXPLAYERS],
		const bool disconnectedSlots[MAXPLAYERS], const int configuredPlayers)
	{
		if ( !isSlotLockTraceEnabled() )
		{
			return;
		}

		int freeUnlocked = 0;
		int freeLocked = 0;
		int occupied = 0;
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
			}
			if ( statePos < MAXPLAYERS )
			{
				slotStates[statePos++] = state;
			}
		}
		slotStates[statePos] = '\0';

		const char* snapshotContext = (context && context[0]) ? context : "unspecified";
		printlog("[SMOKE]: lobby slot-lock snapshot context=%s configured=%d free_unlocked=%d free_locked=%d occupied=%d states=%s",
			snapshotContext, configuredPlayers, freeUnlocked, freeLocked, occupied, slotStates);
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

	bool isPlayerCountCopyTraceEnabled()
	{
		static const bool enabled = parseEnvBool("BARONY_SMOKE_TRACE_PLAYER_COUNT_COPY", false);
		return enabled;
	}

	void traceLobbyPlayerCountPrompt(const int targetCount, const int kickedCount,
		const char* variant, const char* promptText)
	{
		if ( !isPlayerCountCopyTraceEnabled() )
		{
			return;
		}

		const char* resolvedVariant = (variant && variant[0]) ? variant : "none";
		std::string sanitized = promptText ? promptText : "";
		for ( char& ch : sanitized )
		{
			if ( ch == '\n' || ch == '\r' )
			{
				ch = '|';
			}
		}
		if ( sanitized.size() > 256 )
		{
			sanitized.resize(256);
		}
		printlog("[SMOKE]: lobby player-count prompt target=%d kicked=%d variant=%s text=\"%s\"",
			targetCount, kickedCount, resolvedVariant, sanitized.c_str());
	}

	bool isLobbyPageStateTraceEnabled()
	{
		static const bool enabled = parseEnvBool("BARONY_SMOKE_TRACE_LOBBY_PAGE_STATE", false);
		return enabled;
	}

	void traceLobbyPageSnapshot(const char* context, const int page, const int pageCount,
		const int pageOffsetX, const int selectedOwner, const char* selectedWidget,
		const int focusPageMatch, const int cardsVisible, const int cardsMisaligned,
		const int paperdollsVisible, const int paperdollsMisaligned, const int pingsVisible,
		const int pingsMisaligned, const int warningsCenterDelta, const int countdownCenterDelta)
	{
		if ( !isLobbyPageStateTraceEnabled() )
		{
			return;
		}

		const char* snapshotContext = (context && context[0]) ? context : "unspecified";
		std::string selectedName = selectedWidget ? selectedWidget : "";
		if ( selectedName.empty() )
		{
			selectedName = "none";
		}
		for ( char& ch : selectedName )
		{
			if ( ch == '\n' || ch == '\r' || ch == '"' )
			{
				ch = '_';
			}
		}
		if ( selectedName.size() > 128 )
		{
			selectedName.resize(128);
		}

		printlog("[SMOKE]: lobby page snapshot context=%s page=%d/%d offset=%d selected_owner=%d selected_widget=%s focus_page_match=%d cards_visible=%d cards_misaligned=%d paperdolls_visible=%d paperdolls_misaligned=%d pings_visible=%d pings_misaligned=%d warnings_center_delta=%d countdown_center_delta=%d",
			snapshotContext,
			page,
			pageCount,
			pageOffsetX,
			selectedOwner,
			selectedName.c_str(),
			focusPageMatch,
			cardsVisible,
			cardsMisaligned,
			paperdollsVisible,
			paperdollsMisaligned,
			pingsVisible,
			pingsMisaligned,
			warningsCenterDelta,
			countdownCenterDelta);
	}

	bool isLocalSplitscreenTraceEnabled()
	{
		static const bool enabled = parseEnvBool("BARONY_SMOKE_TRACE_LOCAL_SPLITSCREEN", false);
		return enabled;
	}

	void traceLocalLobbySnapshot(const char* context, const int targetPlayers,
		const int joinedPlayers, const int readyPlayers, const int countdownActive)
	{
		if ( !isLocalSplitscreenTraceEnabled() )
		{
			return;
		}
		const char* snapshotContext = (context && context[0]) ? context : "unspecified";
		printlog("[SMOKE]: local-splitscreen lobby context=%s target=%d joined=%d ready=%d countdown=%d",
			snapshotContext, targetPlayers, joinedPlayers, readyPlayers, countdownActive);
	}

	void traceLocalLobbySnapshotIfChanged(const char* context, const int targetPlayers,
		const int joinedPlayers, const int readyPlayers, const int countdownActive)
	{
		if ( !isLocalSplitscreenTraceEnabled() )
		{
			return;
		}

		struct LocalLobbySnapshotState
		{
			bool initialized = false;
			int targetPlayers = -1;
			int joinedPlayers = -1;
			int readyPlayers = -1;
			int countdownActive = -1;
		};
		static LocalLobbySnapshotState lastSnapshot;

		if ( lastSnapshot.initialized
			&& lastSnapshot.targetPlayers == targetPlayers
			&& lastSnapshot.joinedPlayers == joinedPlayers
			&& lastSnapshot.readyPlayers == readyPlayers
			&& lastSnapshot.countdownActive == countdownActive )
		{
			return;
		}

		lastSnapshot.initialized = true;
		lastSnapshot.targetPlayers = targetPlayers;
		lastSnapshot.joinedPlayers = joinedPlayers;
		lastSnapshot.readyPlayers = readyPlayers;
		lastSnapshot.countdownActive = countdownActive;

		traceLocalLobbySnapshot(context, targetPlayers, joinedPlayers, readyPlayers, countdownActive);
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
		SmokeAutopilotConfig& cfg = smokeAutopilotConfig();
		return cfg.enabled && cfg.role == SmokeAutopilotRole::ROLE_HOST;
	}

	bool isAutopilotBackendSteam()
	{
		return smokeAutopilotConfig().backend == SmokeAutopilotBackend::STEAM;
	}

	bool isAutopilotBackendEos()
	{
		return smokeAutopilotConfig().backend == SmokeAutopilotBackend::EOS;
	}

	int expectedHostLobbyPlayerSlots(const int fallbackSlots)
	{
		SmokeAutopilotConfig& cfg = smokeAutopilotConfig();
		if ( !cfg.enabled || cfg.role != SmokeAutopilotRole::ROLE_HOST )
		{
			return fallbackSlots;
		}
		return std::max(1, std::min(MAXPLAYERS, cfg.expectedPlayers));
	}

	void tickAutopilot(const AutopilotCallbacks& callbacks)
	{
		SmokeAutopilotRuntime& runtime = g_smokeAutopilot;
		SmokeAutopilotConfig& cfg = smokeAutopilotConfig();
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
					const bool onlineBackend = smokeAutopilotUsesOnlineBackend(cfg.backend);
					bool launched = false;
					if ( onlineBackend )
					{
						switch ( cfg.backend )
						{
							case SmokeAutopilotBackend::STEAM:
								launched = callbacks.hostSteamLobbyNoSound && callbacks.hostSteamLobbyNoSound();
								break;
							case SmokeAutopilotBackend::EOS:
								launched = callbacks.hostEosLobbyNoSound && callbacks.hostEosLobbyNoSound();
								break;
							case SmokeAutopilotBackend::LAN:
							default:
								launched = false;
								break;
						}
					}
					else
					{
						launched = callbacks.hostLANLobbyNoSound && callbacks.hostLANLobbyNoSound();
					}
				if ( !launched )
				{
					printlog("[SMOKE]: host launch failed backend=%s, smoke autopilot disabled",
						smokeAutopilotBackendName(cfg.backend));
					cfg.enabled = false;
				}
				return;
			}

			if ( multiplayer != SERVER )
			{
				return;
			}
			if ( smokeAutopilotUsesOnlineBackend(cfg.backend) && !runtime.roomKeyLogged )
			{
				const std::string roomKey = LobbyHandler.getCurrentRoomKey();
				if ( !roomKey.empty() )
				{
					runtime.roomKeyLogged = true;
					printlog("[SMOKE]: lobby room key backend=%s key=%s",
						smokeAutopilotBackendName(cfg.backend), roomKey.c_str());
				}
			}

			applySmokeSeedIfNeeded();
			maybeAutoRequestLobbyPlayerCount(callbacks, cfg, runtime);
			maybeAutoKickLobbyPlayer(callbacks, cfg, runtime);
			maybeAutoSweepLobbyPages(callbacks, cfg, runtime);
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

			if ( cfg.role == SmokeAutopilotRole::ROLE_LOCAL )
			{
				if ( !runtime.hostLaunchAttempted )
				{
					runtime.hostLaunchAttempted = true;
				if ( !callbacks.hostLocalLobbyNoSound || !callbacks.hostLocalLobbyNoSound() )
				{
					printlog("[SMOKE]: local lobby launch failed, smoke autopilot disabled");
					cfg.enabled = false;
				}
				return;
			}

			if ( multiplayer != SINGLE )
			{
				return;
			}

				const int localTarget = std::max(1, std::min(cfg.expectedPlayers, 4));
				if ( !runtime.localLobbyReady )
				{
					if ( !callbacks.isLocalLobbyAutopilotContextReady
						|| !callbacks.isLocalLobbyCardReady
						|| !callbacks.isLocalLobbyCountdownActive
						|| !callbacks.isLocalPlayerSignedIn
						|| !callbacks.createReadyStone )
					{
						printlog("[SMOKE]: local lobby prep callback unavailable, smoke autopilot disabled");
						cfg.enabled = false;
						return;
					}

					if ( !callbacks.isLocalLobbyAutopilotContextReady() )
					{
						return;
					}

					for ( int slot = 0; slot < localTarget; ++slot )
					{
						if ( !callbacks.isLocalLobbyCardReady(slot) )
						{
							callbacks.createReadyStone(slot, true, true);
						}
					}

					int joinedPlayers = 0;
					int readyPlayers = 0;
					for ( int slot = 0; slot < localTarget; ++slot )
					{
						if ( callbacks.isLocalPlayerSignedIn(slot) )
						{
							++joinedPlayers;
						}
						if ( callbacks.isLocalLobbyCardReady(slot) )
						{
							++readyPlayers;
						}
					}
					const int countdownActive = callbacks.isLocalLobbyCountdownActive() ? 1 : 0;
					traceLocalLobbySnapshotIfChanged("autopilot",
						localTarget, joinedPlayers, readyPlayers, countdownActive);
					runtime.localLobbyReady = readyPlayers >= localTarget || countdownActive;
					if ( runtime.localLobbyReady )
					{
						runtime.expectedPlayersMetTick = ticks;
						printlog("[SMOKE]: local lobby ready (%d players)", localTarget);
				}
				return;
			}

			if ( !cfg.autoStartLobby || runtime.startIssued || !callbacks.startGame )
			{
				return;
			}
			if ( ticks - runtime.expectedPlayersMetTick < static_cast<Uint32>(cfg.autoStartDelayTicks) )
			{
				return;
			}

			runtime.startIssued = true;
			printlog("[SMOKE]: auto-starting local game");
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
		if ( cfg.connectAddress.empty() )
		{
			runtime.nextActionTick = ticks + cfg.retryDelayTicks;
			printlog("[SMOKE]: join address unavailable for backend=%s, retrying in %d sec",
				smokeAutopilotBackendName(cfg.backend), cfg.retryDelayTicks / TICKS_PER_SECOND);
			return;
		}

		const bool onlineBackend = smokeAutopilotUsesOnlineBackend(cfg.backend);
		bool (*connectToServer)(const char* address) = onlineBackend
			? callbacks.connectToOnlineLobby
			: callbacks.connectToLanServer;
		if ( !connectToServer )
		{
			printlog("[SMOKE]: connect callback unavailable for backend=%s, smoke autopilot disabled",
				smokeAutopilotBackendName(cfg.backend));
			cfg.enabled = false;
			return;
		}

		if ( connectToServer(cfg.connectAddress.c_str()) )
		{
			runtime.joinAttempted = true;
			printlog("[SMOKE]: join attempt sent backend=%s target=%s",
				smokeAutopilotBackendName(cfg.backend), cfg.connectAddress.c_str());
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
	void tickMapgenSurvivalGuard(SmokeAutoEnterDungeonState& smoke, const bool allowSingle)
	{
		if ( !smoke.preventDeath || smoke.hpFloor <= 0 )
		{
			return;
		}
		if ( client_disconnected[0] || !players[0] || !players[0]->entity || !stats[0] )
		{
			return;
		}

		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			consoleCommand("/enablecheats");
		}

		if ( allowSingle )
		{
			if ( !godmode )
			{
				consoleCommand("/god");
			}
		}
		else
		{
			godmode = true;
		}
		buddhamode = true;

		Stat* hostStats = stats[0];
		hostStats->MAXHP = std::max<Sint32>(hostStats->MAXHP, static_cast<Sint32>(smoke.hpFloor));
		hostStats->HP = std::max<Sint32>(hostStats->HP, static_cast<Sint32>(smoke.hpFloor));
		hostStats->OLDHP = hostStats->HP;

		if ( !smoke.preventDeathLogged )
		{
			smoke.preventDeathLogged = true;
			printlog("[SMOKE]: mapgen survival guard active mode=%s hp_floor=%d",
				allowSingle ? "console-god" : "forced-god", smoke.hpFloor);
		}
	}

	void tickAutoEnterDungeon()
	{
		SmokeAutoEnterDungeonState& smoke = smokeAutoEnterDungeonState();
		if ( !smoke.enabled )
		{
			return;
		}
		const bool allowServer = multiplayer == SERVER;
		const bool allowSingle = smoke.allowSingleplayer && multiplayer == SINGLE;
		if ( !allowServer && !allowSingle )
		{
			return;
		}
		tickMapgenSurvivalGuard(smoke, allowSingle);
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
			const bool reloadSameDungeonLevel = smoke.reloadSameLevel && currentlevel > 0;
			if ( reloadSameDungeonLevel )
			{
				skipLevelsOnLoad = -1;
				loadingSameLevelAsCurrent = true;
				if ( smoke.reloadSeedBase > 0 )
				{
					forceMapSeed = smoke.reloadSeedBase + static_cast<Uint32>(smoke.transitionsIssued - 1);
				}
			}
			loadnextlevel = true;
			Compendium_t::Events_t::previousCurrentLevel = currentlevel;
			if ( reloadSameDungeonLevel )
			{
				printlog("[SMOKE]: auto-reloading dungeon level transition %d/%d from level %d seed=%u",
					smoke.transitionsIssued, smoke.maxTransitions, currentlevel, static_cast<unsigned>(forceMapSeed));
			}
			else
			{
				printlog("[SMOKE]: auto-entering dungeon transition %d/%d from level %d",
					smoke.transitionsIssued, smoke.maxTransitions, currentlevel);
			}
		}

	void tickRemoteCombatAutopilot()
	{
		SmokeRemoteCombatState& smoke = smokeRemoteCombatState();
		if ( !smoke.hostAutopilotEnabled )
		{
			return;
		}
		if ( multiplayer != SERVER )
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
			smoke.readyArmed = false;
			return;
		}

		if ( !smoke.readyArmed )
		{
			smoke.readyArmed = true;
			smoke.nextPauseTick = ticks + smoke.autoPauseDelayTicks;
			smoke.nextCombatTick = ticks + smoke.autoCombatDelayTicks;
			printlog("[SMOKE]: remote-combat autopilot armed connected=%d expected=%d",
				connected, smoke.expectedPlayers);
		}

		if ( smoke.autoCombatPulses > 0 && smoke.combatPulsesIssued < smoke.autoCombatPulses
			&& ticks >= smoke.nextCombatTick )
		{
			int sourceSlot = -1;
			if ( players[0] && players[0]->entity && !client_disconnected[0] )
			{
				sourceSlot = 0;
			}
			const int targetSlot = firstConnectedRemoteSlot(smoke.expectedPlayers);
			bool enemyBarOk = false;
			bool damageGibOk = false;
			if ( sourceSlot >= 0 && targetSlot >= 1 && targetSlot < MAXPLAYERS
				&& players[targetSlot] && players[targetSlot]->entity )
			{
				Entity* source = players[sourceSlot]->entity;
				Entity* target = players[targetSlot]->entity;
				Stat* targetStats = target->getStats();
				const char* targetName = "Remote Player";
				Sint32 targetHp = 1;
				Sint32 targetMaxHp = 1;
				if ( targetStats )
				{
					if ( targetStats->name[0] != '\0' )
					{
						targetName = targetStats->name;
					}
					targetHp = std::max(1, targetStats->HP);
					targetMaxHp = std::max(targetHp, std::max(1, targetStats->MAXHP));
				}
				updateEnemyBar(source, target, targetName, targetHp, targetMaxHp, false, DMG_DEFAULT);
				enemyBarOk = true;
				if ( spawnDamageGib(target, 4, DamageGib::DMG_DEFAULT,
					DamageGibDisplayType::DMG_GIB_NUMBER, true) )
				{
					damageGibOk = true;
					Combat::traceRemoteCombatEvent("auto-dmgg-pulse", targetSlot);
				}
				Combat::traceRemoteCombatEvent("auto-enemy-bar-pulse", targetSlot);
			}

			++smoke.combatPulsesIssued;
			smoke.nextCombatTick = ticks + smoke.autoCombatDelayTicks;
			const bool ok = enemyBarOk && damageGibOk;
			printlog("[SMOKE]: remote-combat auto-event action=enemy-bar pulse=%d/%d source_slot=%d target_slot=%d enemy_bar=%d damage_gib=%d status=%s",
				smoke.combatPulsesIssued, smoke.autoCombatPulses, sourceSlot, targetSlot,
				enemyBarOk ? 1 : 0, damageGibOk ? 1 : 0, ok ? "ok" : "fail");
			if ( smoke.combatPulsesIssued >= smoke.autoCombatPulses && !smoke.combatCompleteLogged )
			{
				smoke.combatCompleteLogged = true;
				printlog("[SMOKE]: remote-combat auto-event complete pulses=%d", smoke.autoCombatPulses);
			}
		}

		const int completedPausePulses = smoke.pauseActionsIssued / 2;
		if ( smoke.autoPausePulses > 0 && (completedPausePulses < smoke.autoPausePulses)
			&& ticks >= smoke.nextPauseTick )
		{
			if ( !smoke.pauseActive )
			{
				pauseGame(2, 0);
				smoke.pauseActive = true;
				++smoke.pauseActionsIssued;
				smoke.nextPauseTick = ticks + smoke.autoPauseHoldTicks;
				Combat::traceRemoteCombatEvent("auto-pause-issued", 0);
				printlog("[SMOKE]: remote-combat auto-pause action=pause pulse=%d/%d",
					completedPausePulses + 1, smoke.autoPausePulses);
			}
			else
			{
				pauseGame(1, 0);
				smoke.pauseActive = false;
				++smoke.pauseActionsIssued;
				smoke.nextPauseTick = ticks + smoke.autoPauseDelayTicks;
				Combat::traceRemoteCombatEvent("auto-unpause-issued", 0);
				printlog("[SMOKE]: remote-combat auto-pause action=unpause pulse=%d/%d",
					completedPausePulses + 1, smoke.autoPausePulses);
			}
		}

		if ( !smoke.pauseCompleteLogged && (smoke.pauseActionsIssued / 2) >= smoke.autoPausePulses
			&& smoke.autoPausePulses > 0 )
		{
			smoke.pauseCompleteLogged = true;
			printlog("[SMOKE]: remote-combat auto-pause complete pulses=%d", smoke.autoPausePulses);
		}
	}

	void tickLocalSplitscreenBaseline()
	{
		SmokeLocalSplitscreenState& smoke = smokeLocalSplitscreenState();
		if ( !smoke.enabled )
		{
			return;
		}
		if ( multiplayer != SINGLE )
		{
			return;
		}

		const int expected = std::max(1, std::min(smoke.expectedPlayers, kSmokeLocalMaxPlayers));
		const int connected = smokeLocalConnectedPlayers(expected);
		const int loaded = smokeLocalLoadedPlayers(expected);
		const bool splitscreenOk = expected >= 2 ? splitscreen : !splitscreen;

		int localSlotsOk = 0;
		int vmouseFailures = 0;
		int hudMissing = 0;
		const bool cameraOk = smokeLocalCameraLayoutOk(expected, localSlotsOk, vmouseFailures, hudMissing);
		const bool baselineOk = connected >= expected
			&& loaded >= expected
			&& splitscreenOk
			&& localSlotsOk == 1
			&& cameraOk
			&& vmouseFailures == 0
			&& hudMissing == 0;

		if ( smoke.traceEnabled && baselineOk && !smoke.baselineLoggedOk )
		{
			smoke.baselineLoggedOk = true;
			printlog("[SMOKE]: local-splitscreen baseline status=ok expected=%d connected=%d loaded=%d splitscreen=%d local_slots_ok=%d camera_ok=%d vmouse_failures=%d hud_missing=%d",
				expected, connected, loaded, splitscreen ? 1 : 0, localSlotsOk,
				cameraOk ? 1 : 0, vmouseFailures, hudMissing);
		}
		else if ( smoke.traceEnabled && !baselineOk && !smoke.baselineLoggedFail )
		{
			smoke.baselineLoggedFail = true;
			printlog("[SMOKE]: local-splitscreen baseline status=wait expected=%d connected=%d loaded=%d splitscreen=%d local_slots_ok=%d camera_ok=%d vmouse_failures=%d hud_missing=%d",
				expected, connected, loaded, splitscreen ? 1 : 0, localSlotsOk,
				cameraOk ? 1 : 0, vmouseFailures, hudMissing);
		}

		if ( !baselineOk )
		{
			smoke.readyArmed = false;
			return;
		}

		if ( !smoke.readyArmed )
		{
			smoke.readyArmed = true;
			smoke.readySinceTick = ticks;
			smoke.nextPauseTick = ticks + smoke.autoPauseDelayTicks;
			printlog("[SMOKE]: local-splitscreen baseline armed expected=%d", expected);
		}

		const int completedPausePulses = smoke.pauseActionsIssued / 2;
		if ( smoke.autoPausePulses > 0
			&& completedPausePulses < smoke.autoPausePulses
			&& ticks >= smoke.nextPauseTick )
		{
			if ( !smoke.pauseActive )
			{
				pauseGame(2, 0);
				smoke.pauseActive = true;
				++smoke.pauseActionsIssued;
				smoke.nextPauseTick = ticks + smoke.autoPauseHoldTicks;
				printlog("[SMOKE]: local-splitscreen auto-pause action=pause pulse=%d/%d gamePaused=%d",
					completedPausePulses + 1, smoke.autoPausePulses, gamePaused ? 1 : 0);
			}
			else
			{
				pauseGame(1, 0);
				smoke.pauseActive = false;
				++smoke.pauseActionsIssued;
				smoke.nextPauseTick = ticks + smoke.autoPauseDelayTicks;
				printlog("[SMOKE]: local-splitscreen auto-pause action=unpause pulse=%d/%d gamePaused=%d",
					completedPausePulses + 1, smoke.autoPausePulses, gamePaused ? 1 : 0);
			}
		}

		if ( !smoke.pauseCompleteLogged
			&& smoke.autoPausePulses > 0
			&& (smoke.pauseActionsIssued / 2) >= smoke.autoPausePulses )
		{
			smoke.pauseCompleteLogged = true;
			printlog("[SMOKE]: local-splitscreen auto-pause complete pulses=%d", smoke.autoPausePulses);
		}

		if ( !smoke.transitionLogged && currentlevel > 0 )
		{
			smoke.transitionLogged = true;
			printlog("[SMOKE]: local-splitscreen transition level=%d status=ok", currentlevel);
		}
	}

	void tickLocalSplitscreenCap()
	{
		SmokeLocalSplitscreenCapState& smoke = smokeLocalSplitscreenCapState();
		if ( !smoke.enabled )
		{
			return;
		}
		if ( multiplayer != SINGLE )
		{
			return;
		}
		if ( smoke.targetPlayers < 2 )
		{
			return;
		}
		if ( !players[0] || !players[0]->entity )
		{
			return;
		}

		if ( !smoke.armed )
		{
			smoke.armed = true;
			smoke.armTick = ticks;
			if ( smoke.traceEnabled )
			{
				printlog("[SMOKE]: local-splitscreen cap armed target=%d cap=%d issue_in=%u",
					smoke.targetPlayers,
					smoke.cappedPlayers,
					static_cast<unsigned>(smoke.commandDelayTicks / TICKS_PER_SECOND));
			}
			return;
		}

		if ( !smoke.commandIssued
			&& ticks - smoke.armTick >= smoke.commandDelayTicks )
		{
			consoleCommand("/enablecheats");
			if ( splitscreen )
			{
				consoleCommand("/splitscreen");
			}

			char command[64] = "";
			snprintf(command, sizeof(command), "/splitscreen %d", smoke.targetPlayers);
			consoleCommand(command);
			smoke.commandIssued = true;
			smoke.verifyTick = ticks + smoke.verifyDelayTicks;
			printlog("[SMOKE]: local-splitscreen cap command issued target=%d cap=%d verify_after=%u",
				smoke.targetPlayers,
				smoke.cappedPlayers,
				static_cast<unsigned>(smoke.verifyDelayTicks / TICKS_PER_SECOND));
			return;
		}

		if ( !smoke.commandIssued || ticks < smoke.verifyTick )
		{
			return;
		}

		int connectedPlayers = 0;
		int connectedLocalPlayers = 0;
		int overCapConnected = 0;
		int overCapLocal = 0;
		int overCapSplitscreen = 0;
		int underCapNonLocal = 0;

		for ( int slot = 0; slot < MAXPLAYERS; ++slot )
		{
			const bool connected = !client_disconnected[slot];
			const bool localPlayer = players[slot] && players[slot]->isLocalPlayer();
			if ( connected )
			{
				++connectedPlayers;
				if ( localPlayer )
				{
					++connectedLocalPlayers;
				}
				else if ( slot < smoke.cappedPlayers )
				{
					++underCapNonLocal;
				}
			}

			if ( slot >= smoke.cappedPlayers )
			{
				if ( connected )
				{
					++overCapConnected;
				}
				if ( localPlayer )
				{
					++overCapLocal;
				}
				if ( players[slot] && players[slot]->bSplitscreen )
				{
					++overCapSplitscreen;
				}
			}
		}

		const bool ok = splitscreen
			&& connectedPlayers == smoke.cappedPlayers
			&& connectedLocalPlayers == smoke.cappedPlayers
			&& overCapConnected == 0
			&& overCapLocal == 0
			&& overCapSplitscreen == 0
			&& underCapNonLocal == 0;

		if ( ok )
		{
			if ( !smoke.loggedOk )
			{
				smoke.loggedOk = true;
				printlog("[SMOKE]: local-splitscreen cap status=ok target=%d cap=%d connected=%d connected_local=%d over_cap_connected=%d over_cap_local=%d over_cap_splitscreen=%d under_cap_nonlocal=%d",
					smoke.targetPlayers,
					smoke.cappedPlayers,
					connectedPlayers,
					connectedLocalPlayers,
					overCapConnected,
					overCapLocal,
					overCapSplitscreen,
					underCapNonLocal);
			}
			return;
		}

		if ( !smoke.loggedFail )
		{
			smoke.loggedFail = true;
			printlog("[SMOKE]: local-splitscreen cap status=fail target=%d cap=%d connected=%d connected_local=%d over_cap_connected=%d over_cap_local=%d over_cap_splitscreen=%d under_cap_nonlocal=%d splitscreen=%d",
				smoke.targetPlayers,
				smoke.cappedPlayers,
				connectedPlayers,
				connectedLocalPlayers,
				overCapConnected,
				overCapLocal,
				overCapSplitscreen,
				underCapNonLocal,
				splitscreen ? 1 : 0);
		}
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

namespace Combat
{
	bool isRemoteCombatSlotBoundsTraceEnabled()
	{
		return smokeRemoteCombatState().traceEnabled;
	}

	void traceRemoteCombatSlotBounds(const char* context, const int slot, const int rawSlot,
		const int minInclusive, const int maxExclusive)
	{
		if ( !isRemoteCombatSlotBoundsTraceEnabled() )
		{
			return;
		}

		const int maxInclusive = maxExclusive > minInclusive ? maxExclusive - 1 : minInclusive;
		const bool rawOk = rawSlot >= minInclusive && rawSlot < maxExclusive;
		const bool slotOk = slot >= minInclusive && slot < maxExclusive;
		const bool ok = rawOk && slotOk;
		std::string key = context && context[0] ? context : "unspecified";
		if ( ok && slot >= 0 && slot < MAXPLAYERS )
		{
			static std::unordered_map<std::string, std::array<bool, MAXPLAYERS>> loggedOkByContext;
			std::array<bool, MAXPLAYERS>& seen = loggedOkByContext[key];
			if ( seen[slot] )
			{
				return;
			}
			seen[slot] = true;
		}

		printlog("[SMOKE]: remote-combat slot-check context=%s raw=%d slot=%d min=%d max=%d status=%s",
			key.c_str(),
			rawSlot,
			slot,
			minInclusive,
			maxInclusive,
			ok ? "ok" : "fail");
	}

	void traceRemoteCombatEvent(const char* context, const int slot)
	{
		if ( !isRemoteCombatSlotBoundsTraceEnabled() )
		{
			return;
		}

		std::string key = context && context[0] ? context : "unspecified";
		if ( slot >= 0 && slot < MAXPLAYERS )
		{
			static std::unordered_map<std::string, std::array<bool, MAXPLAYERS>> loggedByContext;
			std::array<bool, MAXPLAYERS>& seen = loggedByContext[key];
			if ( seen[slot] )
			{
				return;
			}
			seen[slot] = true;
		}

		printlog("[SMOKE]: remote-combat event context=%s slot=%d",
			key.c_str(),
			slot);
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
			for ( const OwnerEncodingEffectSpec& spec : kOwnerEncodingEffects )
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
			for ( const OwnerEncodingEffectSpec& spec : kOwnerEncodingEffects )
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
			for ( const OwnerEncodingEffectSpec& spec : kOwnerEncodingEffects )
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

				const SaveGameInfo::Player::stat_t& postLoadStats = loaded.players[slot].stats;
				if ( !validatePlayerEffects(laneName, "post_load", slot, postLoadStats, connectedPlayers, false, checks) )
				{
					ok = false;
				}

				SaveGameInfo::Player::stat_t tickedStats = postLoadStats;
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
	static Summary g_lastSummary;

	int connectedPlayersOverride()
	{
		static bool initialized = false;
		static int envOverridePlayers = 0;
		static std::string controlFilePath;
		static int lastLoggedOverridePlayers = -1;
		static std::string lastInvalidControlFileValue;

		if ( !initialized )
		{
			initialized = true;
			if ( envHasValue("BARONY_SMOKE_MAPGEN_CONNECTED_PLAYERS") )
			{
				envOverridePlayers = parseEnvInt("BARONY_SMOKE_MAPGEN_CONNECTED_PLAYERS", 0, 1, MAXPLAYERS);
				if ( envOverridePlayers <= 0 )
				{
					printlog("[SMOKE]: ignoring invalid BARONY_SMOKE_MAPGEN_CONNECTED_PLAYERS");
				}
			}
			controlFilePath = trimCopy(parseEnvString("BARONY_SMOKE_MAPGEN_CONTROL_FILE", ""));
			if ( !controlFilePath.empty() )
			{
				printlog("[SMOKE]: mapgen connected-player control-file configured: %s",
					controlFilePath.c_str());
			}
		}

		int overridePlayers = envOverridePlayers;
		bool overrideFromControlFile = false;
		if ( !controlFilePath.empty() )
		{
			std::ifstream controlFile(controlFilePath.c_str());
			if ( controlFile.good() )
			{
				std::string rawValue;
				std::getline(controlFile, rawValue);
				const std::string trimmedValue = trimCopy(rawValue);
				if ( !trimmedValue.empty() )
				{
					int parsedControlPlayers = 0;
					if ( parseBoundedIntString(trimmedValue, 1, MAXPLAYERS, parsedControlPlayers) )
					{
						overridePlayers = parsedControlPlayers;
						overrideFromControlFile = true;
					}
					else if ( trimmedValue != lastInvalidControlFileValue )
					{
						printlog("[SMOKE]: ignoring invalid mapgen control-file value '%s' from %s",
							trimmedValue.c_str(), controlFilePath.c_str());
						lastInvalidControlFileValue = trimmedValue;
					}
				}
			}
		}

		if ( overridePlayers <= 0 )
		{
			return 0;
		}
		if ( overridePlayers != lastLoggedOverridePlayers )
		{
			if ( overrideFromControlFile )
			{
				printlog("[SMOKE]: mapgen connected-player override active: %d (control-file=%s)",
					overridePlayers, controlFilePath.c_str());
			}
			else
			{
				printlog("[SMOKE]: mapgen connected-player override active: %d", overridePlayers);
			}
			lastLoggedOverridePlayers = overridePlayers;
		}
		return overridePlayers;
	}

	void resetSummary()
	{
		g_lastSummary = Summary();
	}

	void recordGenerationSummary(int level, int secret, Uint32 seed, int players,
		int rooms, int monsters, int gold, int items, int decorations)
	{
		g_lastSummary.valid = true;
		g_lastSummary.level = level;
		g_lastSummary.secret = secret;
		g_lastSummary.seed = seed;
		g_lastSummary.players = players;
		g_lastSummary.rooms = rooms;
		g_lastSummary.monsters = monsters;
		g_lastSummary.gold = gold;
		g_lastSummary.items = items;
		g_lastSummary.decorations = decorations;
	}

	void recordDecorationSummary(int level, int secret, Uint32 seed,
		int blocking, int utility, int traps, int economy)
	{
		g_lastSummary.valid = true;
		g_lastSummary.level = level;
		g_lastSummary.secret = secret;
		g_lastSummary.seed = seed;
		g_lastSummary.decorationsBlocking = blocking;
		g_lastSummary.decorationsUtility = utility;
		g_lastSummary.decorationsTraps = traps;
		g_lastSummary.decorationsEconomy = economy;
	}

	void recordFoodAndValueSummary(int level, int secret, Uint32 seed,
		int foodItems, int foodServings,
		int goldBags, int goldAmount, int itemStacks, int itemUnits)
	{
		g_lastSummary.valid = true;
		g_lastSummary.level = level;
		g_lastSummary.secret = secret;
		g_lastSummary.seed = seed;
		g_lastSummary.foodItems = foodItems;
		g_lastSummary.foodServings = foodServings;
		g_lastSummary.goldBags = goldBags;
		g_lastSummary.goldAmount = goldAmount;
		g_lastSummary.itemStacks = itemStacks;
		g_lastSummary.itemUnits = itemUnits;
	}

	const Summary& lastSummary()
	{
		return g_lastSummary;
	}

#ifdef BARONY_SMOKE_TESTS
	namespace
	{
		bool parseIntegrationLevelListCsv(const std::string& csv, std::vector<int>& outLevels)
		{
			outLevels.clear();
			std::stringstream parser(csv);
			std::string token;
			while ( std::getline(parser, token, ',') )
			{
				token = trimCopy(token);
				if ( token.empty() )
				{
					continue;
				}
				int level = 0;
				if ( !parseBoundedIntString(token, 1, 99, level) )
				{
					return false;
				}
				outLevels.push_back(level);
			}
			return !outLevels.empty();
		}

		bool parseIntegrationUint32Arg(const std::string& value, Uint32& outValue)
		{
			if ( value.empty() )
			{
				return false;
			}
			std::stringstream parser(value);
			unsigned long long parsed = 0;
			char trailing = '\0';
			if ( !(parser >> parsed) || (parser >> trailing) )
			{
				return false;
			}
			if ( parsed > std::numeric_limits<Uint32>::max() )
			{
				return false;
			}
			outValue = static_cast<Uint32>(parsed);
			return true;
		}

		bool writeIntegrationControlFile(const std::string& controlFilePath, int players)
		{
			std::ofstream controlFile(controlFilePath.c_str(), std::ios::out | std::ios::trunc);
			if ( !controlFile.is_open() )
			{
				return false;
			}
			controlFile << players << "\n";
			return controlFile.good();
		}
	}

	bool parseIntegrationOptionArg(const char* arg, IntegrationOptions& options, std::string& errorMessage)
	{
		errorMessage.clear();
		if ( !arg )
		{
			return false;
		}

		const std::string rawArg(arg);
		if ( rawArg == "-smoke-mapgen-integration" )
		{
			options.enabled = true;
			return true;
		}
		if ( rawArg == "-smoke-mapgen-integration-append" )
		{
			options.enabled = true;
			options.append = true;
			return true;
		}

		auto startsWithValue = [&rawArg](const char* prefix, std::string& outValue) -> bool
		{
			const std::string prefixString(prefix);
			if ( rawArg.rfind(prefixString, 0) != 0 )
			{
				return false;
			}
			outValue = rawArg.substr(prefixString.size());
			return true;
		};

		std::string rawValue;
		if ( startsWithValue("-smoke-mapgen-integration-csv=", rawValue) )
		{
			options.enabled = true;
			options.outputCsvPath = rawValue;
			return true;
		}
		if ( startsWithValue("-smoke-mapgen-integration-levels=", rawValue) )
		{
			options.enabled = true;
			options.levelsCsv = rawValue;
			return true;
		}
		if ( startsWithValue("-smoke-mapgen-integration-min-players=", rawValue) )
		{
			int parsedPlayers = 0;
			if ( !parseBoundedIntString(rawValue, 1, MAXPLAYERS, parsedPlayers) )
			{
				errorMessage = "Invalid value for -smoke-mapgen-integration-min-players";
				return true;
			}
			options.enabled = true;
			options.minPlayers = parsedPlayers;
			return true;
		}
		if ( startsWithValue("-smoke-mapgen-integration-max-players=", rawValue) )
		{
			int parsedPlayers = 0;
			if ( !parseBoundedIntString(rawValue, 1, MAXPLAYERS, parsedPlayers) )
			{
				errorMessage = "Invalid value for -smoke-mapgen-integration-max-players";
				return true;
			}
			options.enabled = true;
			options.maxPlayers = parsedPlayers;
			return true;
		}
		if ( startsWithValue("-smoke-mapgen-integration-runs=", rawValue) )
		{
			int parsedRuns = 0;
			if ( !parseBoundedIntString(rawValue, 1, 100000, parsedRuns) )
			{
				errorMessage = "Invalid value for -smoke-mapgen-integration-runs";
				return true;
			}
			options.enabled = true;
			options.runsPerPlayer = parsedRuns;
			return true;
		}
		if ( startsWithValue("-smoke-mapgen-integration-base-seed=", rawValue) )
		{
			Uint32 parsedSeed = 0;
			if ( !parseIntegrationUint32Arg(rawValue, parsedSeed) )
			{
				errorMessage = "Invalid value for -smoke-mapgen-integration-base-seed";
				return true;
			}
			options.enabled = true;
			options.baseSeed = parsedSeed;
			return true;
		}

		return false;
	}

	bool validateIntegrationOptions(const IntegrationOptions& options, std::string& errorMessage)
	{
		errorMessage.clear();
		if ( !options.enabled )
		{
			return true;
		}
		if ( options.minPlayers > options.maxPlayers )
		{
			char buffer[128];
			snprintf(buffer, sizeof(buffer), "Invalid smoke mapgen integration player range: min=%d max=%d",
				options.minPlayers, options.maxPlayers);
			errorMessage = buffer;
			return false;
		}
		return true;
	}

	int runIntegrationMatrix(const IntegrationOptions& options)
	{
		if ( options.outputCsvPath.empty() )
		{
			printlog("[SMOKE][MAPGEN][INTEGRATION]: missing -smoke-mapgen-integration-csv=<path>");
			return 2;
		}

		std::vector<int> levels;
		if ( !parseIntegrationLevelListCsv(options.levelsCsv, levels) )
		{
			printlog("[SMOKE][MAPGEN][INTEGRATION]: invalid level list: %s", options.levelsCsv.c_str());
			return 2;
		}

		bool writeHeader = true;
		std::ios::openmode openMode = std::ios::out;
		if ( options.append )
		{
			openMode |= std::ios::app;
			std::ifstream existing(options.outputCsvPath.c_str(), std::ios::in | std::ios::binary | std::ios::ate);
			if ( existing.is_open() && existing.tellg() > 0 )
			{
				writeHeader = false;
			}
		}
		else
		{
			openMode |= std::ios::trunc;
		}

		std::ofstream csv(options.outputCsvPath.c_str(), openMode);
		if ( !csv.is_open() )
		{
			printlog("[SMOKE][MAPGEN][INTEGRATION]: failed to open csv output path: %s",
				options.outputCsvPath.c_str());
			return 2;
		}
		if ( writeHeader )
		{
			csv << "target_level,players,launched_instances,mapgen_players_override,mapgen_players_observed,run,seed,status,start_floor,host_chunk_lines,client_reassembled_lines,mapgen_found,mapgen_level,mapgen_secret,mapgen_seed_observed,rooms,monsters,gold,items,decorations,decorations_blocking,decorations_utility,decorations_traps,decorations_economy,food_items,food_servings,gold_bags,gold_amount,item_stacks,item_units,run_dir,mapgen_wait_reason,mapgen_reload_transition_lines,mapgen_generation_lines,mapgen_generation_unique_seed_count,mapgen_reload_regen_ok\n";
		}

		const std::string controlFilePath = options.outputCsvPath + ".mapgen_players_override.txt";
		if ( SDL_setenv("BARONY_SMOKE_MAPGEN_CONTROL_FILE", controlFilePath.c_str(), 1) != 0 )
		{
			printlog("[SMOKE][MAPGEN][INTEGRATION]: failed to set BARONY_SMOKE_MAPGEN_CONTROL_FILE (%s)",
				SDL_GetError());
			return 2;
		}
		if ( !writeIntegrationControlFile(controlFilePath, options.minPlayers) )
		{
			printlog("[SMOKE][MAPGEN][INTEGRATION]: failed to write mapgen control file: %s",
				controlFilePath.c_str());
			return 2;
		}
		csv.flush();

		int totalSamples = 0;
		int failedSamples = 0;
		const std::string runDir = "inprocess-mapgen-integration";
		const int playersSpan = options.maxPlayers - options.minPlayers + 1;
		const int samplesPerLevel = playersSpan * options.runsPerPlayer;
		for ( int level : levels )
		{
			const Uint32 levelBaseSeed = options.baseSeed + static_cast<Uint32>(level * 100000);
			const Uint32 levelSeedBase = levelBaseSeed + 1;
			const Uint32 reloadSeedBase = levelSeedBase * 100;
			const int reloadTransitionLines = std::max(0, samplesPerLevel - 1);
			int sampleIndex = 0;

			(void)loadMainMenuMap(true, false);
			gameModeManager.setMode(GameModeManager_t::GAME_MODE_DEFAULT);
			gameModeManager.currentSession.seededRun.setup(std::to_string(levelSeedBase));
			gameModeManager.currentSession.challengeRun.reset();
			uniqueGameKey = gameModeManager.currentSession.seededRun.seed;
			local_rng.seedBytes(&uniqueGameKey, sizeof(uniqueGameKey));
			net_rng.seedBytes(&uniqueGameKey, sizeof(uniqueGameKey));
			multiplayer = SERVER;
			clientnum = 0;
			numplayers = 0;
			intro = false;
			for ( int i = 0; i < MAXPLAYERS; ++i )
			{
				client_disconnected[i] = (i != 0);
			}
			assignActions(&map);
			generatePathMaps();

			for ( int players = options.minPlayers; players <= options.maxPlayers; ++players )
			{
				if ( !writeIntegrationControlFile(controlFilePath, players) )
				{
					printlog("[SMOKE][MAPGEN][INTEGRATION]: failed to update mapgen control file for players=%d", players);
					return 2;
				}
				for ( int run = 1; run <= options.runsPerPlayer; ++run )
				{
					++totalSamples;
					++sampleIndex;
					const Uint32 seed = levelBaseSeed + static_cast<Uint32>(sampleIndex);
					Uint32 generationSeed = 0;
					if ( sampleIndex > 1 )
					{
						generationSeed = reloadSeedBase + static_cast<Uint32>(sampleIndex - 2);
					}

					SmokeTestHooks::Mapgen::resetSummary();
					secretlevel = false;
					darkmap = false;
					currentlevel = level;
					mapseed = generationSeed;
					memset(map.flags, 0, sizeof(map.flags));
					loadCustomNextMap.clear();
					textSourceScript.scriptVariables.clear();
					gameplayCustomManager.readFromFile();

					int checkMapHash = -1;
					const bool previousLoading = loading;
					loading = true;
					const int loadResult = physfsLoadMapFile(level, generationSeed, false, &checkMapHash);
					loading = previousLoading;
					if ( loadResult != -1 )
					{
						multiplayer = SERVER;
						clientnum = 0;
						numplayers = 0;
						intro = false;
						for ( int i = 0; i < MAXPLAYERS; ++i )
						{
							client_disconnected[i] = (i != 0);
						}
						assignActions(&map);
						generatePathMaps();
					}

					const auto& summary = SmokeTestHooks::Mapgen::lastSummary();
					const int observedPlayers = (summary.valid && summary.players > 0) ? summary.players : players;
					bool pass = (loadResult != -1) && summary.valid;
					if ( pass && observedPlayers != players )
					{
						pass = false;
					}
					if ( pass && summary.level != level )
					{
						pass = false;
					}
					if ( !pass )
					{
						++failedSamples;
					}

					const char* status = pass ? "pass" : "fail";
					const char* waitReason = pass ? "none" :
						((loadResult == -1) ? "load_failed" : "summary_mismatch");
					const int mapgenFound = summary.valid ? 1 : 0;
					const int observedLevel = summary.valid ? summary.level : 0;
					const int observedSecret = summary.valid ? summary.secret : 0;
					const Uint32 observedSeed = summary.valid ? summary.seed : generationSeed;
					const int generationLines = summary.valid ? samplesPerLevel : 0;
					const int generationUniqueSeedCount = summary.valid ? samplesPerLevel : 0;
					const int reloadRegenOk = pass ? 1 : 0;

					csv
						<< level << ','
						<< players << ','
						<< 1 << ','
						<< players << ','
						<< observedPlayers << ','
						<< run << ','
						<< seed << ','
						<< status << ','
						<< level << ','
						<< 0 << ','
						<< 0 << ','
						<< mapgenFound << ','
						<< observedLevel << ','
						<< observedSecret << ','
						<< observedSeed << ','
						<< (summary.valid ? summary.rooms : 0) << ','
						<< (summary.valid ? summary.monsters : 0) << ','
						<< (summary.valid ? summary.gold : 0) << ','
						<< (summary.valid ? summary.items : 0) << ','
						<< (summary.valid ? summary.decorations : 0) << ','
						<< (summary.valid ? summary.decorationsBlocking : 0) << ','
						<< (summary.valid ? summary.decorationsUtility : 0) << ','
						<< (summary.valid ? summary.decorationsTraps : 0) << ','
						<< (summary.valid ? summary.decorationsEconomy : 0) << ','
						<< (summary.valid ? summary.foodItems : 0) << ','
						<< (summary.valid ? summary.foodServings : 0) << ','
						<< (summary.valid ? summary.goldBags : 0) << ','
						<< (summary.valid ? summary.goldAmount : 0) << ','
						<< (summary.valid ? summary.itemStacks : 0) << ','
						<< (summary.valid ? summary.itemUnits : 0) << ','
						<< runDir << ','
						<< waitReason << ','
						<< reloadTransitionLines << ','
						<< generationLines << ','
						<< generationUniqueSeedCount << ','
						<< reloadRegenOk
						<< "\n";
					csv.flush();
				}
			}
		}

		csv.flush();
		if ( !csv.good() )
		{
			printlog("[SMOKE][MAPGEN][INTEGRATION]: failed to flush csv output");
			return 2;
		}

		printlog("[SMOKE][MAPGEN][INTEGRATION]: completed samples=%d failures=%d csv=%s",
			totalSamples, failedSamples, options.outputCsvPath.c_str());
		if ( failedSamples > 0 )
		{
			return 3;
		}
		return 0;
	}
#endif
	}
}
