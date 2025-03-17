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
