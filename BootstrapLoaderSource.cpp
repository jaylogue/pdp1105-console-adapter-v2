#include "ConsoleAdapter.h"
#include "BootstrapLoaderSource.h"

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

bool BootstrapLoaderSource::GetWord(uint16_t& data, uint16_t& addr)
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

void BootstrapLoaderSource::Advance(void)
{
    if (mCurWord < sBootstrapLoaderLen) {
        mCurWord++;
    }
}

bool BootstrapLoaderSource::AtEOF(void)
{
    return mCurWord >= sBootstrapLoaderLen;
}
