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

#include <ctype.h>
#include <inttypes.h>

#include "ConsoleAdapter.h"
#include "FileLib.h"
#include "crc32.h"

struct alignas(4) FileHeader final
{
    uint32_t Checksum;
    uint32_t Length;
    char Name[MAX_FILE_NAME_LEN];
    uint8_t Data[0];

    bool IsValid(void) const;
    const FileHeader * Next(void) const;
};

const FileHeader * FileLib::sFileIndex[MAX_FILES];
size_t FileLib::sNumFiles;

// Start and end of file storage area in flash.
// Defined in linker script.
extern uint8_t __FileStorageStart;
extern uint8_t __FileStorageEnd;

void FileLib::Init(void)
{
    // Scan the file storage area in flash for valid files and build an index
    for (auto file = (const FileHeader *)&__FileStorageStart; file != NULL && file->IsValid(); file = file->Next()) {
        sFileIndex[sNumFiles] = file;
        sNumFiles++;
        if (sNumFiles == MAX_FILES) {
            break;
        }
    }
}

size_t FileLib::NumFiles(void)
{
    return sNumFiles;
}

bool FileLib::GetFile(size_t index, const char *& fileName, const uint8_t*& data, size_t& len)
{
    if (index < sNumFiles) {
        fileName = sFileIndex[index]->Name;
        data = sFileIndex[index]->Data;
        len = sFileIndex[index]->Length;
        return true;
    }
    else {
        fileName = NULL;
        data = NULL;
        len = 0;
        return false;
    }
}

const char * FileLib::GetFileName(size_t index)
{
    return (index < sNumFiles) ? sFileIndex[index]->Name : NULL;
}

bool FileHeader::IsValid(void) const
{
    // Verify the file falls entirely within the expected flash range.
    const uint8_t * fileStart = (const uint8_t *)this;
    if (fileStart < &__FileStorageStart || fileStart >= &__FileStorageEnd) {
        return false;
    }
    const uint8_t * fileHeaderEnd = fileStart + sizeof(FileHeader);
    if (fileHeaderEnd > &__FileStorageEnd) {
        return false;
    }
    const uint8_t * fileEnd = fileHeaderEnd + Length;
    if (fileEnd > &__FileStorageEnd) {
        return false;
    }

    // Verify the Length is sane
    if (Length == 0) {
        return false;
    }

    // Verify the checksum
    size_t checksumLen = (sizeof(FileHeader) + Length) - sizeof(Checksum);
    uint32_t expectedChecksum = crc32((const uint8_t *)&Length, checksumLen);
    if (Checksum != expectedChecksum) {
        return false;
    }

    return true;
}

const FileHeader * FileHeader::Next(void) const
{
    // Get a pointer to the file immediately following the current file.
    auto nextFileStart = ((const uint8_t *)this) + sizeof(FileHeader) + Length;
    nextFileStart = (const uint8_t *)
        (((((uintptr_t)nextFileStart) + (alignof(FileHeader) - 1)) / alignof(FileHeader)) * alignof(FileHeader));

    // Verify the next file falls entirely within the expected flash range.
    // Return NULL if not.
    if (nextFileStart < &__FileStorageStart || nextFileStart >= &__FileStorageEnd) {
        return NULL;
    }
    const uint8_t * nextFileHeaderEnd = nextFileStart + sizeof(FileHeader);
    if (nextFileHeaderEnd > &__FileStorageEnd) {
        return NULL;
    }
    const FileHeader * nextFile = (const FileHeader *)nextFileStart;
    const uint8_t * nextFileEnd = nextFileHeaderEnd + nextFile->Length;
    if (nextFileEnd > &__FileStorageEnd) {
        return NULL;
    }

    return nextFile;
}
