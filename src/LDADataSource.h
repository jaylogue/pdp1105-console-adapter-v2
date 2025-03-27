/*
 * Copyright 2024-2025 Jay Logue
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef LDA_DATA_SOURCE_H
#define LDA_DATA_SOURCE_H

class LDAReader final
{
public:
    LDAReader(const uint8_t * buf, size_t len);

    const uint8_t * const mInputBuf;
    const uint8_t * mReadPtr;
    size_t mRemainingLen;
    const uint8_t *mDataPtr;
    size_t mBlockLen;
    size_t mDataLen;
    uint16_t mLoadAddr;
    uint16_t mStartAddr;
    bool mReadError;

    bool NextBlock(void);
    bool AtEnd(void) const;

    static bool IsValidLDAFile(const uint8_t* fileData, size_t fileLen);
};

class LDADataSource final : public LoadDataSource
{
public:
    LDADataSource(const uint8_t * buf, size_t len);
    ~LDADataSource() = default;
    LDADataSource(const LDADataSource&) = delete;

    virtual bool GetWord(uint16_t& data, uint16_t& addr);
    virtual void Advance(void);
    virtual bool AtEnd(void);
    virtual uint16_t GetStartAddress(void);

    void SetOverrideLoadAddress(uint16_t loadAddr);

private:
    LDAReader mReader;
    uint16_t mOverrideLoadAddr;
};

#endif // LDA_DATA_SOURCE_H
