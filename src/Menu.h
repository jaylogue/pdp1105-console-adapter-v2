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

#ifndef MENU_H
#define MENU_H

struct MenuItem
{
    char Selector;
    const char * Text;
    const char * Value;

    static const MenuItem SEPARATOR(const char * text = "-----")  { return (MenuItem) { '-', text }; }
    static const MenuItem HIDDEN(char selector) { return (MenuItem) { selector, "" }; }
    static const MenuItem END(void) { return (MenuItem) { 0, NULL }; };

    bool IsEnd(void) const { return Text == NULL; }
    bool IsSeparator(void) const { return !IsEnd() && Selector == '-'; }
    bool IsHidden(void) const { return !IsEnd() && Text[0] == 0; }
    bool IsSelectable(void) const { return !IsEnd() && !IsSeparator(); }

private:
    friend class Menu;

    size_t PrintSelector(Port& uiPort) const;
    size_t GetSelectorWidth(void) const;
};

struct Menu
{
    const char * Title;
    const MenuItem * Items;
    size_t NumCols;
    int ColWidth;
    size_t ColMargin;

    void Show(Port& uiPort) const;
    char GetSelection(Port& uiPort, const char * prompt = INPUT_PROMPT,
        bool echoSel = true, bool newline = true) const;
    bool IsValidSelection(char sel) const;

private:
    size_t GetColumnWidth(void) const;
};

#endif // MENU_H
