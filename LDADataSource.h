#ifndef LDA_DATA_SOURCE_H
#define LDA_DATA_SOURCE_H

class LDAReader final
{
public:
    LDAReader(const uint8_t * buf, size_t len);

    const uint8_t * const mInputBuf;
    const uint8_t * mReadPtr;
    size_t mRemainingLen;
    const uint8_t *mDataPtr;
    size_t mBlockLen;
    size_t mDataLen;
    uint16_t mLoadAddr;
    uint16_t mStartAddr;
    bool mReadError;

    bool NextBlock(void);
    bool AtEnd(void) const;

    static bool IsValidLDAFile(const uint8_t* fileData, size_t fileLen);
};

class LDADataSource final : public LoadDataSource
{
public:
    LDADataSource(const char * name, const uint8_t * buf, size_t len);
    ~LDADataSource() = default;
    LDADataSource(const LDADataSource&) = delete;

    virtual const char * Name(void) const;
    virtual bool GetWord(uint16_t& data, uint16_t& addr);
    virtual void Advance(void);
    virtual bool AtEnd(void);
    virtual uint16_t GetStartAddress(void);

    void SetOverrideLoadAddress(uint16_t loadAddr);

private:
    LDAReader mReader;
    uint16_t mOverrideLoadAddr;
    const char * const mName;
};

#endif // LDA_DATA_SOURCE_H
