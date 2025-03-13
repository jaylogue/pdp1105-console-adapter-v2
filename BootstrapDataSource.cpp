#include "ConsoleAdapter.h"
#include "BootstrapDataSource.h"

/** DEC PDP-11 Bootstrap Loader Code
 * 
 * See https://gunkies.org/wiki/PDP-11_Bootstrap_Loader#Code.
 */
const static uint16_t sBootstrapLoader[] = {
    0016701,
    0000026,
    0012702,
    0000352,
    0005211,
    0105711,
    0100376,
    0116162,
    0000002,
    0007400, // <-- dynamically adjusted according to load address
    0005267,
    0177756,
    0000765,
    0177560  // <-- CSR of paper tape reader; defaults to console reader
};
constexpr static size_t sBootstrapLoaderLen = sizeof(sBootstrapLoader) / sizeof(sBootstrapLoader[0]);

const char * BootstrapDataSource::Name(void) const
{
    static const char nameFormat[] = "Bootstrap Loader @ %06o";
    static char nameBuf[sizeof(nameFormat) + 6];
    snprintf(nameBuf, sizeof(nameBuf), nameFormat, mLoadAddr);
    return nameBuf;
}

bool BootstrapDataSource::GetWord(uint16_t& data, uint16_t& addr)
{
    if (mCurWord < sBootstrapLoaderLen) {
        addr = mLoadAddr + ((uint16_t)mCurWord * 2);
        data = sBootstrapLoader[mCurWord];
        if (mCurWord == 9) {
            data |= (mLoadAddr & 0770000);
        }
        return true;
    }
    else {
        return false;
    }
}

void BootstrapDataSource::Advance(void)
{
    if (mCurWord < sBootstrapLoaderLen) {
        mCurWord++;
    }
}

bool BootstrapDataSource::AtEnd(void)
{
    return mCurWord >= sBootstrapLoaderLen;
}

uint16_t BootstrapDataSource::GetStartAddress(void)
{
    return mLoadAddr;
}

uint16_t BootstrapDataSource::MemSizeToLoadAddr(uint32_t memSizeKW)
{
    if (memSizeKW < 4) {
        memSizeKW = 4;
    }
    else if (memSizeKW > 28) {
        memSizeKW = 28;
    }
    return (uint16_t)(memSizeKW * 2048) - 28; // Load address is 28 bytes before end of memory
}
