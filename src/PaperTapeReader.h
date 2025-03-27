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

#ifndef PAPER_TAPE_READER_H
#define PAPER_TAPE_READER_H

class PaperTapeReader final
{
public:
    static void Init(void);
    static bool TryRead(char& ch);
    static void Mount(const char * tapeName, const uint8_t * data, size_t len);
    static void Unmount(void);
    static bool IsMounted(void);
    static const char * TapeName(void);
    static const uint8_t * TapeDataBuf(void);
    static size_t TapeLength(void);
    static size_t TapePosition(void);

private:
    static const char * sName;
    static const uint8_t * sData;
    static size_t sLength;
    static size_t sStartOffset;
    static size_t sReadPos;
};

inline
bool PaperTapeReader::IsMounted(void)
{
    return sData != NULL;
}

inline
const char* PaperTapeReader::TapeName(void)
{
    return sName;
}

inline
const uint8_t * PaperTapeReader::TapeDataBuf(void)
{
    return sData;
}

inline
size_t PaperTapeReader::TapeLength(void)
{
    return (IsMounted()) ? sLength : SIZE_MAX;
}

inline
size_t PaperTapeReader::TapePosition(void)
{
    return (IsMounted()) ? sReadPos : SIZE_MAX;
}

#endif // PAPER_TAPE_READER_H
