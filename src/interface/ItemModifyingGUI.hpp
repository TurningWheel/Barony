/*-------------------------------------------------------------------------------

BARONY
File: ItemModifyingGUI.hpp
Desc: ItemModifyingGUI class definition

Copyright 2013-2017 (c) Turning Wheel LLC, all rights reserved.
See LICENSE for details.

Additional Author(s): Lutz Kellen

-------------------------------------------------------------------------------*/

#pragma once

#include "interface.hpp"

namespace GUI
{

class ItemModifyingGUI
{
public:
    ItemModifyingGUI();
    ~ItemModifyingGUI();

    /* ItemModifyingGUI.cpp
     * @param GUIType - The type of GUI to be opened: 0,1,2,3,4 - Identify, Remove Curse, Repair, Enchant Weapon, Enchant Armor
     * @param scrollUsed - A pointer to the Item being used to open the GUI. The Item will be deleted if processing completes successfully. Use nullptr if a Spell is used to open the GUI
     * Initializes the GUI, sets 'shootmode' to false, and 'gui_mode' to 'GUI_MODE_INVENTORY'. The GUI Inventory will be built using 'buildItemModifyingGUIInventory()'
     * In the case of a Spell, itemModifyingGUI_ScrollBeatitude will be 0, as Spells do not give any extra or negative effects
     * If a ItemModifyingGUI is already opened, it will be closed before opening the new one
     * If a Unidentified Scroll is used, it will be Identified here and message the Player accordingly
     */
    void openItemModifyingGUI(const Uint8 GUIType, Item* const scrollUsed);
    /* ItemModifyingGUI.cpp
     * Handles the Drawing of the GUI, along with setting up and processing the Mouse input collision bounds through their various function calls
     * Handles the GUI Inventory slot bounds and Mouse input to call processGUIEffectOnItem()
     */
    void updateItemModifyingGUI();
    /* ItemModifyingGUI.cpp
     * Resets all member variables back to their base states
     */
    void closeItemModifyingGUI();
    /* ItemModifyingGUI.cpp
     * @param direction - Will either be -1 for Up or 1 for Down. The direction in which the User wants to move the cursor
     * Called by GameController::handleItemModifyingGUIMovement(). Evaluates the requested movement, updating 'itemModifyingGUI_InventorySelectedSlot' as needed
     */
    void gamepadMoveCursor(Sint8 direction);
    /* ItemModifyingGUI.cpp
     * @returns 'bIsActive'
     */
    bool isActive() const;
    /* ItemModifyingGUI.cpp
     * @param GUIType - The type of GUI to be opened: 0,1,2,3,4 - Identify, Remove Curse, Repair, Enchant Weapon, Enchant Armor
     * @returns true - If the Player's Inventory has valid Items for processing
     * @returns false - If the Player's Inventory has no valid Items for processing
     * The main usage of this function is to prevent a Spell from being cast needlessly
     * Sets 'itemModifyingGUI_Type' = @GUIType. 'itemModifyingGUI_Type' is set back to default value before returning via closeItemModifyingGUI()
     */
    bool areThereValidItems(const Uint8 GUIType);
    /* ItemModifyingGUI.cpp
     * @returns true - If 'itemModifyingGUI_InventorySelectedSlot' is < 0
     * @returns false - If 'itemModifyingGUI_InventorySelectedSlot' is 0 or greater
     * Returns whether or not the mouse is currently hovering over a GUI Inventory Slot
     */
    bool isSelectedSlotInvalid() const;
    /* ItemModifyingGUI.cpp
     * @returns true - If the Mouse is currently within the bounds of the GUI
     * @returns false - If the Mouse is not within the bounds of the GUI
     * Returns whether or not the Mouse is currently within the bounds of the ItemModifyingGUI
     */
    bool isMouseWithinGUIBounds() const;

private:
    // ItemModifying GUI
    static const Uint8 NUM_ITEM_MODIFYING_GUI_ITEMS = 4; // The maximum number of Items that can be displayed in the window

