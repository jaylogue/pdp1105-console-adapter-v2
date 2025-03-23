#ifndef SETTINGS_H
#define SETTINGS_H

class Settings final
{
public:
    static SerialConfig SCLConfig;
    static bool SCLConfigFollowsUSB;

    static SerialConfig AuxConfig;
    static bool AuxConfigFollowsUSB;

    enum ShowPTRProgress_t : uint8_t {
        ShowPTRProgress_Enabled,
        ShowPTRProgress_USBOnly,
        ShowPTRProgress_Disabled
    };
    static ShowPTRProgress_t ShowPTRProgress;

    static void Init(void);
    static void Save(void);
    static bool ShouldShowPTRProgress(const Port * uiPort);
};

inline
bool Settings::ShouldShowPTRProgress(const Port* uiPort)
{
    return (ShowPTRProgress == ShowPTRProgress_Enabled ||
            (ShowPTRProgress == ShowPTRProgress_USBOnly && uiPort == &gHostPort));
}


#endif // SETTINGS_H
