#ifndef BOOTSTRAP_LOADER_SOURCE_H
#define BOOTSTRAP_LOADER_SOURCE_H

class BootstrapLoaderSource final : public LoadDataSource
{
public:
    BootstrapLoaderSource(uint16_t loadAddr);
    ~BootstrapLoaderSource() = default;
    BootstrapLoaderSource(const BootstrapLoaderSource&) = delete;

    virtual bool GetWord(uint16_t& data, uint16_t& addr);
    virtual void Advance(void);
    virtual bool AtEOF(void);

    static bool IsValidLoadAddr(uint16_t addr);

private:
    const uint16_t mLoadAddr;
    size_t mCurWord;
};

inline BootstrapLoaderSource::BootstrapLoaderSource(uint16_t loadAddr)
: mLoadAddr(loadAddr), mCurWord(0)
{
}

inline bool BootstrapLoaderSource::IsValidLoadAddr(uint16_t addr)
{
    return (addr & 07777) == 07744;
}

#endif // BOOTSTRAP_LOADER_SOURCE_H
