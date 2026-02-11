#pragma once

#include "../main.hpp"

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
		bool (*hostLocalLobbyNoSound)() = nullptr;
		bool (*connectToLanServer)(const char* address) = nullptr;
		void (*startGame)() = nullptr;
		void (*createReadyStone)(int index, bool local, bool ready) = nullptr;
		bool (*prepareLocalLobbyPlayers)(int targetCount) = nullptr;
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
	bool isHeloChunkPayloadOverrideEnvEnabled();
	bool isHeloChunkTxModeOverrideEnvEnabled();

	bool isAutopilotEnabled();
	bool isAutopilotHostEnabled();
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
	int connectedPlayersOverride();
}
}
