/*-------------------------------------------------------------------------------

BARONY
File: AppraisalGUI.hpp
Desc: AppraisalGUI class definition

Copyright 2013-2017 (c) Turning Wheel LLC, all rights reserved.
See LICENSE for details.

Additional Author(s): Lutz Kellen

-------------------------------------------------------------------------------*/

#pragma once

#include "interface.hpp"

namespace GUI
{

class AppraisalGUI
{

public:
	AppraisalGUI();
	~AppraisalGUI();

	/* AppraisalGUI.cpp
	 * @param appraisalItem - A pointer to the Item that is going to be appraised
	 * @param localPlayer - A pointer to the Entity of the Player who is appraising (Local Player only), used for accessing their skill and PER Stats
	 * Initializes the GUI, with two initial checks for instantaneous appraisal. If the Player's appraisal skill is >= 100 or the Item is a GEM_ROCK
	 * In the case of a non-instant appraisal, 'initialAppraisalTime' is calculated and 'remainingAppraisalTime' is set to 'initialAppraisalTime'
	 */
	void OpenGUI(Item* const apprisalItem, Entity* const localPlayer);
	/* AppraisalGUI.cpp
	 * Handles the updating of the appraisal timer by decreasing 'remainingAppraisalTime' by 1 every call (every tick)
	 * Once 'remainingAppraisalTime' == 0, IsAppraisalSuccessful() will be called. On success, the Item is Identified
	 * In both success and failure cases, the Player is messaged on the result, and AttemptSkillUp() is called before closing the GUI
	 */
	void UpdateGUI();
	/* AppraisalGUI.cpp
	 * Resets all member variables back to their base states
	 */
	void CloseGUI();
	/* AppraisalGUI.cpp
	 * Draws the AppraisalGUI box, along with the text and Item icon. The GUI's position is shifted if the Player's Inventory is open
	 */
	void DrawGUI();
	/* AppraisalGUI.cpp
	 * @returns bIsActive
	 * Returns the status of whether or not the AppraisalGUI is currently active
	 */
	bool IsGUIOpen() const;
	/* AppraisalGUI.cpp
	 * @param item - The Item being compared to 'itemToAppraise'
	 * @returns true - If the AppraisalGUI's currently appraising Item is the same as @item
	 * @returns false - If they are not the same
	 */
	bool IsItemBeingAppraised(const Item* const item) const;
	/* AppraisalGUI.cpp
	 * @param item - A pointer to the Item having it's appraisal time evaluated
	 * @returns true - If @item's appraisal time is lower than 'lowestAppraisalTime'
	 * @returns false - Default case
	 * Evaluates and compares @item's appraisal time to 'lowestAppraisalTime'. In the event that it is shorter, 'lowestAppraisalTime' is set to @item's appraisal time
	 * Used for finding the next target for auto-appraisal
	 */
	bool IsItemAppraisalTimeShortest(const Item* const item);

private:
	bool bIsActive; // Flag for whether or not the AppraisalGUI is currently displayed
	Uint32 initialAppraisalTime; // The initial appraisal time, calculated by GetGUIItemAppraisalTime()
	Uint32 remainingAppraisalTime; // The remaining time before the appraisal is finished, originally set to initialAppraisalTime
	Uint32 lowestAppraisalTime; // The lowest appraisal time out of all Items in the Player's Inventory that can be auto-appraised

	Item* itemToAppraise = nullptr; // A pointer to the Item being appraised
	Entity* localPlayerEntity = nullptr; // A pointer to the Player who is preforming the appraisal, used for getting the PER of the Player, and for increasing their skill

	/* AppraisalGUI.cpp
	 * @returns - The calcuated appraisal time based off of (value * 60) / (PRO_APPRAISAL + 1). Minimum of 1, maximum of 36,000 ticks
	 * Calculates the appraisal time of the selected appraisal target, 'itemToAppraise'. Value is used internally only
	 */
	Uint32 GetGUIItemAppraisalTime() const;
	/* AppraisalGUI.cpp
	 * @param item - A pointer to the Item that is having the appraisal time calculated
	 * @returns - The calcuated appraisal time based off of (value * 60) / (PRO_APPRAISAL + 1). Minimum of 1, maximum of 36,000 ticks
	 * Calculates the appraisal time of the given Item, @item. Value is used for IsItemAppraisalTimeShortest() for finding the next auto-appraise target
	 */
	Uint32 GetExternalItemAppraisalTime(const Item* const item);
	/* AppraisalGUI.cpp
	 * @returns true - If the Player's Appraisal Skill + (PER * 5) is >= 'itemToAppraise' value / 10 WHEN itemToAppraise->type != GEM_GLASS
	 * @returns true - If the Player's Appraisal Skill + (PER * 5) is >= 100 WHEN itemToAppraise->type == GEM_GLASS
	 * @returns false - Default case
	 * Calculates whether or not the appraisal is successful, based off the Player's appraisal skill and PER
	 */
	bool IsAppraisalSuccessful();
	/* AppraisalGUI.cpp
	 * Checks to see if the Player's appraisal skill should be increased. Only Items with a value above 0 will be considered
	 * If the Item was successfully appraised, then the Player's skill will be increased. Else, 1 in 5 chance of increasing their skill
	 */
	void AttemptSkillUp();
}; // class AppraisalGUI

} // namespace GUI