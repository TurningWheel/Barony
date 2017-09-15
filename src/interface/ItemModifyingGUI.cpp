/*-------------------------------------------------------------------------------

BARONY
File: ItemModifyingGUI.cpp
Desc: ItemModifyingGUI class implementation

Copyright 2013-2017 (c) Turning Wheel LLC, all rights reserved.
See LICENSE for details.

Additional Author(s): Lutz Kellen

-------------------------------------------------------------------------------*/

#include "ItemModifyingGUI.hpp"
#include "../player.hpp"
#include "../stat.hpp"
#include "../net.hpp"
#include "../items.hpp"

namespace GUI
{

ItemModifyingGUI::ItemModifyingGUI() :
    bIsActive(false),
    bIsCursed(false),
    bIsItemModifyingGUI_Dragging(false),
    itemModifyingGUI_ScrollBeatitude(0),
    itemModifyingGUI_Type(0),
    itemModifyingGUI_InventoryScrollOffset(0),
    itemModifyingGUI_InventorySelectedSlot(-1),
    itemModifyingGUI_OffsetX(0),
    itemModifyingGUI_OffsetY(0)
{
    itemModifyingGUI_IMG = loadImage("images/system/itemModifyingGUI.png");
} // ItemModifyingGUI()

ItemModifyingGUI::~ItemModifyingGUI()
{
    closeItemModifyingGUI();
    itemModifyingGUI_IMG = nullptr;
} // ~ItemModifyingGUI()

/* ItemModifyingGUI.cpp
 * @param GUIType - The type of GUI to be opened: 0,1,2,3,4 - Identify, Remove Curse, Repair, Enchant Weapon, Enchant Armor
 * @param scrollUsed - A pointer to the Item being used to open the GUI. The Item will be deleted if processing completes successfully. Use nullptr if a Spell is used to open the GUI
 * Initializes the GUI, sets 'shootmode' to false, and 'gui_mode' to 'GUI_MODE_INVENTORY'. The GUI Inventory will be built using 'buildItemModifyingGUIInventory()'
 * In the case of a Spell, itemModifyingGUI_ScrollBeatitude will be 0, as Spells do not give any extra or negative effects
 * If a ItemModifyingGUI is already opened, it will be closed before opening the new one
 * If a Unidentified Scroll is used, it will be Identified here and message the Player accordingly
 */
void ItemModifyingGUI::openItemModifyingGUI(const Uint8 GUIType, Item* const scrollUsed)
{
    // If a GUI is already opened, close the GUI before opening the new one
    if ( bIsActive == true )
    {
        closeItemModifyingGUI();
    }

    // Initialize the values for the GUI
    bIsActive = true;
    
    itemModifyingGUI_Type = GUIType;
    itemModifyingGUI_ScrollUsed = scrollUsed;

    // Check to see if a Scroll is being used to open the GUI
    if ( itemModifyingGUI_ScrollUsed != nullptr )
    {
        itemModifyingGUI_ScrollBeatitude = itemModifyingGUI_ScrollUsed->beatitude;
        if ( itemModifyingGUI_ScrollBeatitude < 0 )
        {
            bIsCursed = true;
        }

        // If the Scroll is Unidentified, Identify it and message the Player what type of Scroll it is
        if ( itemModifyingGUI_ScrollUsed->identified == false )
        {
            itemModifyingGUI_ScrollUsed->identified = true;
            switch ( itemModifyingGUI_Type )
            {
                case 0: // Identify
                    messagePlayer(clientnum, language[849]); // "This is a scroll of identify!"
                    break;
                case 1: // Remove Curse
                    messagePlayer(clientnum, language[2501]); // "This is a scroll of remove curse!"
                    break;
                case 2: // Repair
                    messagePlayer(clientnum, language[2502]); // "This is a scroll of repair!"
                    break;
                case 3: // Enchant Weapon
                    messagePlayer(clientnum, language[2503]); // "This is a scroll of Enchant weapon!"
                    break;
                case 4: // Enchant Armor
                    messagePlayer(clientnum, language[2504]); // "This is a scroll of Enchant armor!"
                    break;
                default: printlog("ERROR: updateItemModifyingGUI() - itemModifyingGUI_Type (%s) out of bounds.", itemModifyingGUI_Type); return;
            }
        }
    }

    // Build the GUI's Inventory for an initial check
    rebuildItemModifyingGUIInventory();

    // If the Inventory is empty, there is nothing to process. The GUI will not open and nothing will be processed
    if ( itemModifyingGUI_Inventory[0] == nullptr )
    {
        // If it is a Cursed Scroll and there are no valid Items, still use up the Scroll
        if ( itemModifyingGUI_ScrollBeatitude < 0 )
        {
            messagePlayer(clientnum, language[863]); // "The scroll smokes and crumbles to ash!"

            // Somehow, a Spell has been used to open the GUI, but was also Cursed, this should never happen, because Spells cannot be Cursed
            if ( itemModifyingGUI_ScrollUsed == nullptr )
            {
                printlog("ERROR: updateItemModifyingGUI() - A Cursed Spell has opened the GUI. This should never happen.");
                return;
            }

            consumeItem(itemModifyingGUI_ScrollUsed);
            closeItemModifyingGUI();
            return;
        }
        messagePlayer(clientnum, language[2500]); // "There are no valid items to use this on!"
        closeItemModifyingGUI();
        return;
    }
    else
    {
        // Open the Inventory, this is for Spells, as they can open the GUI without having the Inventory open
        shootmode = false;
        gui_mode = GUI_MODE_INVENTORY;

        // If the Scroll is Cursed, don't warp the Mouse
        if ( itemModifyingGUI_ScrollBeatitude >= 0 )
        {
            // The actual logic is handled by updateItemModifyingGUI()
            itemModifyingGUI_InventorySelectedSlot = 0;
            warpMouseToSelectedGUISlot();
        }
    }
} // openItemModifyingGUI()

/* ItemModifyingGUI.cpp
 * Handles the Drawing of the GUI, along with setting up and processing the Mouse input collision bounds through their various function calls
 * Handles the GUI Inventory slot bounds and Mouse input to call processGUIEffectOnItem()
 */
void ItemModifyingGUI::updateItemModifyingGUI()
{
    // If the GUI is not active, there is no reason to go further
    if ( bIsActive != true )
    {
        return;
    }

    // Cursed Scrolls do not use the GUI, their processing is determined at random
    if ( bIsCursed != true )
    {
        // If the Player's Inventory cannot be accessed, nothing will work, therefore this is a check to prevent needless processing
        list_t* playerInventoryList = &stats[clientnum]->inventory;
        if ( playerInventoryList == nullptr )
        {
            messagePlayer(0, "Warning: stats[%d]->inventory is not a valid list. This should not happen.", clientnum);
            printlog("ERROR: updateItemModifyingGUI() - stats[%d]->inventory is not a valid list.", clientnum);
            return;
        }

        // playerInventoryList is no longer used, nullify it
        playerInventoryList = nullptr;

        SDL_Rect GUIRect; // The location of the GUI window

        // Draw the GUI Background at the center off the Game Window
        GUIRect.x = (((xres / 2) - (itemModifyingGUI_IMG->w / 2)) + itemModifyingGUI_OffsetX);
        GUIRect.y = (((yres / 2) - (itemModifyingGUI_IMG->h / 2)) + itemModifyingGUI_OffsetY);
        drawImage(itemModifyingGUI_IMG, nullptr, &GUIRect);

        // Setup the collision bounds for the GUI buttons
        itemModifyingGUI_HandleButtons();

        // Setup the collision bounds for the Mouse Wheel
        itemModifyingGUI_HandleMouseWheel();
        
        // Setup the collision bounds for the GUI Dragging Bar
        itemModifyingGUI_HandleDraggingBar();

        // Print the Window Label signifying this GUI
        char* windowName;
        windowName = language[2499]; // "Repair an Item"

        const Sint32 windowLabelX = ((((xres / 2) - (itemModifyingGUI_IMG->w / 2)) + itemModifyingGUI_OffsetX) + 2 + ((itemModifyingGUI_IMG->w / 2) - ((TTF8_WIDTH * longestline(windowName)) / 2)));
        const Sint32 windowLabelY = (((yres / 2) - (itemModifyingGUI_IMG->h / 2)) + itemModifyingGUI_OffsetY) + 4;

        // Print the Window Label
        ttfPrintText(ttf8, windowLabelX, windowLabelY, windowName);

        // The GUI Window's position after being offset
        const Sint32 GUIOffsetPosX = (((xres / 2) - (itemModifyingGUI_IMG->w / 2)) + itemModifyingGUI_OffsetX);
        const Sint32 GUIOffsetPosY = (((yres / 2) - (itemModifyingGUI_IMG->h / 2)) + itemModifyingGUI_OffsetY);

        // Draw the Images for the GUI buttons when they are being clicked
        itemModifyingGUI_HandleButtonImages(GUIOffsetPosX, GUIOffsetPosY);

        SDL_Rect inventorySlotRect; // The position of all the Inventory Slots combined
        inventorySlotRect.x = GUIOffsetPosX;
        inventorySlotRect.w = inventoryoptionChest_bmp->w;
        inventorySlotRect.y = GUIOffsetPosY + 16;
        inventorySlotRect.h = inventoryoptionChest_bmp->h;

        SDL_Rect currentInventorySlotRect; // The current Inventory Slot Image to be drawn

        bool bIsMouseHoveringOverSlot = false; // True if the User's Mouse is currently over an Inventory Slot

        // Draw every Inventory Slot and check for Mouse clicks on those Inventory Slots
        for ( Uint8 iSlotIndex = 0; iSlotIndex < NUM_ITEM_MODIFYING_GUI_ITEMS; ++iSlotIndex, inventorySlotRect.y += inventorySlotRect.h )
        {
            // Update the position of the currentInventorySlotRect
            currentInventorySlotRect.x = inventorySlotRect.x + 12;
            currentInventorySlotRect.w = 0;
            currentInventorySlotRect.h = 0;

            // If the mouse is currently hovering over an Inventory Slot, and there's an Item there, check for Mouse input
            if ( omousey >= inventorySlotRect.y && omousey < inventorySlotRect.y + inventorySlotRect.h && itemModifyingGUI_Inventory[iSlotIndex] )
            {
                // Update the Y position of the currentInventorySlotRect before drawing
                currentInventorySlotRect.y = inventorySlotRect.y;
                drawImage(inventoryoptionChest_bmp, nullptr, &currentInventorySlotRect);

                itemModifyingGUI_InventorySelectedSlot = iSlotIndex;
                bIsMouseHoveringOverSlot = true;

                // If the User clicks on the current Inventory Slot, process the Item
                if ( mousestatus[SDL_BUTTON_LEFT] || *inputPressed(joyimpulses[INJOY_MENU_USE]) )
                {
                    *inputPressed(joyimpulses[INJOY_MENU_USE]) = 0;
                    mousestatus[SDL_BUTTON_LEFT] = 0;

                    processGUIEffectOnItem(itemModifyingGUI_Inventory[iSlotIndex]);
                }
            }
        }

        // Only have a slot selected if hovering over a slot
        if ( !bIsMouseHoveringOverSlot )
        {
            itemModifyingGUI_InventorySelectedSlot = -1;
        }

        // Draw the Images for each Item in the GUI Inventory
        // This is done at the end to prevent the Inventory Slot highlight from being drawn on top of the Item information
        itemModifyingGUI_HandleItemImages();
    }
    else 
    {
        if ( itemModifyingGUI_ScrollUsed == nullptr )
        {
            // Somehow, a Spell has been used to open the GUI, but was also Cursed, this should never happen, because Spells cannot be Cursed
            printlog("ERROR: updateItemModifyingGUI() - A Cursed Spell has opened the GUI. This should never happen.");
            return;
        }

        messagePlayer(clientnum, language[2505]); // "Oh no! The scroll was cursed!"

        itemModifyingGUIProcessRandomItem();

        consumeItem(itemModifyingGUI_ScrollUsed);
        closeItemModifyingGUI();
    }
} // updateItemModifyingGUI()

/* ItemModifyingGUI.cpp
 * Resets all member variables back to their base states
 */
void ItemModifyingGUI::closeItemModifyingGUI()
{
    bIsActive = false;
    bIsCursed = false;
    bIsItemModifyingGUI_Dragging = false;
    itemModifyingGUI_ScrollBeatitude = 0;
    itemModifyingGUI_Type = 0;
    itemModifyingGUI_InventoryScrollOffset = 0;
    itemModifyingGUI_InventorySelectedSlot = -1;

    itemModifyingGUI_ScrollUsed = nullptr;
    for ( int i = 0; i < NUM_ITEM_MODIFYING_GUI_ITEMS; i++ )
    {
        itemModifyingGUI_Inventory[i] = nullptr;
    }
} // closeItemModifyingGUI()

/* ItemModifyingGUI.cpp
 * @param direction - Will either be -1 for Up or 1 for Down. The direction in which the User wants to move the cursor
 * Called by GameController::handleItemModifyingGUIMovement(). Evaluates the requested movement, updating 'itemModifyingGUI_InventorySelectedSlot' as needed
 */
void ItemModifyingGUI::gamepadMoveCursor(Sint8 direction)
{
    // Up will be -1, Down will be 1
    Sint8 newSlot = itemModifyingGUI_InventorySelectedSlot + direction;
    
    // D-Pad Up was pressed
    if ( newSlot < itemModifyingGUI_InventorySelectedSlot )
    {
        // Possible cases:
        // * 1) Move cursor up the GUI through different itemModifyingGUI_InventorySelectedSlot
        // * * 2) Page up through itemModifyingGUI_InventoryScrollOffset--
        // * * 3) Scrolling up past top of the GUI's Inventory, no itemModifyingGUI_InventoryScrollOffset (move back to Player Inventory)

        if ( itemModifyingGUI_InventorySelectedSlot <= 0 )
        {
            // Covers cases 2 & 3
            // Possible cases:
            // * A) Hit very top of the GUI's Inventory, can't go any further. Return to Player Inventory
            // * * B) Page up, scrolling through itemModifyingGUI_InventoryScrollOffset--

            if ( itemModifyingGUI_InventoryScrollOffset <= 0 )
            {
                // Case 3/A: Return to Player Inventory
                // A check will happen in playerinventory.cpp immediately after this which will call warpMouseToSelectedInventorySlot()
                itemModifyingGUI_InventorySelectedSlot = -1;
            }
            else
            {
                // Case 2/B: Page up through GUI's Inventory
                itemModifyingGUI_InventoryScrollOffset--;
            }
        }
        else
        {
            // Covers case 1
            // Move cursor up the GUI through different itemModifyingGUI_InventorySelectedSlot (itemModifyingGUI_InventorySelectedSlot--)
            itemModifyingGUI_InventorySelectedSlot--;
            warpMouseToSelectedGUISlot();
        }
    }
    else if ( newSlot > itemModifyingGUI_InventorySelectedSlot ) // D-Pad Down was pressed
    {
        // Possible cases:
        // * 1) Moving cursor down through GUI through different itemModifyingGUI_InventorySelectedSlot
        // * * 2) Scrolling down past bottom of GUI's Inventory through itemModifyingGUI_InventoryScrollOffset++
        // * * 3) Scrolling down past bottom of GUI's Inventory, past maximum GUI Inventory size (Undo move -- can't go beyond limit of the GUI's Inventory)

        if ( itemModifyingGUI_InventorySelectedSlot >= NUM_ITEM_MODIFYING_GUI_ITEMS - 1 )
        {
            // Covers cases 2 & 3
            itemModifyingGUI_InventoryScrollOffset++; // itemModifyingGUI_InventoryScrollOffset is automatically sanitized in updateItemModifyingGUI()
        }
        else
        {
            // Covers case 1
            // Move cursor down through the GUI through different itemModifyingGUI_InventorySelectedSlot (itemModifyingGUI_InventorySelectedSlot++)
            // This is a little bit trickier since must undo movement if there is no Item in the next slot
            // Two possible cases:
            // * A) There are Items below this. Advance itemModifyingGUI_InventorySelectedSlot to them
            // * * B) On last Item already. Do nothing (undo movement)

            Item* item = getItemInfoFromGUI(itemModifyingGUI_InventorySelectedSlot + 1);

            if ( item != nullptr )
            {
                // Case 1/A
                itemModifyingGUI_InventorySelectedSlot++;
                warpMouseToSelectedGUISlot();
            }
            else
            {
                // Case 1/B
                //No more Items. Prevent movement (undo movement)
            }
        }
    }
}

/* ItemModifyingGUI.cpp
 * @returns 'bIsActive'
 */
bool ItemModifyingGUI::isActive() const
{
    return bIsActive;
}

/* ItemModifyingGUI.cpp
 * @param GUIType - The type of GUI to be opened: 0,1,2,3,4 - Identify, Remove Curse, Repair, Enchant Weapon, Enchant Armor
 * @returns true - If the Player's Inventory has valid Items for processing
 * @returns false - If the Player's Inventory has no valid Items for processing
 * The main usage of this function is to prevent a Spell from being cast needlessly
 * Sets 'itemModifyingGUI_Type' = @GUIType. 'itemModifyingGUI_Type' is set back to default value before returning via closeItemModifyingGUI()
 */
bool ItemModifyingGUI::areThereValidItems(const Uint8 GUIType)
{
    itemModifyingGUI_Type = GUIType;

    // Build the GUI's Inventory for an initial check
    rebuildItemModifyingGUIInventory();

    // If the Inventory is empty, there is nothing to process
    if ( itemModifyingGUI_Inventory[0] == nullptr )
    {
        messagePlayer(clientnum, language[2500]); // "There are no valid items to use this on!"
        closeItemModifyingGUI(); // Reset all values to prevent any crossover
        return false;
    }
    else
    {
        closeItemModifyingGUI(); // Reset all values to prevent any crossover
        return true;
    }
} // areThereValidItems()

/* ItemModifyingGUI.cpp
 * @returns true - If 'itemModifyingGUI_InventorySelectedSlot' is < 0
 * @returns false - If 'itemModifyingGUI_InventorySelectedSlot' is 0 or greater
 * Returns whether or not the mouse is currently hovering over a GUI Inventory Slot
 */
bool ItemModifyingGUI::isSelectedSlotInvalid() const
{
    return (itemModifyingGUI_InventorySelectedSlot < 0);
}

/* ItemModifyingGUI.cpp
*  Used to get the reference to the Item in the given slot in itemModifyingGUI_Inventory[]
*  @param int slot - The slot to be accessed in itemModifyingGUI_Inventory[]
*  @returns The Item* from itemModifyingGUI_Inventory[slot]
*  @returns nullptr if slot >= 4 (Out of bounds)
*/
inline Item* ItemModifyingGUI::getItemInfoFromGUI(Uint8 slot)
{
    if ( slot >= 4 )
    {
        return nullptr; // Out of bounds
    }

    return itemModifyingGUI_Inventory[slot];
} // getItemInfoFromGUI()

// ITEM MODIFYING GUI LOGIC HANDLERS

/* ItemModifyingGUI.cpp
 * @param selectedItem - The Item that is being processed, selected from the GUI Inventory
 * Selects the correct function to process the Item according to 'itemModifyingGUI_Type'
 */
void ItemModifyingGUI::processGUIEffectOnItem(Item* const selectedItem)
{
    switch ( itemModifyingGUI_Type )
    {
        case 0: // Identify
            processIdentifyGUIEffectOnItem(selectedItem);
            break;
        case 1: // Remove Curse
            processRemoveCurseGUIEffectOnItem(selectedItem);
            break;
        case 2: // Repair
            processRepairGUIEffectOnItem(selectedItem);
            break;
        case 3: // Enchant Weapon
            processEnchantWeaponGUIEffectOnItem(selectedItem);
            break;
        case 4: // Enchant Armor
            processEnchantArmorGUIEffectOnItem(selectedItem);
            break;
        default: printlog("ERROR: processGUIEffectOnItem() - itemModifyingGUI_Type (%s) out of bounds.", itemModifyingGUI_Type); return;
    }
} // processGUIEffectOnItem()

/* ItemModifyingGUI.cpp
 * Used for Cursed Scrolls. Selects the correct function to randomly process the Item(s) according to 'itemModifyingGUI_Type'
 */
void ItemModifyingGUI::itemModifyingGUIProcessRandomItem()
{
    Uint8 numberOfItemsToProcess = 0;
    switch ( itemModifyingGUI_ScrollBeatitude )
    {
        case -2:
            numberOfItemsToProcess = 2;
            break;
        case -1:
            numberOfItemsToProcess = 1;
            break;
        default: printlog("ERROR: itemModifyingGUIProcessRandomItem() - itemModifyingGUI_ScrollBeatitude (%s) out of bounds.", itemModifyingGUI_ScrollBeatitude); return;
    }

    switch ( itemModifyingGUI_Type )
    {
        case 0: // Identify
            identifyGUIProcessRandomItem(numberOfItemsToProcess);
            break;
        case 1: // Remove Curse
            removeCurseGUIProcessRandomItem(numberOfItemsToProcess);
            break;
        case 2: // Repair
            repairGUIProcessRandomItem(numberOfItemsToProcess);
            break;
        case 3: // Enchant Weapon
            enchantWeaponGUIProcessRandomItem(numberOfItemsToProcess);
            break;
        case 4: // Enchant Armor
            enchantArmorGUIProcessRandomItem(numberOfItemsToProcess);
            break;
        default: printlog("ERROR: itemModifyingGUIProcessRandomItem() - itemModifyingGUI_Type (%s) out of bounds.", itemModifyingGUI_Type); return;
    }
} // itemModifyingGUIProcessRandomItem()

/* ItemModifyingGUI.cpp
 * Selects the correct function to rebuild the GUI Inventory according to 'itemModifyingGUI_Type'
 */
void ItemModifyingGUI::rebuildItemModifyingGUIInventory()
{
    switch ( itemModifyingGUI_Type )
    {
        case 0: // Identify
            rebuildIdentifyGUIInventory();
            break;
        case 1: // Remove Curse
            rebuildRemoveCurseGUIInventory();
            break;
        case 2: // Repair
            rebuildRepairGUIInventory();
            break;
        case 3: // Enchant Weapon
            rebuildEnchantWeaponGUIInventory();
            break;
        case 4: // Enchant Armor
            rebuildEnchantArmorGUIInventory();
            break;
        default: printlog("ERROR: rebuildItemModifyingGUIInventory() - itemModifyingGUI_Type (%s) out of bounds.", itemModifyingGUI_Type); return;
    }
} // rebuildItemModifyingGUIInventory()

/* ItemModifyingGUI.cpp
 * Calls SDL_WarpMouseInWindow() to move the Mouse cursor to the currently selected GUI slot for controllers
 */
void ItemModifyingGUI::warpMouseToSelectedGUISlot()
{
    SDL_Rect slotPos;
    slotPos.x = (((xres / 2) - (itemModifyingGUI_IMG->w / 2)) + itemModifyingGUI_OffsetX);
    slotPos.w = inventoryoptionChest_bmp->w;
    slotPos.h = inventoryoptionChest_bmp->h;
    slotPos.y = (((yres / 2) - (itemModifyingGUI_IMG->h / 2)) + itemModifyingGUI_OffsetY) + 16 + (slotPos.h * itemModifyingGUI_InventorySelectedSlot);

    SDL_WarpMouseInWindow(screen, slotPos.x + (slotPos.w / 2), slotPos.y + (slotPos.h / 2));
} // warpMouseToSelectedGUISlot()

// ITEM MODIFYING GUI DISPLAY HANDLERS

/* ItemModifyingGUI.cpp
 * Handles setting up the collision bounds and checking for Mouse input on those collision bounds for each of the GUI's buttons
 * The Top Bar of the GUI used for dragging is considered a button
 */
void ItemModifyingGUI::itemModifyingGUI_HandleButtons()
{
    // GUI Button Collision Bounds TODOR: I may have these swapped, the lower bound may be the upper bound
    // Scroll Up Button Y
    const Sint32 scrollUpButtonY_LowerBound = (((yres / 2) - (itemModifyingGUI_IMG->h / 2)) + itemModifyingGUI_OffsetY) + 16;
    const Sint32 scrollUpButtonY_UpperBound = (((yres / 2) - (itemModifyingGUI_IMG->h / 2)) + itemModifyingGUI_OffsetY) + 52;
    // Scroll Up Button X
    const Sint32 scrollUpButtonX_LowerBound = (((xres / 2) - (itemModifyingGUI_IMG->w / 2)) + itemModifyingGUI_OffsetX) + (itemModifyingGUI_IMG->w - 28);
    const Sint32 scrollUpButtonX_UpperBound = (((xres / 2) - (itemModifyingGUI_IMG->w / 2)) + itemModifyingGUI_OffsetX) + (itemModifyingGUI_IMG->w - 12);
    // Scroll Down Button Y
    const Sint32 scrollDownButtonY_LowerBound = (((yres / 2) - (itemModifyingGUI_IMG->h / 2)) + itemModifyingGUI_OffsetY) + 52;
    const Sint32 scrollDownButtonY_UpperBound = (((yres / 2) - (itemModifyingGUI_IMG->h / 2)) + itemModifyingGUI_OffsetY) + 88;
    // Scroll Down Button X
    const Sint32 scrollDownButtonX_LowerBound = (((xres / 2) - (itemModifyingGUI_IMG->w / 2)) + itemModifyingGUI_OffsetX) + (itemModifyingGUI_IMG->w - 28);
    const Sint32 scrollDownButtonX_UpperBound = (((xres / 2) - (itemModifyingGUI_IMG->w / 2)) + itemModifyingGUI_OffsetX) + (itemModifyingGUI_IMG->w - 12);
    // Close Button Y
    const Sint32 closeButtonY_LowerBound = (((yres / 2) - (itemModifyingGUI_IMG->h / 2)) + itemModifyingGUI_OffsetY);
    const Sint32 closeButtonY_UpperBound = (((yres / 2) - (itemModifyingGUI_IMG->h / 2)) + itemModifyingGUI_OffsetY) + 15;
    // Close Button X
    const Sint32 closeButtonX_LowerBound = (((xres / 2) - (itemModifyingGUI_IMG->w / 2)) + itemModifyingGUI_OffsetX) + 393;
    const Sint32 closeButtonX_UpperBound = (((xres / 2) - (itemModifyingGUI_IMG->w / 2)) + itemModifyingGUI_OffsetX) + 407;
    // Dragging Bar Y
    const Sint32 draggingBarY_LowerBound = (((yres / 2) - (itemModifyingGUI_IMG->h / 2)) + itemModifyingGUI_OffsetY);
    const Sint32 draggingBarY_UpperBound = (((yres / 2) - (itemModifyingGUI_IMG->h / 2)) + itemModifyingGUI_OffsetY) + 15;
    // Dragging Bar X
    const Sint32 draggingBarX_LowerBound = (((xres / 2) - (itemModifyingGUI_IMG->w / 2)) + itemModifyingGUI_OffsetX);
    const Sint32 draggingBarX_UpperBound = (((xres / 2) - (itemModifyingGUI_IMG->w / 2)) + itemModifyingGUI_OffsetX) + 377;

    // Buttons
    if ( mousestatus[SDL_BUTTON_LEFT] )
    {
        // GUI Scroll Up Button
        if ( omousey >= scrollUpButtonY_LowerBound && omousey < scrollUpButtonY_UpperBound )
        {
            if ( omousex >= scrollUpButtonX_LowerBound && omousex < scrollUpButtonX_UpperBound )
            {
                buttonclick = 7;
                if ( itemModifyingGUI_InventoryScrollOffset != 0 )
                {
                    itemModifyingGUI_InventoryScrollOffset--;
                }
                mousestatus[SDL_BUTTON_LEFT] = 0;
            }
        }

        // GUI Scroll Down Button
        else if ( omousey >= scrollDownButtonY_LowerBound && omousey < scrollDownButtonY_UpperBound )
        {
            if ( omousex >= scrollDownButtonX_LowerBound && omousex < scrollDownButtonX_UpperBound )
            {
                buttonclick = 8;
                itemModifyingGUI_InventoryScrollOffset++;
                mousestatus[SDL_BUTTON_LEFT] = 0;
            }
        }

        // Top Bar of GUI
        else if ( omousey >= closeButtonY_LowerBound && omousey < closeButtonY_UpperBound )
        {
            // GUI Close Button
            if ( omousex >= closeButtonX_LowerBound && omousex < closeButtonX_UpperBound )
            {
                buttonclick = 9;
                mousestatus[SDL_BUTTON_LEFT] = 0;
            }

            // GUI Dragging Top Bar
            if ( omousex >= draggingBarX_LowerBound && omousex < draggingBarX_UpperBound && omousey >= draggingBarY_LowerBound && omousey < draggingBarY_UpperBound )
            {
                gui_clickdrag = true;
                bIsItemModifyingGUI_Dragging = true;
                dragoffset_x = omousex - (((xres / 2) - (itemModifyingGUI_IMG->w / 2)) + itemModifyingGUI_OffsetX);
                dragoffset_y = omousey - (((yres / 2) - (itemModifyingGUI_IMG->h / 2)) + itemModifyingGUI_OffsetY);
                mousestatus[SDL_BUTTON_LEFT] = 0;
            }
        }
    }
} // itemModifyingGUI_HandleButtons()

/* ItemModifyingGUI.cpp
 * Handles setting up the collision bounds and checking for Mouse input on those collision bounds for Mouse scroll wheel input
 */
void ItemModifyingGUI::itemModifyingGUI_HandleMouseWheel()
{
    // GUI Mouse Wheel Collision Bounds
    // Mouse Wheel Y
    const Sint32 mouseWheelY_LowerBound = (((yres / 2) - (itemModifyingGUI_IMG->h / 2)) + itemModifyingGUI_OffsetY) + 16;
    const Sint32 mouseWheelY_UpperBound = (((yres / 2) - (itemModifyingGUI_IMG->h / 2)) + itemModifyingGUI_OffsetY) + (itemModifyingGUI_IMG->h - 8);
    // Mouse Wheel X
    const Sint32 mouseWheelX_LowerBound = (((xres / 2) - (itemModifyingGUI_IMG->w / 2)) + itemModifyingGUI_OffsetX) + 12;
    const Sint32 mouseWheelX_UpperBound = (((xres / 2) - (itemModifyingGUI_IMG->w / 2)) + itemModifyingGUI_OffsetX) + (itemModifyingGUI_IMG->w - 28);

    // Mouse wheel
    if ( omousex >= mouseWheelX_LowerBound && omousex < mouseWheelX_UpperBound )
    {
        if ( omousey >= mouseWheelY_LowerBound && omousey < mouseWheelY_UpperBound )
        {
            if ( mousestatus[SDL_BUTTON_WHEELDOWN] )
            {
                mousestatus[SDL_BUTTON_WHEELDOWN] = 0;
                itemModifyingGUI_InventoryScrollOffset++;
            }
            else if ( mousestatus[SDL_BUTTON_WHEELUP] )
            {
                mousestatus[SDL_BUTTON_WHEELUP] = 0;
                if ( itemModifyingGUI_InventoryScrollOffset != 0 )
                {
                    itemModifyingGUI_InventoryScrollOffset--;
                }
            }
        }
    }
} // itemModifyingGUI_HandleMouseWheel()

/* ItemModifyingGUI.cpp
 * Handles the actual movement of the GUI Window by modifying 'itemModifyingGUI_OffsetX/Y' when 'bIsItemModifyingGUI_Dragging' == true
 */
void ItemModifyingGUI::itemModifyingGUI_HandleDraggingBar()
{
    // GUI Dragging
    // The GUI Window's centered position in the Game Window
    Sint32 GUICenterPosY = ((yres / 2) - (itemModifyingGUI_IMG->h / 2));
    Sint32 GUICenterPosX = ((xres / 2) - (itemModifyingGUI_IMG->w / 2));
    // The GUI Window's position after being offset
    Sint32 GUIOffsetPosY = (((yres / 2) - (itemModifyingGUI_IMG->h / 2)) + itemModifyingGUI_OffsetY);
    Sint32 GUIOffsetPosX = (((xres / 2) - (itemModifyingGUI_IMG->w / 2)) + itemModifyingGUI_OffsetX);

    // Dragging the Window
    if ( bIsItemModifyingGUI_Dragging == true)
    {
        if ( gui_clickdrag == true ) // If the User is dragging, process the movement
        {
            itemModifyingGUI_OffsetX = (omousex - dragoffset_x) - GUICenterPosX;
            itemModifyingGUI_OffsetY = (omousey - dragoffset_y) - GUICenterPosY;
            if ( GUIOffsetPosX <= camera.winx )
            {
                itemModifyingGUI_OffsetX = camera.winx - GUICenterPosX;
            }
            if ( GUIOffsetPosX > camera.winx + camera.winw - itemModifyingGUI_IMG->w )
            {
                itemModifyingGUI_OffsetX = (camera.winx + camera.winw - itemModifyingGUI_IMG->w) - GUICenterPosX;
            }
            if ( GUIOffsetPosY <= camera.winy )
            {
                itemModifyingGUI_OffsetY = camera.winy - GUICenterPosY;
            }
            if ( GUIOffsetPosY > camera.winy + camera.winh - itemModifyingGUI_IMG->h )
            {
                itemModifyingGUI_OffsetY = (camera.winy + camera.winh - itemModifyingGUI_IMG->h) - GUICenterPosY;
            }
        }
        else // Else, reset the flag
        {
            bIsItemModifyingGUI_Dragging = false;
        }
    }
} // itemModifyingGUI_HandleDraggingBar()

/* ItemModifyingGUI.cpp
 * @param GUIPosX - The GUI position in the center of the screen + itemModifyingGUI_OffsetX
 * @param GUIPosY - The GUI position in the center of the screen + itemModifyingGUI_OffsetY
 * Handles drawing the actual GUI button Images when they are being clicked. The unclicked versions are part of the original Image
 */
void ItemModifyingGUI::itemModifyingGUI_HandleButtonImages(const Sint32 GUIPosX, const Sint32 GUIPosY)
{
    // GUI Scroll Up Button
    if ( buttonclick == 7 )
    {
        SDL_Rect GUIInventoryUpButtonRect;
        GUIInventoryUpButtonRect.x = GUIPosX + (itemModifyingGUI_IMG->w - 28);
        GUIInventoryUpButtonRect.y = GUIPosY + 16;
        GUIInventoryUpButtonRect.w = 0;
        GUIInventoryUpButtonRect.h = 0;
        drawImage(invup_bmp, nullptr, &GUIInventoryUpButtonRect);
    }

    // GUI Scroll Down Button
    if ( buttonclick == 8 )
    {
        SDL_Rect GUIInventoryDownButtonRect;
        GUIInventoryDownButtonRect.x = GUIPosX + (itemModifyingGUI_IMG->w - 28);
        GUIInventoryDownButtonRect.y = GUIPosY + 52;
        GUIInventoryDownButtonRect.w = 0;
        GUIInventoryDownButtonRect.h = 0;
        drawImage(invdown_bmp, nullptr, &GUIInventoryDownButtonRect);
    }

    // GUI Close Button
    if ( buttonclick == 9 )
    {
        SDL_Rect GUICloseButtonRect;
        GUICloseButtonRect.x = GUIPosX + 393;
        GUICloseButtonRect.y = GUIPosY;
        GUICloseButtonRect.w = 0;
        GUICloseButtonRect.h = 0;
        drawImage(invclose_bmp, nullptr, &GUICloseButtonRect);
        closeItemModifyingGUI();
    }
} // itemModifyingGUI_HandleButtonImages()

/* ItemModifyingGUI.cpp
 * Rebuilds the GUI Inventory before selecting the correct function to render the Images for each Item according to 'itemModifyingGUI_Type'
 */
void ItemModifyingGUI::itemModifyingGUI_HandleItemImages()
{
    // Rebuild the GUI Inventory to prevent scrolling infinitely
    rebuildItemModifyingGUIInventory();

    switch ( itemModifyingGUI_Type )
    {
        case 0: // Identify
            identifyGUI_HandleItemImages();
            break;
        case 1: // Remove Curse
            removeCurseGUI_HandleItemImages();
            break;
        case 2: // Repair
            repairGUI_HandleItemImages();
            break;
        case 3: // Enchant Weapon
            enchantWeaponGUI_HandleItemImages();
            break;
        case 4: // Enchant Armor
            enchantArmorGUI_HandleItemImages();
            break;
        default: printlog("ERROR: itemModifyingGUI_HandleItemImages() - itemModifyingGUI_Type (%s) out of bounds.", itemModifyingGUI_Type); return;
    }
} // itemModifyingGUI_HandleItemImages()

// IDENTIFY GUI

/* ItemModifyingGUI.cpp
 * @param selectedItem - The Item that is being processed, selected from the GUI Inventory
 * Identifies the selected Item based on 'itemModifyingGUI_ScrollBeatitude'. If 'itemModifyingGUI_ScrollBeatitude' is > 0 then identifyGUIProcessRandomItem() will be called
 * Messages the Player that their Item has been Identified, then if the GUI was opened with a Scroll, it is consumed before closing the GUI
 */
void ItemModifyingGUI::processIdentifyGUIEffectOnItem(Item* const selectedItem)
{
    if ( selectedItem == nullptr )
    {
        printlog("ERROR: processIdentifyGUIEffectOnItem() - selectedItem is null.");
        return;
    }

    // Cursed Scrolls don't use the GUI so they will never call this function
    switch ( itemModifyingGUI_ScrollBeatitude )
    {
        case 0:
            // Identify the selected Item
            selectedItem->identified = true;
            messagePlayer(clientnum, language[320], selectedItem->description()); // "Identified "%s"."
            break;
        case 1:
            // Identify the selected Item and 1 random Unidentified Item in their Inventory
            selectedItem->identified = true;
            messagePlayer(clientnum, language[320], selectedItem->description()); // "Identified "%s"."
            identifyGUIProcessRandomItem(1);
            break;
        case 2:
            // Identify the selected Item and 2 random Unidentified Items in their Inventory
            selectedItem->identified = true;
            messagePlayer(clientnum, language[320], selectedItem->description()); // "Identified "%s"."
            identifyGUIProcessRandomItem(2);
            break;
        default: printlog("ERROR: processIdentifyGUIEffectOnItem() - itemModifyingGUI_ScrollBeatitude (%d) out of bounds.", itemModifyingGUI_ScrollBeatitude); return;
    }

    // If the Player used a Scroll, consume it
    if ( itemModifyingGUI_ScrollUsed != nullptr )
    {
        consumeItem(itemModifyingGUI_ScrollUsed);
        itemModifyingGUI_ScrollUsed = nullptr;
    }

    closeItemModifyingGUI();
} // processIdentifyGUIEffectOnItem()

/* ItemModifyingGUI.cpp
 * @param numItems - The number of random Items to be processed. Currently this number should never exceed 2, as the highest naturally spawning Scrolls are +-2
 * Processes a random amount of valid Items equal to 'numItems'. Processing will either Identify or Unidentify the random Items according to 'itemModifyingGUI_ScrollBeatitude'
 * Messages the Player that their Item has been Identified or Unidentified because of the beatitude of the Scroll used
 */
void ItemModifyingGUI::identifyGUIProcessRandomItem(const Uint8 numItems)
{
    // Grab the Player's Inventory again, as it may have been modified prior to this
    list_t* playerInventoryList = &stats[clientnum]->inventory;

    if ( playerInventoryList == nullptr )
    {
        messagePlayer(0, "Warning: stats[%d]->inventory is not a valid list. This should not happen.", clientnum);
        printlog("ERROR: identifyGUIProcessRandomItem() - stats[%d]->inventory is not a valid list.", clientnum);
        return;
    }

    // Count the number of Items in the GUI Inventory
    Uint8 totalAmountOfItems = 0;
    for ( node_t* iInventoryNode = playerInventoryList->first; iInventoryNode != nullptr; iInventoryNode = iInventoryNode->next )
    {
        Item* item = static_cast<Item*>(iInventoryNode->element);

        if ( item == nullptr )
        {
            continue;
        }

        switch ( itemModifyingGUI_ScrollBeatitude )
        {
            case -2:
                // Intentional fall-through
            case -1:
                if ( item->identified == true ) // Skip over all Unidentified Items
                {
                    totalAmountOfItems++;
                }
                break;
            case 1:
                // Intentional fall-through
            case 2:
                if ( item->identified != true ) // Skip over all Identified Items
                {
                    totalAmountOfItems++;
                }
                break;
            default: printlog("ERROR: identifyGUIProcessRandomItem() - itemModifyingGUI_ScrollBeatitude (%d) out of bounds", itemModifyingGUI_ScrollBeatitude); return;
        }
    }

    // There are no valid Items to be processed
    if ( totalAmountOfItems == 0 && itemModifyingGUI_ScrollBeatitude > 0 )
    {
        messagePlayer(clientnum, language[2510]); // "You have no remaining items for the scroll's blessing..."
        return;
    }
    else if ( totalAmountOfItems == 0 && itemModifyingGUI_ScrollBeatitude < 0 )
    {
        messagePlayer(clientnum, language[2510]); // "You have no items for the scroll's curse..."
        return;
    }

    Uint8 amountOfItemsLeftToProcess = numItems;
    bool isThereASingleItem = false;

    // If there is only a single Item that can be processed, but it's a +-2 Scroll, adjust the loop
    if ( totalAmountOfItems == 1 && itemModifyingGUI_ScrollBeatitude >= 2 )
    {
        isThereASingleItem = true;
        amountOfItemsLeftToProcess--;
    }
    else if ( totalAmountOfItems == 1 && itemModifyingGUI_ScrollBeatitude <= -2 )
    {
        isThereASingleItem = true;
        amountOfItemsLeftToProcess--;
    }

    Sint8 iPreviousItemProcessedIndex = -1;
    Sint8 iItemToProcessIndex = -1;
    Uint8 iIdentifyGUIInventoryIndex = 0;

    for ( Uint8 i = 0; i < numItems; i++ )
    {
        while ( iItemToProcessIndex == iPreviousItemProcessedIndex )
        {
            iItemToProcessIndex = rand() % totalAmountOfItems;
        }
        
        iIdentifyGUIInventoryIndex = 0;

        for ( node_t* iInventoryNode = playerInventoryList->first; iInventoryNode != nullptr; iInventoryNode = iInventoryNode->next )
        {
            Item* itemToProcess = static_cast<Item*>(iInventoryNode->element);

            if ( itemToProcess == nullptr )
            {
                continue;
            }

            switch ( itemModifyingGUI_ScrollBeatitude )
            {
                case -2:
                    // Intentional fall-through
                case -1:
                    if ( itemToProcess->identified == true ) // Skip over all Unidentified Items
                    {
                        if ( iIdentifyGUIInventoryIndex == iItemToProcessIndex )
                        {
                            if ( numItems == 2 && amountOfItemsLeftToProcess == 1 && isThereASingleItem != true )
                            {
                                messagePlayer(clientnum, language[2514], itemToProcess->description()); // "Oh no! Your %s was unidentified too!"
                            }
                            else
                            {
                                messagePlayer(clientnum, language[2506], itemToProcess->description()); // "Your %s was unidentified!"
                            }

                            itemToProcess->identified = false;
                            amountOfItemsLeftToProcess--;

                            iPreviousItemProcessedIndex = iIdentifyGUIInventoryIndex;
                        }
                        iIdentifyGUIInventoryIndex++;
                    }
                    break;
                case 1:
                    // Intentional fall-through
                case 2:
                    if ( itemToProcess->identified != true ) // Skip over all Identified Items
                    {
                        if ( iIdentifyGUIInventoryIndex == iItemToProcessIndex )
                        {
                            itemToProcess->identified = true;
                            amountOfItemsLeftToProcess--;
                            messagePlayer(clientnum, language[2511], itemToProcess->description()); // "Wow! Your %s was identified too!"
                            iPreviousItemProcessedIndex = iIdentifyGUIInventoryIndex;
                        }
                        iIdentifyGUIInventoryIndex++;
                    }
                    break;
                default: printlog("ERROR: identifyGUIProcessRandomItem() - itemModifyingGUI_ScrollBeatitude (%d) out of bounds", itemModifyingGUI_ScrollBeatitude); return;
            }

            // If processing is done, stop searching the Inventory
            if ( amountOfItemsLeftToProcess == 0 )
            {
                break;
            }
        }

        // If there's only a single valid Item, but it's a +-2 scroll, this will happen
        if ( amountOfItemsLeftToProcess == 0 )
        {
            break;
        }
    }

    // Tell the Player they were an Item short of the full effect
    if ( isThereASingleItem == true && itemModifyingGUI_ScrollBeatitude > 0 )
    {
        messagePlayer(clientnum, language[2510]); // "You have no remaining items for the scroll's blessing..."
    }
    else if ( isThereASingleItem == true && itemModifyingGUI_ScrollBeatitude < 0 )
    {
        messagePlayer(clientnum, language[2524]); // "You have no remaining items for the scroll's curse..."
    }
} // identifyGUIProcessRandomItem()

/* ItemModifyingGUI.cpp
 * Builds the GUI's Inventory based off of 'itemModifyingGUI_Type' and 'itemModifyingGUI_ScrollBeatitude'
 * Counts the number of Items to add to the GUI's Inventory, then assigns the visible ones to their position in 'itemModifyingGUI_Inventory[]'
 */
void ItemModifyingGUI::rebuildIdentifyGUIInventory()
{
    // Grab the Player's Inventory again, as it may have been modified by processGUIEffectOnItem()
    list_t* playerInventoryList = &stats[clientnum]->inventory;
    Uint8 iIdentifyGUIInventoryIndex = 0;

    if ( playerInventoryList == nullptr )
    {
        messagePlayer(0, "Warning: stats[%d]->inventory is not a valid list. This should not happen.", clientnum);
        printlog("ERROR: rebuildIdentifyGUIInventory() - stats[%d]->inventory is not a valid list.", clientnum);
        return;
    }

    // Count the number of Items in the GUI Inventory
    for ( node_t* iInventoryNode = playerInventoryList->first; iInventoryNode != nullptr; iInventoryNode = iInventoryNode->next )
    {
        Item* item = static_cast<Item*>(iInventoryNode->element);

        if ( item == nullptr )
        {
            continue;
        }

        switch ( itemModifyingGUI_ScrollBeatitude )
        {
            case -2:
                // Intentional fall-through
            case -1:
                if ( item->identified == true ) // Skip over all Unidentified Items
                {
                    ++iIdentifyGUIInventoryIndex;
                }
                break;
            case 0:
                // Intentional fall-through
            case 1:
                // Intentional fall-through
            case 2:
                if ( item->identified != true ) // Skip over all Identified Items
                {
                    ++iIdentifyGUIInventoryIndex;
                }
                break;
            default: printlog("ERROR: rebuildRepairGUIInventory() - itemModifyingGUI_ScrollBeatitude (%d) out of bounds", itemModifyingGUI_ScrollBeatitude); return;
        }
    }

    itemModifyingGUI_InventoryScrollOffset = std::max(0, std::min(static_cast<int>(itemModifyingGUI_InventoryScrollOffset), iIdentifyGUIInventoryIndex - 4));

    // Reset the current Inventory
    for ( iIdentifyGUIInventoryIndex = 0; iIdentifyGUIInventoryIndex < 4; ++iIdentifyGUIInventoryIndex )
    {
        itemModifyingGUI_Inventory[iIdentifyGUIInventoryIndex] = nullptr;
    }

    iIdentifyGUIInventoryIndex = 0;

    // Assign the visible Items to the GUI slots
    bool bBreakFromLoop = false;
    for ( node_t* iInventoryNode = playerInventoryList->first; iInventoryNode != nullptr; iInventoryNode = iInventoryNode->next )
    {
        Item* item = static_cast<Item*>(iInventoryNode->element);

        if ( item == nullptr )
        {
            continue;
        }

        switch ( itemModifyingGUI_ScrollBeatitude )
        {
            case -2:
                // Intentional fall-through
            case -1:
                if ( item->identified == true ) // Skip over all Unidentified Items
                {
                    ++iIdentifyGUIInventoryIndex;

                    if ( iIdentifyGUIInventoryIndex <= itemModifyingGUI_InventoryScrollOffset )
                    {
                        continue;
                    }

                    itemModifyingGUI_Inventory[iIdentifyGUIInventoryIndex - itemModifyingGUI_InventoryScrollOffset - 1] = item;

                    if ( iIdentifyGUIInventoryIndex > 3 + itemModifyingGUI_InventoryScrollOffset )
                    {
                        bBreakFromLoop = true;
                    }
                }
                break;
            case 0:
                // Intentional fall-through
            case 1:
                // Intentional fall-through
            case 2:
                if ( item->identified != true ) // Skip over all Identified Items
                {
                    ++iIdentifyGUIInventoryIndex;

                    if ( iIdentifyGUIInventoryIndex <= itemModifyingGUI_InventoryScrollOffset )
                    {
                        continue;
                    }

                    itemModifyingGUI_Inventory[iIdentifyGUIInventoryIndex - itemModifyingGUI_InventoryScrollOffset - 1] = item;

                    if ( iIdentifyGUIInventoryIndex > 3 + itemModifyingGUI_InventoryScrollOffset )
                    {
                        bBreakFromLoop = true;
                    }
                }
                break;
            default: printlog("ERROR: rebuildIdentifyGUIInventory() - itemModifyingGUI_ScrollBeatitude (%d) out of bounds", itemModifyingGUI_ScrollBeatitude); return;
        }

        if ( bBreakFromLoop == true )
        {
            break;
        }
    }
} // rebuildIdentifyGUIInventory()

/* ItemModifyingGUI.cpp
 * Draws the Sprite Image and description for the given Item in each of the visible GUI Inventory slots
 */
void ItemModifyingGUI::identifyGUI_HandleItemImages()
{
    Uint8 iIdentifyGUIInventoryIndex = 0; // The position in the GUI Inventory array of the given Item
    Sint32 iIdentifyGUIInventoryItemOffset = (((yres / 2) - (itemModifyingGUI_IMG->h / 2)) + itemModifyingGUI_OffsetY) + 22; // The amount that each Item is offset vertically from each other

    Item* item = nullptr; // The given Item being drawn from the GUI Inventory
    SDL_Rect itemImageRect; // The position of the Image of the Item in the GUI Inventory

    // Grab the Player's Inventory again, as it may have been modified by processGUIEffectOnItem()
    list_t* playerInventoryList = &stats[clientnum]->inventory;

    for ( node_t* iInventoryNode = playerInventoryList->first; iInventoryNode != nullptr; iInventoryNode = iInventoryNode->next )
    {
        if ( iInventoryNode->element != nullptr )
        {
            item = static_cast<Item*>(iInventoryNode->element);

            if ( item == nullptr )
            {
                continue;
            }

            // Cursed Scrolls don't need to be shown, as they wont have a GUI
            if ( item->identified != true ) // Skip over all Identified Items
            {
                iIdentifyGUIInventoryIndex++;

                // If the Item is not visible (within 0 - 3 in Inventory), skip over it
                if ( iIdentifyGUIInventoryIndex <= itemModifyingGUI_InventoryScrollOffset )
                {
                    continue;
                }

                // The name and description of the Item in the GUI Inventory
                char itemDescription[64] = {0};
                strncpy(itemDescription, item->description(), 46);

                // Check to make sure the text doesn't go out of bounds
                if ( strlen(itemDescription) == 46 )
                {
                    strcat(itemDescription, " ...");
                }

                // Print the name and description of the Item
                ttfPrintText(ttf8, (((xres / 2) - (itemModifyingGUI_IMG->w / 2)) + itemModifyingGUI_OffsetX) + 36, iIdentifyGUIInventoryItemOffset, itemDescription);

                // Draw the Image of the Item
                itemImageRect.x = (((xres / 2) - (itemModifyingGUI_IMG->w / 2)) + itemModifyingGUI_OffsetX) + 16;
                itemImageRect.y = (((yres / 2) - (itemModifyingGUI_IMG->h / 2)) + itemModifyingGUI_OffsetY) + 17 + 18 * (iIdentifyGUIInventoryIndex - itemModifyingGUI_InventoryScrollOffset - 1);
                itemImageRect.w = 16;
                itemImageRect.h = 16;
                drawImageScaled(itemSprite(item), nullptr, &itemImageRect);

                iIdentifyGUIInventoryItemOffset += 18;

                // If the Item's index is out of bounds (above the amount of Items in the Inventory), stop drawing
                if ( iIdentifyGUIInventoryIndex > 3 + itemModifyingGUI_InventoryScrollOffset )
                {
                    break;
                }
            }
        }
    }
} // identifyGUI_HandleItemImages()

// REMOVE CURSE GUI

/* ItemModifyingGUI.cpp
 * @param selectedItem - The Item that is being processed, selected from the GUI Inventory
 * Uncurses the selected Item based on 'itemModifyingGUI_ScrollBeatitude'. If 'itemModifyingGUI_ScrollBeatitude' is > 0 then identifyGUIProcessRandomItem() will be called
 * Messages the Player that their Item has been Identified, then if the GUI was opened with a Scroll, it is consumed before closing the GUI
 */
void ItemModifyingGUI::processRemoveCurseGUIEffectOnItem(Item* const selectedItem)
{
    if ( selectedItem == nullptr )
    {
        printlog("ERROR: processRemoveCurseGUIEffectOnItem() - selectedItem is null.");
        return;
    }

    // Cursed Scrolls don't use the GUI so they will never call this function
    switch ( itemModifyingGUI_ScrollBeatitude )
    {
        case 0:
            // Uncurse the selected Item
            messagePlayer(clientnum, language[348], selectedItem->description()); // "Uncursed "%s"."
            selectedItem->beatitude = 0;
            break;
        case 1:
            // Uncurse the selected Item and 1 random Cursed Item in their Inventory
            messagePlayer(clientnum, language[348], selectedItem->description()); // "Uncursed "%s"."
            selectedItem->beatitude = 0;
            removeCurseGUIProcessRandomItem(1);
            break;
        case 2:
            // Uncurse the selected Item and 2 random Cursed Items in their Inventory
            messagePlayer(clientnum, language[348], selectedItem->description()); // "Uncursed "%s"."
            selectedItem->beatitude = 0;
            removeCurseGUIProcessRandomItem(2);
            break;
        default: printlog("ERROR: processRemoveCurseGUIEffectOnItem() - itemModifyingGUI_ScrollBeatitude (%d) out of bounds.", itemModifyingGUI_ScrollBeatitude); return;
    } 

    // If the Player used a Scroll, consume it
    if ( itemModifyingGUI_ScrollUsed != nullptr )
    {
        consumeItem(itemModifyingGUI_ScrollUsed);
        itemModifyingGUI_ScrollUsed = nullptr;
    }

    closeItemModifyingGUI();

    // TODOR: Why does the Client need to be told, this doesn't need to be networked likely
    if ( multiplayer == CLIENT && itemIsEquipped(selectedItem, clientnum) )
    {
        // The Client needs to inform the Server that their equipment was Uncursed

        // Convert the Item type to an int for the packet
        Uint8 itemType = 0;
        if ( selectedItem == stats[clientnum]->helmet )
        {
            itemType = 0;
        }
        else if ( selectedItem == stats[clientnum]->breastplate )
        {
            itemType = 1;
        }
        else if ( selectedItem == stats[clientnum]->gloves )
        {
            itemType = 2;
        }
        else if ( selectedItem == stats[clientnum]->shoes )
        {
            itemType = 3;
        }
        else if ( selectedItem == stats[clientnum]->shield )
        {
            itemType = 4;
        }
        else if ( selectedItem == stats[clientnum]->weapon )
        {
            itemType = 5;
        }
        else if ( selectedItem == stats[clientnum]->cloak )
        {
            itemType = 6;
        }
        else if ( selectedItem == stats[clientnum]->amulet )
        {
            itemType = 7;
        }
        else if ( selectedItem == stats[clientnum]->ring )
        {
            itemType = 8;
        }
        else if ( selectedItem == stats[clientnum]->mask )
        {
            itemType = 9;
        }

        strcpy((char*)net_packet->data, "RCUR");
        net_packet->data[4] = clientnum;
        net_packet->data[5] = itemType;
        net_packet->address.host = net_server.host;
        net_packet->address.port = net_server.port;
        net_packet->len = 6;
        sendPacketSafe(net_sock, -1, net_packet, 0);
    }
} // processRemoveCurseGUIEffectOnItem()

/* ItemModifyingGUI.cpp
 * @param numItems - The number of random Items to be processed. Currently this number should never exceed 2, as the highest naturally spawning Scrolls are +-2
 * Processes a random amount of valid Items equal to 'numItems'. Processing will either Uncurse or Curse the random Items according to 'itemModifyingGUI_ScrollBeatitude'
 * Messages the Player that their Item has been Uncursed or Cursed because of the beatitude of the Scroll used
 */
void ItemModifyingGUI::removeCurseGUIProcessRandomItem(const Uint8 numItems)
{
    // Grab the Player's Inventory again, as it may have been modified prior to this
    list_t* playerInventoryList = &stats[clientnum]->inventory;

    if ( playerInventoryList == nullptr )
    {
        messagePlayer(0, "Warning: stats[%d]->inventory is not a valid list. This should not happen.", clientnum);
        printlog("ERROR: removeCurseGUIProcessRandomItem() - stats[%d]->inventory is not a valid list.", clientnum);
        return;
    }

    // Count the number of Items in the GUI Inventory
    Uint8 totalAmountOfItems = 0;
    for ( node_t* iInventoryNode = playerInventoryList->first; iInventoryNode != nullptr; iInventoryNode = iInventoryNode->next )
    {
        Item* item = static_cast<Item*>(iInventoryNode->element);

        if ( item == nullptr )
        {
            continue;
        }

        switch ( itemModifyingGUI_ScrollBeatitude )
        {
            case -2:
                // Intentional fall-through
            case -1:
                if ( item->beatitude >= 0 ) // Skip over all Cursed Items
                {
                    totalAmountOfItems++;
                }
                break;
            case 1:
                // Intentional fall-through
            case 2:
                if ( item->beatitude < 0 ) // Skip over all Uncursed Items
                {
                    totalAmountOfItems++;
                }
                break;
            default: printlog("ERROR: removeCurseGUIProcessRandomItem() - itemModifyingGUI_ScrollBeatitude (%d) out of bounds", itemModifyingGUI_ScrollBeatitude); return;
        }
    }

    // There are no valid Items to be processed
    if ( totalAmountOfItems == 0 && itemModifyingGUI_ScrollBeatitude > 0 )
    {
        messagePlayer(clientnum, language[2510]); // "You have no remaining items for the scroll's blessing..."
        return;
    }
    else if ( totalAmountOfItems == 0 && itemModifyingGUI_ScrollBeatitude < 0 )
    {
        messagePlayer(clientnum, language[2510]); // "You have no items for the scroll's curse..."
        return;
    }

    Uint8 amountOfItemsLeftToProcess = numItems;
    bool isThereASingleItem = false;

    // If there is only a single Item that can be processed, but it's a +-2 Scroll, adjust the loop
    if ( totalAmountOfItems == 1 && itemModifyingGUI_ScrollBeatitude >= 2 )
    {
        isThereASingleItem = true;
        amountOfItemsLeftToProcess--;
    }
    else if ( totalAmountOfItems == 1 && itemModifyingGUI_ScrollBeatitude <= -2 )
    {
        isThereASingleItem = true;
        amountOfItemsLeftToProcess--;
    }

    Sint8 iPreviousItemProcessedIndex = -1;
    Sint8 iItemToProcessIndex = -1;
    Uint8 iRemoveCurseGUIInventoryIndex = 0;

    for ( Uint8 i = 0; i < numItems; i++ )
    {
        while ( iItemToProcessIndex == iPreviousItemProcessedIndex )
        {
            iItemToProcessIndex = rand() % totalAmountOfItems;
        }

        iRemoveCurseGUIInventoryIndex = 0;

        for ( node_t* iInventoryNode = playerInventoryList->first; iInventoryNode != nullptr; iInventoryNode = iInventoryNode->next )
        {
            Item* itemToProcess = static_cast<Item*>(iInventoryNode->element);

            if ( itemToProcess == nullptr )
            {
                continue;
            }

            switch ( itemModifyingGUI_ScrollBeatitude )
            {
                case -2:
                    // Intentional fall-through
                case -1:
                    if ( itemToProcess->beatitude >= 0 ) // Skip over all Cursed Items
                    {
                        if ( iRemoveCurseGUIInventoryIndex == iItemToProcessIndex )
                        {
                            if ( numItems == 2 && amountOfItemsLeftToProcess == 1 && isThereASingleItem != true )
                            {
                                messagePlayer(clientnum, language[2515], itemToProcess->description()); // "Oh no! Your %s was cursed too!"
                            }
                            else
                            {
                                messagePlayer(clientnum, language[2507], itemToProcess->description()); // "Your %s was cursed!"
                            }
                            
                            itemToProcess->beatitude = -1;
                            amountOfItemsLeftToProcess--;
                            iPreviousItemProcessedIndex = iRemoveCurseGUIInventoryIndex;
                        }
                        iRemoveCurseGUIInventoryIndex++;
                    }
                    break;
                case 1:
                    // Intentional fall-through
                case 2:
                    if ( itemToProcess->beatitude < 0 ) // Skip over all Uncursed Items
                    {
                        if ( iRemoveCurseGUIInventoryIndex == iItemToProcessIndex )
                        {
                            messagePlayer(clientnum, language[2512], itemToProcess->description()); // "Wow! Your %s was uncursed too!"
                            itemToProcess->beatitude = 0;
                            amountOfItemsLeftToProcess--;
                            iPreviousItemProcessedIndex = iRemoveCurseGUIInventoryIndex;
                        }
                        iRemoveCurseGUIInventoryIndex++;
                    }
                    break;
                default: printlog("ERROR: removeCurseGUIProcessRandomItem() - itemModifyingGUI_ScrollBeatitude (%d) out of bounds", itemModifyingGUI_ScrollBeatitude); return;
            }

            // If processing is done, stop searching the Inventory
            if ( amountOfItemsLeftToProcess == 0 )
            {
                break;
            }
        }

        // If there's only a single valid Item, but it's a +-2 scroll, this will happen
        if ( amountOfItemsLeftToProcess == 0 )
        {
            break;
        }
    }

    // Tell the Player they were an Item short of the full effect
    if ( isThereASingleItem == true && itemModifyingGUI_ScrollBeatitude > 0 )
    {
        messagePlayer(clientnum, language[2510]); // "You have no remaining items for the scroll's blessing..."
    }
    else if ( isThereASingleItem == true && itemModifyingGUI_ScrollBeatitude < 0 )
    {
        messagePlayer(clientnum, language[2524]); // "You have no remaining items for the scroll's curse..."
    }
} // removeCurseGUIProcessRandomItem()

/* ItemModifyingGUI.cpp
 * Builds the GUI's Inventory based off of 'itemModifyingGUI_Type' and 'itemModifyingGUI_ScrollBeatitude'
 * Counts the number of Items to add to the GUI's Inventory, then assigns the visible ones to their position in 'itemModifyingGUI_Inventory[]'
 */
void ItemModifyingGUI::rebuildRemoveCurseGUIInventory()
{
    // Grab the Player's Inventory again, as it may have been modified by processGUIEffectOnItem()
    list_t* playerInventoryList = &stats[clientnum]->inventory;
    Uint8 iRemoveCurseGUIInventoryIndex = 0;

    if ( playerInventoryList == nullptr )
    {
        messagePlayer(0, "Warning: stats[%d]->inventory is not a valid list. This should not happen.", clientnum);
        printlog("ERROR: rebuildRemoveCurseGUIInventory() - stats[%d]->inventory is not a valid list.", clientnum);
        return;
    }

    // Count the number of Items in the GUI Inventory
    for ( node_t* iInventoryNode = playerInventoryList->first; iInventoryNode != nullptr; iInventoryNode = iInventoryNode->next )
    {
        Item* item = static_cast<Item*>(iInventoryNode->element);

        if ( item == nullptr )
        {
            continue;
        }

        switch ( itemModifyingGUI_ScrollBeatitude )
        {
            case -2:
                // Intentional fall-through
            case -1:
                if ( item->beatitude >= 0 ) // Skip over all Cursed Items
                {
                    ++iRemoveCurseGUIInventoryIndex;
                }
                break;
            case 0:
                // Intentional fall-through
            case 1:
                // Intentional fall-through
            case 2:
                if ( item->identified == true && item->beatitude < 0 ) // Skip over all Unidentified and Uncursed Items
                {
                    ++iRemoveCurseGUIInventoryIndex;
                }
                break;
            default: printlog("ERROR: rebuildRepairGUIInventory() - itemModifyingGUI_ScrollBeatitude (%d) out of bounds", itemModifyingGUI_ScrollBeatitude); return;
        }
    }

    itemModifyingGUI_InventoryScrollOffset = std::max(0, std::min(static_cast<int>(itemModifyingGUI_InventoryScrollOffset), iRemoveCurseGUIInventoryIndex - 4));

    // Reset the current Inventory
    for ( iRemoveCurseGUIInventoryIndex = 0; iRemoveCurseGUIInventoryIndex < 4; ++iRemoveCurseGUIInventoryIndex )
    {
        itemModifyingGUI_Inventory[iRemoveCurseGUIInventoryIndex] = nullptr;
    }

    iRemoveCurseGUIInventoryIndex = 0;

    // Assign the visible Items to the GUI slots
    bool bBreakFromLoop = false;
    for ( node_t* iInventoryNode = playerInventoryList->first; iInventoryNode != nullptr; iInventoryNode = iInventoryNode->next )
    {
        Item* item = static_cast<Item*>(iInventoryNode->element);

        if ( item == nullptr )
        {
            continue;
        }

        switch ( itemModifyingGUI_ScrollBeatitude )
        {
            case -2:
                // Intentional fall-through
            case -1:
                if ( item->beatitude >= 0 ) // Skip over all Cursed Items
                {
                    ++iRemoveCurseGUIInventoryIndex;

                    if ( iRemoveCurseGUIInventoryIndex <= itemModifyingGUI_InventoryScrollOffset )
                    {
                        continue;
                    }

                    itemModifyingGUI_Inventory[iRemoveCurseGUIInventoryIndex - itemModifyingGUI_InventoryScrollOffset - 1] = item;

                    if ( iRemoveCurseGUIInventoryIndex > 3 + itemModifyingGUI_InventoryScrollOffset )
                    {
                        bBreakFromLoop = true;
                    }
                }
                break;
            case 0:
                // Intentional fall-through
            case 1:
                // Intentional fall-through
            case 2:
                if ( item->identified == true && item->beatitude < 0 ) // Skip over all Unidentified and Uncursed Items
                {
                    ++iRemoveCurseGUIInventoryIndex;

                    if ( iRemoveCurseGUIInventoryIndex <= itemModifyingGUI_InventoryScrollOffset )
                    {
                        continue;
                    }

                    itemModifyingGUI_Inventory[iRemoveCurseGUIInventoryIndex - itemModifyingGUI_InventoryScrollOffset - 1] = item;

                    if ( iRemoveCurseGUIInventoryIndex > 3 + itemModifyingGUI_InventoryScrollOffset )
                    {
                        bBreakFromLoop = true;
                    }
                }
                break;
            default: printlog("ERROR: rebuildRemoveCurseGUIInventory() - itemModifyingGUI_ScrollBeatitude (%d) out of bounds", itemModifyingGUI_ScrollBeatitude); return;
        }

        if ( bBreakFromLoop == true )
        {
            break;
        }
    }
} // rebuildRemoveCurseGUIInventory()

/* ItemModifyingGUI.cpp
 * Draws the Sprite Image and description for the given Item in each of the visible GUI Inventory slots
 */
void ItemModifyingGUI::removeCurseGUI_HandleItemImages()
{
    Uint8 iRemoveCurseGUIInventoryIndex = 0; // The position in the GUI Inventory array of the given Item
    Sint32 iRemoveCurseGUIInventoryItemOffset = (((yres / 2) - (itemModifyingGUI_IMG->h / 2)) + itemModifyingGUI_OffsetY) + 22; // The amount that each Item is offset vertically from each other

    Item* item = nullptr; // The given Item being drawn from the GUI Inventory
    SDL_Rect itemImageRect; // The position of the Image of the Item in the GUI Inventory

    // Grab the Player's Inventory again, as it may have been modified by processGUIEffectOnItem()
    list_t* playerInventoryList = &stats[clientnum]->inventory;

    for ( node_t* iInventoryNode = playerInventoryList->first; iInventoryNode != nullptr; iInventoryNode = iInventoryNode->next )
    {
        if ( iInventoryNode->element != nullptr )
        {
            item = static_cast<Item*>(iInventoryNode->element);

            if ( item == nullptr )
            {
                continue;
            }

            // Cursed Scrolls don't need to be shown, as they wont have a GUI
            if ( item->identified == true && item->beatitude < 0 )
            {
                iRemoveCurseGUIInventoryIndex++;

                // If the Item is not visible (within 0 - 3 in Inventory), skip over it
                if ( iRemoveCurseGUIInventoryIndex <= itemModifyingGUI_InventoryScrollOffset )
                {
                    continue;
                }

                // The name and description of the Item in the GUI Inventory
                char itemDescription[64] = {0};
                strncpy(itemDescription, item->description(), 46);

                // Check to make sure the text doesn't go out of bounds
                if ( strlen(itemDescription) == 46 )
                {
                    strcat(itemDescription, " ...");
                }

                // Print the name and description of the Item
                ttfPrintText(ttf8, (((xres / 2) - (itemModifyingGUI_IMG->w / 2)) + itemModifyingGUI_OffsetX) + 36, iRemoveCurseGUIInventoryItemOffset, itemDescription);

                // Draw the Image of the Item
                itemImageRect.x = (((xres / 2) - (itemModifyingGUI_IMG->w / 2)) + itemModifyingGUI_OffsetX) + 16;
                itemImageRect.y = (((yres / 2) - (itemModifyingGUI_IMG->h / 2)) + itemModifyingGUI_OffsetY) + 17 + 18 * (iRemoveCurseGUIInventoryIndex - itemModifyingGUI_InventoryScrollOffset - 1);
                itemImageRect.w = 16;
                itemImageRect.h = 16;
                drawImageScaled(itemSprite(item), nullptr, &itemImageRect);

                iRemoveCurseGUIInventoryItemOffset += 18;

                // If the Item's index is out of bounds (above the amount of Items in the Inventory), stop drawing
                if ( iRemoveCurseGUIInventoryIndex > 3 + itemModifyingGUI_InventoryScrollOffset )
                {
                    break;
                }
            }
        }
    }
} // removeCurseGUI_HandleItemImages()

// REPAIR GUI

/* ItemModifyingGUI.cpp
 * @param selectedItem - The Item that is being processed, selected from the GUI Inventory
 * Repairs the selected Item based on 'itemModifyingGUI_ScrollBeatitude' and 'itemModifyingGUI_ScrollBeatitude'
 * +0 Scrolls cannot repair Broken or Serviceable Items. +0 can repair Decrepit and Worn up to Serviceable
 * +1 Scrolls cannot repair Serviceable Items. +1 can repair Broken, Decrepit, and Worn up to Serviceable
 * +2 Scrolls can repair all Items up to Excellent
 * Messages the Player that their Item has been Repaired, then if the GUI was opened with a Scroll, it is consumed before closing the GUI
 */
void ItemModifyingGUI::processRepairGUIEffectOnItem(Item* const selectedItem)
{
    if ( selectedItem == nullptr )
    {
        printlog("ERROR: processRepairGUIEffectOnItem() - selectedItem is null.");
        return;
    }

    // Cursed Scrolls don't use the GUI so they will never call this function
    switch ( itemModifyingGUI_ScrollBeatitude )
    {
        case 0:
            // Intentional fall-through
        case 1:
            // Repair the Item if it is Broken (+1 only), Decrepit, or Worn up to Serviceable
            selectedItem->status = SERVICABLE;
            messagePlayer(clientnum, language[872], selectedItem->description()); // "Your %s looks better!"
            break;
        case 2:
            // Repair the Item if it is Broken, Decrepit, Worn, or Serviceable up to Excellent
            selectedItem->status = EXCELLENT;
            messagePlayer(clientnum, language[2517], selectedItem->description()); // "Your %s looks perfect now!"
            break;
        default: printlog("ERROR: processRepairGUIEffectOnItem() - itemModifyingGUI_ScrollBeatitude (%d) out of bounds.", itemModifyingGUI_ScrollBeatitude); return;
    }

    // If the Player used a Scroll, consume it. Currently there is no Spell of Repair
    if ( itemModifyingGUI_ScrollUsed != nullptr )
    {
        consumeItem(itemModifyingGUI_ScrollUsed);
        itemModifyingGUI_ScrollUsed = nullptr;
    }

    closeItemModifyingGUI();

    // TODOR: Why does the Client need to be told, this doesn't need to be networked likely
    if ( multiplayer == CLIENT && itemIsEquipped(selectedItem, clientnum) )
    {
        // The Client needs to inform the Server that their equipment was repaired

        // Convert the Item type to an int for the packet
        Uint8 itemType = 0;
        if ( selectedItem == stats[clientnum]->helmet )
        {
            itemType = 0;
        }
        else if ( selectedItem == stats[clientnum]->breastplate )
        {
            itemType = 1;
        }
        else if ( selectedItem == stats[clientnum]->gloves )
        {
            itemType = 2;
        }
        else if ( selectedItem == stats[clientnum]->shoes )
        {
            itemType = 3;
        }
        else if ( selectedItem == stats[clientnum]->shield )
        {
            itemType = 4;
        }
        else if ( selectedItem == stats[clientnum]->weapon )
        {
            itemType = 5;
        }
        else if ( selectedItem == stats[clientnum]->cloak )
        {
            itemType = 6;
        }
        else if ( selectedItem == stats[clientnum]->amulet )
        {
            itemType = 7;
        }
        else if ( selectedItem == stats[clientnum]->ring )
        {
            itemType = 8;
        }
        else if ( selectedItem == stats[clientnum]->mask )
        {
            itemType = 9;
        }

        strcpy((char*)net_packet->data, "ARMR");
        net_packet->data[4] = itemType;
        net_packet->data[5] = selectedItem->status;
        net_packet->address.host = net_server.host;
        net_packet->address.port = net_server.port;
        net_packet->len = 6;
        sendPacketSafe(net_sock, -1, net_packet, 0);
    }
} // processRepairGUIEffectOnItem()

/* ItemModifyingGUI.cpp
 * @param numItems - The number of random Items to be processed. Currently this number should never exceed 2, as the highest naturally spawning Scrolls are +-2
 * Processes a random amount of valid Items equal to 'numItems'. Processing will reduce the random Items to Broken
 * Messages the Player that their Item has been Broken because of the beatitude of the Scroll used
 */
void ItemModifyingGUI::repairGUIProcessRandomItem(const Uint8 numItems)
{
    // Grab the Player's Inventory again, as it may have been modified prior to this
    list_t* playerInventoryList = &stats[clientnum]->inventory;

    if ( playerInventoryList == nullptr )
    {
        messagePlayer(0, "Warning: stats[%d]->inventory is not a valid list. This should not happen.", clientnum);
        printlog("ERROR: repairGUIProcessRandomItem() - stats[%d]->inventory is not a valid list.", clientnum);
        return;
    }

    // Count the number of Items in the GUI Inventory
    Uint8 totalAmountOfItems = 0;
    for ( node_t* iInventoryNode = playerInventoryList->first; iInventoryNode != nullptr; iInventoryNode = iInventoryNode->next )
    {
        Item* item = static_cast<Item*>(iInventoryNode->element);

        if ( item == nullptr )
        {
            continue;
        }

        // Skip all Items that don't need to be repaired TODOR: Item's should have a "repairable" component
        if ( itemCategory(item) != WEAPON && itemCategory(item) != ARMOR && item->type != AMULET_LIFESAVING && item->type != AMULET_MAGICREFLECTION && itemCategory(item) != MAGICSTAFF && item->type != TOOL_PICKAXE )
        {
            continue;
        }

        switch ( itemModifyingGUI_ScrollBeatitude )
        {
            case -2:
                // Intentional fall-through
            case -1:
                if ( item->status != BROKEN ) // Skip over all Broken Items
                {
                    totalAmountOfItems++;
                }
                break;
            default: printlog("ERROR: repairGUIProcessRandomItem() - itemModifyingGUI_ScrollBeatitude (%d) out of bounds", itemModifyingGUI_ScrollBeatitude); return;
        }
    }

    // There are no valid Items to be processed
    if ( totalAmountOfItems == 0 && itemModifyingGUI_ScrollBeatitude > 0 )
    {
        messagePlayer(clientnum, language[2510]); // "You have no remaining items for the scroll's blessing..."
        return;
    }
    else if ( totalAmountOfItems == 0 && itemModifyingGUI_ScrollBeatitude < 0 )
    {
        messagePlayer(clientnum, language[2510]); // "You have no items for the scroll's curse..."
        return;
    }

    Uint8 amountOfItemsLeftToProcess = numItems;
    bool isThereASingleItem = false;

    // If there is only a single Item that can be processed, but it's a +-2 Scroll, adjust the loop
    if ( totalAmountOfItems == 1 && itemModifyingGUI_ScrollBeatitude >= 2 )
    {
        isThereASingleItem = true;
        amountOfItemsLeftToProcess--;
    }
    else if ( totalAmountOfItems == 1 && itemModifyingGUI_ScrollBeatitude <= -2 )
    {
        isThereASingleItem = true;
        amountOfItemsLeftToProcess--;
    }

    Sint8 iPreviousItemProcessedIndex = -1;
    Sint8 iItemToProcessIndex = -1;
    Uint8 iRepairGUIInventoryIndex = 0;

    for ( Uint8 i = 0; i < numItems; i++ )
    {
        while ( iItemToProcessIndex == iPreviousItemProcessedIndex )
        {
            iItemToProcessIndex = rand() % totalAmountOfItems;
        }

        iRepairGUIInventoryIndex = 0;

        for ( node_t* iInventoryNode = playerInventoryList->first; iInventoryNode != nullptr; iInventoryNode = iInventoryNode->next )
        {
            Item* itemToProcess = static_cast<Item*>(iInventoryNode->element);

            if ( itemToProcess == nullptr )
            {
                continue;
            }

            // Skip all Items that don't need to be repaired TODOR: Item's should have a "repairable" component
            if ( itemCategory(itemToProcess) == POTION || itemCategory(itemToProcess) == SCROLL || itemCategory(itemToProcess) == SPELLBOOK || itemCategory(itemToProcess) == GEM || itemCategory(itemToProcess) == THROWN || itemCategory(itemToProcess) == FOOD || itemCategory(itemToProcess) == BOOK || itemCategory(itemToProcess) == SPELL_CAT )
            {
                continue;
            }

            switch ( itemModifyingGUI_ScrollBeatitude )
            {
                case -2:
                    // Intentional fall-through
                case -1:
                    if ( itemToProcess->status != BROKEN ) // Skip over all Cursed Items
                    {
                        if ( iRepairGUIInventoryIndex == iItemToProcessIndex )
                        {
                            if ( numItems == 2 && amountOfItemsLeftToProcess == 1 && isThereASingleItem != true )
                            {
                                messagePlayer(clientnum, language[2516], itemToProcess->description()); // "Oh no! Your %s was broken too!"
                            }
                            else
                            {
                                messagePlayer(clientnum, language[2508], itemToProcess->description()); // "Your %s is now broken!"
                            }

                            itemToProcess->status = BROKEN;
                            amountOfItemsLeftToProcess--;
                            iPreviousItemProcessedIndex = iRepairGUIInventoryIndex;
                        }
                        iRepairGUIInventoryIndex++;
                    }
                    break;
                default: printlog("ERROR: repairGUIProcessRandomItem() - itemModifyingGUI_ScrollBeatitude (%d) out of bounds", itemModifyingGUI_ScrollBeatitude); return;
            }

            // If processing is done, stop searching the Inventory
            if ( amountOfItemsLeftToProcess == 0 )
            {
                break;
            }
        }

        // If there's only a single valid Item, but it's a +-2 scroll, this will happen
        if ( amountOfItemsLeftToProcess == 0 )
        {
            break;
        }
    }

    // Tell the Player they were an Item short of the full effect
    if ( isThereASingleItem == true && itemModifyingGUI_ScrollBeatitude > 0 )
    {
        messagePlayer(clientnum, language[2510]); // "You have no remaining items for the scroll's blessing..."
    }
    else if ( isThereASingleItem == true && itemModifyingGUI_ScrollBeatitude < 0 )
    {
        messagePlayer(clientnum, language[2524]); // "You have no remaining items for the scroll's curse..."
    }
} // repairGUIProcessRandomItem()

/* ItemModifyingGUI.cpp
 * Builds the GUI's Inventory based off of 'itemModifyingGUI_Type' and 'itemModifyingGUI_ScrollBeatitude'
 * Counts the number of Items to add to the GUI's Inventory, then assigns the visible ones to their position in 'itemModifyingGUI_Inventory[]'
 */
void ItemModifyingGUI::rebuildRepairGUIInventory()
{
    // Grab the Player's Inventory again, as it may have been modified by processGUIEffectOnItem()
    list_t* playerInventoryList = &stats[clientnum]->inventory;
    Uint8 iRepairGUIInventoryIndex = 0;

    if ( playerInventoryList == nullptr )
    {
        messagePlayer(0, "Warning: stats[%d]->inventory is not a valid list. This should not happen.", clientnum);
        printlog("ERROR: rebuildRepairGUIInventory() - stats[%d]->inventory is not a valid list.", clientnum);
        return;
    }

    // Count the number of Items in the Repair GUI Inventory
    for ( node_t* iInventoryNode = playerInventoryList->first; iInventoryNode != nullptr; iInventoryNode = iInventoryNode->next )
    {
        Item* item = static_cast<Item*>(iInventoryNode->element);

        if ( item == nullptr )
        {
            continue;
        }

        // Skip all Items that don't need to be repaired TODOR: Item's should have a "repairable" component
        if ( itemCategory(item) != WEAPON && itemCategory(item) != ARMOR && item->type != AMULET_LIFESAVING && item->type != AMULET_MAGICREFLECTION && itemCategory(item) != MAGICSTAFF && item->type != TOOL_PICKAXE )
        {
            continue;
        }

        switch ( itemModifyingGUI_ScrollBeatitude )
        {
            case -2:
                // Intentional fall-through
            case -1:
                if ( item->status != BROKEN ) // Skip over all Broken Items
                {
                    ++iRepairGUIInventoryIndex;
                }
                break;
            case 0:
                if ( item->status != BROKEN && item->status != SERVICABLE && item->status != EXCELLENT ) // Skip over all Broken, Serviceable, and Excellent Items
                {
                    ++iRepairGUIInventoryIndex;
                }
                break;
            case 1:
                if ( item->status != SERVICABLE && item->status != EXCELLENT ) // Skip over all Serviceable and Excellent Items
                {
                    ++iRepairGUIInventoryIndex;
                }
                break;
            case 2:
                if ( item->status != EXCELLENT ) // Skip over all Excellent Items
                {
                    ++iRepairGUIInventoryIndex;
                }
                break;
            default: printlog("ERROR: rebuildRepairGUIInventory() - itemModifyingGUI_ScrollBeatitude (%d) out of bounds", itemModifyingGUI_ScrollBeatitude); return;
        }
    }

    itemModifyingGUI_InventoryScrollOffset = std::max(0, std::min(static_cast<int>(itemModifyingGUI_InventoryScrollOffset), iRepairGUIInventoryIndex - 4));
        
    // Reset the current Inventory
    for ( iRepairGUIInventoryIndex = 0; iRepairGUIInventoryIndex < 4; ++iRepairGUIInventoryIndex )
    {
        itemModifyingGUI_Inventory[iRepairGUIInventoryIndex] = nullptr;
    }
        
    iRepairGUIInventoryIndex = 0;

    // Assign the visible Items to the GUI slots
    bool bBreakFromLoop = false;
    for ( node_t* iInventoryNode = playerInventoryList->first; iInventoryNode != nullptr; iInventoryNode = iInventoryNode->next )
    {
        Item* item = static_cast<Item*>(iInventoryNode->element);

        if ( item == nullptr )
        {
            continue;
        }

        // Skip all Items that don't need to be repaired TODOR: Item's should have a "repairable" component
        if ( itemCategory(item) != WEAPON && itemCategory(item) != ARMOR && item->type != AMULET_LIFESAVING && item->type != AMULET_MAGICREFLECTION && itemCategory(item) != MAGICSTAFF && item->type != TOOL_PICKAXE )
        {
            continue;
        }

        switch ( itemModifyingGUI_ScrollBeatitude )
        {
            case -2:
                // Intentional fall-through
            case -1:
                if ( item->status != BROKEN ) // Skip over all Broken Items
                {
                    ++iRepairGUIInventoryIndex;

                    if ( iRepairGUIInventoryIndex <= itemModifyingGUI_InventoryScrollOffset )
                    {
                        continue;
                    }

                    itemModifyingGUI_Inventory[iRepairGUIInventoryIndex - itemModifyingGUI_InventoryScrollOffset - 1] = item;

                    if ( iRepairGUIInventoryIndex > 3 + itemModifyingGUI_InventoryScrollOffset )
                    {
                        bBreakFromLoop = true;
                    }
                }
                break;
            case 0:
                if ( item->status != BROKEN && item->status != SERVICABLE && item->status != EXCELLENT ) // Skip over all Broken, Serviceable, and Excellent Items
                {
                    ++iRepairGUIInventoryIndex;

                    if ( iRepairGUIInventoryIndex <= itemModifyingGUI_InventoryScrollOffset )
                    {
                        continue;
                    }

                    itemModifyingGUI_Inventory[iRepairGUIInventoryIndex - itemModifyingGUI_InventoryScrollOffset - 1] = item;

                    if ( iRepairGUIInventoryIndex > 3 + itemModifyingGUI_InventoryScrollOffset )
                    {
                        bBreakFromLoop = true;
                    }
                }
                break;
            case 1:
                if ( item->status != SERVICABLE && item->status != EXCELLENT ) // Skip over all Serviceable, and Excellent Items
                {
                    ++iRepairGUIInventoryIndex;

                    if ( iRepairGUIInventoryIndex <= itemModifyingGUI_InventoryScrollOffset )
                    {
                        continue;
                    }

                    itemModifyingGUI_Inventory[iRepairGUIInventoryIndex - itemModifyingGUI_InventoryScrollOffset - 1] = item;

                    if ( iRepairGUIInventoryIndex > 3 + itemModifyingGUI_InventoryScrollOffset )
                    {
                        bBreakFromLoop = true;
                    }
                }
                break;
            case 2:
                if ( item->status != EXCELLENT ) // Skip over all Excellent Items
                {
                    ++iRepairGUIInventoryIndex;

                    if ( iRepairGUIInventoryIndex <= itemModifyingGUI_InventoryScrollOffset )
                    {
                        continue;
                    }

                    itemModifyingGUI_Inventory[iRepairGUIInventoryIndex - itemModifyingGUI_InventoryScrollOffset - 1] = item;

                    if ( iRepairGUIInventoryIndex > 3 + itemModifyingGUI_InventoryScrollOffset )
                    {
                        bBreakFromLoop = true;
                    }
                }
                break;
            default: printlog("ERROR: rebuildRepairGUIInventory() - itemModifyingGUI_ScrollBeatitude (%d) out of bounds", itemModifyingGUI_ScrollBeatitude); return;
        }

        if ( bBreakFromLoop == true )
        {
            break;
        }
    }
} // rebuildRepairGUIInventory()

/* ItemModifyingGUI.cpp
 * Draws the Sprite Image and description for the given Item in each of the visible GUI Inventory slots
 */
void ItemModifyingGUI::repairGUI_HandleItemImages()
{
    bool bBreakFromLoop = false; // True if evaluation of drawing Images has ended (all viable Images have been processed)
    Uint8 iRepairGUIInventoryIndex = 0; // The position in the GUI Inventory array of the given Item
    Sint32 iRepairGUIInventoryItemOffset = (((yres / 2) - (itemModifyingGUI_IMG->h / 2)) + itemModifyingGUI_OffsetY) + 22; // The amount that each Item is offset vertically from each other

    Item* item = nullptr; // The given Item being drawn from the GUI Inventory
    SDL_Rect itemImageRect; // The position of the Image of the Item in the GUI Inventory

    // Grab the Player's Inventory again, as it may have been modified by processGUIEffectOnItem()
    list_t* playerInventoryList = &stats[clientnum]->inventory;

    for ( node_t* iInventoryNode = playerInventoryList->first; iInventoryNode != nullptr; iInventoryNode = iInventoryNode->next )
    {
        if ( iInventoryNode->element != nullptr )
        {
            item = static_cast<Item*>(iInventoryNode->element);

            if ( item == nullptr )
            {
                continue;
            }

            // Skip all Items that don't need to be repaired TODOR: Item's should have a "repairable" component
            if ( itemCategory(item) != WEAPON && itemCategory(item) != ARMOR && item->type != AMULET_LIFESAVING && item->type != AMULET_MAGICREFLECTION && itemCategory(item) != MAGICSTAFF && item->type != TOOL_PICKAXE )
            {
                continue;
            }

            // Cursed Scrolls don't need to be shown, as they wont have a GUI
            switch ( itemModifyingGUI_ScrollBeatitude )
            {
                case 0:
                    if ( item->status != BROKEN && item->status != SERVICABLE && item->status != EXCELLENT ) // Skip over all Broken, Serviceable, and Excellent Items
                    {
                        iRepairGUIInventoryIndex++;

                        // If the Item is not visible (within 0 - 3 in Inventory), skip over it
                        if ( iRepairGUIInventoryIndex <= itemModifyingGUI_InventoryScrollOffset )
                        {
                            continue;
                        }

                        // The name and description of the Item in the GUI Inventory
                        char itemDescription[64] = {0};
                        strncpy(itemDescription, item->description(), 46);

                        // Check to make sure the text doesn't go out of bounds
                        if ( strlen(itemDescription) == 46 )
                        {
                            strcat(itemDescription, " ...");
                        }

                        // Print the name and description of the Item
                        ttfPrintText(ttf8, (((xres / 2) - (itemModifyingGUI_IMG->w / 2)) + itemModifyingGUI_OffsetX) + 36, iRepairGUIInventoryItemOffset, itemDescription);

                        // Draw the Image of the Item
                        itemImageRect.x = (((xres / 2) - (itemModifyingGUI_IMG->w / 2)) + itemModifyingGUI_OffsetX) + 16;
                        itemImageRect.y = (((yres / 2) - (itemModifyingGUI_IMG->h / 2)) + itemModifyingGUI_OffsetY) + 17 + 18 * (iRepairGUIInventoryIndex - itemModifyingGUI_InventoryScrollOffset - 1);
                        itemImageRect.w = 16;
                        itemImageRect.h = 16;
                        drawImageScaled(itemSprite(item), nullptr, &itemImageRect);

                        iRepairGUIInventoryItemOffset += 18;

                        // If the Item's index is out of bounds (above the amount of Items in the Inventory), stop drawing
                        if ( iRepairGUIInventoryIndex > 3 + itemModifyingGUI_InventoryScrollOffset )
                        {
                            bBreakFromLoop = true;
                        }
                    }
                    break;
                case 1:
                    if ( item->status != SERVICABLE && item->status != EXCELLENT ) // Skip over all Serviceable, and Excellent Items
                    {
                        iRepairGUIInventoryIndex++;

                        // If the Item is not visible (within 0 - 3 in Inventory), skip over it
                        if ( iRepairGUIInventoryIndex <= itemModifyingGUI_InventoryScrollOffset )
                        {
                            continue;
                        }

                        // The name and description of the Item in the GUI Inventory
                        char itemDescription[64] = {0};
                        strncpy(itemDescription, item->description(), 46);

                        // Check to make sure the text doesn't go out of bounds
                        if ( strlen(itemDescription) == 46 )
                        {
                            strcat(itemDescription, " ...");
                        }

                        // Print the name and description of the Item
                        ttfPrintText(ttf8, (((xres / 2) - (itemModifyingGUI_IMG->w / 2)) + itemModifyingGUI_OffsetX) + 36, iRepairGUIInventoryItemOffset, itemDescription);

                        // Draw the Image of the Item
                        itemImageRect.x = (((xres / 2) - (itemModifyingGUI_IMG->w / 2)) + itemModifyingGUI_OffsetX) + 16;
                        itemImageRect.y = (((yres / 2) - (itemModifyingGUI_IMG->h / 2)) + itemModifyingGUI_OffsetY) + 17 + 18 * (iRepairGUIInventoryIndex - itemModifyingGUI_InventoryScrollOffset - 1);
                        itemImageRect.w = 16;
                        itemImageRect.h = 16;
                        drawImageScaled(itemSprite(item), nullptr, &itemImageRect);

                        iRepairGUIInventoryItemOffset += 18;

                        // If the Item's index is out of bounds (above the amount of Items in the Inventory), stop drawing
                        if ( iRepairGUIInventoryIndex > 3 + itemModifyingGUI_InventoryScrollOffset )
                        {
                            bBreakFromLoop = true;
                        }
                    }
                    break;
                case 2:
                    if ( item->status != EXCELLENT ) // Skip over all Excellent Items
                    {
                        iRepairGUIInventoryIndex++;

                        // If the Item is not visible (within 0 - 3 in Inventory), skip over it
                        if ( iRepairGUIInventoryIndex <= itemModifyingGUI_InventoryScrollOffset )
                        {
                            continue;
                        }

                        // The name and description of the Item in the GUI Inventory
                        char itemDescription[64] = {0};
                        strncpy(itemDescription, item->description(), 46);

                        // Check to make sure the text doesn't go out of bounds
                        if ( strlen(itemDescription) == 46 )
                        {
                            strcat(itemDescription, " ...");
                        }

                        // Print the name and description of the Item
                        ttfPrintText(ttf8, (((xres / 2) - (itemModifyingGUI_IMG->w / 2)) + itemModifyingGUI_OffsetX) + 36, iRepairGUIInventoryItemOffset, itemDescription);

                        // Draw the Image of the Item
                        itemImageRect.x = (((xres / 2) - (itemModifyingGUI_IMG->w / 2)) + itemModifyingGUI_OffsetX) + 16;
                        itemImageRect.y = (((yres / 2) - (itemModifyingGUI_IMG->h / 2)) + itemModifyingGUI_OffsetY) + 17 + 18 * (iRepairGUIInventoryIndex - itemModifyingGUI_InventoryScrollOffset - 1);
                        itemImageRect.w = 16;
                        itemImageRect.h = 16;
                        drawImageScaled(itemSprite(item), nullptr, &itemImageRect);

                        iRepairGUIInventoryItemOffset += 18;

                        // If the Item's index is out of bounds (above the amount of Items in the Inventory), stop drawing
                        if ( iRepairGUIInventoryIndex > 3 + itemModifyingGUI_InventoryScrollOffset )
                        {
                            bBreakFromLoop = true;
                        }
                    }
                    break;
                default: printlog("ERROR: repairGUI_HandleItemImages() - itemModifyingGUI_ScrollBeatitude (%d) out of bounds", itemModifyingGUI_ScrollBeatitude); return;
            }

            if ( bBreakFromLoop == true )
            {
                break;
            }
        }
    }
} // repairGUI_HandleItemImages()

// ENCHANT WEAPON GUI

/* ItemModifyingGUI.cpp
 * @param selectedItem - The Item that is being processed, selected from the GUI Inventory
 * Enchants the selected Item based on 'itemModifyingGUI_ScrollBeatitude' and 'itemModifyingGUI_ScrollBeatitude'
 * +0 Scrolls can raise a +0 or +1 weapon up to +2. Cannot Enchant Artifacts
 * +1 Scrolls can raise a +0, +1, or +2 weapon up to +3. Cannot Enchant Artifacts
 * +2 Scrolls can raise a +0, +1, +2, +3, or +4 weapon up to +5. Raises any Artifact Weapon by one level
 * Messages the Player that their Item has been Enchanted, then if the GUI was opened with a Scroll, it is consumed before closing the GUI
 */
void ItemModifyingGUI::processEnchantWeaponGUIEffectOnItem(Item* const selectedItem)
{
    if ( selectedItem == nullptr )
    {
        printlog("ERROR: processEnchantWeaponGUIEffectOnItem() - selectedItem is null.");
        return;
    }

    // Cursed Scrolls don't use the GUI so they will never call this function
    ItemType itemType = WOODEN_SHIELD;
    switch ( itemModifyingGUI_ScrollBeatitude )
    {
        case 0:
            // Enchant the Item if it is a weapon +0 or +1 up to +2. Cannot Enchant Artifacts
            messagePlayer(clientnum, language[2520], selectedItem->description()); // "Your %s glows blue!"
            selectedItem->beatitude = 2;
            break;
        case 1:
            // Enchant the Item if it is a weapon +0, +1, or +2 up to +3. Cannot Enchant Artifacts
            messagePlayer(clientnum, language[2521], selectedItem->description()); // "Your %s violently glows blue!"
            selectedItem->beatitude = 3;
            break;
        case 2:
            // Enchant the Item if it is a weapon +0, +1, +2, +3, or +4 up to +5. Raises any Artifact Weapon by one level
            messagePlayer(clientnum, language[2522], selectedItem->description()); // "Your %s radiates power!"
            itemType = selectedItem->type;
            if ( itemType == ARTIFACT_SWORD || itemType == ARTIFACT_MACE || itemType == ARTIFACT_SPEAR || itemType == ARTIFACT_AXE || itemType == ARTIFACT_BOW )
            {
                selectedItem->beatitude++;
            }
            else
            {
                selectedItem->beatitude = 5;
            }
            break;
        default: printlog("ERROR: processEnchantWeaponGUIEffectOnItem() - itemModifyingGUI_ScrollBeatitude (%d) out of bounds.", itemModifyingGUI_ScrollBeatitude); return;
    }

    // If the Player used a Scroll, consume it. Currently there is no Spell of Enchant Weapon
    if ( itemModifyingGUI_ScrollUsed != nullptr )
    {
        consumeItem(itemModifyingGUI_ScrollUsed);
        itemModifyingGUI_ScrollUsed = nullptr;
    }

    closeItemModifyingGUI();
} // processEnchantWeaponGUIEffectOnItem()

/* ItemModifyingGUI.cpp
 * @param numItems - The number of random Items to be processed. Currently this number should never exceed 2, as the highest naturally spawning Scrolls are +-2
 * Processes a random amount of valid Items equal to 'numItems'. Processing will reduce the random Items to +0
 * Messages the Player that their Item has been Disenchanted because of the beatitude of the Scroll used
 */
void ItemModifyingGUI::enchantWeaponGUIProcessRandomItem(const Uint8 numItems)
{
    // Grab the Player's Inventory again, as it may have been modified prior to this
    list_t* playerInventoryList = &stats[clientnum]->inventory;

    if ( playerInventoryList == nullptr )
    {
        messagePlayer(0, "Warning: stats[%d]->inventory is not a valid list. This should not happen.", clientnum);
        printlog("ERROR: enchantWeaponGUIProcessRandomItem() - stats[%d]->inventory is not a valid list.", clientnum);
        return;
    }

    // Count the number of Items in the GUI Inventory
    Uint8 totalAmountOfItems = 0;
    for ( node_t* iInventoryNode = playerInventoryList->first; iInventoryNode != nullptr; iInventoryNode = iInventoryNode->next )
    {
        Item* item = static_cast<Item*>(iInventoryNode->element);

        if ( item == nullptr )
        {
            continue;
        }

        // Skip all Items that aren't Weapons
        if ( itemCategory(item) != WEAPON )
        {
            continue;
        }

        switch ( itemModifyingGUI_ScrollBeatitude )
        {
            case -2:
                // Intentional fall-through
            case -1:
                if ( item->beatitude > 0) // Skip over all Cursed and Unenchanted Items
                {
                    totalAmountOfItems++;
                }
                break;
            default: printlog("ERROR: enchantWeaponGUIProcessRandomItem() - itemModifyingGUI_ScrollBeatitude (%d) out of bounds", itemModifyingGUI_ScrollBeatitude); return;
        }
    }

    // There are no valid Items to be processed
    if ( totalAmountOfItems == 0 && itemModifyingGUI_ScrollBeatitude > 0 )
    {
        messagePlayer(clientnum, language[2510]); // "You have no remaining items for the scroll's blessing..."
        return;
    }
    else if ( totalAmountOfItems == 0 && itemModifyingGUI_ScrollBeatitude < 0 )
    {
        messagePlayer(clientnum, language[2510]); // "You have no items for the scroll's curse..."
        return;
    }

    Uint8 amountOfItemsLeftToProcess = numItems;
    bool isThereASingleItem = false;

    // If there is only a single Item that can be processed, but it's a +-2 Scroll, adjust the loop
    if ( totalAmountOfItems == 1 && itemModifyingGUI_ScrollBeatitude >= 2 )
    {
        isThereASingleItem = true;
        amountOfItemsLeftToProcess--;
    }
    else if ( totalAmountOfItems == 1 && itemModifyingGUI_ScrollBeatitude <= -2 )
    {
        isThereASingleItem = true;
        amountOfItemsLeftToProcess--;
    }

    Sint8 iPreviousItemProcessedIndex = -1;
    Sint8 iItemToProcessIndex = -1;
    Uint8 iEnchantWeaponGUIInventoryIndex = 0;

    for ( Uint8 i = 0; i < numItems; i++ )
    {
        while ( iItemToProcessIndex == iPreviousItemProcessedIndex )
        {
            iItemToProcessIndex = rand() % totalAmountOfItems;
        }

        iEnchantWeaponGUIInventoryIndex = 0;

        for ( node_t* iInventoryNode = playerInventoryList->first; iInventoryNode != nullptr; iInventoryNode = iInventoryNode->next )
        {
            Item* itemToProcess = static_cast<Item*>(iInventoryNode->element);

            if ( itemToProcess == nullptr )
            {
                continue;
            }

            // Skip all Items that aren't Weapons
            if ( itemCategory(itemToProcess) != WEAPON )
            {
                continue;
            }

            switch ( itemModifyingGUI_ScrollBeatitude )
            {
                case -2:
                    // Intentional fall-through
                case -1:
                    if ( itemToProcess->beatitude > 0 ) // Skip over all Cursed and Unenchanted Items
                    {
                        if ( iEnchantWeaponGUIInventoryIndex == iItemToProcessIndex )
                        {
                            if ( numItems == 2 && amountOfItemsLeftToProcess == 1 && isThereASingleItem != true )
                            {
                                messagePlayer(clientnum, language[2523], itemToProcess->description()); // "Oh no! Your %s was disenchanted too!"
                            }
                            else
                            {
                                messagePlayer(clientnum, language[2509], itemToProcess->description()); // "Your %s was disenchanted!"
                            }

                            itemToProcess->beatitude = 0;
                            amountOfItemsLeftToProcess--;
                            iPreviousItemProcessedIndex = iEnchantWeaponGUIInventoryIndex;
                        }
                        iEnchantWeaponGUIInventoryIndex++;
                    }
                    break;
                default: printlog("ERROR: enchantWeaponGUIProcessRandomItem() - itemModifyingGUI_ScrollBeatitude (%d) out of bounds", itemModifyingGUI_ScrollBeatitude); return;
            }

            // If processing is done, stop searching the Inventory
            if ( amountOfItemsLeftToProcess == 0 )
            {
                break;
            }
        }

        // If there's only a single valid Item, but it's a +-2 scroll, this will happen
        if ( amountOfItemsLeftToProcess == 0 )
        {
            break;
        }
    }

    // Tell the Player they were an Item short of the full effect
    if ( isThereASingleItem == true && itemModifyingGUI_ScrollBeatitude > 0 )
    {
        messagePlayer(clientnum, language[2510]); // "You have no remaining items for the scroll's blessing..."
    }
    else if ( isThereASingleItem == true && itemModifyingGUI_ScrollBeatitude < 0 )
    {
        messagePlayer(clientnum, language[2524]); // "You have no remaining items for the scroll's curse..."
    }
} // enchantWeaponGUIProcessRandomItem()

/* ItemModifyingGUI.cpp
 * Builds the GUI's Inventory based off of 'itemModifyingGUI_Type' and 'itemModifyingGUI_ScrollBeatitude'
 * Counts the number of Items to add to the GUI's Inventory, then assigns the visible ones to their position in 'itemModifyingGUI_Inventory[]'
 */
void ItemModifyingGUI::rebuildEnchantWeaponGUIInventory()
{
    // Grab the Player's Inventory again, as it may have been modified by processGUIEffectOnItem()
    list_t* playerInventoryList = &stats[clientnum]->inventory;
    Uint8 iEnchantWeaponGUIInventoryIndex = 0;

    if ( playerInventoryList == nullptr )
    {
        messagePlayer(0, "Warning: stats[%d]->inventory is not a valid list. This should not happen.", clientnum);
        printlog("ERROR: rebuildEnchantWeaponGUIInventory() - stats[%d]->inventory is not a valid list.", clientnum);
        return;
    }

    // Count the number of Items in the Repair GUI Inventory
    for ( node_t* iInventoryNode = playerInventoryList->first; iInventoryNode != nullptr; iInventoryNode = iInventoryNode->next )
    {
        Item* item = static_cast<Item*>(iInventoryNode->element);

        if ( item == nullptr )
        {
            continue;
        }

        // Skip all Items that aren't Weapons
        if ( itemCategory(item) != WEAPON )
        {
            continue;
        }

        switch ( itemModifyingGUI_ScrollBeatitude )
        {
            case -2:
                // Intentional fall-through
            case -1:
                if ( item->beatitude > 0 ) // Skip over all Cursed and Unenchanted Items
                {
                    ++iEnchantWeaponGUIInventoryIndex;
                }
                break;
            case 0:
                if ( item->beatitude >= 0 && item->beatitude < 2 ) // Skip over all Cursed Items and Items +2 and greater. Skip Artifacts
                {
                    if ( item->type == ARTIFACT_SWORD || item->type == ARTIFACT_MACE || item->type == ARTIFACT_SPEAR || item->type == ARTIFACT_AXE || item->type == ARTIFACT_BOW )
                    {
                        continue;
                    }
                    ++iEnchantWeaponGUIInventoryIndex;
                }
                break;
            case 1:
                if ( item->beatitude >= 0 && item->beatitude < 3 ) // Skip over all Cursed Items and Items +3 and greater. Skip Artifacts
                {
                    if ( item->type == ARTIFACT_SWORD || item->type == ARTIFACT_MACE || item->type == ARTIFACT_SPEAR || item->type == ARTIFACT_AXE || item->type == ARTIFACT_BOW )
                    {
                        continue;
                    }
                    ++iEnchantWeaponGUIInventoryIndex;
                }
                break;
            case 2:
                if ( (item->beatitude >= 0 && item->beatitude < 5) || (item->beatitude >= 5 && (item->type == ARTIFACT_SWORD || item->type == ARTIFACT_MACE || item->type == ARTIFACT_SPEAR || item->type == ARTIFACT_AXE || item->type == ARTIFACT_BOW)) ) // Skip over all Cursed Items and Items +5 and greater, unless it's an Artifact Weapon
                {
                    ++iEnchantWeaponGUIInventoryIndex;
                }
                break;
            default: printlog("ERROR: rebuildEnchantWeaponGUIInventory() - itemModifyingGUI_ScrollBeatitude (%d) out of bounds", itemModifyingGUI_ScrollBeatitude); return;
        }
    }

    itemModifyingGUI_InventoryScrollOffset = std::max(0, std::min(static_cast<int>(itemModifyingGUI_InventoryScrollOffset), iEnchantWeaponGUIInventoryIndex - 4));

    // Reset the current Inventory
    for ( iEnchantWeaponGUIInventoryIndex = 0; iEnchantWeaponGUIInventoryIndex < 4; ++iEnchantWeaponGUIInventoryIndex )
    {
        itemModifyingGUI_Inventory[iEnchantWeaponGUIInventoryIndex] = nullptr;
    }

    iEnchantWeaponGUIInventoryIndex = 0;

    // Assign the visible Items to the GUI slots
    bool bBreakFromLoop = false;
    for ( node_t* iInventoryNode = playerInventoryList->first; iInventoryNode != nullptr; iInventoryNode = iInventoryNode->next )
    {
        Item* item = static_cast<Item*>(iInventoryNode->element);

        if ( item == nullptr )
        {
            continue;
        }

        // Skip all Items that aren't Weapons
        if ( itemCategory(item) != WEAPON )
        {
            continue;
        }

        switch ( itemModifyingGUI_ScrollBeatitude )
        {
            case -2:
                // Intentional fall-through
            case -1:
                if ( item->beatitude > 0 ) // Skip over all Cursed and Unenchanted Items
                {
                    ++iEnchantWeaponGUIInventoryIndex;

                    if ( iEnchantWeaponGUIInventoryIndex <= itemModifyingGUI_InventoryScrollOffset )
                    {
                        continue;
                    }

                    itemModifyingGUI_Inventory[iEnchantWeaponGUIInventoryIndex - itemModifyingGUI_InventoryScrollOffset - 1] = item;

                    if ( iEnchantWeaponGUIInventoryIndex > 3 + itemModifyingGUI_InventoryScrollOffset )
                    {
                        bBreakFromLoop = true;
                    }
                }
                break;
            case 0:
                if ( item->beatitude >= 0 && item->beatitude < 2 ) // Skip over all Cursed Items and Items +2 and greater. Skip Artifacts
                {
                    if ( item->type == ARTIFACT_SWORD || item->type == ARTIFACT_MACE || item->type == ARTIFACT_SPEAR || item->type == ARTIFACT_AXE || item->type == ARTIFACT_BOW )
                    {
                        continue;
                    }

                    ++iEnchantWeaponGUIInventoryIndex;

                    if ( iEnchantWeaponGUIInventoryIndex <= itemModifyingGUI_InventoryScrollOffset )
                    {
                        continue;
                    }

                    itemModifyingGUI_Inventory[iEnchantWeaponGUIInventoryIndex - itemModifyingGUI_InventoryScrollOffset - 1] = item;

                    if ( iEnchantWeaponGUIInventoryIndex > 3 + itemModifyingGUI_InventoryScrollOffset )
                    {
                        bBreakFromLoop = true;
                    }
                }
                break;
            case 1:
                if ( item->beatitude >= 0 && item->beatitude < 3 ) // Skip over all Cursed Items and Items +3 and greater. Skip Artifacts
                {
                    if ( item->type == ARTIFACT_SWORD || item->type == ARTIFACT_MACE || item->type == ARTIFACT_SPEAR || item->type == ARTIFACT_AXE || item->type == ARTIFACT_BOW )
                    {
                        continue;
                    }

                    ++iEnchantWeaponGUIInventoryIndex;

                    if ( iEnchantWeaponGUIInventoryIndex <= itemModifyingGUI_InventoryScrollOffset )
                    {
                        continue;
                    }

                    itemModifyingGUI_Inventory[iEnchantWeaponGUIInventoryIndex - itemModifyingGUI_InventoryScrollOffset - 1] = item;

                    if ( iEnchantWeaponGUIInventoryIndex > 3 + itemModifyingGUI_InventoryScrollOffset )
                    {
                        bBreakFromLoop = true;
                    }
                }
                break;
            case 2:
                if ( (item->beatitude >= 0 && item->beatitude < 5) || (item->beatitude >= 5 && (item->type == ARTIFACT_SWORD || item->type == ARTIFACT_MACE || item->type == ARTIFACT_SPEAR || item->type == ARTIFACT_AXE || item->type == ARTIFACT_BOW)) ) // Skip over all Cursed Items and Items +5 and greater, unless it's an Artifact Weapon
                {
                    ++iEnchantWeaponGUIInventoryIndex;

                    if ( iEnchantWeaponGUIInventoryIndex <= itemModifyingGUI_InventoryScrollOffset )
                    {
                        continue;
                    }

                    itemModifyingGUI_Inventory[iEnchantWeaponGUIInventoryIndex - itemModifyingGUI_InventoryScrollOffset - 1] = item;

                    if ( iEnchantWeaponGUIInventoryIndex > 3 + itemModifyingGUI_InventoryScrollOffset )
                    {
                        bBreakFromLoop = true;
                    }
                }
                break;
            default: printlog("ERROR: rebuildEnchantWeaponGUIInventory() - itemModifyingGUI_ScrollBeatitude (%d) out of bounds", itemModifyingGUI_ScrollBeatitude); return;
        }

        if ( bBreakFromLoop == true )
        {
            break;
        }
    }
} // rebuildEnchantWeaponGUIInventory()

/* ItemModifyingGUI.cpp
 * Draws the Sprite Image and description for the given Item in each of the visible GUI Inventory slots
 */
void ItemModifyingGUI::enchantWeaponGUI_HandleItemImages()
{
    bool bBreakFromLoop = false; // True if evaluation of drawing Images has ended (all viable Images have been processed)
    Uint8 iEnchantWeaponGUIInventoryIndex = 0; // The position in the GUI Inventory array of the given Item
    Sint32 iEnchantWeaponGUIInventoryItemOffset = (((yres / 2) - (itemModifyingGUI_IMG->h / 2)) + itemModifyingGUI_OffsetY) + 22; // The amount that each Item is offset vertically from each other

    Item* item = nullptr; // The given Item being drawn from the GUI Inventory
    SDL_Rect itemImageRect; // The position of the Image of the Item in the GUI Inventory

    // Grab the Player's Inventory again, as it may have been modified by processGUIEffectOnItem()
    list_t* playerInventoryList = &stats[clientnum]->inventory;

    for ( node_t* iInventoryNode = playerInventoryList->first; iInventoryNode != nullptr; iInventoryNode = iInventoryNode->next )
    {
        if ( iInventoryNode->element != nullptr )
        {
            item = static_cast<Item*>(iInventoryNode->element);

            if ( item == nullptr )
            {
                continue;
            }

            // Skip all Items that aren't Weapons
            if ( itemCategory(item) != WEAPON )
            {
                continue;
            }

            // Cursed Scrolls don't need to be shown, as they wont have a GUI
            switch ( itemModifyingGUI_ScrollBeatitude )
            {
                case 0:
                    if ( item->beatitude >= 0 && item->beatitude < 2 ) // Skip over all Cursed Items and Items +2 and greater. Skip Artifacts
                    {
                        if ( item->type == ARTIFACT_SWORD || item->type == ARTIFACT_MACE || item->type == ARTIFACT_SPEAR || item->type == ARTIFACT_AXE || item->type == ARTIFACT_BOW )
                        {
                            continue;
                        }

                        iEnchantWeaponGUIInventoryIndex++;

                        // If the Item is not visible (within 0 - 3 in Inventory), skip over it
                        if ( iEnchantWeaponGUIInventoryIndex <= itemModifyingGUI_InventoryScrollOffset )
                        {
                            continue;
                        }

                        // The name and description of the Item in the GUI Inventory
                        char itemDescription[64] = {0};
                        strncpy(itemDescription, item->description(), 46);

                        // Check to make sure the text doesn't go out of bounds
                        if ( strlen(itemDescription) == 46 )
                        {
                            strcat(itemDescription, " ...");
                        }

                        // Print the name and description of the Item
                        ttfPrintText(ttf8, (((xres / 2) - (itemModifyingGUI_IMG->w / 2)) + itemModifyingGUI_OffsetX) + 36, iEnchantWeaponGUIInventoryItemOffset, itemDescription);

                        // Draw the Image of the Item
                        itemImageRect.x = (((xres / 2) - (itemModifyingGUI_IMG->w / 2)) + itemModifyingGUI_OffsetX) + 16;
                        itemImageRect.y = (((yres / 2) - (itemModifyingGUI_IMG->h / 2)) + itemModifyingGUI_OffsetY) + 17 + 18 * (iEnchantWeaponGUIInventoryIndex - itemModifyingGUI_InventoryScrollOffset - 1);
                        itemImageRect.w = 16;
                        itemImageRect.h = 16;
                        drawImageScaled(itemSprite(item), nullptr, &itemImageRect);

                        iEnchantWeaponGUIInventoryItemOffset += 18;

                        // If the Item's index is out of bounds (above the amount of Items in the Inventory), stop drawing
                        if ( iEnchantWeaponGUIInventoryIndex > 3 + itemModifyingGUI_InventoryScrollOffset )
                        {
                            bBreakFromLoop = true;
                        }
                    }
                    break;
                case 1:
                    if ( item->beatitude >= 0 && item->beatitude < 3 ) // Skip over all Cursed Items and Items +3 and greater. Skip Artifacts
                    {
                        if ( item->type == ARTIFACT_SWORD || item->type == ARTIFACT_MACE || item->type == ARTIFACT_SPEAR || item->type == ARTIFACT_AXE || item->type == ARTIFACT_BOW )
                        {
                            continue;
                        }

                        iEnchantWeaponGUIInventoryIndex++;

                        // If the Item is not visible (within 0 - 3 in Inventory), skip over it
                        if ( iEnchantWeaponGUIInventoryIndex <= itemModifyingGUI_InventoryScrollOffset )
                        {
                            continue;
                        }

                        // The name and description of the Item in the GUI Inventory
                        char itemDescription[64] = {0};
                        strncpy(itemDescription, item->description(), 46);

                        // Check to make sure the text doesn't go out of bounds
                        if ( strlen(itemDescription) == 46 )
                        {
                            strcat(itemDescription, " ...");
                        }

                        // Print the name and description of the Item
                        ttfPrintText(ttf8, (((xres / 2) - (itemModifyingGUI_IMG->w / 2)) + itemModifyingGUI_OffsetX) + 36, iEnchantWeaponGUIInventoryItemOffset, itemDescription);

                        // Draw the Image of the Item
                        itemImageRect.x = (((xres / 2) - (itemModifyingGUI_IMG->w / 2)) + itemModifyingGUI_OffsetX) + 16;
                        itemImageRect.y = (((yres / 2) - (itemModifyingGUI_IMG->h / 2)) + itemModifyingGUI_OffsetY) + 17 + 18 * (iEnchantWeaponGUIInventoryIndex - itemModifyingGUI_InventoryScrollOffset - 1);
                        itemImageRect.w = 16;
                        itemImageRect.h = 16;
                        drawImageScaled(itemSprite(item), nullptr, &itemImageRect);

                        iEnchantWeaponGUIInventoryItemOffset += 18;

                        // If the Item's index is out of bounds (above the amount of Items in the Inventory), stop drawing
                        if ( iEnchantWeaponGUIInventoryIndex > 3 + itemModifyingGUI_InventoryScrollOffset )
                        {
                            bBreakFromLoop = true;
                        }
                    }
                    break;
                case 2:
                    if ( (item->beatitude >= 0 && item->beatitude < 5) || (item->beatitude >= 5 && (item->type == ARTIFACT_SWORD || item->type == ARTIFACT_MACE || item->type == ARTIFACT_SPEAR || item->type == ARTIFACT_AXE || item->type == ARTIFACT_BOW)) ) // Skip over all Cursed Items and Items +5 and greater, unless it's an Artifact Weapon
                    {
                        iEnchantWeaponGUIInventoryIndex++;

                        // If the Item is not visible (within 0 - 3 in Inventory), skip over it
                        if ( iEnchantWeaponGUIInventoryIndex <= itemModifyingGUI_InventoryScrollOffset )
                        {
                            continue;
                        }

                        // The name and description of the Item in the GUI Inventory
                        char itemDescription[64] = {0};
                        strncpy(itemDescription, item->description(), 46);

                        // Check to make sure the text doesn't go out of bounds
                        if ( strlen(itemDescription) == 46 )
                        {
                            strcat(itemDescription, " ...");
                        }

                        // Print the name and description of the Item
                        ttfPrintText(ttf8, (((xres / 2) - (itemModifyingGUI_IMG->w / 2)) + itemModifyingGUI_OffsetX) + 36, iEnchantWeaponGUIInventoryItemOffset, itemDescription);

                        // Draw the Image of the Item
                        itemImageRect.x = (((xres / 2) - (itemModifyingGUI_IMG->w / 2)) + itemModifyingGUI_OffsetX) + 16;
                        itemImageRect.y = (((yres / 2) - (itemModifyingGUI_IMG->h / 2)) + itemModifyingGUI_OffsetY) + 17 + 18 * (iEnchantWeaponGUIInventoryIndex - itemModifyingGUI_InventoryScrollOffset - 1);
                        itemImageRect.w = 16;
                        itemImageRect.h = 16;
                        drawImageScaled(itemSprite(item), nullptr, &itemImageRect);

                        iEnchantWeaponGUIInventoryItemOffset += 18;

                        // If the Item's index is out of bounds (above the amount of Items in the Inventory), stop drawing
                        if ( iEnchantWeaponGUIInventoryIndex > 3 + itemModifyingGUI_InventoryScrollOffset )
                        {
                            bBreakFromLoop = true;
                        }
                    }
                    break;
                default: printlog("ERROR: enchantWeaponGUI_HandleItemImages() - itemModifyingGUI_ScrollBeatitude (%d) out of bounds", itemModifyingGUI_ScrollBeatitude); return;
            }

            if ( bBreakFromLoop == true )
            {
                break;
            }
        }
    }
} // enchantWeaponGUI_HandleItemImages()

// ENCHANT ARMOR GUI

/* ItemModifyingGUI.cpp
 * @param selectedItem - The Item that is being processed, selected from the GUI Inventory
 * Enchants the selected Item based on 'itemModifyingGUI_ScrollBeatitude' and 'itemModifyingGUI_ScrollBeatitude'
 * +0 Scrolls can raise a +0 or +1 Shield, Breastpiece, or Helm up to +2. Cannot Enchant Artifacts
 * +1 Scrolls can raise a +0, +1, or +2 Shield, Breastpiece, Helm, Boots, or Gloves up to +3. Cannot Enchant Artifacts
 * +2 Scrolls can raise a +0, +1, +2, +3, or +4 Shield, Breastpiece, Helm, Boots, Gloves, Rings, or Cloaks up to +5. Raises any Artifact Armor by one level
 * Messages the Player that their Item has been Enchanted, then if the GUI was opened with a Scroll, it is consumed before closing the GUI
 */
void ItemModifyingGUI::processEnchantArmorGUIEffectOnItem(Item* const selectedItem)
{
    if ( selectedItem == nullptr )
    {
        printlog("ERROR: processEnchantArmorGUIEffectOnItem() - selectedItem is null.");
        return;
    }

    // Cursed Scrolls don't use the GUI so they will never call this function
    ItemType itemType = WOODEN_SHIELD;
    switch ( itemModifyingGUI_ScrollBeatitude )
    {
        case 0:
            // Enchant the Item if it is a Shield, Breastpiece, or Helm +0 or +1 up to +2. Cannot Enchant Artifacts
            messagePlayer(clientnum, language[2520], selectedItem->description()); // "Your %s glows blue!"
            selectedItem->beatitude = 2;
            break;
        case 1:
            // Enchant the Item if it is a Shield, Breastpiece, Helm, Boots, or Gloves +0, +1, or +2 up to +3. Cannot Enchant Artifacts
            messagePlayer(clientnum, language[2521], selectedItem->description()); // "Your %s violently glows blue!"
            selectedItem->beatitude = 3;
            break;
        case 2:
            // Enchant the Item if it is a Shield, Breastpiece, Helm, Boots, Gloves, Rings, or Cloaks +0, +1, +2, +3, or +4 up to +5. Raises any Artifact Armor by one level
            messagePlayer(clientnum, language[2522], selectedItem->description()); // "Your %s radiates power!"
            itemType = selectedItem->type;
            if ( itemType == ARTIFACT_BREASTPIECE || itemType == ARTIFACT_HELM   || itemType == ARTIFACT_BOOTS || itemType == ARTIFACT_CLOAK || itemType == ARTIFACT_GLOVES )
            {
                selectedItem->beatitude++;
            }
            else
            {
                selectedItem->beatitude = 5;
            }
            break;
        default: printlog("ERROR: processEnchantArmorGUIEffectOnItem() - itemModifyingGUI_ScrollBeatitude (%d) out of bounds.", itemModifyingGUI_ScrollBeatitude); return;
    }

    // If the Player used a Scroll, consume it. Currently there is no Spell of Enchant Weapon
    if ( itemModifyingGUI_ScrollUsed != nullptr )
    {
        consumeItem(itemModifyingGUI_ScrollUsed);
        itemModifyingGUI_ScrollUsed = nullptr;
    }

    closeItemModifyingGUI();
} // processEnchantArmorGUIEffectOnItem()

/* ItemModifyingGUI.cpp
 * @param numItems - The number of random Items to be processed. Currently this number should never exceed 2, as the highest naturally spawning Scrolls are +-2
 * Processes a random amount of valid Items equal to 'numItems'. Processing will reduce the random Items to +0
 * Messages the Player that their Item has been Disenchanted because of the beatitude of the Scroll used
 */
void ItemModifyingGUI::enchantArmorGUIProcessRandomItem(const Uint8 numItems)
{
    // Grab the Player's Inventory again, as it may have been modified prior to this
    list_t* playerInventoryList = &stats[clientnum]->inventory;

    if ( playerInventoryList == nullptr )
    {
        messagePlayer(0, "Warning: stats[%d]->inventory is not a valid list. This should not happen.", clientnum);
        printlog("ERROR: enchantArmorGUIProcessRandomItem() - stats[%d]->inventory is not a valid list.", clientnum);
        return;
    }

    // Count the number of Items in the GUI Inventory
    Uint8 totalAmountOfItems = 0;
    for ( node_t* iInventoryNode = playerInventoryList->first; iInventoryNode != nullptr; iInventoryNode = iInventoryNode->next )
    {
        Item* item = static_cast<Item*>(iInventoryNode->element);

        if ( item == nullptr )
        {
            continue;
        }

        ItemArmorType itemArmorType = getItemArmorType(item->type);

        // Skip all Items that aren't Armor
        if ( itemArmorType == ItemArmorType::NOT_ARMOR )
        {
            continue;
        }

        switch ( itemModifyingGUI_ScrollBeatitude )
        {
            case -2:
                // Intentional fall-through
            case -1:
                if ( item->beatitude > 0 ) // Skip over all Cursed and Unenchanted Items
                {
                    totalAmountOfItems++;
                }
                break;
            default: printlog("ERROR: enchantArmorGUIProcessRandomItem() - itemModifyingGUI_ScrollBeatitude (%d) out of bounds", itemModifyingGUI_ScrollBeatitude); return;
        }
    }

    // There are no valid Items to be processed
    if ( totalAmountOfItems == 0 && itemModifyingGUI_ScrollBeatitude > 0 )
    {
        messagePlayer(clientnum, language[2510]); // "You have no remaining items for the scroll's blessing..."
        return;
    }
    else if ( totalAmountOfItems == 0 && itemModifyingGUI_ScrollBeatitude < 0 )
    {
        messagePlayer(clientnum, language[2510]); // "You have no items for the scroll's curse..."
        return;
    }

    Uint8 amountOfItemsLeftToProcess = numItems;
    bool isThereASingleItem = false;

    // If there is only a single Item that can be processed, but it's a +-2 Scroll, adjust the loop
    if ( totalAmountOfItems == 1 && itemModifyingGUI_ScrollBeatitude >= 2 )
    {
        isThereASingleItem = true;
        amountOfItemsLeftToProcess--;
    }
    else if ( totalAmountOfItems == 1 && itemModifyingGUI_ScrollBeatitude <= -2 )
    {
        isThereASingleItem = true;
        amountOfItemsLeftToProcess--;
    }

    Sint8 iPreviousItemProcessedIndex = -1;
    Sint8 iItemToProcessIndex = -1;
    Uint8 iEnchantArmorGUIInventoryIndex = 0;

    for ( Uint8 i = 0; i < numItems; i++ )
    {
        while ( iItemToProcessIndex == iPreviousItemProcessedIndex )
        {
            iItemToProcessIndex = rand() % totalAmountOfItems;
        }

        iEnchantArmorGUIInventoryIndex = 0;

        for ( node_t* iInventoryNode = playerInventoryList->first; iInventoryNode != nullptr; iInventoryNode = iInventoryNode->next )
        {
            Item* itemToProcess = static_cast<Item*>(iInventoryNode->element);

            if ( itemToProcess == nullptr )
            {
                continue;
            }

            ItemArmorType itemArmorType = getItemArmorType(itemToProcess->type);

            // Skip all Items that aren't Armor
            if ( itemArmorType == ItemArmorType::NOT_ARMOR )
            {
                continue;
            }

            switch ( itemModifyingGUI_ScrollBeatitude )
            {
                case -2:
                    // Intentional fall-through
                case -1:
                    if ( itemToProcess->beatitude > 0 ) // Skip over all Cursed and Unenchanted Items
                    {
                        if ( iEnchantArmorGUIInventoryIndex == iItemToProcessIndex )
                        {
                            if ( numItems == 2 && amountOfItemsLeftToProcess == 1 && isThereASingleItem != true )
                            {
                                messagePlayer(clientnum, language[2523], itemToProcess->description()); // "Oh no! Your %s was disenchanted too!"
                            }
                            else
                            {
                                messagePlayer(clientnum, language[2509], itemToProcess->description()); // "Your %s was disenchanted!"
                            }

                            itemToProcess->beatitude = 0;
                            amountOfItemsLeftToProcess--;
                            iPreviousItemProcessedIndex = iEnchantArmorGUIInventoryIndex;
                        }
                        iEnchantArmorGUIInventoryIndex++;
                    }
                    break;
                default: printlog("ERROR: enchantArmorGUIProcessRandomItem() - itemModifyingGUI_ScrollBeatitude (%d) out of bounds", itemModifyingGUI_ScrollBeatitude); return;
            }

            // If processing is done, stop searching the Inventory
            if ( amountOfItemsLeftToProcess == 0 )
            {
                break;
            }
        }

        // If there's only a single valid Item, but it's a +-2 scroll, this will happen
        if ( amountOfItemsLeftToProcess == 0 )
        {
            break;
        }
    }

    // Tell the Player they were an Item short of the full effect
    if ( isThereASingleItem == true && itemModifyingGUI_ScrollBeatitude > 0 )
    {
        messagePlayer(clientnum, language[2510]); // "You have no remaining items for the scroll's blessing..."
    }
    else if ( isThereASingleItem == true && itemModifyingGUI_ScrollBeatitude < 0 )
    {
        messagePlayer(clientnum, language[2524]); // "You have no remaining items for the scroll's curse..."
    }
} // enchantArmorGUIProcessRandomItem()

/* ItemModifyingGUI.cpp
 * Builds the GUI's Inventory based off of 'itemModifyingGUI_Type' and 'itemModifyingGUI_ScrollBeatitude'
 * Counts the number of Items to add to the GUI's Inventory, then assigns the visible ones to their position in 'itemModifyingGUI_Inventory[]'
 */
void ItemModifyingGUI::rebuildEnchantArmorGUIInventory()
{
    // Grab the Player's Inventory again, as it may have been modified by processGUIEffectOnItem()
    list_t* playerInventoryList = &stats[clientnum]->inventory;
    Uint8 iEnchantArmorGUIInventoryIndex = 0;

    if ( playerInventoryList == nullptr )
    {
        messagePlayer(0, "Warning: stats[%d]->inventory is not a valid list. This should not happen.", clientnum);
        printlog("ERROR: rebuildEnchantArmorGUIInventory() - stats[%d]->inventory is not a valid list.", clientnum);
        return;
    }

    // Count the number of Items in the Repair GUI Inventory
    for ( node_t* iInventoryNode = playerInventoryList->first; iInventoryNode != nullptr; iInventoryNode = iInventoryNode->next )
    {
        Item* item = static_cast<Item*>(iInventoryNode->element);

        if ( item == nullptr )
        {
            continue;
        }

        ItemArmorType itemArmorType = getItemArmorType(item->type);

        // Skip all Items that aren't Armor
        if ( itemArmorType == ItemArmorType::NOT_ARMOR )
        {
            continue;
        }

        switch ( itemModifyingGUI_ScrollBeatitude )
        {
            case -2:
                // Intentional fall-through
            case -1:
                if ( item->beatitude > 0 ) // Skip over all Cursed and Unenchanted Items
                {
                    ++iEnchantArmorGUIInventoryIndex;
                }
                break;
            case 0:
                if ( item->beatitude >= 0 && item->beatitude < 2 ) // Skip over all Cursed Items and Items +2 and greater. Skip Artifacts
                {
                    // +0 can enchant Shields, Breastpiece, Masks, and Helms
                    if ( itemArmorType == ItemArmorType::BOOTS || itemArmorType == ItemArmorType::GLOVES || itemArmorType == ItemArmorType::RING || itemArmorType == ItemArmorType::AMULET || itemArmorType == ItemArmorType::CLOAK )
                    {
                        continue;
                    }

                    // +0 cannot enchant Artifacts
                    if ( item->type == ARTIFACT_BREASTPIECE || item->type == ARTIFACT_HELM || item->type == ARTIFACT_BOOTS || item->type == ARTIFACT_CLOAK || item->type == ARTIFACT_GLOVES )
                    {
                        continue;
                    }

                    ++iEnchantArmorGUIInventoryIndex;
                }
                break;
            case 1:
                if ( item->beatitude >= 0 && item->beatitude < 3 ) // Skip over all Cursed Items and Items +3 and greater. Skip Artifacts
                {
                    // +1 can enchant Shields, Breastpiece, Masks, Helms, Boots, and Gloves
                    if ( itemArmorType == ItemArmorType::RING || itemArmorType == ItemArmorType::AMULET || itemArmorType == ItemArmorType::CLOAK )
                    {
                        continue;
                    }

                    // +1 cannot enchant Artifacts
                    if ( item->type == ARTIFACT_BREASTPIECE || item->type == ARTIFACT_HELM || item->type == ARTIFACT_BOOTS || item->type == ARTIFACT_CLOAK || item->type == ARTIFACT_GLOVES )
                    {
                        continue;
                    }

                    ++iEnchantArmorGUIInventoryIndex;
                }
                break;
            case 2:
                if ( (item->beatitude >= 0 && item->beatitude < 5) || (item->beatitude >= 5 && (item->type == ARTIFACT_BREASTPIECE || item->type == ARTIFACT_HELM || item->type == ARTIFACT_BOOTS || item->type == ARTIFACT_CLOAK || item->type == ARTIFACT_GLOVES)) ) // Skip over all Cursed Items and Items +5 and greater, unless it's an Artifact Armor
                {
                    // +2 can enchant Shields, Breastpiece, Masks, Helms, Boots, Gloves, Rings, Amulets, and Cloaks
                    ++iEnchantArmorGUIInventoryIndex;
                }
                break;
            default: printlog("ERROR: rebuildEnchantArmorGUIInventory() - itemModifyingGUI_ScrollBeatitude (%d) out of bounds", itemModifyingGUI_ScrollBeatitude); return;
        }
    }

    itemModifyingGUI_InventoryScrollOffset = std::max(0, std::min(static_cast<int>(itemModifyingGUI_InventoryScrollOffset), iEnchantArmorGUIInventoryIndex - 4));

    // Reset the current Inventory
    for ( iEnchantArmorGUIInventoryIndex = 0; iEnchantArmorGUIInventoryIndex < 4; ++iEnchantArmorGUIInventoryIndex )
    {
        itemModifyingGUI_Inventory[iEnchantArmorGUIInventoryIndex] = nullptr;
    }

    iEnchantArmorGUIInventoryIndex = 0;

    // Assign the visible Items to the GUI slots
    bool bBreakFromLoop = false;
    for ( node_t* iInventoryNode = playerInventoryList->first; iInventoryNode != nullptr; iInventoryNode = iInventoryNode->next )
    {
        Item* item = static_cast<Item*>(iInventoryNode->element);

        if ( item == nullptr )
        {
            continue;
        }

        ItemArmorType itemArmorType = getItemArmorType(item->type);

        // Skip all Items that aren't Armor
        if ( itemArmorType == ItemArmorType::NOT_ARMOR )
        {
            continue;
        }

        switch ( itemModifyingGUI_ScrollBeatitude )
        {
            case -2:
                // Intentional fall-through
            case -1:
                if ( item->beatitude > 0 ) // Skip over all Cursed and Unenchanted Items
                {
                    ++iEnchantArmorGUIInventoryIndex;

                    if ( iEnchantArmorGUIInventoryIndex <= itemModifyingGUI_InventoryScrollOffset )
                    {
                        continue;
                    }

                    itemModifyingGUI_Inventory[iEnchantArmorGUIInventoryIndex - itemModifyingGUI_InventoryScrollOffset - 1] = item;

                    if ( iEnchantArmorGUIInventoryIndex > 3 + itemModifyingGUI_InventoryScrollOffset )
                    {
                        bBreakFromLoop = true;
                    }
                }
                break;
            case 0:
                if ( item->beatitude >= 0 && item->beatitude < 2 ) // Skip over all Cursed Items and Items +2 and greater. Skip Artifacts
                {
                    // +0 can enchant Shields, Breastpiece, Masks, and Helms
                    if ( itemArmorType == ItemArmorType::BOOTS || itemArmorType == ItemArmorType::GLOVES || itemArmorType == ItemArmorType::RING || itemArmorType == ItemArmorType::AMULET || itemArmorType == ItemArmorType::CLOAK )
                    {
                        continue;
                    }

                    if ( item->type == ARTIFACT_BREASTPIECE || item->type == ARTIFACT_HELM || item->type == ARTIFACT_BOOTS || item->type == ARTIFACT_CLOAK || item->type == ARTIFACT_GLOVES )
                    {
                        continue;
                    }

                    ++iEnchantArmorGUIInventoryIndex;

                    if ( iEnchantArmorGUIInventoryIndex <= itemModifyingGUI_InventoryScrollOffset )
                    {
                        continue;
                    }

                    itemModifyingGUI_Inventory[iEnchantArmorGUIInventoryIndex - itemModifyingGUI_InventoryScrollOffset - 1] = item;

                    if ( iEnchantArmorGUIInventoryIndex > 3 + itemModifyingGUI_InventoryScrollOffset )
                    {
                        bBreakFromLoop = true;
                    }
                }
                break;
            case 1:
                if ( item->beatitude >= 0 && item->beatitude < 3 ) // Skip over all Cursed Items and Items +3 and greater. Skip Artifacts
                {
                    // +1 can enchant Shields, Breastpiece, Masks, Helms, Boots, and Gloves
                    if ( itemArmorType == ItemArmorType::RING || itemArmorType == ItemArmorType::AMULET || itemArmorType == ItemArmorType::CLOAK )
                    {
                        continue;
                    }

                    if ( item->type == ARTIFACT_BREASTPIECE || item->type == ARTIFACT_HELM || item->type == ARTIFACT_BOOTS || item->type == ARTIFACT_CLOAK || item->type == ARTIFACT_GLOVES )
                    {
                        continue;
                    }

                    ++iEnchantArmorGUIInventoryIndex;

                    if ( iEnchantArmorGUIInventoryIndex <= itemModifyingGUI_InventoryScrollOffset )
                    {
                        continue;
                    }

                    itemModifyingGUI_Inventory[iEnchantArmorGUIInventoryIndex - itemModifyingGUI_InventoryScrollOffset - 1] = item;

                    if ( iEnchantArmorGUIInventoryIndex > 3 + itemModifyingGUI_InventoryScrollOffset )
                    {
                        bBreakFromLoop = true;
                    }
                }
                break;
            case 2:
                if ( (item->beatitude >= 0 && item->beatitude < 5) || (item->beatitude >= 5 && (item->type == ARTIFACT_BREASTPIECE || item->type == ARTIFACT_HELM || item->type == ARTIFACT_BOOTS || item->type == ARTIFACT_CLOAK || item->type == ARTIFACT_GLOVES)) ) // Skip over all Cursed Items and Items +5 and greater, unless it's an Artifact Armor
                {
                    // +2 can enchant Shields, Breastpiece, Masks, Helms, Boots, Gloves, Rings, Amulets, and Cloaks
                    ++iEnchantArmorGUIInventoryIndex;

                    if ( iEnchantArmorGUIInventoryIndex <= itemModifyingGUI_InventoryScrollOffset )
                    {
                        continue;
                    }

                    itemModifyingGUI_Inventory[iEnchantArmorGUIInventoryIndex - itemModifyingGUI_InventoryScrollOffset - 1] = item;

                    if ( iEnchantArmorGUIInventoryIndex > 3 + itemModifyingGUI_InventoryScrollOffset )
                    {
                        bBreakFromLoop = true;
                    }
                }
                break;
            default: printlog("ERROR: rebuildEnchantArmorGUIInventory() - itemModifyingGUI_ScrollBeatitude (%d) out of bounds", itemModifyingGUI_ScrollBeatitude); return;
        }

        if ( bBreakFromLoop == true )
        {
            break;
        }
    }
} // rebuildEnchantArmorGUIInventory()

/* ItemModifyingGUI.cpp
 * Draws the Sprite Image and description for the given Item in each of the visible GUI Inventory slots
 */
void ItemModifyingGUI::enchantArmorGUI_HandleItemImages()
{
    bool bBreakFromLoop = false; // True if evaluation of drawing Images has ended (all viable Images have been processed)
    Uint8 iEnchantWeaponGUIInventoryIndex = 0; // The position in the GUI Inventory array of the given Item
    Sint32 iEnchantWeaponGUIInventoryItemOffset = (((yres / 2) - (itemModifyingGUI_IMG->h / 2)) + itemModifyingGUI_OffsetY) + 22; // The amount that each Item is offset vertically from each other

    Item* item = nullptr; // The given Item being drawn from the GUI Inventory
    SDL_Rect itemImageRect; // The position of the Image of the Item in the GUI Inventory

    // Grab the Player's Inventory again, as it may have been modified by processGUIEffectOnItem()
    list_t* playerInventoryList = &stats[clientnum]->inventory;

    for ( node_t* iInventoryNode = playerInventoryList->first; iInventoryNode != nullptr; iInventoryNode = iInventoryNode->next )
    {
        if ( iInventoryNode->element != nullptr )
        {
            item = static_cast<Item*>(iInventoryNode->element);

            if ( item == nullptr )
            {
                continue;
            }

            ItemArmorType itemArmorType = getItemArmorType(item->type);

            // Skip all Items that aren't Armor
            if ( itemArmorType == ItemArmorType::NOT_ARMOR )
            {
                continue;
            }

            // Cursed Scrolls don't need to be shown, as they wont have a GUI
            switch ( itemModifyingGUI_ScrollBeatitude )
            {
                case 0:
                    if ( item->beatitude >= 0 && item->beatitude < 2 ) // Skip over all Cursed Items and Items +2 and greater. Skip Artifacts
                    {
                        // +0 can enchant Shields, Breastpiece, Masks, and Helms
                        if ( itemArmorType == ItemArmorType::BOOTS || itemArmorType == ItemArmorType::GLOVES || itemArmorType == ItemArmorType::RING || itemArmorType == ItemArmorType::AMULET || itemArmorType == ItemArmorType::CLOAK )
                        {
                            continue;
                        }

                        if ( item->type == ARTIFACT_BREASTPIECE || item->type == ARTIFACT_HELM || item->type == ARTIFACT_BOOTS || item->type == ARTIFACT_CLOAK || item->type == ARTIFACT_GLOVES )
                        {
                            continue;
                        }

                        iEnchantWeaponGUIInventoryIndex++;

                        // If the Item is not visible (within 0 - 3 in Inventory), skip over it
                        if ( iEnchantWeaponGUIInventoryIndex <= itemModifyingGUI_InventoryScrollOffset )
                        {
                            continue;
                        }

                        // The name and description of the Item in the GUI Inventory
                        char itemDescription[64] = {0};
                        strncpy(itemDescription, item->description(), 46);

                        // Check to make sure the text doesn't go out of bounds
                        if ( strlen(itemDescription) == 46 )
                        {
                            strcat(itemDescription, " ...");
                        }

                        // Print the name and description of the Item
                        ttfPrintText(ttf8, (((xres / 2) - (itemModifyingGUI_IMG->w / 2)) + itemModifyingGUI_OffsetX) + 36, iEnchantWeaponGUIInventoryItemOffset, itemDescription);

                        // Draw the Image of the Item
                        itemImageRect.x = (((xres / 2) - (itemModifyingGUI_IMG->w / 2)) + itemModifyingGUI_OffsetX) + 16;
                        itemImageRect.y = (((yres / 2) - (itemModifyingGUI_IMG->h / 2)) + itemModifyingGUI_OffsetY) + 17 + 18 * (iEnchantWeaponGUIInventoryIndex - itemModifyingGUI_InventoryScrollOffset - 1);
                        itemImageRect.w = 16;
                        itemImageRect.h = 16;
                        drawImageScaled(itemSprite(item), nullptr, &itemImageRect);

                        iEnchantWeaponGUIInventoryItemOffset += 18;

                        // If the Item's index is out of bounds (above the amount of Items in the Inventory), stop drawing
                        if ( iEnchantWeaponGUIInventoryIndex > 3 + itemModifyingGUI_InventoryScrollOffset )
                        {
                            bBreakFromLoop = true;
                        }
                    }
                    break;
                case 1:
                    if ( item->beatitude >= 0 && item->beatitude < 3 ) // Skip over all Cursed Items and Items +3 and greater. Skip Artifacts
                    {
                        // +1 can enchant Shields, Breastpiece, Masks, Helms, Boots, and Gloves
                        if ( itemArmorType == ItemArmorType::RING || itemArmorType == ItemArmorType::AMULET || itemArmorType == ItemArmorType::CLOAK )
                        {
                            continue;
                        }

                        if ( item->type == ARTIFACT_BREASTPIECE || item->type == ARTIFACT_HELM || item->type == ARTIFACT_BOOTS || item->type == ARTIFACT_CLOAK || item->type == ARTIFACT_GLOVES )
                        {
                            continue;
                        }

                        iEnchantWeaponGUIInventoryIndex++;

                        // If the Item is not visible (within 0 - 3 in Inventory), skip over it
                        if ( iEnchantWeaponGUIInventoryIndex <= itemModifyingGUI_InventoryScrollOffset )
                        {
                            continue;
                        }

                        // The name and description of the Item in the GUI Inventory
                        char itemDescription[64] = {0};
                        strncpy(itemDescription, item->description(), 46);

                        // Check to make sure the text doesn't go out of bounds
                        if ( strlen(itemDescription) == 46 )
                        {
                            strcat(itemDescription, " ...");
                        }

                        // Print the name and description of the Item
                        ttfPrintText(ttf8, (((xres / 2) - (itemModifyingGUI_IMG->w / 2)) + itemModifyingGUI_OffsetX) + 36, iEnchantWeaponGUIInventoryItemOffset, itemDescription);

                        // Draw the Image of the Item
                        itemImageRect.x = (((xres / 2) - (itemModifyingGUI_IMG->w / 2)) + itemModifyingGUI_OffsetX) + 16;
                        itemImageRect.y = (((yres / 2) - (itemModifyingGUI_IMG->h / 2)) + itemModifyingGUI_OffsetY) + 17 + 18 * (iEnchantWeaponGUIInventoryIndex - itemModifyingGUI_InventoryScrollOffset - 1);
                        itemImageRect.w = 16;
                        itemImageRect.h = 16;
                        drawImageScaled(itemSprite(item), nullptr, &itemImageRect);

                        iEnchantWeaponGUIInventoryItemOffset += 18;

                        // If the Item's index is out of bounds (above the amount of Items in the Inventory), stop drawing
                        if ( iEnchantWeaponGUIInventoryIndex > 3 + itemModifyingGUI_InventoryScrollOffset )
                        {
                            bBreakFromLoop = true;
                        }
                    }
                    break;
                case 2:
                    if ( (item->beatitude >= 0 && item->beatitude < 5) || (item->beatitude >= 5 && (item->type == ARTIFACT_BREASTPIECE || item->type == ARTIFACT_HELM || item->type == ARTIFACT_BOOTS || item->type == ARTIFACT_CLOAK || item->type == ARTIFACT_GLOVES)) ) // Skip over all Cursed Items and Items +5 and greater, unless it's an Artifact Armor
                    {
                        // +2 can enchant Shields, Breastpiece, Masks, Helms, Boots, Gloves, Rings, Amulets, and Cloaks
                        iEnchantWeaponGUIInventoryIndex++;

                        // If the Item is not visible (within 0 - 3 in Inventory), skip over it
                        if ( iEnchantWeaponGUIInventoryIndex <= itemModifyingGUI_InventoryScrollOffset )
                        {
                            continue;
                        }

                        // The name and description of the Item in the GUI Inventory
                        char itemDescription[64] = {0};
                        strncpy(itemDescription, item->description(), 46);

                        // Check to make sure the text doesn't go out of bounds
                        if ( strlen(itemDescription) == 46 )
                        {
                            strcat(itemDescription, " ...");
                        }

                        // Print the name and description of the Item
                        ttfPrintText(ttf8, (((xres / 2) - (itemModifyingGUI_IMG->w / 2)) + itemModifyingGUI_OffsetX) + 36, iEnchantWeaponGUIInventoryItemOffset, itemDescription);

                        // Draw the Image of the Item
                        itemImageRect.x = (((xres / 2) - (itemModifyingGUI_IMG->w / 2)) + itemModifyingGUI_OffsetX) + 16;
                        itemImageRect.y = (((yres / 2) - (itemModifyingGUI_IMG->h / 2)) + itemModifyingGUI_OffsetY) + 17 + 18 * (iEnchantWeaponGUIInventoryIndex - itemModifyingGUI_InventoryScrollOffset - 1);
                        itemImageRect.w = 16;
                        itemImageRect.h = 16;
                        drawImageScaled(itemSprite(item), nullptr, &itemImageRect);

                        iEnchantWeaponGUIInventoryItemOffset += 18;

                        // If the Item's index is out of bounds (above the amount of Items in the Inventory), stop drawing
                        if ( iEnchantWeaponGUIInventoryIndex > 3 + itemModifyingGUI_InventoryScrollOffset )
                        {
                            bBreakFromLoop = true;
                        }
                    }
                    break;
                default: printlog("ERROR: enchantWeaponGUI_HandleItemImages() - itemModifyingGUI_ScrollBeatitude (%d) out of bounds", itemModifyingGUI_ScrollBeatitude); return;
            }

            if ( bBreakFromLoop == true )
            {
                break;
            }
        }
    }
} // enchantArmorGUI_HandleItemImages()

} // namespace GUI
