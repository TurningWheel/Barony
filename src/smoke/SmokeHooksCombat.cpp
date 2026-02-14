#include "SmokeTestHooks.hpp"
#include "SmokeHooksCommon.hpp"

#include <array>
#include <string>
#include <unordered_map>

namespace
{
	using namespace SmokeHooksCommon;
}

namespace SmokeTestHooks
{
namespace Combat
{
	bool isRemoteCombatSlotBoundsTraceEnabled()
	{
		static const bool enabled =
			parseEnvBool("BARONY_SMOKE_AUTOPILOT", false)
			&& parseEnvBool("BARONY_SMOKE_TRACE_REMOTE_COMBAT_SLOT_BOUNDS", false);
		return enabled;
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

}
