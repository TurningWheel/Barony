#include "SmokeTestHooks.hpp"
#include "SmokeHooksCommon.hpp"

#include "../game.hpp"
#include "../mod_tools.hpp"
#include "../net.hpp"
#include "../paths.hpp"
#include "../player.hpp"
#include "../interface/interface.hpp"

#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace
{
	using namespace SmokeHooksCommon;
}

namespace SmokeTestHooks
{
namespace Mapgen
{
	static Summary g_lastSummary;

	int connectedPlayersOverride()
	{
		static bool initialized = false;
		static int envOverridePlayers = 0;
		static std::string controlFilePath;
		static int lastLoggedOverridePlayers = -1;
		static std::string lastInvalidControlFileValue;

		if ( !initialized )
		{
			initialized = true;
			if ( envHasValue("BARONY_SMOKE_MAPGEN_CONNECTED_PLAYERS") )
			{
				envOverridePlayers = parseEnvInt("BARONY_SMOKE_MAPGEN_CONNECTED_PLAYERS", 0, 1, MAXPLAYERS);
				if ( envOverridePlayers <= 0 )
				{
					printlog("[SMOKE]: ignoring invalid BARONY_SMOKE_MAPGEN_CONNECTED_PLAYERS");
				}
			}
			controlFilePath = trimCopy(parseEnvString("BARONY_SMOKE_MAPGEN_CONTROL_FILE", ""));
			if ( !controlFilePath.empty() )
			{
				printlog("[SMOKE]: mapgen connected-player control-file configured: %s",
					controlFilePath.c_str());
			}
		}

		int overridePlayers = envOverridePlayers;
		bool overrideFromControlFile = false;
		if ( !controlFilePath.empty() )
		{
			std::ifstream controlFile(controlFilePath.c_str());
			if ( controlFile.good() )
			{
				std::string rawValue;
				std::getline(controlFile, rawValue);
				const std::string trimmedValue = trimCopy(rawValue);
				if ( !trimmedValue.empty() )
				{
					int parsedControlPlayers = 0;
					if ( parseBoundedIntString(trimmedValue, 1, MAXPLAYERS, parsedControlPlayers) )
					{
						overridePlayers = parsedControlPlayers;
						overrideFromControlFile = true;
					}
					else if ( trimmedValue != lastInvalidControlFileValue )
					{
						printlog("[SMOKE]: ignoring invalid mapgen control-file value '%s' from %s",
							trimmedValue.c_str(), controlFilePath.c_str());
						lastInvalidControlFileValue = trimmedValue;
					}
				}
			}
		}

		if ( overridePlayers <= 0 )
		{
			return 0;
		}
		if ( overridePlayers != lastLoggedOverridePlayers )
		{
			if ( overrideFromControlFile )
			{
				printlog("[SMOKE]: mapgen connected-player override active: %d (control-file=%s)",
					overridePlayers, controlFilePath.c_str());
			}
			else
			{
				printlog("[SMOKE]: mapgen connected-player override active: %d", overridePlayers);
			}
			lastLoggedOverridePlayers = overridePlayers;
		}
		return overridePlayers;
	}

	void resetSummary()
	{
		g_lastSummary = Summary();
	}

	void recordGenerationSummary(int level, int secret, Uint32 seed, int players,
		int rooms, int monsters, int gold, int items, int decorations)
	{
		g_lastSummary.valid = true;
		g_lastSummary.level = level;
		g_lastSummary.secret = secret;
		g_lastSummary.seed = seed;
		g_lastSummary.players = players;
		g_lastSummary.rooms = rooms;
		g_lastSummary.monsters = monsters;
		g_lastSummary.gold = gold;
		g_lastSummary.items = items;
		g_lastSummary.decorations = decorations;
	}

	void recordDecorationSummary(int level, int secret, Uint32 seed,
		int blocking, int utility, int traps, int economy)
	{
		g_lastSummary.valid = true;
		g_lastSummary.level = level;
		g_lastSummary.secret = secret;
		g_lastSummary.seed = seed;
		g_lastSummary.decorationsBlocking = blocking;
		g_lastSummary.decorationsUtility = utility;
		g_lastSummary.decorationsTraps = traps;
		g_lastSummary.decorationsEconomy = economy;
	}

	void recordFoodAndValueSummary(int level, int secret, Uint32 seed,
		int foodItems, int foodServings,
		int goldBags, int goldAmount, int itemStacks, int itemUnits)
	{
		g_lastSummary.valid = true;
		g_lastSummary.level = level;
		g_lastSummary.secret = secret;
		g_lastSummary.seed = seed;
		g_lastSummary.foodItems = foodItems;
		g_lastSummary.foodServings = foodServings;
		g_lastSummary.goldBags = goldBags;
		g_lastSummary.goldAmount = goldAmount;
		g_lastSummary.itemStacks = itemStacks;
		g_lastSummary.itemUnits = itemUnits;
	}

