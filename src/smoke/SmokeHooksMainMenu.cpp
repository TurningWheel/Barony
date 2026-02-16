#include "SmokeTestHooks.hpp"
#include "SmokeHooksCommon.hpp"

#include "../game.hpp"
#include "../lobbies.hpp"
#include "../mod_tools.hpp"
#include "../net.hpp"
#include "../player.hpp"

#include <algorithm>
#include <cstdio>
#include <limits>

namespace
{
	using namespace SmokeHooksCommon;
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

}
