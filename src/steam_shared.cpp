/*-------------------------------------------------------------------------------

BARONY
File: steam_shared.cpp
Desc: various callback functions for steam for editor and barony

Copyright 2013-2018 (c) Turning Wheel LLC, all rights reserved.
See LICENSE for details.

-------------------------------------------------------------------------------*/
#include "main.hpp"
#ifdef STEAMWORKS
#include <steam/steam_api.h>
#include "steam.hpp"
#endif

#ifdef STEAMWORKS
CSteamLeaderboards::CSteamLeaderboards() :
	m_CurrentLeaderboard(NULL),
	m_nLeaderboardEntries(0)
{

}

CSteamWorkshop::CSteamWorkshop() :
	createItemResult(),
	SubmitItemUpdateResult(),
	UGCUpdateHandle(0)
{

}

void CSteamLeaderboards::FindLeaderboard(const char *pchLeaderboardName)
{
	m_CurrentLeaderboard = NULL;

	SteamAPICall_t hSteamAPICall = SteamUserStats()->FindLeaderboard(pchLeaderboardName);
	m_callResultFindLeaderboard.Set(hSteamAPICall, this,
		&CSteamLeaderboards::OnFindLeaderboard);
	// call OnFindLeaderboard when result of async API call
}

void CSteamLeaderboards::OnFindLeaderboard(LeaderboardFindResult_t *pCallback, bool bIOFailure)
{
	// see if we encountered an error during the call
	if ( !pCallback->m_bLeaderboardFound || bIOFailure )
	{
		return;
	}

	m_CurrentLeaderboard = pCallback->m_hSteamLeaderboard;
}

bool CSteamLeaderboards::DownloadScores()
{
	if ( !m_CurrentLeaderboard )
	{
		return false;
	}

	// load the specified leaderboard data around the current user
	SteamAPICall_t hSteamAPICall = SteamUserStats()->DownloadLeaderboardEntries(
		m_CurrentLeaderboard, k_ELeaderboardDataRequestGlobalAroundUser, -4, 5);
	m_callResultDownloadScore.Set(hSteamAPICall, this,
		&CSteamLeaderboards::OnDownloadScore);

	return true;
}

void CSteamLeaderboards::OnDownloadScore(LeaderboardScoresDownloaded_t *pCallback, bool bIOFailure)
{
	if ( !bIOFailure )
	{
		m_nLeaderboardEntries = std::min(pCallback->m_cEntryCount, 10);

		for ( int index = 0; index < m_nLeaderboardEntries; ++index )
		{
			SteamUserStats()->GetDownloadedLeaderboardEntry(
				pCallback->m_hSteamLeaderboardEntries, index, &m_leaderboardEntries[index], NULL, 0);
		}
	}
}

void CSteamWorkshop::CreateItem()
{
	SteamAPICall_t hSteamAPICall = SteamUGC()->CreateItem(STEAM_APPID, k_EWorkshopFileTypeCommunity);
	m_callResultCreateItem.Set(hSteamAPICall, this,
		&CSteamWorkshop::OnCreateItem);
}

void CSteamWorkshop::OnCreateItem(CreateItemResult_t *pResult, bool bIOFailure)
{
	if ( !bIOFailure )
	{
		createItemResult = *pResult;
	}
}

void CSteamWorkshop::StartItemUpdate()
{
	if ( g_SteamWorkshop->createItemResult.m_nPublishedFileId != 0 )
	{
		UGCUpdateHandle = SteamUGC()->StartItemUpdate(STEAM_APPID, g_SteamWorkshop->createItemResult.m_nPublishedFileId);
	}
}

void CSteamWorkshop::SubmitItemUpdate(char* changeNote)
{
	if ( UGCUpdateHandle != 0 )
	{
		SteamAPICall_t hSteamAPICall = SteamUGC()->SubmitItemUpdate(UGCUpdateHandle, changeNote);
		m_callResultSubmitItemUpdateResult.Set(hSteamAPICall, this,
			&CSteamWorkshop::OnSubmitItemUpdate);
	}
}

void CSteamWorkshop::OnSubmitItemUpdate(SubmitItemUpdateResult_t *pResult, bool bIOFailure)
{
	if ( !bIOFailure )
	{
		SubmitItemUpdateResult = *pResult;
	}
}

void CSteamWorkshop::CreateQuerySubscribedItems(EUGCMatchingUGCType searchType, EUserUGCListSortOrder sortOrder)
{
	// searchType can look for all results, items only, guides only etc.
	// sortOrder will sort results by creation date, subscribed date etc.
	CSteamID steamID = SteamUser()->GetSteamID();
	UGCQueryHandle = SteamUGC()->CreateQueryUserUGCRequest(steamID.GetAccountID(), k_EUserUGCList_Subscribed,
		searchType, sortOrder, STEAM_APPID, STEAM_APPID, 1);
	if ( UGCQueryHandle != k_UGCQueryHandleInvalid )
	{
		SteamAPICall_t hSteamAPICall = SteamUGC()->SendQueryUGCRequest(UGCQueryHandle);
		m_callResultSendQueryUGCRequest.Set(hSteamAPICall, this,
			&CSteamWorkshop::OnSendQueryUGCRequest);
	}
	SteamUGC()->ReleaseQueryUGCRequest(UGCQueryHandle);
}

void CSteamWorkshop::OnSendQueryUGCRequest(SteamUGCQueryCompleted_t *pResult, bool bIOFailure)
{
	if ( !bIOFailure )
	{
		SteamUGCQueryCompleted = *pResult;
	}
}

void CSteamWorkshop::ReadSubscribedItems()
{
	if ( SteamUGCQueryCompleted.m_eResult == k_EResultOK )
	{
		for ( int i = 0; i < SteamUGCQueryCompleted.m_unNumResultsReturned; ++i )
		{
				SteamUGC()->GetQueryUGCResult(SteamUGCQueryCompleted.m_handle,
					i, &m_subscribedItemListDetails[i]);
		}
		SteamUGC()->ReleaseQueryUGCRequest(SteamUGCQueryCompleted.m_handle);
	}
}

#endif // STEAMWORKS
