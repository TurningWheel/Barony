/*-------------------------------------------------------------------------------

BARONY
File: AppraisalGUI.cpp
Desc: AppraisalGUI class implementation

Copyright 2013-2017 (c) Turning Wheel LLC, all rights reserved.
See LICENSE for details.

Additional Author(s): Lutz Kellen

-------------------------------------------------------------------------------*/

#include "AppraisalGUI.hpp"
#include "../items.hpp"
#include "../net.hpp"

namespace GUI
{

AppraisalGUI::AppraisalGUI() :
	bIsActive(false),
	initialAppraisalTime(0),
	remainingAppraisalTime(0)
{
	lowestAppraisalTime = std::numeric_limits<int>::max();
} // AppraisalGUI()

AppraisalGUI::~AppraisalGUI()
{
	CloseGUI();
	itemToAppraise = nullptr;
	localPlayerEntity = nullptr;
} // ~AppraisalGUI()

/* AppraisalGUI.cpp
 * @param appraisalItem - A pointer to the Item that is going to be appraised
 * @param localPlayer - A pointer to the Entity of the Player who is appraising (Local Player only), used for accessing their skill and PER Stats
 * Initializes the GUI, with two initial checks for instantaneous appraisal. If the Player's appraisal skill is >= 100 or the Item is a GEM_ROCK
 * In the case of a non-instant appraisal, 'initialAppraisalTime' is calculated and 'remainingAppraisalTime' is set to 'initialAppraisalTime'
 */
void AppraisalGUI::OpenGUI(Item* const appraisalItem, Entity* const localPlayer)
{
	// If a GUI is already opened, close the GUI before opening the new one
	if ( bIsActive == true )
	{
		CloseGUI();
	}

	// Update the Appraisal GUI
	bIsActive = true;
	itemToAppraise = appraisalItem;
	localPlayerEntity = localPlayer;

	// If the Player has is Legendary in Appraisal then complete the action immediately
	if ( stats[clientnum]->PROFICIENCIES[PRO_APPRAISAL] >= CAPSTONE_UNLOCK_LEVEL[PRO_APPRAISAL] )
	{
		itemToAppraise->identified = true;
		messagePlayer(clientnum, language[320], itemToAppraise->description()); // "Identified "%s"."
		AttemptSkillUp();
		CloseGUI();
		return;
	}
	else if ( itemToAppraise->type == GEM_ROCK ) // Rocks are appraised instantly
	{
		itemToAppraise->identified = true;
		messagePlayer(clientnum, language[320], itemToAppraise->description()); // "Identified "%s"."
		AttemptSkillUp();
		CloseGUI();
		return;
	}

	// Since it wasn't autocompleted, initialize the appraisal timer
	initialAppraisalTime = GetGUIItemAppraisalTime();
	remainingAppraisalTime = initialAppraisalTime;

	messagePlayer(clientnum, language[321], appraisalItem->description()); // "Appraising "%s"..."
} // OpenGUI()

/* AppraisalGUI.cpp
 * Handles the updating of the appraisal timer by decreasing 'remainingAppraisalTime' by 1 every call (every tick)
 * Once 'remainingAppraisalTime' == 0, IsAppraisalSuccessful() will be called. On success, the Item is Identified
 * In both success and failure cases, the Player is messaged on the result, and AttemptSkillUp() is called before closing the GUI
 */
void AppraisalGUI::UpdateGUI()
{
	// If the GUI is not active, there is no reason to go further
	if ( bIsActive != true )
	{
		return;
	}

	// The GUI should never be active with the itemToAppraise being null
	if ( itemToAppraise == nullptr )
	{
		printlog("ERROR: updateAppraisalGUI() - itemToAppraise is null.");
		CloseGUI();
		return;
	}

	remainingAppraisalTime--; // Tick downward while appraising

	if ( remainingAppraisalTime == 0 )
	{
		if ( IsAppraisalSuccessful() == true )
		{
			itemToAppraise->identified = true;
			messagePlayer(clientnum, language[320], itemToAppraise->description()); // "Identified "%s"."
			AttemptSkillUp();
			CloseGUI();
		}
		else
		{
			messagePlayer(clientnum, language[571], itemToAppraise->description()); // "Failed to identify "%s"."
			AttemptSkillUp();
			CloseGUI();
		}
	}
} // UpdateGUI()

/* AppraisalGUI.cpp
 * Resets all member variables back to their base states
 */
void AppraisalGUI::CloseGUI()
{
	bIsActive = false;
	initialAppraisalTime = 0;
	remainingAppraisalTime = 0;
	lowestAppraisalTime = std::numeric_limits<int>::max();

	itemToAppraise = nullptr;
	localPlayerEntity = nullptr;
} // CloseGUI()

/* AppraisalGUI.cpp
 * Draws the AppraisalGUI box, along with the text and Item icon. The GUI's position is shifted if the Player's Inventory is open
 * Will not draw anything if AppraisalGUI is not active
 */
void AppraisalGUI::DrawGUI()
{
	if ( bIsActive == false )
	{
		return;
	}

	if ( itemToAppraise == nullptr )
	{
		CloseGUI();
		return;
	}

	// The offset of the Appraisal GUI when the Inventory is open
	const Sint32 appraisalBox_OffsetX = ((xres) / 2 - (INVENTORY_SIZEX)*(INVENTORY_SLOTSIZE) / 2 - inventory_mode_item_img->w / 2);
	const Sint32 appraisalBox_OffsetY = 10;

	// Draw the Appraisal GUI Box
	SDL_Rect appraisalBoxRect;

	// If the Player is in an Inventory, offset so it isn't in the way
	if ( shootmode == false )
	{
		appraisalBoxRect.x = appraisalBox_OffsetX + 16;
		appraisalBoxRect.y = appraisalBox_OffsetY + INVENTORY_SIZEY * INVENTORY_SLOTSIZE + 16;
	}
	else
	{
		appraisalBoxRect.x = 16;
		appraisalBoxRect.y = 16;
	}

	int appraisingTextWidth = 0; // The width of the text "Appraising:     "
	int itemNameTextWidth = 0; // The width of the text of the name of the Item being appraised

							   // Get the size of the text within the Appraisal GUI Box
	TTF_SizeUTF8(ttf12, language[340], &appraisingTextWidth, nullptr);
	TTF_SizeUTF8(ttf12, itemToAppraise->getName(), &itemNameTextWidth, nullptr);

	// Resize the Appraisal GUI Box based on the text within it
	itemNameTextWidth += 48;
	appraisalBoxRect.w = std::max(appraisingTextWidth, itemNameTextWidth) + 8;
	appraisalBoxRect.h = 68;

	// Draw the actual Appraisal GUI Box
	drawTooltip(&appraisalBoxRect);

	// Draw the appraising timer % in the Appraisal GUI Box
	char appraisalBoxFormattedText[64] = {0};
	snprintf(appraisalBoxFormattedText, 63, language[341], ((static_cast<real_t>((initialAppraisalTime - remainingAppraisalTime))) / (static_cast<real_t>(initialAppraisalTime))) * 100); // "Appraising:%3.0f%%"
	ttfPrintText(ttf12, appraisalBoxRect.x + 8, appraisalBoxRect.y + 8, appraisalBoxFormattedText);

	// Draw the Item's Sprite and name
	//SDL_Rect itemSpriteRect;

	// If the Player is in an Inventory, offset so it isn't in the way
	if ( shootmode == false )
	{
		appraisalBoxRect.x = appraisalBox_OffsetX + 24;
		appraisalBoxRect.y = appraisalBox_OffsetY + INVENTORY_SIZEY * INVENTORY_SLOTSIZE + 16 + 24;
	}
	else
	{
		appraisalBoxRect.x = 24;
		appraisalBoxRect.y = 16 + 24;
	}

	// Draw the name of the Item being appraised
	ttfPrintText(ttf12, appraisalBoxRect.x + 40, appraisalBoxRect.y + 8, itemToAppraise->getName());
	appraisalBoxRect.w = 32;
	appraisalBoxRect.h = 32;

	// Draw the Sprite of the Item being appraised
	drawImageScaled(itemSprite(itemToAppraise), nullptr, &appraisalBoxRect);
} // DrawGUI()

/* AppraisalGUI.cpp
 * @returns bIsActive
 * Returns the status of whether or not the AppraisalGUI is currently active
 */
bool AppraisalGUI::IsGUIOpen() const
{
	return bIsActive;
} // IsGUIOpen()

/* AppraisalGUI.cpp
 * @param item - The Item being compared to 'itemToAppraise'
 * @returns true - If the AppraisalGUI's currently appraising Item is the same as @item
 * @returns false - If they are not the same
 */
bool AppraisalGUI::IsItemBeingAppraised(const Item* const item) const
{
	return (itemToAppraise == item);
} // IsItemBeingAppraised()

/* AppraisalGUI.cpp
 * @param item - A pointer to the Item having it's appraisal time evaluated
 * @returns true - If @item's appraisal time is lower than 'lowestAppraisalTime'
 * @returns false - Default case
 * Evaluates and compares @item's appraisal time to 'lowestAppraisalTime'. In the event that it is shorter, 'lowestAppraisalTime' is set to @item's appraisal time
 * Used for finding the next target for auto-appraisal
 */
bool AppraisalGUI::IsItemAppraisalTimeShortest(const Item* const item)
{
	Uint32 itemAppraisalTime = GetExternalItemAppraisalTime(item);

	// Check if the Item's appraisal time is lower than the previous lowest time
	if ( itemAppraisalTime < lowestAppraisalTime )
	{
		lowestAppraisalTime = itemAppraisalTime;
		return true;
	}

	return false;
} // IsItemAppraisalTimeShortest()

// PRIVATE FUNCTIONS

/* AppraisalGUI.cpp
 * @returns - The calcuated appraisal time based off of (value * 60) / (PRO_APPRAISAL + 1). Minimum of 1, maximum of 36,000 ticks
 * Calculates the appraisal time of the selected appraisal target, 'itemToAppraise'. Value is used internally only
 */
Uint32 AppraisalGUI::GetGUIItemAppraisalTime() const
{
	Uint32 calculatedTime = 0; // The time calculated based on the type and value of the Item and Proficiency of the Player

	if ( itemToAppraise->type != GEM_GLASS )
	{
		// All Items other than Worthless glass have an appraisal time based on their value, reduced by the Player's proficiency
		calculatedTime = (items[itemToAppraise->type].value * 60) / (stats[clientnum]->PROFICIENCIES[PRO_APPRAISAL] + 1);
	}
	else
	{
		// Worthless glass has a flat time of 60,000 ticks, reduced by the Player's proficiency
		calculatedTime = (1000 * 60) / (stats[clientnum]->PROFICIENCIES[PRO_APPRAISAL] + 1);
	}

	// Appraisal time has a minimum of 1 tick and a maximum of 36,000 ticks
	calculatedTime = std::min(std::max(1, static_cast<int>(calculatedTime)), 36000);

	return calculatedTime;
} // GetGUIItemAppraisalTime()

/* AppraisalGUI.cpp
 * @param item - A pointer to the Item that is having the appraisal time calculated
 * @returns - The calcuated appraisal time based off of (value * 60) / (PRO_APPRAISAL + 1). Minimum of 1, maximum of 36,000 ticks
 * Calculates the appraisal time of the given Item, @item. Value is used for IsItemAppraisalTimeShortest() for finding the next auto-appraise target
 */
Uint32 AppraisalGUI::GetExternalItemAppraisalTime(const Item* const item)
{
	Uint32 calculatedTime = 0; // The time calculated based on the type and value of the Item and Proficiency of the Player

	if ( item->type != GEM_GLASS )
	{
		// All Items other than Worthless glass have an appraisal time based on their value, reduced by the Player's proficiency
		calculatedTime = (items[item->type].value * 60) / (stats[clientnum]->PROFICIENCIES[PRO_APPRAISAL] + 1);
	}
	else
	{
		// Worthless glass has a flat time of 60,000 ticks, reduced by the Player's proficiency
		calculatedTime = (1000 * 60) / (stats[clientnum]->PROFICIENCIES[PRO_APPRAISAL] + 1);
	}

	// Appraisal time has a minimum of 1 tick and a maximum of 36,000 ticks
	calculatedTime = std::min(std::max(1, static_cast<int>(calculatedTime)), 36000);

	return calculatedTime;
} // GetExternalItemAppraisalTime()

/* AppraisalGUI.cpp
 * @returns true - If the Player's Appraisal Skill + (PER * 5) is >= 'itemToAppraise' value / 10 WHEN itemToAppraise->type != GEM_GLASS
 * @returns true - If the Player's Appraisal Skill + (PER * 5) is >= 100 WHEN itemToAppraise->type == GEM_GLASS
 * @returns false - Default case
 * Calculates whether or not the appraisal is successful, based off the Player's appraisal skill and PER
 */
bool AppraisalGUI::IsAppraisalSuccessful()
{
	bool bIsAppraisalSuccessful = false;

	if ( itemToAppraise->type != GEM_GLASS )
	{
		// For Items other than Worthless glass, (Prof + (PER * 5)) must be >= to (ItemValue / 10)
		bIsAppraisalSuccessful = ((stats[clientnum]->PROFICIENCIES[PRO_APPRAISAL] + (localPlayerEntity->getPER() * 5)) >= (items[itemToAppraise->type].value / 10));
	}
	else
	{
		// For Worthless glass, (Prof + (PER * 5)) must be >= to 100
		bIsAppraisalSuccessful = ((stats[clientnum]->PROFICIENCIES[PRO_APPRAISAL] + (localPlayerEntity->getPER() * 5)) >= 100);
	}

	return bIsAppraisalSuccessful;
} // IsAppraisalSuccessful()

/* AppraisalGUI.cpp
 * Checks to see if the Player's appraisal skill should be increased. Only Items with a value above 0 will be considered
 * If the Item was successfully appraised, then the Player's skill will be increased. Else, 1 in 5 chance of increasing their skill
 */
void AppraisalGUI::AttemptSkillUp()
{
	if ( items[itemToAppraise->type].value > 0 )
	{
		// If the Player succeeded in appraising the Item, increase their skill
		if ( itemToAppraise->identified )
		{
			localPlayerEntity->increaseSkill(PRO_APPRAISAL);
		}
		else if ( rand() % 5 == 0 ) // Else random chance for an increase
		{
			localPlayerEntity->increaseSkill(PRO_APPRAISAL);
		}
	}
} // AttemptSkillUp()

} // namespace GUI