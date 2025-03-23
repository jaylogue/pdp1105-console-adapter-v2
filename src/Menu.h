#ifndef MENU_H
#define MENU_H

#define INPUT_PROMPT    ">>> "
#define MENU_PREFIX     "*** "

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

    int PrintSelector(Port& uiPort) const;
    int GetSelectorWidth(void) const;
};

struct Menu
{
    const char * Title;
    const MenuItem * Items;
    int NumCols;
    int ColWidth;
    int ColMargin;

    void Show(Port& uiPort) const;
    char GetSelection(Port& uiPort, const char * prompt = INPUT_PROMPT,
        bool echoSel = true, bool newline = true) const;
    bool IsValidSelection(char sel) const;

private:
    int GetColumnWidth(void) const;
};

#endif // MENU_H
