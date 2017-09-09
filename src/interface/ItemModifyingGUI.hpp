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

    void openItemModifyingGUI(const Uint8 GUIType, Item* const scrollUsed);
    void updateItemModifyingGUI();
    bool areThereValidItems(const Uint8 GUIType);

private:
    // ItemModifying GUI
    static const Uint8 NUM_ITEM_MODIFYING_GUI_ITEMS = 4; // The maximum number of Items that can be displayed in the window

    bool bIsActive; // Flag for whether or not the ItemModifyingGUI is currently displayed
    bool bIsCursed; // Flag for whether or not the Scroll used is Cursed. If it is, bypasses the GUI and skips to processing
    bool bIsItemModifyingGUI_Dragging; // Flag for whether or not the GUI Window is being dragged around the Screen
    Sint8 itemModifyingGUI_ScrollBeatitude; // The Beatitude value of the Scroll being used
    Uint8 itemModifyingGUI_Type; // Which type of GUI is being displayed? 0, 1, 2, 3, 4 - Identify, Remove Curse, Repair, Enchant Weapon, Enchant Armor
    Uint8 itemModifyingGUI_InventoryScrollOffset; // The amount Items existing past the limit of visible Items #TODOR: I actually am not 100% certain what this is
    Uint8 itemModifyingGUI_InventorySelectedSlot; // The GUI Inventory Slot that has been selected by the Player for processing
    Sint32 itemModifyingGUI_OffsetX; // The GUI Window's X offset, to allow the Window to be moved around the screen. Offset is remembered for future use
    Sint32 itemModifyingGUI_OffsetY; // The GUI Window's Y offset, to allow the Window to be moved around the screen. Offset is remembered for future use

    SDL_Surface* itemModifyingGUI_IMG = nullptr; // The background image for Identify, Remove Curse, and Repair GUIs
    Item* itemModifyingGUI_Inventory[NUM_ITEM_MODIFYING_GUI_ITEMS] = {nullptr}; // The GUI Inventory which holds all of the Player's Inventory Items that match the given GUIType
    Item* itemModifyingGUI_ScrollUsed = nullptr; // A pointer to the Item of the Scroll being used to open the GUI. If the Scroll finishes it's processing, the Item will be deleted

    void closeItemModifyingGUI();

    Item* getItemInfoFromGUI(Uint8 slot);

    void processGUIEffectOnItem(Item* const selectedItem);
    void itemModifyingGUIProcessRandomItem();
    void rebuildItemModifyingGUIInventory();
    void warpMouseToSelectedGUISlot();

    // Display Update Handlers
    void itemModifyingGUI_HandleButtons();
    void itemModifyingGUI_HandleMouseWheel();
    void itemModifyingGUI_HandleDraggingBar();
    void itemModifyingGUI_HandleButtonImages(const Sint32 GUIPosX, const Sint32 GUIPosY);
    void itemModifyingGUI_HandleItemImages();

    // Identify GUI
    void processIdentifyGUIEffectOnItem(Item* const selectedItem);
    void identifyGUIProcessRandomItem(const Uint8 numItems);
    void rebuildIdentifyGUIInventory();
    void identifyGUI_HandleItemImages();

    // Remove Curse GUI
    void processRemoveCurseGUIEffectOnItem(Item* const selectedItem);
    void removeCurseGUIProcessRandomItem(const Uint8 numItems);
    void rebuildRemoveCurseGUIInventory();
    void removeCurseGUI_HandleItemImages();

    // Repair GUI
    void processRepairGUIEffectOnItem(Item* const selectedItem);
    void repairGUIProcessRandomItem(const Uint8 numItems);
    void rebuildRepairGUIInventory();
    void repairGUI_HandleItemImages();
};
}