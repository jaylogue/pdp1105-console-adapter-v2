#ifndef KEY_SEQ_MATCHER_H
#define KEY_SEQ_MATCHER_H

/** @brief Matches a stream of input characters against predefined key sequences
 *
 * This class provides functionality to match characters received one at a time
 * against a set of predefined key sequences. It supports:
 *     - Detecting complete matches of key sequences
 *     - Tracking partial matches that may become complete matches with more characters
 *     - Identifying partial matches that fail to become complete matches
 *     - Handling timeout of partial matches if no matching characters are received
 * 
 * Common uses include detecting escape sequences, keyboard shortcuts, and
 * special command sequences in terminal applications.
 *
 * @note Limitations on key sequences:
 *     - Key sequences must be provided as an array of null-terminated strings
 *     - The array must be terminated with a NULL pointer
 *     - All key sequences must match exactly (wildcards are not supported)
 *     - A key sequence cannot be a proper substring of another key sequence
 *       (e.g., if "ABC" is a sequence, "AB" cannot also be a sequence)
 *     - Key sequences can share common prefixes, but must differ in their
 *       following characters (e.g., "ABC" and "ABD" are allowed)
 *     - The matcher will try all possible sequences that match the current
 *       prefix before giving up on a match
 */
class KeySeqMatcher final
{
public:
    KeySeqMatcher(const char * const keySeqList[], uint32_t timeoutUS);
    int Match(char ch, const char * & queuedChars, size_t & numQueuedChars);
    bool CheckTimeout(const char * & queuedChars, size_t & numQueuedChars);
    void Reset(void);
    void SetTimeout(uint32_t timeoutUS);

    static constexpr int NO_MATCH = -1;
    static constexpr int PARTIAL_MATCH = -2;

private:
    uint64_t mLastMatchTimeUS;
    const char * const * mKeySeqList;
    size_t mCurKeySeq;
    size_t mCurMatchLen;
    uint32_t mTimeoutUS;
};

/** @brief Constructs a key sequence matcher
 * 
 * @param keySeqList Array of null-terminated key sequences to match against
 *                   (must be terminated with a NULL entry)
 * @param timeoutUS  Timeout in microseconds after which a partial match is reset
 */
inline
KeySeqMatcher::KeySeqMatcher(const char * const keySeqList[], uint32_t timeoutUS)
: mKeySeqList(keySeqList), mCurKeySeq(0), mCurMatchLen(0), mTimeoutUS(timeoutUS)
{
}

/** @brief Resets the matcher to its initial state
 * 
 * Clears any partial matches and prepares the matcher to begin
 * matching from the start of each key sequence again.
 */
inline
void KeySeqMatcher::Reset(void)
{
    mCurKeySeq = 0;
    mCurMatchLen = 0;
}

/** @brief Sets the timeout for partial matches
 * 
 * @param timeoutUS Timeout in microseconds after which a partial match is reset
 */
inline
void KeySeqMatcher::SetTimeout(uint32_t timeoutUS)
{
    mTimeoutUS = timeoutUS;
}

#endif // KEY_SEQ_MATCHER_H
