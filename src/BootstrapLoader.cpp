#include "ConsoleAdapter.h"
#include "BootstrapLoader.h"

/** DEC PDP-11 Bootstrap Loader Code
 * 
 * See https://gunkies.org/wiki/PDP-11_Bootstrap_Loader#Code
 */
const static uint16_t sBootstrapLoaderCode[] = {
    0016701,
    0000026,
    0012702,
    0000352,
    0005211,
    0105711,
    0100376,
    0116162,
    0000002,
    0007400, // <-- word offset 9: dynamically adjusted according to load address
    0005267,
    0177756,
    0000765,
    0177560  // <-- CSR of paper tape reader; defaults to console reader
};
constexpr static size_t sBootstrapLoaderCodeLen = sizeof(sBootstrapLoaderCode) / sizeof(sBootstrapLoaderCode[0]);
constexpr static size_t sBootstrapLoaderDynamicWordOffset = 9;

bool BootstrapLoaderDataSource::GetWord(uint16_t& data, uint16_t& addr)
{
    if (mCurWord < sBootstrapLoaderCodeLen) {
        addr = mLoadAddr + ((uint16_t)mCurWord * 2);
        data = sBootstrapLoaderCode[mCurWord];
        if (mCurWord == sBootstrapLoaderDynamicWordOffset) {
            data |= (mLoadAddr & 0770000);
        }
        return true;
    }
    else {
        return false;
    }
}

void BootstrapLoaderDataSource::Advance(void)
{
    if (mCurWord < sBootstrapLoaderCodeLen) {
        mCurWord++;
    }
}

bool BootstrapLoaderDataSource::AtEnd(void)
{
    return mCurWord >= sBootstrapLoaderCodeLen;
}

uint16_t BootstrapLoaderDataSource::GetStartAddress(void)
{
    return mLoadAddr;
}

uint16_t BootstrapLoaderDataSource::MemSizeToLoadAddr(uint32_t memSizeKW)
{
    if (memSizeKW < 4) {
        memSizeKW = 4;
    }
    else if (memSizeKW > 28) {
        memSizeKW = 28;
    }
    // Bootstrap loader placed at end of memory
    return (uint16_t)(memSizeKW * 2048) - sizeof(sBootstrapLoaderCode); 
}
