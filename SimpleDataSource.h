#ifndef SIMPLE_DATA_SOURCE_H
#define SIMPLE_DATA_SOURCE_H

class SimpleDataSource final : public LoadDataSource
{
public:
    SimpleDataSource(const uint8_t * buf, size_t len, uint16_t loadAddr);
    ~SimpleDataSource() = default;
    SimpleDataSource(const SimpleDataSource&) = delete;

    virtual bool GetWord(uint16_t& data, uint16_t& addr);
    virtual void Advance(void);
    virtual bool AtEnd(void);
    virtual uint16_t GetStartAddress(void);

private:
    const uint8_t * const mDataBuf;
    const size_t mDataLen;
    const uint16_t mLoadAddr;
    size_t mCurWord;
};

inline SimpleDataSource::SimpleDataSource(const uint8_t * buf, size_t len, uint16_t loadAddr)
: mDataBuf(buf), mDataLen(len), mLoadAddr(loadAddr), mCurWord(0)
{
}

#endif // SIMPLE_DATA_SOURCE_H