    bool bIsActive; // Flag for whether or not the ItemModifyingGUI is currently displayed
    bool bIsCursed; // Flag for whether or not the Scroll used is Cursed. If it is, bypasses the GUI and skips to processing
    bool bIsItemModifyingGUI_Dragging; // Flag for whether or not the GUI Window is being dragged around the Screen
    Sint16 itemModifyingGUI_ScrollBeatitude; // The Beatitude value of the Scroll being used
    Uint8 itemModifyingGUI_Type; // Which type of GUI is being displayed? 0, 1, 2, 3, 4 - Identify, Remove Curse, Repair, Enchant Weapon, Enchant Armor
    Uint8 itemModifyingGUI_InventoryScrollOffset; // The amount Items existing past the limit of visible Items #TODOR: I actually am not 100% certain what this is
    Sint8 itemModifyingGUI_InventorySelectedSlot; // The GUI Inventory Slot that has been selected by the Player for processing
    Sint32 itemModifyingGUI_OffsetX; // The GUI Window's X offset, to allow the Window to be moved around the screen. Offset is remembered for future use
    Sint32 itemModifyingGUI_OffsetY; // The GUI Window's Y offset, to allow the Window to be moved around the screen. Offset is remembered for future use

    SDL_Surface* itemModifyingGUI_IMG = nullptr; // The background image for Identify, Remove Curse, and Repair GUIs
    Item* itemModifyingGUI_Inventory[NUM_ITEM_MODIFYING_GUI_ITEMS] = {nullptr}; // The GUI Inventory which holds all of the Player's Inventory Items that match the given GUIType
    Item* itemModifyingGUI_ScrollUsed = nullptr; // A pointer to the Item of the Scroll being used to open the GUI. If the Scroll finishes it's processing, the Item will be deleted

    /* ItemModifyingGUI.cpp
     *  Used to get the reference to the Item in the given slot in itemModifyingGUI_Inventory[]
     *  @param int slot - The slot to be accessed in itemModifyingGUI_Inventory[]
     *  @returns The Item* from itemModifyingGUI_Inventory[slot]
     *  @returns nullptr if slot >= 4 (Out of bounds)
     */
    Item* getItemInfoFromGUI(Uint8 slot);

    /* ItemModifyingGUI.cpp
     * @param selectedItem - The Item that is being processed, selected from the GUI Inventory
     * Selects the correct function to process the Item according to 'itemModifyingGUI_Type'
     */
    void processGUIEffectOnItem(Item* const selectedItem);
    /* ItemModifyingGUI.cpp
     * Used for Cursed Scrolls. Selects the correct function to randomly process the Item(s) according to 'itemModifyingGUI_Type'
     */
    void itemModifyingGUIProcessRandomItem();
    /* ItemModifyingGUI.cpp
     * Selects the correct function to rebuild the GUI Inventory according to 'itemModifyingGUI_Type'
     */
    void rebuildItemModifyingGUIInventory();
    /* ItemModifyingGUI.cpp
     * Calls SDL_WarpMouseInWindow() to move the Mouse cursor to the currently selected GUI slot for controllers
     */
    void warpMouseToSelectedGUISlot();

    // Display Update Handlers
    /* ItemModifyingGUI.cpp
     * Handles setting up the collision bounds and checking for Mouse input on those collision bounds for each of the GUI's buttons
     * The Top Bar of the GUI used for dragging is considered a button
     */
    void itemModifyingGUI_HandleButtons();
    /* ItemModifyingGUI.cpp
     * Handles setting up the collision bounds and checking for Mouse input on those collision bounds for Mouse scroll wheel input
     */
    void itemModifyingGUI_HandleMouseWheel();
    /* ItemModifyingGUI.cpp
     * Handles the actual movement of the GUI Window by modifying 'itemModifyingGUI_OffsetX/Y' when 'bIsItemModifyingGUI_Dragging' == true
     */
    void itemModifyingGUI_HandleDraggingBar();
    /* ItemModifyingGUI.cpp
     * @param GUIPosX - The GUI position in the center of the screen + itemModifyingGUI_OffsetX
     * @param GUIPosY - The GUI position in the center of the screen + itemModifyingGUI_OffsetY
     * Handles drawing the actual GUI button Images when they are being clicked. The unclicked versions are part of the original Image
     */
    void itemModifyingGUI_HandleButtonImages(const Sint32 GUIPosX, const Sint32 GUIPosY);
    /* ItemModifyingGUI.cpp
     * Rebuilds the GUI Inventory before selecting the correct function to render the Images for each Item according to 'itemModifyingGUI_Type'
     */
    void itemModifyingGUI_HandleItemImages();

