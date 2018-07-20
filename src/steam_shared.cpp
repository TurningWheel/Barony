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
	m_myWorkshopItemToModify(),
	subscribedCallStatus(0)
{

}

CSteamStatistics::CSteamStatistics(SteamStat_t* gStats, int numStatistics) :
	m_CallbackUserStatsReceived(this, &CSteamStatistics::OnUserStatsReceived),
	m_CallbackUserStatsStored(this, &CSteamStatistics::OnUserStatsStored),
	m_bInitialized(false)
{
	m_iNumStats = numStatistics;
	m_pStats = gStats;
	RequestStats();
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
	if ( itemListType == k_EUserUGCList_Subscribed )
	{
		subscribedCallStatus = 1;
	}
	else
	{
		subscribedCallStatus = 0;
	}
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
			if ( subscribedCallStatus == 1 )
			{
				subscribedCallStatus = 2;
			}
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

bool CSteamStatistics::RequestStats()
{
	// Is Steam loaded? If not we can't get stats.
	if ( NULL == SteamUserStats() || NULL == SteamUser() )
	{
		return false;
	}
	// Is the user logged on?  If not we can't get stats.
	if ( !SteamUser()->BLoggedOn() )
	{
		return false;
	}
	// Request user stats.
	return SteamUserStats()->RequestCurrentStats();
}

void CSteamStatistics::OnUserStatsReceived(UserStatsReceived_t *pCallback)
{
	// we may get callbacks for other games' stats arriving, ignore them
	if ( pCallback->m_nGameID == STEAM_APPID )
	{
		if ( pCallback->m_eResult == k_EResultOK )
		{
			// load stats
			for ( int iStat = 0; iStat < m_iNumStats; ++iStat )
			{
				SteamStat_t &stat = m_pStats[iStat];
				switch ( stat.m_eStatType )
				{
					case STEAM_STAT_INT:
						SteamUserStats()->GetStat(stat.m_pchStatName, &stat.m_iValue);
						//printlog("%s: %d", stat.m_pchStatName, stat.m_iValue);
						break;
					case STEAM_STAT_FLOAT:
					case STEAM_STAT_AVGRATE:
						SteamUserStats()->GetStat(stat.m_pchStatName, &stat.m_flValue);
						break;
					default:
						break;
				}
			}
			m_bInitialized = true;
			printlog("[STEAM]: successfully received Steam user statistics.");
		}
		else
		{
			printlog("[STEAM]: unsuccessfully received Steam user statistics!");
		}
	}
	else
	{
		printlog("[STEAM]: unsuccessfully received Steam user statistics, appID (%d) was invalid!", pCallback->m_nGameID);
	}
}

bool CSteamStatistics::StoreStats()
{
	if ( m_bInitialized )
	{
		// load stats
		for ( int iStat = 0; iStat < m_iNumStats; ++iStat )
		{
			SteamStat_t &stat = m_pStats[iStat];
			switch ( stat.m_eStatType )
			{
				case STEAM_STAT_INT:
					SteamUserStats()->SetStat(stat.m_pchStatName, stat.m_iValue);
					break;
				case STEAM_STAT_FLOAT:
					SteamUserStats()->SetStat(stat.m_pchStatName, stat.m_flValue);
					break;
				case STEAM_STAT_AVGRATE:
					SteamUserStats()->UpdateAvgRateStat(stat.m_pchStatName, stat.m_flAvgNumerator, stat.m_flAvgDenominator);
					// The averaged result is calculated for us
					SteamUserStats()->GetStat(stat.m_pchStatName, &stat.m_flValue);
					break;
				default:
					break;
			}
		}
		return SteamUserStats()->StoreStats();
	}
	else
	{
		return false;
	}
}

void CSteamStatistics::OnUserStatsStored(UserStatsStored_t *pCallback)
{
	// we may get callbacks for other games' stats arriving, ignore them
	if ( pCallback->m_nGameID == STEAM_APPID )
	{
		if ( k_EResultOK == pCallback->m_eResult )
		{
			printlog("[STEAM]: successfully stored Steam user statistics.");
		}
		else if ( k_EResultInvalidParam == pCallback->m_eResult )
		{
			// One or more stats we set broke a constraint. They've been reverted,
			// and we should re-iterate the values now to keep in sync.
			printlog("[STEAM]: some Steam user statistics failed to validate, refreshing request!");
			// Fake up a callback here so that we re-load the values.
			UserStatsReceived_t callback;
			callback.m_eResult = k_EResultOK;
			callback.m_nGameID = STEAM_APPID;
			OnUserStatsReceived(&callback);
		}
		else
		{
			printlog("[STEAM]: unsuccessfully stored Steam user statistics!");
		}
	}
}

bool CSteamStatistics::ClearAllStats()
{
	if ( m_bInitialized )
	{
		if ( SteamUserStats()->ResetAllStats(false) )
		{
			RequestStats();
		}
	}
	else
	{
		return false;
	}
}

#endif // STEAMWORKS
