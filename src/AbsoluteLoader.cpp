#include "ConsoleAdapter.h"
#include "AbsoluteLoader.h"

/** DEC PDP-11 Absolute Loader paper tape image
 */
const uint8_t gAbsoluteLoaderPaperTapeFile[] = {
    0xe9, 0xe9, 0xe9, 0xe9, 0xe9, 0xe9, 0xe9, 0xe9, 0xe9, 0xe9, 0xe9, 0xe9, 0xe9, 0xe9, 0xe9, 0xe9, 
    0xe9, 0xe9, 0xe9, 0xe9, 0xe9, 0xe9, 0xe9, 0xe9, 0xe9, 0xe9, 0xe9, 0xe9, 0xe9, 0xe9, 0xe9, 0xe9, 
    0xe9, 0xe9, 0xe9, 0x3d, 0x00, 0x00, 0xc6, 0x11, 0xa6, 0x29, 0xc5, 0x11, 0xc5, 0x65, 0x4a, 0x00, 
    0x01, 0x0a, 0xce, 0x17, 0x78, 0xff, 0x0e, 0x0c, 0x02, 0x87, 0x0e, 0x0a, 0x03, 0x01, 0xce, 0x0c, 
    0x01, 0x02, 0x4e, 0x10, 0x00, 0x0a, 0xcd, 0x09, 0xc3, 0x8a, 0xfc, 0x02, 0xcd, 0x09, 0xf7, 0x09, 
    0x3c, 0x00, 0x02, 0x11, 0xc2, 0xe5, 0x04, 0x00, 0xc2, 0x25, 0x02, 0x00, 0x21, 0x03, 0xf7, 0x09, 
    0x2c, 0x00, 0x84, 0x63, 0x01, 0x11, 0xcd, 0x09, 0x04, 0x04, 0xc0, 0x8b, 0xeb, 0x03, 0x00, 0x00, 
    0xe9, 0x01, 0xd1, 0x90, 0xf8, 0x01, 0xc3, 0x1d, 0x6a, 0x00, 0x8b, 0x8a, 0xcb, 0x8b, 0xfe, 0x80, 
    0xc3, 0x9c, 0x02, 0x00, 0xc0, 0x60, 0xc3, 0x45, 0x00, 0xff, 0xc2, 0x0a, 0x87, 0x00, 0xb7, 0x15, 
    0x26, 0x00, 0xcd, 0x09, 0xc4, 0x10, 0xcd, 0x09, 0xc3, 0x00, 0xc4, 0x50, 0xc7, 0x1d, 0x18, 0x00, 
    0xf7, 0x09, 0xea, 0xff, 0xcd, 0x09, 0xc0, 0x8b, 0xe2, 0x02, 0x84, 0x0c, 0x02, 0x86, 0x00, 0x00, 
    0xc0, 0x01, 0xc4, 0x0c, 0x84, 0x63, 0x4c, 0x00, 0x00, 0x00, 0xf7, 0x15, 0xea, 0x00, 0x10, 0x00, 
    0xf7, 0x15, 0xf5, 0x01, 0x1c, 0x00, 0x77, 0x00, 0x5a, 0xff, 0xc1, 0x1d, 0x16, 0x00, 0xc2, 0x15, 
    0xfb, 0xeb
};
const size_t gAbsoluteLoaderPaperTapeFileLen = sizeof(gAbsoluteLoaderPaperTapeFile);


/** Combined DEC PDP-11 Absolute Loader + Bootstrap Loader code
 * 
 * See https://gunkies.org/wiki/PDP-11_Absolute_Loader
 */
static const uint16_t sAbsoluteLoaderCode[] = {

    // Absolute Loader code
    0000000, // <-- Initial HALT instruction
    0010706, // <-- Start location
    0024646,
    0010705,
    0062705,
    0000112,
    0005001,
    0013716,
    0177570,
    0006016,
    0103402,
    0005016,
    0000403,
    0006316,
    0001001,
    0010116,
    0005000,
    0004715,
    0105303,
    0001374,
    0004715,
    0004767,
    0000074,
    0010402,
    0162702,
    0000004,
    0022702,
    0000002,
    0001441,
    0004767,
    0000054,
    0061604,
    0010401,
    0004715,
    0002004,
    0105700,
    0001753,
    0000000,
    0000751,
    0110321,
    0000770,
    0016703,
    0000152,
    0105213,
    0105713,
    0100376,
    0116303,
    0000002,
    0060300,
    0042703,
    0177400,
    0005302,
    0000207,
    0012667,
    0000046,
    0004715,
    0010304,
    0004715,
    0000303,
    0050304,
    0016707,
    0000030,
    0004767,
    0177752,
    0004715,
    0105700,
    0001342,
    0006204,
    0103002,
    0000000,
    0000700,
    0006304,
    0061604,
    0000114,
    0000000,
    0012767,
    0000352,
    0000020,
    0012767,
    0000765,
    0000034,
    0000167,
    0177532,

    // Bootstrap Loader code
    0016701,
    0000026,
    0012702,
    0000352,
    0005211,
    0105711,
    0100376,
    0116162,
    0000002,
    0007400, // <-- word offset 92: dynamically adjusted according to load address
    0005267,
    0177756,
    0000765,
    0177560  // <-- CSR of paper tape reader; defaults to console reader
};
constexpr static size_t sAbsoluteLoaderCodeLen = sizeof(sAbsoluteLoaderCode) / sizeof(sAbsoluteLoaderCode[0]);
constexpr static size_t sAbsoluteLoaderDynamicWordOffset = 92;

bool AbsoluteLoaderDataSource::GetWord(uint16_t& data, uint16_t& addr)
{
    if (mCurWord < sAbsoluteLoaderCodeLen) {
        addr = mLoadAddr + ((uint16_t)mCurWord * 2);
        data = sAbsoluteLoaderCode[mCurWord];
        if (mCurWord == sAbsoluteLoaderDynamicWordOffset) {
            data |= (mLoadAddr & 0770000);
        }
        return true;
    }
    else {
        return false;
    }
}

void AbsoluteLoaderDataSource::Advance(void)
{
    if (mCurWord < sAbsoluteLoaderCodeLen) {
        mCurWord++;
    }
}

bool AbsoluteLoaderDataSource::AtEnd(void)
{
    return mCurWord >= sAbsoluteLoaderCodeLen;
}

uint16_t AbsoluteLoaderDataSource::GetStartAddress(void)
{
    // Start address is first word after initial HALT instruction
    return mLoadAddr + 2;
}

uint16_t AbsoluteLoaderDataSource::MemSizeToLoadAddr(uint32_t memSizeKW)
{
    if (memSizeKW < 4) {
        memSizeKW = 4;
    }
    else if (memSizeKW > 28) {
        memSizeKW = 28;
    }
    // Absolute loader placed immediately before Bootstrap Loader at end of memory
    return (uint16_t)(memSizeKW * 2048) - sizeof(sAbsoluteLoaderCode); 
}
