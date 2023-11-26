#pragma once

#ifdef USE_PLAYFAB
#include "main.hpp"
#include <playfab/PlayFabClientApi.h>
#include <playfab/PlayFabClientDataModels.h>
#include <playfab/PlayFabClientInstanceApi.h>
#include "playfab/PlayFabSettings.h"
#include "playfab/PlayFabApiSettings.h"
#include <playfab/PlayFabCloudScriptApi.h>
#include <playfab/PlayFabCloudScriptDataModels.h>
#include <playfab/PlayFabCloudScriptInstanceApi.h>
#include "stat.hpp"
#include "scores.hpp"

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
	Uint32 processedOnTick = 0;
	void loginEpic();
	void loginSteam();
	void resetLogin();
	void postScore(const int player);
	void update();
	void getLeaderboardTop100Data(std::string lid, int start, int numEntries);
	void getLeaderboardTop100(std::string lid);
	void getLeaderboardAroundMe(std::string lid);

	bool bInit = false;
	void init();

	struct PostScoreHandler_t
	{
		struct ScoreUpdate_t
		{
			std::string hash = "";
			std::string score = "";
			PlayFab::PlayFabErrorCode code = PlayFab::PlayFabErrorCode::PlayFabErrorUnknownError;
			bool inprogress = false;
			int sequence = 0;
			int retryTicks = 0;
			Uint32 creationTick = 0;
			Uint32 postTick = 0;
			bool expired = false;
			std::string writtenToFile = "";

			ScoreUpdate_t(std::string _score, std::string _hash)
			{
				hash = _hash;
				score = _score;
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
		std::string getLeaderboardDisplayName();
		std::string getLeaderboardID()
		{
			int victory = this->victory;
			if ( win == false ) { victory = 0; }
			std::string lid = ((scoreType == RANK_TOTALSCORE || win) ? "lid_" : "lid_time_");
			if ( daily )
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

	static void OnLoginFail(const PlayFab::PlayFabError& error, void* customData);
	static void OnLoginSuccess(const PlayFab::ClientModels::LoginResult& result, void* customData);
	static void OnDisplayNameUpdateSuccess(const PlayFab::ClientModels::UpdateUserTitleDisplayNameResult& result, void* customData);
	static void OnCloudScriptExecute(const PlayFab::ClientModels::ExecuteCloudScriptResult& result, void* customData);
	static void OnFunctionExecute(const PlayFab::CloudScriptModels::ExecuteFunctionResult& result, void* customData);
	static void OnCloudScriptFailure(const PlayFab::PlayFabError& error, void* customData);
	static void OnLeaderboardGet(const PlayFab::ClientModels::GetLeaderboardResult& result, void* customData);
	static void OnLeaderboardAroundMeGet(const PlayFab::ClientModels::GetLeaderboardAroundPlayerResult& result, void* customData);
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