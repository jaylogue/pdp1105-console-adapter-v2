#include "ConsoleAdapter.h"
#include "PaperTapeReader.h"

static struct {
    const char* Name;
    const uint8_t* Data;
    size_t Length;
    size_t Position;
} sMountedTape;

void PaperTapeReader::Init(void)
{
    // nothing to do
}

bool PaperTapeReader::TryRead(char& ch)
{
    // If not at the end of the tape...
    if (IsMounted() && sMountedTape.Position < sMountedTape.Length) {

        // Read the next character from the tape
        ch = (char)sMountedTape.Data[sMountedTape.Position++];
        
        // Automatically unmount the tape when the end is reached
        if (sMountedTape.Position == sMountedTape.Length) {
            Unmount();
        }

        return true;
    }
    else {
        ch = 0;
        return false;
    }
}

void PaperTapeReader::Mount(const char* tapeName, const uint8_t* data, size_t len)
{
    sMountedTape.Name = tapeName;
    sMountedTape.Data = data;
    sMountedTape.Length = len;
    sMountedTape.Position = 0;

    // Skip any nul characters at the beginning of the tape image
    while (sMountedTape.Length > 0 && *sMountedTape.Data == 0) {
        sMountedTape.Data++;
        sMountedTape.Length--;
    }

    // Truncate any nul characters at the end of the tape image
    while (sMountedTape.Length > 0 && sMountedTape.Data[sMountedTape.Length-1] == 0) {
        sMountedTape.Length--;
    }
}

void PaperTapeReader::Unmount(void)
{
    sMountedTape.Name = NULL;
    sMountedTape.Data = NULL;
    sMountedTape.Length = 0;
    sMountedTape.Position = 0;
}

bool PaperTapeReader::IsMounted(void)
{
    return sMountedTape.Data != NULL;
}

const char* PaperTapeReader::TapeName(void)
{
    return sMountedTape.Name;
}

size_t PaperTapeReader::TapePosition(void)
{
    return (IsMounted()) ? sMountedTape.Position : SIZE_MAX;
}

size_t PaperTapeReader::TapeLength(void)
{
    return (IsMounted()) ? sMountedTape.Length : SIZE_MAX;
}
