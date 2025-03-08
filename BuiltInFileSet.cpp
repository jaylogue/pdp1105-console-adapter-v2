#include <ctype.h>
#include <inttypes.h>

#include "ConsoleAdapter.h"
#include "BuiltInFileSet.h"
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

constexpr static char kBaseFileKey = '0';
constexpr static size_t kMaxFiles = 36; // '0'-'9','a'-'z'
constexpr static size_t kMaxSingleColumnFiles = 8; // Max files to show in a single column

static const FileHeader * const sFileSetStart = (const FileHeader *)(XIP_BASE + 1 * 1024 * 1024);
static const FileHeader * const sFileSetEnd   = (const FileHeader *)(XIP_BASE + PICO_FLASH_SIZE_BYTES);
static const FileHeader *sFileIndex[kMaxFiles];
static size_t sNumFiles;
static size_t sMaxNameLen = 0;

// Convert index to file key: 0-9 -> '0'-'9', 10-35 -> 'a'-'z'
static inline char FileKey(size_t index)
{
    return (index < 10) ? ('0' + index) : ('a' + (index - 10));
}

// Convert file key to index: '0'-'9' -> 0-9, 'a'-'z' -> 10-35
// Returns SIZE_MAX if file key is out of range
static inline size_t FileKeyIndex(char key)
{
    if (key >= '0' && key <= '9') {
        return key - '0';
    } else if (key >= 'a' && key <= 'z') {
        return 10 + (key - 'a');
    } else {
        return SIZE_MAX;
    }
}

void BuiltInFileSet::Init(void)
{
    // Scan the file set in flash for valid files; for each file...
    for (auto file = sFileSetStart; sNumFiles < kMaxFiles && file->IsValid(); file = file->Next()) {

        // Add it to the index of built-in files
        sFileIndex[sNumFiles] = file;
        sNumFiles++;

        // Determine the length of the longest file name
        size_t nameLen = strnlen(file->Name, sizeof(file->Name));
        if (nameLen > sMaxNameLen) {
            sMaxNameLen = nameLen;
        }
    }
}

void BuiltInFileSet::ShowMenu(Port& uiPort)
{
    // Determine whether to use one or two columns
    bool useDoubleColumn = (sNumFiles > kMaxSingleColumnFiles);
    size_t rows = useDoubleColumn ? (sNumFiles + 1) / 2 : sNumFiles;
    char fileKey;
    
    for (size_t row = 0; row < rows; row++) {
        // Print first column item
        fileKey = FileKey(row);
        uiPort.Printf("  %c: %.*s", fileKey, MAX_FILE_NAME_LEN, sFileIndex[row]->Name);
        
        // Print second column item if using double column and it exists
        if (useDoubleColumn) {
            size_t col2Index = row + rows;
            if (col2Index < sNumFiles) {
                fileKey = FileKey(col2Index);
                // Add padding for first column if displaying two columns
                uiPort.Printf("%-*s%c: %.*s", 
                    (int)(sMaxNameLen - strnlen(sFileIndex[row]->Name, MAX_FILE_NAME_LEN) + 4),
                    "", fileKey, MAX_FILE_NAME_LEN, sFileIndex[col2Index]->Name);
            }
        }
        
        uiPort.Printf("\r\n");
    }

    if (sNumFiles == 0) {
        uiPort.Write("  0 files\r\n");
    }
}

bool BuiltInFileSet::IsValidFile(char fileKey)
{
    size_t index = FileKeyIndex(fileKey);
    return index < sNumFiles;
}

bool BuiltInFileSet::GetFile(char fileKey, const char *& fileName, const uint8_t*& data, size_t& len)
{
    size_t index = FileKeyIndex(fileKey);
    
    if (index < sNumFiles) {
        fileName = sFileIndex[index]->Name;
        data = sFileIndex[index]->Data;
        len = sFileIndex[index]->Length;
        return true;
    }

    fileName = NULL;
    data = NULL;
    len = 0;
    return false;
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
