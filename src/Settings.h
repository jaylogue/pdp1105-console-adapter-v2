#ifndef SETTINGS_H
#define SETTINGS_H

struct SettingsRecord;

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

    static void PrintStats(Port& uiPort);

private:
    static const SettingsRecord * sLatestRec;
    static uint32_t sEraseCount;

    static const SettingsRecord * FindEmptyRecord(bool& eraseSector);
    static bool IsSupportedRecord(uint16_t recVer);
};

inline
bool Settings::ShouldShowPTRProgress(const Port* uiPort)
{
    return (ShowPTRProgress == ShowPTRProgress_Enabled ||
            (ShowPTRProgress == ShowPTRProgress_USBOnly && uiPort == &gHostPort));
}

#endif // SETTINGS_H
