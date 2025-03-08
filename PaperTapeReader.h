#ifndef PAPER_TAPE_READER_H
#define PAPER_TAPE_READER_H

class PaperTapeReader final
{
public:
    static void Init(void);
    static bool TryRead(char& ch);
    static void Mount(const char *tapeName, const uint8_t *data, size_t len);
    static void Unmount(void);
    static bool IsMounted(void);
    static const char *TapeName(void);
    static size_t TapePosition(void);
    static size_t TapeLength(void);
};

#endif // PAPER_TAPE_READER_H
