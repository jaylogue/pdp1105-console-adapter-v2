#include "ConsoleAdapter.h"
#include "PaperTapeReader.h"

void PaperTapeReader::Init(void)
{
    // TODO: implement me
}

bool PaperTapeReader::TryRead(char& ch)
{
    // TODO: implement me
    return false;
}

void PaperTapeReader::Mount(const char* tapeName, const uint8_t* data, size_t len)
{
    // TODO: implement me
}

void PaperTapeReader::Unmount(void)
{
    // TODO: implement me
}

bool PaperTapeReader::IsMounted(void)
{
    // TODO: implement me
    return false;
}

const char* PaperTapeReader::TapeName(void)
{
    // TODO: implement me
    return nullptr;
}

size_t PaperTapeReader::TapePosition(void)
{
    // TODO: implement me
    return SIZE_MAX;
}

size_t PaperTapeReader::TapeLength(void)
{
    // TODO: implement me
    return SIZE_MAX;
}