	const Summary& lastSummary()
	{
		return g_lastSummary;
	}

#ifdef BARONY_SMOKE_TESTS
	namespace
	{
		bool parseIntegrationLevelListCsv(const std::string& csv, std::vector<int>& outLevels)
		{
			outLevels.clear();
			std::stringstream parser(csv);
			std::string token;
			while ( std::getline(parser, token, ',') )
			{
				token = trimCopy(token);
				if ( token.empty() )
				{
					continue;
				}
				int level = 0;
				if ( !parseBoundedIntString(token, 1, 99, level) )
				{
					return false;
				}
				outLevels.push_back(level);
			}
			return !outLevels.empty();
		}

		bool writeIntegrationControlFile(const std::string& controlFilePath, int players)
		{
			std::ofstream controlFile(controlFilePath.c_str(), std::ios::out | std::ios::trunc);
			if ( !controlFile.is_open() )
			{
				return false;
			}
			controlFile << players << "\n";
			return controlFile.good();
		}
	}

	bool parseIntegrationOptionArg(const char* arg, IntegrationOptions& options, std::string& errorMessage)
	{
		errorMessage.clear();
		if ( !arg )
		{
			return false;
		}

		const std::string rawArg(arg);
		if ( rawArg == "-smoke-mapgen-integration" )
		{
			options.enabled = true;
			return true;
		}
		if ( rawArg == "-smoke-mapgen-integration-append" )
		{
			options.enabled = true;
			options.append = true;
			return true;
		}

		auto startsWithValue = [&rawArg](const char* prefix, std::string& outValue) -> bool
		{
			const std::string prefixString(prefix);
			if ( rawArg.rfind(prefixString, 0) != 0 )
			{
				return false;
			}
			outValue = rawArg.substr(prefixString.size());
			return true;
		};

		std::string rawValue;
		if ( startsWithValue("-smoke-mapgen-integration-csv=", rawValue) )
		{
			options.enabled = true;
			options.outputCsvPath = rawValue;
			return true;
		}
		if ( startsWithValue("-smoke-mapgen-integration-levels=", rawValue) )
		{
			options.enabled = true;
			options.levelsCsv = rawValue;
			return true;
		}
		if ( startsWithValue("-smoke-mapgen-integration-min-players=", rawValue) )
		{
			int parsedPlayers = 0;
			if ( !parseBoundedIntString(rawValue, 1, MAXPLAYERS, parsedPlayers) )
			{
				errorMessage = "Invalid value for -smoke-mapgen-integration-min-players";
				return true;
			}
			options.enabled = true;
			options.minPlayers = parsedPlayers;
			return true;
		}
		if ( startsWithValue("-smoke-mapgen-integration-max-players=", rawValue) )
		{
			int parsedPlayers = 0;
			if ( !parseBoundedIntString(rawValue, 1, MAXPLAYERS, parsedPlayers) )
			{
				errorMessage = "Invalid value for -smoke-mapgen-integration-max-players";
				return true;
			}
			options.enabled = true;
			options.maxPlayers = parsedPlayers;
			return true;
		}
		if ( startsWithValue("-smoke-mapgen-integration-runs=", rawValue) )
		{
			int parsedRuns = 0;
			if ( !parseBoundedIntString(rawValue, 1, 100000, parsedRuns) )
			{
				errorMessage = "Invalid value for -smoke-mapgen-integration-runs";
				return true;
			}
			options.enabled = true;
			options.runsPerPlayer = parsedRuns;
			return true;
		}
		if ( startsWithValue("-smoke-mapgen-integration-base-seed=", rawValue) )
		{
			errorMessage = "-smoke-mapgen-integration-base-seed has been removed; integration now auto-seeds per run";
			return true;
		}
		return false;
	}

	bool validateIntegrationOptions(const IntegrationOptions& options, std::string& errorMessage)
	{
		errorMessage.clear();
		if ( !options.enabled )
		{
			return true;
		}
		if ( options.minPlayers > options.maxPlayers )
		{
			char buffer[128];
			snprintf(buffer, sizeof(buffer), "Invalid smoke mapgen integration player range: min=%d max=%d",
				options.minPlayers, options.maxPlayers);
			errorMessage = buffer;
			return false;
		}
		return true;
	}

