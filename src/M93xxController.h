/*
 * Copyright 2024-2025 Jay Logue
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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

    bool ProcessOutput(char ch);
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
        kStart,
        kWaitingForSyncChar,
        kWaitingForInitialPrompt,
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

    bool IsValidOutputChar(char ch);
    void ArmPromptTimeout(void);
    bool PromptTimeoutExpired(void);
    void CancelPromptTimeout(void);
    void ArmIdleTimeout(void);
    bool IdleTimeoutExpired(void);
    void CancelIdleTimeout(void);

    static constexpr uint32_t kPromptTimeoutUS = 5 * 1000 * 1000;
    static constexpr uint32_t kIdleTimeoutUS = 200 * 1000;
    static constexpr char kSyncChar = '.';
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

inline
void M93xxController::ArmPromptTimeout(void)
{
    mPromptTimeoutTime = time_us_64() + kPromptTimeoutUS;
}

inline
bool M93xxController::PromptTimeoutExpired(void)
{
    bool expired = (time_us_64() >= mPromptTimeoutTime);
    if (expired) {
        CancelPromptTimeout();
    }
    return expired;
}

inline
void M93xxController::CancelPromptTimeout(void)
{
    mPromptTimeoutTime = UINT64_MAX;
}

inline
void M93xxController::ArmIdleTimeout(void)
{
    mIdleTimeoutTime = time_us_64() + kIdleTimeoutUS;
}

inline
bool M93xxController::IdleTimeoutExpired(void)
{
    bool expired = (time_us_64() >= mIdleTimeoutTime);
    if (expired) {
        CancelIdleTimeout();
    }
    return expired;
}

inline
void M93xxController::CancelIdleTimeout(void)
{
    mIdleTimeoutTime = UINT64_MAX;
}


#endif // M93XX_CONTROLLER_H
