#ifndef M93XX_CONTROLLER_H
#define M93XX_CONTROLLER_H

#include <stdint.h>

/** Provides an API for interacting with a PDP-11 M9301/M9312 console emulator
 */
class M93xxController final
{
public:
    M93xxController();
    ~M93xxController() = default;

    void ProcessOutput(char ch);
    bool ProcessTimeouts(void);
    bool IsReadyForCommand(void) const;
    void SetAddress(uint16_t addr);
    void Examine(void);
    void Deposit(uint16_t val);
    void Start(void);
    void SendCR(void);
    void Reset(void);
    char LastCommand(void) const;
    uint16_t LastAddress(void) const;
    uint16_t LastExamineValue(void) const;
    uint16_t NextDepositAddress(void) const;
    uint16_t NextExamineAddress(void) const;

    static constexpr uint16_t kUnknownAddress = UINT16_MAX;

private:
    enum {
        kPromptSync_Start,
        kPromptSync_WaitingForPrompt,
        kPromptSync_WaitingForIdle,
        kWaitingForPrompt,
        kReadyForCommand,
        kWaitingForResponse,
        kParsingAddr,
        kParsingValue
    } mState;
    char mLastCmd;
    uint16_t mLastAddr;
    uint16_t mLastVal;
    int mDigitsParsed;
    uint64_t mPromptTimeoutTime;
    uint64_t mIdleTimeoutTime;

    void ArmPromptTimeout(void);
    bool PromptTimeoutExpired(void);
    void ArmIdleTimeout(void);
    bool IdleTimeoutExpired(void);

    static constexpr uint32_t kPromptTimeoutUS = 5 * 1000 * 1000;
    static constexpr uint32_t kIdleTimeoutUS = 200 * 1000;
};

inline
M93xxController::M93xxController()
{
    Reset();
}

inline
char M93xxController::LastCommand(void) const
{
    return mLastCmd;
}

inline
uint16_t M93xxController::LastAddress(void) const
{
    return mLastAddr;
}

inline
uint16_t M93xxController::LastExamineValue(void) const
{
    return mLastVal;
}

inline
uint16_t M93xxController::NextDepositAddress(void) const
{
    return (mLastAddr != kUnknownAddress && mLastCmd == 'D') ? mLastAddr + 2 : mLastAddr;
}

inline
uint16_t M93xxController::NextExamineAddress(void) const
{
    return (mLastAddr != kUnknownAddress && mLastCmd == 'E') ? mLastAddr + 2 : mLastAddr;
}

inline
bool M93xxController::IsReadyForCommand(void) const
{
    return mState == kReadyForCommand;
}

#endif // M93XX_CONTROLLER_H
