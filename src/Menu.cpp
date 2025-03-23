#include <inttypes.h>
#include <string.h>
#include <ctype.h>
#include <functional>

#include "ConsoleAdapter.h"
#include "Menu.h"


void Menu::Show(Port& uiPort) const
{
    int colWidth = ColWidth;

    // Compute minimal column width if not specified
    if (colWidth < 0) {
        for (auto item = Items; !item->IsEnd(); item++) {
            int itemWidth = (iscntrl(item->Selector) ? 6 : 1) // "CTRL+x" or "x"
                          + 2                                 // ": "
                          + strlen(item->Text);               // "<item-text>"
            if (item->Value != NULL) {
                itemWidth += strlen(item->Value);             // "<item-value>"
            }
            if (colWidth < itemWidth) {
                colWidth = itemWidth;
            }
        }
    }

    uiPort.Printf("\r\n" MENU_PREFIX "%s\r\n", Title);

    // Render all the visible items in the menu
    for (auto menu = Items; !menu->IsEnd(); ) {

        // If the current item is hidden, skip it.
        if (menu->IsHidden()) {
            menu++;
            continue;
        }

        // If the current item is a separator, print it and skip to the next item.
        if (menu->IsSeparator()) {
            uiPort.Printf("%-*s%s\r\n", ColMargin, "", menu->Text);
            menu++;
            continue;
        }

        // Determine the number of menu items in the current group
        size_t numItemsInGroup = 0;
        for (auto item = menu; item->IsSelectable() && !item->IsHidden(); item++) {
            numItemsInGroup++;
        }

        // Determine the number of rows needed to display group
        size_t numRowsInGroup = (numItemsInGroup + (NumCols - 1)) / NumCols;

        // Print each row...
        for (size_t row = 0; row < numRowsInGroup; row++) {
            int padding = 0;

            // Print each item in the current row...
            for (size_t i = row; i < numItemsInGroup; i += numRowsInGroup) {
                auto item = menu + i;
                int itemWidth = 0;

                // Pad the previous column out to the column width and add the
                // column margin (whitespace before column)
                uiPort.Printf("%-*s", padding + ColMargin, "");

                // Print the selector for the current menu item, translating control
                // characters to printable text (e.g. "CTRL+x")
                if (item->Selector == '\r') {
                    uiPort.Write("ENTER");
                    itemWidth += 5;
                }
                else if (item->Selector == '\e') {
                    uiPort.Write("ESC");
                    itemWidth += 3;
                }
                else if (iscntrl(item->Selector)) {
                    uiPort.Write("CTRL+");
                    uiPort.Write(item->Selector + 0x40);
                    itemWidth += 6;
                }
                else {
                    uiPort.Write(item->Selector);
                    itemWidth += 1;
                }

                // Print the menu item text
                uiPort.Write(") ");
                uiPort.Write(item->Text);
                itemWidth += (2 + strlen(item->Text));

                // If the menu item has a value, print it right-justified in the 
                // column.
                if (item->Value != NULL) {
                    int valueWidth = strlen(item->Value);
                    int valuePadding = colWidth - (itemWidth + valueWidth);
                    while (valuePadding-- > 0) { uiPort.Write('.'); }
                    uiPort.Write(item->Value);
                    itemWidth = colWidth;
                }

                // Arrange for necessary padding when printing the next column
                padding = MAX(colWidth - itemWidth, 0);
            }

            uiPort.Write("\r\n");
        }

        menu += numItemsInGroup;
    }

    uiPort.Write("\r\n");
}

char Menu::GetSelection(Port& uiPort, const char * prompt, bool echoSel, bool newline) const
{
    char ch;

    uiPort.Write(prompt);

    while (true) {
        // Update the state of the activity LEDs
        ActivityLED::UpdateState();

        // Update the connection status of the SCL port
        gSCLPort.CheckConnected();

        if (!uiPort.TryRead(ch)) {
            continue;
        }
    
        if (IsValidSelection(ch)) {
            break;
        }

        if (echoSel) {
            uiPort.Write("?\x08");
        }
    }

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
