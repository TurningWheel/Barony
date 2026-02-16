#include "SmokeTestHooks.hpp"
#include "SmokeHooksCommon.hpp"

#include "../net.hpp"

namespace
{
	using namespace SmokeHooksCommon;
}

namespace SmokeTestHooks
{
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

}
