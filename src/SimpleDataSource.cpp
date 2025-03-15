#include "ConsoleAdapter.h"
#include "SimpleDataSource.h"

bool SimpleDataSource::GetWord(uint16_t& data, uint16_t& addr)
{
    if (!AtEnd()) {
        data = mDataBuf[mCurWord];
        if ((mCurWord + 1) < mDataLen) {
            data |= ((uint16_t)mDataBuf[mCurWord+1]) << 8;
        }
        addr = mLoadAddr + mCurWord;
        return true;
    }
    else {
        data = 0;
        addr = NO_ADDR;
        return false;
    }
}

void SimpleDataSource::Advance(void)
{
    if (!AtEnd()) {
        mCurWord += 2;
    }
}

bool SimpleDataSource::AtEnd(void)
{
    return mCurWord >= mDataLen;
}

uint16_t SimpleDataSource::GetStartAddress(void)
{
    return NO_ADDR;
}