    // Identify GUI
    /* ItemModifyingGUI.cpp
     * @param selectedItem - The Item that is being processed, selected from the GUI Inventory
     * Identifies the selected Item based on 'itemModifyingGUI_ScrollBeatitude'. If 'itemModifyingGUI_ScrollBeatitude' is > 0 then identifyGUIProcessRandomItem() will be called
     * Messages the Player that their Item has been Identified, then if the GUI was opened with a Scroll, it is consumed before closing the GUI
     */
    void processIdentifyGUIEffectOnItem(Item* const selectedItem);
    /* ItemModifyingGUI.cpp
     * @param numItems - The number of random Items to be processed. Currently this number should never exceed 2, as the highest naturally spawning Scrolls are +-2
     * Processes a random amount of valid Items equal to 'numItems'. Processing will either Identify or Unidentify the random Items according to 'itemModifyingGUI_ScrollBeatitude'
     * Messages the Player that their Item has been Identified or Unidentified because of the beatitude of the Scroll used
     */
    void identifyGUIProcessRandomItem(const Uint8 numItems);
    /* ItemModifyingGUI.cpp
     * Builds the GUI's Inventory based off of 'itemModifyingGUI_Type' and 'itemModifyingGUI_ScrollBeatitude'
     * Counts the number of Items to add to the GUI's Inventory, then assigns the visible ones to their position in 'itemModifyingGUI_Inventory[]'
     */
    void rebuildIdentifyGUIInventory();
    /* ItemModifyingGUI.cpp
     * Draws the Sprite Image and description for the given Item in each of the visible GUI Inventory slots
     */
    void identifyGUI_HandleItemImages();

    // Remove Curse GUI
    /* ItemModifyingGUI.cpp
     * @param selectedItem - The Item that is being processed, selected from the GUI Inventory
     * Uncurses the selected Item based on 'itemModifyingGUI_ScrollBeatitude'. If 'itemModifyingGUI_ScrollBeatitude' is > 0 then identifyGUIProcessRandomItem() will be called
     * Messages the Player that their Item has been Identified, then if the GUI was opened with a Scroll, it is consumed before closing the GUI
     */
    void processRemoveCurseGUIEffectOnItem(Item* const selectedItem);
    /* ItemModifyingGUI.cpp
     * @param numItems - The number of random Items to be processed. Currently this number should never exceed 2, as the highest naturally spawning Scrolls are +-2
     * Processes a random amount of valid Items equal to 'numItems'. Processing will either Uncurse or Curse the random Items according to 'itemModifyingGUI_ScrollBeatitude'
     * Messages the Player that their Item has been Uncursed or Cursed because of the beatitude of the Scroll used
     */
    void removeCurseGUIProcessRandomItem(const Uint8 numItems);
    /* ItemModifyingGUI.cpp
     * Builds the GUI's Inventory based off of 'itemModifyingGUI_Type' and 'itemModifyingGUI_ScrollBeatitude'
     * Counts the number of Items to add to the GUI's Inventory, then assigns the visible ones to their position in 'itemModifyingGUI_Inventory[]'
     */
    void rebuildRemoveCurseGUIInventory();
    /* ItemModifyingGUI.cpp
     * Draws the Sprite Image and description for the given Item in each of the visible GUI Inventory slots
     */
    void removeCurseGUI_HandleItemImages();

    // Repair GUI
    /* ItemModifyingGUI.cpp
     * @param selectedItem - The Item that is being processed, selected from the GUI Inventory
     * Repairs the selected Item based on 'itemModifyingGUI_ScrollBeatitude' and 'itemModifyingGUI_ScrollBeatitude'
     * +0 Scrolls cannot repair Broken or Serviceable Items. +0 can repair Decrepit and Worn up to Serviceable
     * +1 Scrolls cannot repair Serviceable Items. +1 can repair Broken, Decrepit, and Worn up to Serviceable
     * +2 Scrolls can repair all Items up to Excellent
     * Messages the Player that their Item has been Repaired, then if the GUI was opened with a Scroll, it is consumed before closing the GUI
     */
    void processRepairGUIEffectOnItem(Item* const selectedItem);
    /* ItemModifyingGUI.cpp
     * @param numItems - The number of random Items to be processed. Currently this number should never exceed 2, as the highest naturally spawning Scrolls are +-2
     * Processes a random amount of valid Items equal to 'numItems'. Processing will reduce the random Items to Broken
     * Messages the Player that their Item has been Broken because of the beatitude of the Scroll used
     */
    void repairGUIProcessRandomItem(const Uint8 numItems);
    /* ItemModifyingGUI.cpp
     * Builds the GUI's Inventory based off of 'itemModifyingGUI_Type' and 'itemModifyingGUI_ScrollBeatitude'
     * Counts the number of Items to add to the GUI's Inventory, then assigns the visible ones to their position in 'itemModifyingGUI_Inventory[]'
     */
    void rebuildRepairGUIInventory();
    /* ItemModifyingGUI.cpp
     * Draws the Sprite Image and description for the given Item in each of the visible GUI Inventory slots
     */
    void repairGUI_HandleItemImages();

