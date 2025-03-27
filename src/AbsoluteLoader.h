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

#ifndef ABSOLUTE_LOADER_H
#define ABSOLUTE_LOADER_H

extern const uint8_t gAbsoluteLoaderPaperTapeFile[];
extern const size_t gAbsoluteLoaderPaperTapeFileLen;

class AbsoluteLoaderDataSource final : public LoadDataSource
{
public:
    AbsoluteLoaderDataSource(uint16_t loadAddr);
    ~AbsoluteLoaderDataSource() = default;
    AbsoluteLoaderDataSource(const AbsoluteLoaderDataSource&) = delete;

    virtual bool GetWord(uint16_t& data, uint16_t& addr);
    virtual void Advance(void);
    virtual bool AtEnd(void);
    virtual uint16_t GetStartAddress(void);

    static uint16_t MemSizeToLoadAddr(uint32_t memSize);

private:
    const uint16_t mLoadAddr;
    size_t mCurWord;
};

inline AbsoluteLoaderDataSource::AbsoluteLoaderDataSource(uint16_t loadAddr)
: mLoadAddr(loadAddr), mCurWord(0)
{
}

#endif // ABSOLUTE_LOADER_H
