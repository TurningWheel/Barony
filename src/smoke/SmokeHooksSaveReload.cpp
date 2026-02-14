#include "SmokeTestHooks.hpp"
#include "SmokeHooksCommon.hpp"

#include "../files.hpp"
#include "../paths.hpp"
#include "../scores.hpp"
#include "../status_effect_owner_encoding.hpp"

#include <array>
#include <sstream>
#include <string>
#include <vector>

namespace
{
	using namespace SmokeHooksCommon;
}

namespace SmokeTestHooks
{
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

}
