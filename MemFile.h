#ifndef MEM_FILE_H
#define MEM_FILE_H

#include <stdint.h>

class MemFile
{
public:
    virtual bool IsEOF(void) = 0;
    virtual bool GetWord(uint16_t &data, uint16_t &addr) = 0;
    virtual void Advance(void);
};

#endif // MEM_FILE_H
