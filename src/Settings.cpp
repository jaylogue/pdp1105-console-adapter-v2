#include "ConsoleAdapter.h"
#include "Settings.h"

struct PersistedSettings
{
    uint32_t Marker;
    uint32_t Length;
    uint32_t StructureRev;
    uint32_t DataRev;
};

struct PersistedSettings_V1 final : public PersistedSettings
{
    SerialConfig SCLConfig;
    SerialConfig AuxConfig;
    bool SCLConfigFollowsHost;
    bool AuxConfigFollowsHost;
    uint8_t ShowPTRProgress;
    uint32_t CheckSum;
};

SerialConfig Settings::SCLConfig = { DEFAULT_BAUD_RATE, 8, 1, SerialConfig::PARITY_NONE };
bool Settings::SCLConfigFollowsHost;

SerialConfig Settings::AuxConfig = { DEFAULT_BAUD_RATE, 8, 1, SerialConfig::PARITY_NONE };
bool Settings::AuxConfigFollowsHost;

Settings::ShowPTRProgress_t Settings::ShowPTRProgress = Settings::ShowPTRProgress_Enabled;

void Settings::Init(void)
{
    // TODO: implement me
}

void Settings::Save(void)
{
    // TODO: implement me
}
