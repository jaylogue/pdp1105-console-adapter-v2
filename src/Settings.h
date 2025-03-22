#ifndef ADAPTER_SETTINGS_H
#define ADAPTER_SETTINGS_H

class Settings final
{
public:
    static SerialConfig SCLConfig;
    static bool SCLConfigFollowsHost;

    static SerialConfig AuxConfig;
    static bool AuxConfigFollowsHost;

    enum ShowPTRProgress_t : uint8_t {
        ShowPTRProgress_Enabled,
        ShowPTRProgress_HostOnly,
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
            (ShowPTRProgress == ShowPTRProgress_HostOnly && uiPort == &gHostPort));
}


#endif // ADAPTER_SETTINGS_H
