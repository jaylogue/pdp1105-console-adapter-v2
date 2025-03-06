#ifndef PAPER_TAPE_READER_H
#define PAPER_TAPE_READER_H

class PaperTapeReader final
{
public:
    static void Init(void);
    static bool TryRead(char& ch);
};

#endif // PAPER_TAPE_READER_H
