#pragma once

#include "../main.hpp"

#include <string>
#include <vector>

namespace SmokeTestHooks
{
namespace MainMenu
{
	enum class HeloChunkTxMode : Uint8
	{
		NORMAL = 0,
		REVERSE,
		EVEN_ODD,
		DUPLICATE_FIRST,
		DROP_LAST,
		DUPLICATE_CONFLICT_FIRST
	};

	struct AutopilotCallbacks
	{
		bool (*hostLANLobbyNoSound)() = nullptr;
		bool (*hostSteamLobbyNoSound)() = nullptr;
		bool (*hostEosLobbyNoSound)() = nullptr;
		bool (*hostLocalLobbyNoSound)() = nullptr;
		bool (*connectToLanServer)(const char* address) = nullptr;
		bool (*connectToOnlineLobby)(const char* address) = nullptr;
		void (*startGame)() = nullptr;
		void (*createReadyStone)(int index, bool local, bool ready) = nullptr;
		bool (*isLocalLobbyAutopilotContextReady)() = nullptr;
		bool (*isLocalLobbyCardReady)(int index) = nullptr;
		bool (*isLocalLobbyCountdownActive)() = nullptr;
		bool (*isLocalPlayerSignedIn)(int index) = nullptr;
		void (*kickPlayer)(int index) = nullptr;
		void (*requestLobbyPlayerCountSelection)(int targetCount) = nullptr;
		void (*requestLobbyVisiblePage)(int pageIndex) = nullptr;
	};

	struct HeloChunkSendPlanEntry
	{
		int chunkIndex = 0;
		bool corruptPayload = false;
	};

	bool hasHeloChunkPayloadOverride();
	int heloChunkPayloadMaxOverride(int defaultPayloadMax, int minPayloadMax = 64);
	bool hasHeloChunkTxModeOverride();
	HeloChunkTxMode heloChunkTxMode();
	const char* heloChunkTxModeName(HeloChunkTxMode mode);
	void applyHeloChunkTxModePlan(std::vector<HeloChunkSendPlanEntry>& sendPlan, int chunkCount, Uint16 transferId);
	bool isReadyStateSyncTraceEnabled();
	void traceReadyStateSnapshotQueued(int player, int attempts, Uint32 firstSendTick);
	void traceReadyStateSnapshotSent(int player, int readyEntries);
	bool isSlotLockTraceEnabled();
	void traceLobbySlotLockSnapshot(const char* context, const bool lockedSlots[MAXPLAYERS],
		const bool disconnectedSlots[MAXPLAYERS], int configuredPlayers);
	bool isAccountLabelTraceEnabled();
	void traceLobbyAccountLabelResolved(int slot, const char* accountName);
	bool isPlayerCountCopyTraceEnabled();
	void traceLobbyPlayerCountPrompt(int targetCount, int kickedCount, const char* variant, const char* promptText);
	bool isLobbyPageStateTraceEnabled();
	void traceLobbyPageSnapshot(const char* context, int page, int pageCount, int pageOffsetX,
		int selectedOwner, const char* selectedWidget, int focusPageMatch, int cardsVisible,
		int cardsMisaligned, int paperdollsVisible, int paperdollsMisaligned,
		int pingsVisible, int pingsMisaligned, int warningsCenterDelta,
		int countdownCenterDelta);
	bool isLocalSplitscreenTraceEnabled();
	void traceLocalLobbySnapshot(const char* context, int targetPlayers, int joinedPlayers,
		int readyPlayers, int countdownActive);
	void traceLocalLobbySnapshotIfChanged(const char* context, int targetPlayers, int joinedPlayers,
		int readyPlayers, int countdownActive);
	bool isHeloChunkPayloadOverrideEnvEnabled();
	bool isHeloChunkTxModeOverrideEnvEnabled();

	bool isAutopilotEnabled();
	bool isAutopilotHostEnabled();
	bool isAutopilotBackendSteam();
	bool isAutopilotBackendEos();
	int expectedHostLobbyPlayerSlots(int fallbackSlots);
	void tickAutopilot(const AutopilotCallbacks& callbacks);
	}

namespace Gameplay
{
	void tickAutoEnterDungeon();
	void tickRemoteCombatAutopilot();
	void tickLocalSplitscreenBaseline();
	void tickLocalSplitscreenCap();
}

namespace GameUI
{
	bool isStatusEffectQueueTraceEnabled();
	void recordStatusEffectQueueInit(int slot, int owner);
	void flushStatusEffectQueueInitTrace();
	void traceStatusEffectQueueCreate(int slot, int owner);
	void traceStatusEffectQueueUpdate(int slot, int owner);
}

namespace Net
{
	bool isForceHeloChunkEnabled();
	int heloChunkPayloadMaxOverride(int defaultPayloadMax, int minPayloadMax = 64);
	bool isJoinRejectTraceEnabled();
	void traceLobbyJoinReject(Uint32 result, Uint8 requestedSlot, const bool lockedSlots[MAXPLAYERS], const bool disconnectedSlots[MAXPLAYERS]);
}

namespace Combat
{
	bool isRemoteCombatSlotBoundsTraceEnabled();
	void traceRemoteCombatSlotBounds(const char* context, int slot, int rawSlot, int minInclusive, int maxExclusive);
	void traceRemoteCombatEvent(const char* context, int slot);
}

namespace SaveReload
{
	bool isOwnerEncodingSweepEnabled();
	bool runOwnerEncodingSweep(bool singleplayer, int saveIndex);
}

namespace Mapgen
{
	struct Summary
	{
		bool valid = false;
		int level = 0;
		int secret = 0;
		Uint32 seed = 0;
		int players = 0;
		int rooms = 0;
		int monsters = 0;
		int gold = 0;
		int items = 0;
		int decorations = 0;
		int decorationsBlocking = 0;
		int decorationsUtility = 0;
		int decorationsTraps = 0;
		int decorationsEconomy = 0;
		int foodItems = 0;
		int foodServings = 0;
		int goldBags = 0;
		int goldAmount = 0;
		int itemStacks = 0;
		int itemUnits = 0;
	};

		int connectedPlayersOverride();
		void resetSummary();
		void recordGenerationSummary(int level, int secret, Uint32 seed, int players,
			int rooms, int monsters, int gold, int items, int decorations);
		void recordDecorationSummary(int level, int secret, Uint32 seed,
			int blocking, int utility, int traps, int economy);
	void recordFoodAndValueSummary(int level, int secret, Uint32 seed,
		int foodItems, int foodServings,
		int goldBags, int goldAmount, int itemStacks, int itemUnits);
	const Summary& lastSummary();

#ifdef BARONY_SMOKE_TESTS
	struct IntegrationOptions
	{
		bool enabled = false;
		bool append = false;
		std::string outputCsvPath;
		std::string levelsCsv = "1,7,16,25,33";
		int minPlayers = 1;
		int maxPlayers = MAXPLAYERS;
		int runsPerPlayer = 2;
		Uint32 baseSeed = 1000;
	};

	bool parseIntegrationOptionArg(const char* arg, IntegrationOptions& options, std::string& errorMessage);
	bool validateIntegrationOptions(const IntegrationOptions& options, std::string& errorMessage);
	int runIntegrationMatrix(const IntegrationOptions& options);
#endif
}
}
