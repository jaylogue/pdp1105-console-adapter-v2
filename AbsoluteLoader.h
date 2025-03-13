#ifndef ABSOLUTE_LOADER_H
#define ABSOLUTE_LOADER_H

extern const uint8_t gAbsoluteLoaderPaperTapeFile[];
extern const size_t gAbsoluteLoaderPaperTapeFileLen;

class AbsoluteLoaderDataSource final : public LoadDataSource
{
public:
    AbsoluteLoaderDataSource(uint16_t loadAddr);
    ~AbsoluteLoaderDataSource() = default;
    AbsoluteLoaderDataSource(const AbsoluteLoaderDataSource&) = delete;

    virtual bool GetWord(uint16_t& data, uint16_t& addr);
    virtual void Advance(void);
    virtual bool AtEnd(void);
    virtual uint16_t GetStartAddress(void);

    static uint16_t MemSizeToLoadAddr(uint32_t memSize);

private:
    const uint16_t mLoadAddr;
    size_t mCurWord;
};

inline AbsoluteLoaderDataSource::AbsoluteLoaderDataSource(uint16_t loadAddr)
: mLoadAddr(loadAddr), mCurWord(0)
{
}

#endif // ABSOLUTE_LOADER_H