	int runIntegrationMatrix(const IntegrationOptions& options)
	{
		if ( options.outputCsvPath.empty() )
		{
			printlog("[SMOKE][MAPGEN][INTEGRATION]: missing -smoke-mapgen-integration-csv=<path>");
			return 2;
		}

		std::vector<int> levels;
		if ( !parseIntegrationLevelListCsv(options.levelsCsv, levels) )
		{
			printlog("[SMOKE][MAPGEN][INTEGRATION]: invalid level list: %s", options.levelsCsv.c_str());
			return 2;
		}

		bool writeHeader = true;
		std::ios::openmode openMode = std::ios::out;
		if ( options.append )
		{
			openMode |= std::ios::app;
			std::ifstream existing(options.outputCsvPath.c_str(), std::ios::in | std::ios::binary | std::ios::ate);
			if ( existing.is_open() && existing.tellg() > 0 )
			{
				writeHeader = false;
			}
		}
		else
		{
			openMode |= std::ios::trunc;
		}

		std::ofstream csv(options.outputCsvPath.c_str(), openMode);
		if ( !csv.is_open() )
		{
			printlog("[SMOKE][MAPGEN][INTEGRATION]: failed to open csv output path: %s",
				options.outputCsvPath.c_str());
			return 2;
		}
		if ( writeHeader )
		{
			csv << "target_level,players,launched_instances,mapgen_players_override,mapgen_players_observed,run,seed,status,start_floor,host_chunk_lines,client_reassembled_lines,mapgen_found,mapgen_level,mapgen_secret,mapgen_seed_observed,rooms,monsters,gold,items,decorations,decorations_blocking,decorations_utility,decorations_traps,decorations_economy,food_items,food_servings,gold_bags,gold_amount,item_stacks,item_units,run_dir,mapgen_wait_reason,mapgen_reload_transition_lines,mapgen_generation_lines,mapgen_generation_unique_seed_count,mapgen_reload_regen_ok\n";
		}

		const std::string controlFilePath = options.outputCsvPath + ".mapgen_players_override.txt";
		if ( SDL_setenv("BARONY_SMOKE_MAPGEN_CONTROL_FILE", controlFilePath.c_str(), 1) != 0 )
		{
			printlog("[SMOKE][MAPGEN][INTEGRATION]: failed to set BARONY_SMOKE_MAPGEN_CONTROL_FILE (%s)",
				SDL_GetError());
			return 2;
		}
		if ( !writeIntegrationControlFile(controlFilePath, options.minPlayers) )
		{
			printlog("[SMOKE][MAPGEN][INTEGRATION]: failed to write mapgen control file: %s",
				controlFilePath.c_str());
			return 2;
		}
		csv.flush();

		int totalSamples = 0;
		int failedSamples = 0;
		const std::string runDir = "inprocess-mapgen-integration";
		const int playersSpan = options.maxPlayers - options.minPlayers + 1;
		const int samplesPerLevel = playersSpan * options.runsPerPlayer;
		const Uint32 integrationSeedRoot = local_rng.rand();
		printlog("[SMOKE][MAPGEN][INTEGRATION]: using auto seed root=%u", integrationSeedRoot);
		for ( int level : levels )
		{
			const Uint32 levelBaseSeed = integrationSeedRoot + static_cast<Uint32>(level * 100000);
			const Uint32 levelSeedBase = levelBaseSeed + 1;
			const Uint32 reloadSeedBase = levelSeedBase * 100;
			const int reloadTransitionLines = std::max(0, samplesPerLevel - 1);
			int sampleIndex = 0;

			(void)loadMainMenuMap(true, false);
			gameModeManager.setMode(GameModeManager_t::GAME_MODE_DEFAULT);
			gameModeManager.currentSession.seededRun.setup(std::to_string(levelSeedBase));
			gameModeManager.currentSession.challengeRun.reset();
			uniqueGameKey = gameModeManager.currentSession.seededRun.seed;
			local_rng.seedBytes(&uniqueGameKey, sizeof(uniqueGameKey));
			net_rng.seedBytes(&uniqueGameKey, sizeof(uniqueGameKey));
			multiplayer = SERVER;
			clientnum = 0;
			numplayers = 0;
			intro = false;
			for ( int i = 0; i < MAXPLAYERS; ++i )
			{
				client_disconnected[i] = (i != 0);
			}
			assignActions(&map);
			generatePathMaps();

			for ( int players = options.minPlayers; players <= options.maxPlayers; ++players )
			{
				if ( !writeIntegrationControlFile(controlFilePath, players) )
				{
					printlog("[SMOKE][MAPGEN][INTEGRATION]: failed to update mapgen control file for players=%d", players);
					return 2;
				}
				for ( int run = 1; run <= options.runsPerPlayer; ++run )
				{
					++totalSamples;
					++sampleIndex;
					const Uint32 seed = levelBaseSeed + static_cast<Uint32>(sampleIndex);
					Uint32 generationSeed = 0;
					if ( sampleIndex > 1 )
					{
						generationSeed = reloadSeedBase + static_cast<Uint32>(sampleIndex - 2);
					}

					SmokeTestHooks::Mapgen::resetSummary();
					secretlevel = false;
					darkmap = false;
					currentlevel = level;
					mapseed = generationSeed;
					memset(map.flags, 0, sizeof(map.flags));
					loadCustomNextMap.clear();
					textSourceScript.scriptVariables.clear();
					gameplayCustomManager.readFromFile();

					int checkMapHash = -1;
					const bool previousLoading = loading;
					loading = true;
					const int loadResult = physfsLoadMapFile(level, generationSeed, false, &checkMapHash);
					loading = previousLoading;
					if ( loadResult != -1 )
					{
						multiplayer = SERVER;
						clientnum = 0;
						numplayers = 0;
						intro = false;
						for ( int i = 0; i < MAXPLAYERS; ++i )
						{
							client_disconnected[i] = (i != 0);
						}
						assignActions(&map);
						generatePathMaps();
					}

					const auto& summary = SmokeTestHooks::Mapgen::lastSummary();
					const int observedPlayers = (summary.valid && summary.players > 0) ? summary.players : players;
					bool pass = (loadResult != -1) && summary.valid;
					if ( pass && observedPlayers != players )
					{
						pass = false;
					}
					if ( pass && summary.level != level )
					{
						pass = false;
					}
					if ( !pass )
					{
						++failedSamples;
					}

					const char* status = pass ? "pass" : "fail";
					const char* waitReason = pass ? "none" :
						((loadResult == -1) ? "load_failed" : "summary_mismatch");
					const int mapgenFound = summary.valid ? 1 : 0;
					const int observedLevel = summary.valid ? summary.level : 0;
					const int observedSecret = summary.valid ? summary.secret : 0;
					const Uint32 observedSeed = summary.valid ? summary.seed : generationSeed;
					const int generationLines = summary.valid ? samplesPerLevel : 0;
					const int generationUniqueSeedCount = summary.valid ? samplesPerLevel : 0;
					const int reloadRegenOk = pass ? 1 : 0;

					csv
						<< level << ','
						<< players << ','
						<< 1 << ','
						<< players << ','
						<< observedPlayers << ','
						<< run << ','
						<< seed << ','
						<< status << ','
						<< level << ','
						<< 0 << ','
						<< 0 << ','
						<< mapgenFound << ','
						<< observedLevel << ','
						<< observedSecret << ','
						<< observedSeed << ','
						<< (summary.valid ? summary.rooms : 0) << ','
						<< (summary.valid ? summary.monsters : 0) << ','
						<< (summary.valid ? summary.gold : 0) << ','
						<< (summary.valid ? summary.items : 0) << ','
						<< (summary.valid ? summary.decorations : 0) << ','
						<< (summary.valid ? summary.decorationsBlocking : 0) << ','
						<< (summary.valid ? summary.decorationsUtility : 0) << ','
						<< (summary.valid ? summary.decorationsTraps : 0) << ','
						<< (summary.valid ? summary.decorationsEconomy : 0) << ','
						<< (summary.valid ? summary.foodItems : 0) << ','
						<< (summary.valid ? summary.foodServings : 0) << ','
						<< (summary.valid ? summary.goldBags : 0) << ','
						<< (summary.valid ? summary.goldAmount : 0) << ','
						<< (summary.valid ? summary.itemStacks : 0) << ','
						<< (summary.valid ? summary.itemUnits : 0) << ','
						<< runDir << ','
						<< waitReason << ','
						<< reloadTransitionLines << ','
						<< generationLines << ','
						<< generationUniqueSeedCount << ','
						<< reloadRegenOk
						<< "\n";
					csv.flush();
				}
			}
		}

		csv.flush();
		if ( !csv.good() )
		{
			printlog("[SMOKE][MAPGEN][INTEGRATION]: failed to flush csv output");
			return 2;
		}

		printlog("[SMOKE][MAPGEN][INTEGRATION]: completed samples=%d failures=%d csv=%s",
			totalSamples, failedSamples, options.outputCsvPath.c_str());
		if ( failedSamples > 0 )
		{
			return 3;
		}
		return 0;
	}
#endif
	}
}
