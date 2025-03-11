#ifndef BOOTSTRAP_LOADER_SOURCE_H
#define BOOTSTRAP_LOADER_SOURCE_H

class BootstrapDataSource final : public LoadDataSource
{
public:
    BootstrapDataSource(uint16_t loadAddr);
    ~BootstrapDataSource() = default;
    BootstrapDataSource(const BootstrapDataSource&) = delete;

    virtual const char * Name(void) const;
    virtual bool GetWord(uint16_t& data, uint16_t& addr);
    virtual void Advance(void);
    virtual bool AtEnd(void);
    virtual uint16_t GetStartAddress(void);

    static bool IsValidLoadAddr(uint16_t addr);

private:
    const uint16_t mLoadAddr;
    size_t mCurWord;
};

inline BootstrapDataSource::BootstrapDataSource(uint16_t loadAddr)
: mLoadAddr(loadAddr), mCurWord(0)
{
}

inline bool BootstrapDataSource::IsValidLoadAddr(uint16_t addr)
{
    return (addr & 07777) == 07744;
}

#endif // BOOTSTRAP_LOADER_SOURCE_H
