#pragma once

#include "main.hpp"
#include "stat.hpp"
#include "scores.hpp"
#include "mod_tools.hpp"
#ifdef USE_PLAYFAB
#include <playfab/PlayFabClientApi.h>
#include <playfab/PlayFabClientDataModels.h>
#include <playfab/PlayFabClientInstanceApi.h>
#include "playfab/PlayFabSettings.h"
#include "playfab/PlayFabApiSettings.h"
#include <playfab/PlayFabCloudScriptApi.h>
#include <playfab/PlayFabCloudScriptDataModels.h>
#include <playfab/PlayFabCloudScriptInstanceApi.h>
#include <playfab/PlayFabEventsApi.h>

class PlayfabUser_t
{
public:
	enum UserTypes
	{
		PlayerType_None = 0,
		PlayerType_Steam,
		PlayerType_Epic,
		PlayerType_Nintendo
	};

	std::string currentUser = "";
	std::string currentUserName = "";
	std::string token;
	UserTypes type = PlayerType_None;
	bool bLoggedIn = false;
	bool loggingIn = false;
	bool errorLogin = false;
	Uint32 authenticationRefresh = 0;
	int loginFailures = 1;
	Uint32 processedOnTick = 0;
	bool newSeedsAvailable = false;
	static Uint32 processTick;
	void loginEpic();
	void loginSteam();
	void resetLogin();
	void postScore(const int player);
	void update();
	void getLeaderboardTop100Data(std::string lid, int start, int numEntries);
	void getLeaderboardTop100(std::string lid);
	void getLeaderboardAroundMe(std::string lid);
	void getLeaderboardTop100Alternate(std::string lid);
	void gameBegin();
	void gameEnd();
	void biomeLeave();
	void compendiumResearch(std::string category, std::string section);
	void globalStat(int index, int player);

	struct PlayerCheckLeaderboardData_t
	{
		bool hasData = false;
		bool loading = true;
		std::map<int, std::pair<Uint32, PlayFab::PlayFabErrorCode>> awaitingResponse;
		static int sequenceIDs;
	};
	std::map<std::string, PlayerCheckLeaderboardData_t> playerCheckLeaderboardData;
	void checkLocalPlayerHasEntryOnLeaderboard(std::string lid);

	bool bInit = false;
	void init();

	struct PeriodicalEvents_t
	{
		void getPeriodicalEvents();
		struct Event_t
		{
			string lid = "";
			string seed_word = "";
			Uint32 seed = 0;
			int hoursLeft = 0;
			int minutesLeft = 0;
			int lid_version = 0;
			bool verifiedForGameStart = false;
			int errorType = 0;
			void verifyGameStartSeedForEvent();
			bool attempted = false;
			bool loading = false;
			int rolloverConflict = 0;
			bool locked = false;
			std::string scenario = "";
			GameModeManager_t::CurrentSession_t::ChallengeRun_t scenarioInfo;
		};
		std::vector<Event_t> periodicalEvents;
		bool awaitingData = false;
		bool error = false;
		int sequence = 0;
		int waitingForSequence = 0;
	} periodicalEvents;

	struct PostScoreHandler_t
	{
		struct ScoreUpdate_t
		{
			std::string hash = "";
			std::string score = "";
			std::string name = "";
			PlayFab::PlayFabErrorCode code = PlayFab::PlayFabErrorCode::PlayFabErrorUnknownError;
			bool inprogress = false;
			int sequence = 0;
			int retryTicks = 0;
			Uint32 creationTick = 0;
			Uint32 postTick = 0;
			bool expired = false;
			std::string writtenToFile = "";

			ScoreUpdate_t(std::string _score, std::string _hash, std::string _name)
			{
				hash = _hash;
				score = _score;
				name = _name;
				sequence = sequenceIDs;
				creationTick = ticks;
				++sequenceIDs;
			}

			~ScoreUpdate_t();

			void post();
			bool saveToFile();
			bool deleteFile();
		};
		static int sequenceIDs;
		static char buf[65535];
		std::set<Uint32> sessionsPosted;
		std::deque<ScoreUpdate_t> queue;
		static Uint32 lastPostTicks;
		void update();
		void readFromFiles();
		void deinit()
		{
			for ( auto& entry : queue )
			{
				entry.writtenToFile = "";
			}
			queue.clear();
		}
	} postScoreHandler;

	struct LeaderboardData_t
	{
		struct LeaderBoard_t
		{
			std::string name;
			std::map<std::string, SaveGameInfo> playerData;
			struct Entry_t
			{
				int rank = 0;
				bool hasData = false;
				bool awaitingData = false;
				bool readIntoScore = false;
				std::string id = "";
				std::string displayName = "";
				int score = 0;
			};
			bool loading = true;
			bool playerDataLoading = false;
			std::vector<Entry_t> ranks;
			std::priority_queue<std::pair<int, std::string>> sortedData;
			std::vector<Entry_t> displayedRanks;
			std::map<int, std::pair<Uint32, PlayFab::PlayFabErrorCode>> awaitingResponse;
			bool errorReceivingLeaderboard();
			void requestPlayerData(int start, int numEntries);
		};
		static int sequenceIDs;
		std::map<std::string, LeaderBoard_t> leaderboards;
		std::string currentSearch = "";
		std::string currentDisplayName = "";
	} leaderboardData;

