#include "ConsoleAdapter.h"
#include "PaperTapeReader.h"
#include "UploadFileMode.h"

const char * PaperTapeReader::sName;
const uint8_t * PaperTapeReader::sData;
size_t PaperTapeReader::sLength;
size_t PaperTapeReader::sStartOffset;
size_t PaperTapeReader::sReadPos;

void PaperTapeReader::Init(void)
{
    // nothing to do
}

bool PaperTapeReader::TryRead(char& ch)
{
    // If not at the end of the tape...
    if (IsMounted() && sReadPos < sLength) {

        // Read the next character from the tape
        ch = (char)sData[sStartOffset + sReadPos];
        sReadPos++;
        
        // Automatically unmount the tape when the end is reached
        if (sReadPos == sLength) {
            Unmount();
        }

        return true;
    }
    else {
        ch = 0;
        return false;
    }
}

void PaperTapeReader::Mount(const char * name, const uint8_t * data, size_t len)
{
    sName = name;
    sData = data;
    sLength = len;
    sStartOffset = 0;
    sReadPos = 0;

    // Skip any nul characters at the beginning of the tape image
    while (sLength > 0 && sData[sStartOffset] == 0) {
        sStartOffset++;
        sLength--;
    }

    // Truncate any nul characters at the end of the tape image
    while (sLength > 0 && sData[sStartOffset + sLength - 1] == 0) {
        sLength--;
    }
}

void PaperTapeReader::Unmount(void)
{
    sName = NULL;
    sData = NULL;
    sLength = 0;
    sStartOffset = 0;
    sReadPos = 0;
}
