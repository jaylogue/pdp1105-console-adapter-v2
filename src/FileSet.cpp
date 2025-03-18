#include <ctype.h>
#include <inttypes.h>

#include "ConsoleAdapter.h"
#include "FileSet.h"
#include "crc32.h"

struct FileHeader
{
    uint32_t Checksum;
    uint32_t Length;
    char Name[MAX_FILE_NAME_LEN];
    uint8_t Data[0];

    bool IsValid(void) const;
    const FileHeader * Next(void) const;
};

const FileHeader * FileSet::sFileIndex[MAX_FILES];
size_t FileSet::sNumFiles;

// Area of Flash in which files are stored
static const FileHeader * const sFileSetStart = (const FileHeader *)(XIP_BASE + 1 * 1024 * 1024);
static const FileHeader * const sFileSetEnd   = (const FileHeader *)(XIP_BASE + PICO_FLASH_SIZE_BYTES);

void FileSet::Init(void)
{
    // Scan the file set in flash for valid files and build an index
    for (auto file = sFileSetStart; sNumFiles < MAX_FILES && file->IsValid(); file = file->Next()) {
        sFileIndex[sNumFiles] = file;
        sNumFiles++;
    }
}

size_t FileSet::NumFiles(void)
{
    return sNumFiles;
}

bool FileSet::GetFile(size_t index, const char *& fileName, const uint8_t*& data, size_t& len)
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

const char * FileSet::GetFileName(size_t index)
{
    return (index < sNumFiles) ? sFileIndex[index]->Name : NULL;
}

bool FileHeader::IsValid(void) const
{
    // Verify the file header exists within the expected range of Flash
    if (this < sFileSetStart || this >= sFileSetEnd) {
        return false;
    }

    // Verify the Length value is sane
    if (Length == 0 || Next() > sFileSetEnd) {
        return false;
    }

    // Verify the checksum
    size_t checksumLen = Length + sizeof(FileHeader) - sizeof(Checksum);
    uint32_t expectedChecksum = crc32((const uint8_t *)&Length, checksumLen);
    if (Checksum != expectedChecksum) {
        return false;
    }

    return true;
}

const FileHeader * FileHeader::Next(void) const
{
    intptr_t nextAddr = ((intptr_t)this) + sizeof(FileHeader) + Length;
    nextAddr = (nextAddr + (alignof(FileHeader) - 1)) & ~(alignof(FileHeader) - 1);
    return (const FileHeader *)nextAddr;
}
