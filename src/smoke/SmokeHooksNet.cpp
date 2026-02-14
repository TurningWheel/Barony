#include "SmokeTestHooks.hpp"
#include "SmokeHooksCommon.hpp"

namespace
{
	using namespace SmokeHooksCommon;
}

namespace SmokeTestHooks
{
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

}