	struct LeaderboardSearch_t
	{
		bool daily = false;
		bool hardcore = false;
		bool multiplayer = false;
		enum ChallengeBoards
		{
			CHALLENGE_BOARD_NONE,
			CHALLENGE_BOARD_ONESHOT,
			CHALLENGE_BOARD_UNLIMITED,
			CHALLENGE_BOARD_CHALLENGE
		};
		ChallengeBoards challengeBoard = CHALLENGE_BOARD_NONE;
		std::map<ChallengeBoards, std::string> savedSearchesFromNotification;
		enum ScoreType
		{
			RANK_TOTALSCORE,
			RANK_TIME
		};
		ScoreType scoreType = RANK_TOTALSCORE;
		bool dlc = false;
		bool scoresNearMe = false;
		int victory = 3;
		bool win = true;
		bool requiresRefresh = false;
		int searchStartIndex = 0;
		void applySavedChallengeSearchIfExists();
		std::string getLeaderboardDisplayName();
		std::string getLeaderboardID()
		{
			int victory = this->victory;
			if ( win == false ) { victory = 0; }
			std::string lid = ((scoreType == RANK_TIME || !win) ? "lid_time_" : "lid_");
			if ( challengeBoard == CHALLENGE_BOARD_ONESHOT )
			{
				if ( victory > 0 )
				{
					lid += "victory_";
				}
				else
				{
					lid += "novictory_";
				}

				if ( scoreType == RANK_TOTALSCORE )
				{
					lid += "seed_oneshot";
				}
				else if ( scoreType == RANK_TIME )
				{
					lid += "seed_oneshot";
				}
			}
			else if ( challengeBoard == CHALLENGE_BOARD_UNLIMITED )
			{
				if ( victory > 0 )
				{
					lid += "victory_";
				}
				else
				{
					lid += "novictory_";
				}

				if ( scoreType == RANK_TOTALSCORE )
				{
					lid += "seed_unlimited";
				}
				else if ( scoreType == RANK_TIME )
				{
					lid += "seed_unlimited";
				}
			}
			else if ( challengeBoard == CHALLENGE_BOARD_CHALLENGE )
			{
				if ( victory > 0 )
				{
					lid += "victory_";
				}
				else
				{
					lid += "novictory_";
				}

				if ( scoreType == RANK_TOTALSCORE )
				{
					lid += "seed_challenge";
				}
				else if ( scoreType == RANK_TIME )
				{
					lid += "seed_challenge";
				}
			}
			else if ( daily )
			{
				if ( victory > 0 )
				{
					lid += "victory_daily";
				}
				else
				{
					lid += "novictory_daily";
				}
			}
			else
			{
				switch ( victory )
				{
				case 0:
					lid += "novictory";
					break;
				case 1:
					lid += "victory_classic";
					break;
				case 2:
					lid += "victory_hell";
					break;
				case 3:
					lid += "victory_citadel";
					break;
				default:
					if ( victory > 3 )
					{
						lid += "victory_citadel";
					}
					break;
				}

				if ( hardcore )
				{
					lid += "_hc";
				}
				if ( dlc )
				{
					lid += "_dlc";
				}
				if ( multiplayer )
				{
					lid += "_multi";
				}
			}
			return lid;
		}
	} leaderboardSearch;

	static void OnEventsWrite(const PlayFab::EventsModels::WriteEventsResponse& result, void* customData);
	static void OnLoginFail(const PlayFab::PlayFabError& error, void* customData);
	static void OnLoginSuccess(const PlayFab::ClientModels::LoginResult& result, void* customData);
	static void OnDisplayNameUpdateSuccess(const PlayFab::ClientModels::UpdateUserTitleDisplayNameResult& result, void* customData);
	static void OnCloudScriptExecute(const PlayFab::ClientModels::ExecuteCloudScriptResult& result, void* customData);
	static void OnFunctionExecute(const PlayFab::CloudScriptModels::ExecuteFunctionResult& result, void* customData);
	static void OnCloudScriptFailure(const PlayFab::PlayFabError& error, void* customData);
	static void OnLeaderboardGet(const PlayFab::ClientModels::GetLeaderboardResult& result, void* customData);
	static void OnLeaderboardAroundMeGet(const PlayFab::ClientModels::GetLeaderboardAroundPlayerResult& result, void* customData);
	static void OnLeaderboardTop100AlternateGet(const PlayFab::ClientModels::GetLeaderboardResult& result, void* customData);
	static void OnLeaderboardFail(const PlayFab::PlayFabError& error, void* customData);

	static void logInfo(const char* str, ...)
	{
		char newstr[1024] = { 0 };
		va_list argptr;

		// format the content
		va_start(argptr, str);
		vsnprintf(newstr, 1023, str, argptr);
		va_end(argptr);
		printlog("[PlayFab Info]: %s", newstr);
	}
	static void logError(const char* str, ...)
	{
		char newstr[1024] = { 0 };
		va_list argptr;

		// format the content
		va_start(argptr, str);
		vsnprintf(newstr, 1023, str, argptr);
		va_end(argptr);
		printlog("[PlayFab Error]: %s", newstr);
	}
};

extern PlayfabUser_t playfabUser;

#endif