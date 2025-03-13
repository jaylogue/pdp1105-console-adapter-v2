#ifndef BOOTSTRAP_LOADER_H
#define BOOTSTRAP_LOADER_H

class BootstrapLoaderDataSource final : public LoadDataSource
{
public:
    BootstrapLoaderDataSource(uint16_t loadAddr);
    ~BootstrapLoaderDataSource() = default;
    BootstrapLoaderDataSource(const BootstrapLoaderDataSource&) = delete;

    virtual bool GetWord(uint16_t& data, uint16_t& addr);
    virtual void Advance(void);
    virtual bool AtEnd(void);
    virtual uint16_t GetStartAddress(void);

    static uint16_t MemSizeToLoadAddr(uint32_t memSize);

private:
    const uint16_t mLoadAddr;
    size_t mCurWord;
};

inline BootstrapLoaderDataSource::BootstrapLoaderDataSource(uint16_t loadAddr)
: mLoadAddr(loadAddr), mCurWord(0)
{
}

#endif // BOOTSTRAP_LOADER_H
