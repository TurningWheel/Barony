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
		bool (*connectToLanServer)(const char* address) = nullptr;
		void (*startGame)() = nullptr;
		void (*createReadyStone)(int index, bool local, bool ready) = nullptr;
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
	bool isAutopilotEnvEnabled();
	bool isHeloChunkPayloadOverrideEnvEnabled();
	bool isHeloChunkTxModeOverrideEnvEnabled();

	bool isAutopilotEnabled();
	bool isAutopilotHostEnabled();
	int expectedHostLobbyPlayerSlots(int fallbackSlots);
	void tickAutopilot(const AutopilotCallbacks& callbacks);
}

namespace Gameplay
{
	bool isHooksEnvEnabled();
	bool isAutoEnterDungeonEnabled();
	void tickAutoEnterDungeon();
}

namespace Mapgen
{
	int connectedPlayersOverride();
}
}