    // Enchant Weapon GUI
    /* ItemModifyingGUI.cpp
     * @param selectedItem - The Item that is being processed, selected from the GUI Inventory
     * Enchants the selected Item based on 'itemModifyingGUI_ScrollBeatitude' and 'itemModifyingGUI_ScrollBeatitude'
     * +0 Scrolls can raise a +0 or +1 weapon up to +2. Cannot Enchant Artifacts
     * +1 Scrolls can raise a +0, +1, or +2 weapon up to +3. Cannot Enchant Artifacts
     * +2 Scrolls can raise a +0, +1, +2, +3, or +4 weapon up to +5. Raises any Artifact Weapon by one level
     * Messages the Player that their Item has been Enchanted, then if the GUI was opened with a Scroll, it is consumed before closing the GUI
     */
    void processEnchantWeaponGUIEffectOnItem(Item* const selectedItem);
    /* ItemModifyingGUI.cpp
     * @param numItems - The number of random Items to be processed. Currently this number should never exceed 2, as the highest naturally spawning Scrolls are +-2
     * Processes a random amount of valid Items equal to 'numItems'. Processing will reduce the random Items to +0
     * Messages the Player that their Item has been Disenchanted because of the beatitude of the Scroll used
     */
    void enchantWeaponGUIProcessRandomItem(const Uint8 numItems);
    /* ItemModifyingGUI.cpp
     * Builds the GUI's Inventory based off of 'itemModifyingGUI_Type' and 'itemModifyingGUI_ScrollBeatitude'
     * Counts the number of Items to add to the GUI's Inventory, then assigns the visible ones to their position in 'itemModifyingGUI_Inventory[]'
     */
    void rebuildEnchantWeaponGUIInventory();
    /* ItemModifyingGUI.cpp
     * Draws the Sprite Image and description for the given Item in each of the visible GUI Inventory slots
     */
    void enchantWeaponGUI_HandleItemImages();

    // Enchant Armor GUI
    /* ItemModifyingGUI.cpp
     * @param selectedItem - The Item that is being processed, selected from the GUI Inventory
     * Enchants the selected Item based on 'itemModifyingGUI_ScrollBeatitude' and 'itemModifyingGUI_ScrollBeatitude'
     * +0 Scrolls can raise a +0 or +1 Shield, Breastpiece, or Helm up to +2. Cannot enchant Artifacts
     * +1 Scrolls can raise a +0, +1, or +2 Shield, Breastpiece, Helm, Boots, or Gloves up to +3. Cannot enchant Artifacts
     * +2 Scrolls can raise a +0, +1, +2, +3, or +4 Shield, Breastpiece, Helm, Boots, Gloves, Rings, or Cloaks up to +5. Raises any Artifact Armor by one level
     * Messages the Player that their Item has been Enchanted, then if the GUI was opened with a Scroll, it is consumed before closing the GUI
     */
    void processEnchantArmorGUIEffectOnItem(Item* const selectedItem);
    /* ItemModifyingGUI.cpp
     * @param numItems - The number of random Items to be processed. Currently this number should never exceed 2, as the highest naturally spawning Scrolls are +-2
     * Processes a random amount of valid Items equal to 'numItems'. Processing will reduce the random Items to +0
     * Messages the Player that their Item has been Disenchanted because of the beatitude of the Scroll used
     */
    void enchantArmorGUIProcessRandomItem(const Uint8 numItems);
    /* ItemModifyingGUI.cpp
     * Builds the GUI's Inventory based off of 'itemModifyingGUI_Type' and 'itemModifyingGUI_ScrollBeatitude'
     * Counts the number of Items to add to the GUI's Inventory, then assigns the visible ones to their position in 'itemModifyingGUI_Inventory[]'
     */
    void rebuildEnchantArmorGUIInventory();
    /* ItemModifyingGUI.cpp
     * Draws the Sprite Image and description for the given Item in each of the visible GUI Inventory slots
     */
    void enchantArmorGUI_HandleItemImages();
}; // class ItemModifyingGUI

} // namespace GUI