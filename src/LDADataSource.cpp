#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>

#include "ConsoleAdapter.h"
#include "LDADataSource.h"

LDADataSource::LDADataSource(const uint8_t * buf, size_t len)
: mReader(buf, len), mOverrideLoadAddr(NO_ADDR)
{
    mReader.NextBlock();
}

bool LDADataSource::GetWord(uint16_t& data, uint16_t& addr)
{
    if (!AtEnd())  {
        data = *mReader.mDataPtr;
        if (mReader.mDataLen > 1) {
            data |= ((uint16_t)mReader.mDataPtr[1]) << 8;
        }
        addr = (mOverrideLoadAddr != NO_ADDR) ? mOverrideLoadAddr : mReader.mLoadAddr;
        return true;
    }
    else {
        data = 0;
        addr = NO_ADDR;
        return false;
    }
}

void LDADataSource::Advance(void)
{
    if (!AtEnd()) {

        // Advance to the next data word, or to the end of the current data block
        if (mReader.mDataLen <= 2) {
            mReader.mDataPtr += mReader.mDataLen;
            mReader.mDataLen = 0;
        }
        else {
            mReader.mDataPtr += 2;
            mReader.mDataLen -= 2;
        }

        // If we've consumed all the data in the current block, advance to the
        // next block.  This will also setup a new load address.
        if (mReader.mDataLen == 0) {
            mReader.NextBlock();
        }

        // Otherwise, advance the load address.
        else {
            mReader.mLoadAddr += 2;
        }

        // Advance the override load address, if active.
        if (mOverrideLoadAddr != NO_ADDR) {
            mOverrideLoadAddr += 2;
        }
    }
}

bool LDADataSource::AtEnd(void)
{
    return (mReader.mDataLen == 0 && mReader.AtEnd()) || mReader.mReadError;
}

uint16_t LDADataSource::GetStartAddress(void)
{
    return mReader.mStartAddr;
}

LDAReader::LDAReader(const uint8_t* buf, size_t len)
: mInputBuf(buf), mReadPtr(buf), mRemainingLen(len), 
  mBlockLen(0), mDataPtr(NULL), mDataLen(0),
  mLoadAddr(NO_ADDR), mStartAddr(NO_ADDR), mReadError(false)
{
}

bool LDAReader::NextBlock(void)
{
    mDataPtr = NULL;
    mDataLen = 0;
    mLoadAddr = NO_ADDR;

    // No more blocks if we've already run into an error
    if (mReadError) {
        return false;
    }

    // Skip over the current block
    mReadPtr += mBlockLen;
    mRemainingLen -= mBlockLen;
    mBlockLen = 0;

    // Skip any zeros before the start of the next block
    while (mRemainingLen > 0 && *mReadPtr == 0) {
        mReadPtr++;
        mRemainingLen--;
    }

    // No more data blocks if at end of input
    if (mRemainingLen == 0) {
        return false;
    }

    // Read error if remainder of input is too short to be a block
    if (mRemainingLen < 6) {
        mReadError = true;
        return false;
    }

    // Read error if data doesn't start with block marker
    if (mReadPtr[0] != 0x01 || mReadPtr[1] != 0x00) {
        mReadError = true;
        return false;
    }

    // Extract the block length.
    uint16_t blockLen = ((uint16_t)mReadPtr[3] << 8U) | mReadPtr[2];

    // Read error if block length isn't at least as long as the header
    if (blockLen < 6) {
        mReadError = true;
        return false;
    }

    // Extract the load address
    mLoadAddr = ((uint16_t)mReadPtr[5] << 8U) | mReadPtr[4];

    // If block doesn't include any data, then this is an end marker block.
    if (blockLen == 6) {

        // Extract the start address if present.
        if ((mLoadAddr & 0x0001) == 0) {
            mStartAddr = mLoadAddr;
        }

        // Skip remainder of the input
        mReadPtr += mRemainingLen;
        mRemainingLen = 0;

        // No more data blocks
        return false;
    }

    // For convenience, include checksum byte in block length
    blockLen++; 

    // Read error if remainder of input is too short to contain the block data
    // and a checksum byte
    if (blockLen > mRemainingLen) {
        mReadError = true;
        return false;
    }

    // Verify block checksum; read error if the checksum is invalid
    uint16_t chksum = 0;
    for (size_t i = 0; i < blockLen; i++) {
        chksum += mReadPtr[i];
    }
    if ((chksum & 0xFF) != 0) {
        mReadError = true;
        return false;
    }

    // The current block is a valid data block
    mBlockLen = blockLen;
    mDataPtr = mReadPtr + 6;
    mDataLen = mBlockLen - 7;

    return true;
}

bool LDAReader::AtEnd(void) const
{
    return mRemainingLen == 0;
}

bool LDAReader::IsValidLDAFile(const uint8_t * fileData, size_t fileLen)
{
    LDAReader reader(fileData, fileLen);
    size_t blockCount = 0;

    while (reader.NextBlock()) {
        blockCount++;
    }

    return (reader.AtEnd() && !reader.mReadError && blockCount > 0);
}
