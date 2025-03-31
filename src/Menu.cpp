/*
 * Copyright 2024-2025 Jay Logue
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <inttypes.h>
#include <string.h>
#include <ctype.h>
#include <functional>

#include "ConsoleAdapter.h"
#include "Menu.h"


void Menu::Show(Port& uiPort) const
{
    size_t colWidth = GetColumnWidth();

    // Display the title
    uiPort.Printf("\r\n" MENU_PREFIX "%s\r\n", Title);

    // Render all the visible items in the menu
    for (auto item = Items; !item->IsEnd(); ) {

        // If the current item is hidden, skip it.
        if (item->IsHidden()) {
            item++;
            continue;
        }

        // If the current item is a separator, print it and skip to the next item.
        if (item->IsSeparator()) {
            uiPort.Printf("%-*s%s\r\n", ColMargin, "", item->Text);
            item++;
            continue;
        }

        // Determine the number of menu items in the current group
        size_t numItemsInGroup = 0;
        for (auto groupItem = item; groupItem->IsSelectable() && !groupItem->IsHidden(); groupItem++) {
            numItemsInGroup++;
        }

        // Determine the number of rows needed to display group
        size_t numRowsInGroup = (numItemsInGroup + (NumCols - 1)) / NumCols;

        // Print each row...
        for (size_t row = 0; row < numRowsInGroup; row++) {
            size_t padding = 0;

            // Print each item in the current row...
            for (size_t i = row; i < numItemsInGroup; i += numRowsInGroup) {
                auto rowItem = item + i;

                // Pad the previous column out to the column width and add the
                // column margin (whitespace before column)
                uiPort.Printf("%-*s", padding + ColMargin, "");

                // Print the selector for the current menu item, translating control
                // characters to printable text (e.g. "CTRL+x")
                size_t itemWidth = rowItem->PrintSelector(uiPort);

                // Print the menu item text
                uiPort.Write(rowItem->Text);
                itemWidth += strlen(rowItem->Text);

                // If the menu item has a value, print it right-justified in the 
                // column.
                if (rowItem->Value != NULL) {
                    size_t valueWidth = strlen(rowItem->Value);
                    size_t valuePadding = colWidth - (itemWidth + valueWidth);
                    while (valuePadding-- > 0) { uiPort.Write('.'); }
                    uiPort.Write(rowItem->Value);
                    itemWidth = colWidth;
                }

                // Arrange for necessary padding when printing the next column
                padding = MAX(colWidth - itemWidth, 0);
            }

            uiPort.Write("\r\n");
        }

        item += numItemsInGroup;
    }

    uiPort.Write("\r\n");
}

char Menu::GetSelection(Port& uiPort, const char * prompt, bool echoSel, bool newline) const
{
    char ch;

    // Display the prompt
    uiPort.Write(prompt);

    while (true) {

        // Update the state of the activity LEDs
        ActivityLED::UpdateState();

        // Update the connection status of the SCL port
        gSCLPort.CheckConnected();

        // Wait until a character is available from the UI port
        if (!uiPort.TryRead(ch)) {
            continue;
        }

        // If the character is a valid selection, return it.
        if (IsValidSelection(ch)) {
            break;
        }

        // Otherwise print an error indicator ('?') if appropriate.
        else if (echoSel) {
            uiPort.Write("?\x08");
        }
    }

    // Echo the selection character if appropriate
    if (echoSel) {
        if (isprint(ch)) {
            uiPort.Write(ch);
        }
        else if (ch == CTRL_C) {
            uiPort.Write("^C");
        }
        else if (ch == '\e') {
            uiPort.Write("ESC");
        }
    }

    // Start a new line if appropriate
    if (newline) {
        uiPort.Write("\r\n");
    }

    return ch;
}

bool Menu::IsValidSelection(char sel) const
{
    for (auto item = Items; !item->IsEnd(); item++) {
        if (item->IsSelectable() && sel == item->Selector) {
            return true;
        }
    }
    return false;
}

size_t Menu::GetColumnWidth(void) const
{
    // Compute minimal column width if not specified
    if (ColWidth < 0) {
        size_t minColWidth = 0;

        for (auto item = Items; !item->IsEnd(); item++) {
            size_t itemWidth = item->GetSelectorWidth();  // "x) "
            itemWidth += strlen(item->Text);                // <item-text>
            if (item->Value != NULL) {
                itemWidth += 1;                             // " "
                itemWidth += strlen(item->Value);           // <item-value>
            }
            if (minColWidth < itemWidth) {
                minColWidth = itemWidth;
            }
        }
        return minColWidth;
    }
    else {
        return (size_t)ColWidth;
    }
}

size_t MenuItem::PrintSelector(Port& uiPort) const
{
    size_t width;

    // Print Selector text
    if (Selector == '\r') {
        uiPort.Write("ENTER");
        width = 5;
    }
    else if (Selector == '\e') {
        uiPort.Write("ESC");
        width = 3;
    }
    else if (iscntrl(Selector)) {
        uiPort.Write("CTRL+");
        uiPort.Write(Selector + 0x40);
        width = 6;
    }
    else {
        uiPort.Write(Selector);
        width = 1;
    }

    // Print separator
    uiPort.Write(") ");
    width += 2;

    return width;
}

size_t MenuItem::GetSelectorWidth(void) const
{
    // Print Selector text
    if (Selector == '\r') {
        return 5 + 2;   // "ENTER) "
    }
    else if (Selector == '\e') {
        return 3 + 2;  // "ESC) "
    }
    else if (iscntrl(Selector)) {
        return 6 + 2;  // "CTRL+x) "
    }
    else {
        return 1 + 2;  // "x) "
    }
}
