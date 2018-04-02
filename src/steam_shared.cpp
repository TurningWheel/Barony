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
	UGCUpdateHandle(0),
	UGCQueryHandle(0),
	SteamUGCQueryCompleted(),
	UnsubscribePublishedFileResult(),
	LastActionResult(),
	workshopItemTags(),
	uploadSuccessTicks(0),
	m_myWorkshopItemToModify()
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
		StoreResultMessage("Item Create: OK", createItemResult.m_eResult);
		return;
	}
	StoreResultMessage("Item Create: ERROR", createItemResult.m_eResult);
}

void CSteamWorkshop::StartItemUpdate()
{
	if ( createItemResult.m_nPublishedFileId != 0 )
	{
		UGCUpdateHandle = SteamUGC()->StartItemUpdate(STEAM_APPID, createItemResult.m_nPublishedFileId);
		StoreResultMessage("Item Update Initialise: OK", createItemResult.m_eResult);
		return;
	}
	StoreResultMessage("Item Update Initialise: ERROR", createItemResult.m_eResult);
}

void CSteamWorkshop::StartItemExistingUpdate(PublishedFileId_t fileId)
{
	if ( fileId != 0 )
	{
		UGCUpdateHandle = SteamUGC()->StartItemUpdate(STEAM_APPID, fileId);
		StoreResultMessage("Item Update Initialise: OK", static_cast<EResult>(1));
		return;
	}
	StoreResultMessage("Item Update Initialise: ERROR", static_cast<EResult>(0));
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
		StoreResultMessage("Item Update: OK", SubmitItemUpdateResult.m_eResult);
		return;
	}
	StoreResultMessage("Item Update: ERROR", SubmitItemUpdateResult.m_eResult);
}

void CSteamWorkshop::CreateQuerySubscribedItems(EUserUGCList itemListType, EUGCMatchingUGCType searchType, EUserUGCListSortOrder sortOrder)
{
	// searchType can look for all results, items only, guides only etc.
	// sortOrder will sort results by creation date, subscribed date etc.
	CSteamID steamID = SteamUser()->GetSteamID();
	UGCQueryHandle = SteamUGC()->CreateQueryUserUGCRequest(steamID.GetAccountID(), itemListType,
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
		if ( SteamUGCQueryCompleted.m_eResult == k_EResultOK )
		{
			ReadSubscribedItems();
			StoreResultMessage("Load Subscribed Items: OK", k_EResultOK);
			return;
		}
	}
	StoreResultMessage("Load Subscribed Items: ERROR", SteamUGCQueryCompleted.m_eResult);
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

void CSteamWorkshop::UnsubscribeItemFileID(PublishedFileId_t fileId)
{
	SteamAPICall_t hSteamAPICall = SteamUGC()->UnsubscribeItem(fileId);
	m_callResultUnsubscribeItemRequest.Set(hSteamAPICall, this,
		&CSteamWorkshop::OnUnsubscribeItemRequest);
}

void CSteamWorkshop::OnUnsubscribeItemRequest(RemoteStorageUnsubscribePublishedFileResult_t *pResult, bool bIOFailure)
{
	if ( !bIOFailure )
	{
		UnsubscribePublishedFileResult = *pResult;
		if ( UnsubscribePublishedFileResult.m_eResult == k_EResultOK )
		{
			CreateQuerySubscribedItems(k_EUserUGCList_Subscribed, k_EUGCMatchingUGCType_All, k_EUserUGCListSortOrder_LastUpdatedDesc);
			StoreResultMessage("Unsubscribe Item: OK", k_EResultOK);
			return;
		}
	}
	StoreResultMessage("Unsubscribe Item: ERROR", UnsubscribePublishedFileResult.m_eResult);
}

void CSteamWorkshop::StoreResultMessage(std::string message, EResult result)
{
	LastActionResult.actionMsg = message;
	LastActionResult.lastResult = result;
	LastActionResult.creationTick = ticks;
}

#endif // STEAMWORKS
