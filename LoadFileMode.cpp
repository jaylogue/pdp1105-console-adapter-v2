
#include <stdio.h>
#include <stdint.h>

#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/pwm.h"

#include "tusb.h"

#include "ConsoleAdapter.h"
#include "M93xxController.h"
#include "MemFile.h"

class TestMemFile final : public MemFile
{
public:
    TestMemFile();
    ~TestMemFile() = default;
    virtual bool IsEOF(void);
    virtual bool GetWord(uint16_t &data, uint16_t &addr);
    virtual void Advance(void);
    void Reset(void);

private:
    size_t mCurWord;
    static const uint16_t sStartAddr;
    static const uint16_t sData[];
};

const uint16_t TestMemFile::sStartAddr = 01000;
const uint16_t TestMemFile::sData[] = {
    000001,
    000002,
    000003,
    000004,
    000005,
    000006,
    000007,
    000011,
    000012,
    000013,
    000014,
    000015,
    000016,
    000017,
};

TestMemFile::TestMemFile()
{
    Reset();
}

bool TestMemFile::IsEOF(void)
{
    return (mCurWord >= (sizeof(sData) / sizeof(sData[0])));
}

bool TestMemFile::GetWord(uint16_t &data, uint16_t &addr)
{
    if (!IsEOF()) {
        data = sData[mCurWord];
        addr = sStartAddr + (mCurWord * 2);
        if (mCurWord > 5)
            addr += 01000;
        return true;
    }
    else {
        return false;
    }
}

void TestMemFile::Advance(void)
{
    if (!IsEOF()) {
        mCurWord++;
    }
}

void TestMemFile::Reset(void)
{
    mCurWord = 0;
}

M93xxController sM93xxCtrlr;
TestMemFile sTestFile;

void LoadFileMode_Start(void)
{
    if (GlobalState::SystemState != SystemState::LoadFileMode_Monitor) {
        WriteHostAux("*** LOAD FILE ***\r\n");
        GlobalState::SystemState = SystemState::LoadFileMode_Monitor;

        sM93xxCtrlr.Reset();
        sTestFile.Reset();
        sM93xxCtrlr.SendCR();
        sM93xxCtrlr.SendCR();
    }
}

void LoadFileMode_ProcessIO(void)
{
    char ch;
    bool charWasRead;

    charWasRead = TryReadSCL(ch);

    if (charWasRead) {
        sM93xxCtrlr.ProcessOutput(ch);
    }

    sM93xxCtrlr.LoadFromFile(&sTestFile);

    if (sTestFile.IsEOF()) {
        if (sM93xxCtrlr.IsReadyForCommand()) {
            TerminalMode_Start();
        }
    }

    if (charWasRead) {
        GlobalState::Active = true;
        WriteHostAux(ch);
    }
}
