#pragma once

#include <cstdint>

namespace StatusEffectOwnerEncoding
{
	static_assert(MAXPLAYERS <= 15, "Packed status-effect owner encoding supports up to 15 players.");

	constexpr std::uint8_t kStrengthNibbleMask = 0x0F;
	constexpr std::uint8_t kOwnerNibbleMask = 0xF0;

	inline std::uint8_t encodeOwnerNibbleFromPlayer(const int player)
	{
		if ( player < 0 || player >= MAXPLAYERS )
		{
			return 0;
		}
		return static_cast<std::uint8_t>(((player + 1) << 4) & kOwnerNibbleMask);
	}

	inline int decodeOwnerNibbleToPlayer(const std::uint8_t packedValue)
	{
		const int ownerOneBased = (packedValue >> 4) & 0xF;
		if ( ownerOneBased >= 1 && ownerOneBased <= MAXPLAYERS )
		{
			return ownerOneBased - 1;
		}
		return -1;
	}

	inline std::uint8_t packStrengthWithOwnerNibble(const std::uint8_t strength, const int player)
	{
		return static_cast<std::uint8_t>((strength & kStrengthNibbleMask) | encodeOwnerNibbleFromPlayer(player));
	}

	inline bool isOwnerNibbleOnlyFormat(const std::uint8_t packedValue)
	{
		return (packedValue & kOwnerNibbleMask) != 0 && (packedValue & kStrengthNibbleMask) == 0;
	}

	// EFF_FAST must accept both legacy bitmask values and new owner-id nibble values.
	inline int decodeFastCasterCompat(const std::uint8_t effectStrength)
	{
		if ( effectStrength == 0 )
		{
			return -1;
		}

		if ( isOwnerNibbleOnlyFormat(effectStrength) )
		{
			return decodeOwnerNibbleToPlayer(effectStrength);
		}

		if ( (effectStrength & 0x1) != 0 && (effectStrength & 0xFE) != 0 )
		{
			for ( int i = 0; i < MAXPLAYERS; ++i )
			{
				if ( effectStrength & (1 << (i + 1)) )
				{
					return i;
				}
			}
		}

		return -1;
	}
}
