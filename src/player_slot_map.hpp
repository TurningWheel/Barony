#pragma once

#include <cstddef>

template <typename T, std::size_t kMaxPlayers>
struct PlayerSlotLookup
{
	T byPlayer[kMaxPlayers]{};

	const T& operator[](const std::size_t index) const
	{
		return byPlayer[index];
	}
};

template <typename T, std::size_t kMaxPlayers>
inline PlayerSlotLookup<T, kMaxPlayers> buildPlayerSlotLookup(
	const T* primary, const std::size_t primaryCount,
	const T* cycle, const std::size_t cycleCount,
	const T fallbackValue)
{
	PlayerSlotLookup<T, kMaxPlayers> out{};

	for ( std::size_t i = 0; i < kMaxPlayers; ++i )
	{
		if ( primary && i < primaryCount )
		{
			out.byPlayer[i] = primary[i];
		}
		else if ( cycle && cycleCount > 0 && i >= primaryCount )
		{
			out.byPlayer[i] = cycle[(i - primaryCount) % cycleCount];
		}
		else if ( primary && primaryCount > 0 )
		{
			out.byPlayer[i] = primary[primaryCount - 1];
		}
		else
		{
			out.byPlayer[i] = fallbackValue;
		}
	}

	return out;
}

template <typename T, std::size_t kMaxPlayers, std::size_t kPrimaryCount, std::size_t kCycleCount>
inline PlayerSlotLookup<T, kMaxPlayers> buildPlayerSlotLookup(
	const T (&primary)[kPrimaryCount],
	const T (&cycle)[kCycleCount],
	const T fallbackValue)
{
	return buildPlayerSlotLookup<T, kMaxPlayers>(
		primary, kPrimaryCount,
		cycle, kCycleCount,
		fallbackValue);
}
