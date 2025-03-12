#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <ctype.h>

#ifndef UNIT_TEST

#include "ConsoleAdapter.h"

#else // UNIT_TEST

#include <string>

struct SCLPort {
    std::string Output;
    void Write(char ch)               { Output += ch; }
    void Write(const char * str)      { Output += str; }
} gSCLPort;

uint64_t gCurTime;

inline uint64_t time_us_64() { return gCurTime; }

#endif // UNIT_TEST

#include "M93xxController.h"

void M93xxController::Reset(void)
{
    mState = kPromptSync_Start;
    mLastCmd = 0;
    mLastAddr = kUnknownAddress;
    mLastVal = 0;
}

void M93xxController::ProcessOutput(char ch)
{
    switch (mState) {
    case kPromptSync_Start:
    case kPromptSync_WaitingForPrompt:
        if (ch == '@' || ch == '$') {
            mState = kPromptSync_WaitingForIdle;
        }
        ArmIdleTimeout();
        break;

    case kPromptSync_WaitingForIdle:
        if (ch != '@' && ch != '$') {
            mState = kPromptSync_Start;
        }
        break;

    case kReadyForCommand:
    case kWaitingForResponse:
        switch (ch) {
        case 'L':
            mLastCmd = ch;
            mState = kParsingAddr;
            mDigitsParsed = 0;
            mLastAddr = 0;
            mLastVal = 0;
            break;
        case 'E':
            mLastCmd = ch;
            mState = kParsingAddr;
            mDigitsParsed = 0;
            mLastAddr = 0;
            break;
        case 'D':
            if (mLastCmd == 'D') {
                mLastAddr += 2;
            }
            mLastCmd = ch;
            mState = kParsingValue;
            mDigitsParsed = 0;
            mLastVal = 0;
            break;
        case 'S':
            mLastCmd = ch;
            mState = kWaitingForPrompt;
            mLastAddr = kUnknownAddress;
            mLastVal = 0;
            break;
        default:
            mLastCmd = 0;
            mState = kWaitingForPrompt;
            break;
        }
        ArmPromptTimeout();
        break;

    case kParsingAddr:
        if (ch >= '0' && ch <= '7') {
            uint16_t chVal = ((uint16_t)ch) - ((uint16_t)'0');
            mLastAddr = (mLastAddr << 3) | chVal;
            mDigitsParsed++;
        }
        else if (isspace(ch)) {
            if (mDigitsParsed > 0) {
                if (mLastCmd == 'E') {
                    mState = kParsingValue;
                    mLastVal = 0;
                }
                else {
                    mState = kWaitingForPrompt;
                }
            }
        }
        else {
            mState = kWaitingForPrompt;
            mLastAddr = kUnknownAddress;
        }
        break;

    case kParsingValue:
        if (ch >= '0' && ch <= '7') {
            uint16_t chVal = ((uint16_t)ch) - ((uint16_t)'0');
            mLastVal = (mLastVal << 3) | chVal;
            mDigitsParsed++;
        }
        else if (isspace(ch)) {
            if (mDigitsParsed > 0) {
                mState = kWaitingForPrompt;
            }
        }
        else {
            mState = kWaitingForPrompt;
        }
        break;

    case kWaitingForPrompt:
        if (ch == '@' || ch == '$') {
            mState = kReadyForCommand;
        }
        break;
    }
}

bool M93xxController::ProcessTimeouts(void)
{
    switch (mState) {
    case kPromptSync_Start:
        // Send CR to elicit a prompt from the M9301/M9312 console
        // and wait for the prompt character.
        SendCR();
        ArmPromptTimeout();
        ArmIdleTimeout();
        mState = kPromptSync_WaitingForPrompt;
        return false;

    case kPromptSync_WaitingForPrompt:
        // Fail if the console doesn't issue a prompt within the
        // prompt timeout.
        if (PromptTimeoutExpired()) {
            return true;
        }

        // If no character received within the idle timeout, send
        // another CR and continue waiting for prompt
        // Note that, because the console ROM always waits for 2 input
        // characters before responding, this second CR will always be
        // necessary if the console was already waiting at a prompt
        // when the prompt sync process started.
        if (IdleTimeoutExpired()) {
            SendCR();
            ArmIdleTimeout();
        }
        return false;

    case kPromptSync_WaitingForIdle:
        // If the console has been sufficiently idle since the prompt
        // was issued then consider it ready to accept commands.
        if (IdleTimeoutExpired()) {
            mState = kReadyForCommand;
        }
        return false;

    case kReadyForCommand:
        // No timeouts while waiting for a command from the application
        return false;

    default:
        // In all other states, fail if the console doesn't re-issue
        // a prompt within the prompt timeout.
        if (PromptTimeoutExpired()) {
            return true;
        }
        return false;
    }
}

void M93xxController::SetAddress(uint16_t addr)
{
    if (mState = kReadyForCommand) {
        char loadCmd[10];
        snprintf(loadCmd, sizeof(loadCmd), "L %06" PRIo16 "\r", addr);
        gSCLPort.Write(loadCmd);
        mState = kWaitingForResponse;
    }
}

void M93xxController::Deposit(uint16_t val)
{
    if (mState = kReadyForCommand) {
        char depositCmd[10];
        snprintf(depositCmd, sizeof(depositCmd), "D %06" PRIo16 "\r", val);
        gSCLPort.Write(depositCmd);
        mState = kWaitingForResponse;
    }
}

void M93xxController::Examine(void)
{
    if (mState = kReadyForCommand) {
        gSCLPort.Write("E ");
        mState = kWaitingForResponse;
    }
}

void M93xxController::Start(void)
{
    if (mState = kReadyForCommand) {
        gSCLPort.Write("S\r");
        mState = kWaitingForResponse;
    }
}

void M93xxController::SendCR(void)
{
    gSCLPort.Write('\r');
}

void M93xxController::ArmPromptTimeout(void)
{
    mPromptTimeoutTime = time_us_64() + kPromptTimeoutUS;
}

bool M93xxController::PromptTimeoutExpired(void)
{
    bool expired = (time_us_64() >= mPromptTimeoutTime);
    if (expired) {
        mPromptTimeoutTime = UINT64_MAX;
    }
    return expired;
}

void M93xxController::ArmIdleTimeout(void)
{
    mIdleTimeoutTime = time_us_64() + kIdleTimeoutUS;
}

bool M93xxController::IdleTimeoutExpired(void)
{
    bool expired = (time_us_64() >= mIdleTimeoutTime);
    if (expired) {
        mIdleTimeoutTime = UINT64_MAX;
    }
    return expired;
}

#ifdef UNIT_TEST

#include <string>
#include <assert.h>

void InitController(M93xxController& c)
{
    c.Reset();
    c.ProcessTimeouts();
    c.ProcessOutput('@');
    gCurTime += 250000;
    c.ProcessTimeouts();
}

void DriveController(M93xxController& c, const char * &p)
{
    while (*p) {
        c.ProcessOutput(*p++);
        if (c.IsReadyForCommand()) {
            break;
        }
    }
}

#define TEST_ASSERT(TST) \
{ \
    if (!(TST)) { \
        printf("FAIL\n    Line %d: %s\n", __LINE__, #TST); \
        return false; \
    } \
}

/** Test 1 -- M9312 Output Parsing */
bool Test1(void)
{
    static const char * ConsoleOutput =
        "000000 000000 000000 000000\r"
        "@L 1000\r"
        "@E 001000 000001 \r"
        "@E 001002 000002 \r"
        "@E 001004 000003 \r"
        "@E 001006 000004 \r"
        "@E 001010 000005 \r"
        "@D 42\r"
        "@E 001010 000042 \r"
        "@D 44\r"
        "@E 001010 000044 \r"
        "@D 46\r"
        "@D 000050\r"
        "@E 001012 000050 \r"
        "@S\r"
        "@";

    M93xxController c;
    const char *p = ConsoleOutput;

    printf("TEST1 ................. ");
    fflush(stdout);

    InitController(c);
    TEST_ASSERT(c.IsReadyForCommand());

    DriveController(c, p);
    TEST_ASSERT(c.LastCommand() == 0);
    TEST_ASSERT(c.LastAddress() == M93xxController::kUnknownAddress);

    DriveController(c, p);
    TEST_ASSERT(c.IsReadyForCommand());
    TEST_ASSERT(c.LastCommand() == 'L');
    TEST_ASSERT(c.LastAddress() == 01000);
    TEST_ASSERT(c.NextExamineAddress() == 01000);
    TEST_ASSERT(c.NextDepositAddress() == 01000);
    
    DriveController(c, p);
    TEST_ASSERT(c.IsReadyForCommand());
    TEST_ASSERT(c.LastCommand() == 'E');
    TEST_ASSERT(c.LastAddress() == 01000);
    TEST_ASSERT(c.LastExamineValue() == 1);
    TEST_ASSERT(c.NextExamineAddress() == 01002);
    TEST_ASSERT(c.NextDepositAddress() == 01000);

    DriveController(c, p);
    TEST_ASSERT(c.IsReadyForCommand());
    TEST_ASSERT(c.LastCommand() == 'E');
    TEST_ASSERT(c.LastAddress() == 01002);
    TEST_ASSERT(c.LastExamineValue() == 2);
    TEST_ASSERT(c.NextExamineAddress() == 01004);
    TEST_ASSERT(c.NextDepositAddress() == 01002);

    DriveController(c, p);
    TEST_ASSERT(c.IsReadyForCommand());
    TEST_ASSERT(c.LastCommand() == 'E');
    TEST_ASSERT(c.LastAddress() == 01004);
    TEST_ASSERT(c.LastExamineValue() == 3);
    TEST_ASSERT(c.NextExamineAddress() == 01006);
    TEST_ASSERT(c.NextDepositAddress() == 01004);

    DriveController(c, p);
    TEST_ASSERT(c.IsReadyForCommand());
    TEST_ASSERT(c.LastCommand() == 'E');
    TEST_ASSERT(c.LastAddress() == 01006);
    TEST_ASSERT(c.LastExamineValue() == 4);
    TEST_ASSERT(c.NextExamineAddress() == 01010);
    TEST_ASSERT(c.NextDepositAddress() == 01006);

    DriveController(c, p);
    TEST_ASSERT(c.IsReadyForCommand());
    TEST_ASSERT(c.LastCommand() == 'E');
    TEST_ASSERT(c.LastAddress() == 01010);
    TEST_ASSERT(c.LastExamineValue() == 5);
    TEST_ASSERT(c.NextExamineAddress() == 01012);
    TEST_ASSERT(c.NextDepositAddress() == 01010);

    DriveController(c, p);
    TEST_ASSERT(c.IsReadyForCommand());
    TEST_ASSERT(c.LastCommand() == 'D');
    TEST_ASSERT(c.LastAddress() == 01010);
    TEST_ASSERT(c.LastExamineValue() == 042);
    TEST_ASSERT(c.NextExamineAddress() == 01010);
    TEST_ASSERT(c.NextDepositAddress() == 01012);

    DriveController(c, p);
    TEST_ASSERT(c.IsReadyForCommand());
    TEST_ASSERT(c.LastCommand() == 'E');
    TEST_ASSERT(c.LastAddress() == 01010);
    TEST_ASSERT(c.LastExamineValue() == 042);
    TEST_ASSERT(c.NextExamineAddress() == 01012);
    TEST_ASSERT(c.NextDepositAddress() == 01010);

    DriveController(c, p);
    TEST_ASSERT(c.IsReadyForCommand());
    TEST_ASSERT(c.LastCommand() == 'D');
    TEST_ASSERT(c.LastAddress() == 01010);
    TEST_ASSERT(c.LastExamineValue() == 044);
    TEST_ASSERT(c.NextExamineAddress() == 01010);
    TEST_ASSERT(c.NextDepositAddress() == 01012);

    DriveController(c, p);
    TEST_ASSERT(c.IsReadyForCommand());
    TEST_ASSERT(c.LastCommand() == 'E');
    TEST_ASSERT(c.LastAddress() == 01010);
    TEST_ASSERT(c.LastExamineValue() == 044);
    TEST_ASSERT(c.NextExamineAddress() == 01012);
    TEST_ASSERT(c.NextDepositAddress() == 01010);

    DriveController(c, p);
    TEST_ASSERT(c.IsReadyForCommand());
    TEST_ASSERT(c.LastCommand() == 'D');
    TEST_ASSERT(c.LastAddress() == 01010);
    TEST_ASSERT(c.LastExamineValue() == 046);
    TEST_ASSERT(c.NextExamineAddress() == 01010);
    TEST_ASSERT(c.NextDepositAddress() == 01012);

    DriveController(c, p);
    TEST_ASSERT(c.IsReadyForCommand());
    TEST_ASSERT(c.LastCommand() == 'D');
    TEST_ASSERT(c.LastAddress() == 01012);
    TEST_ASSERT(c.LastExamineValue() == 050);
    TEST_ASSERT(c.NextExamineAddress() == 01012);
    TEST_ASSERT(c.NextDepositAddress() == 01014);

    DriveController(c, p);
    TEST_ASSERT(c.IsReadyForCommand());
    TEST_ASSERT(c.LastCommand() == 'E');
    TEST_ASSERT(c.LastAddress() == 01012);
    TEST_ASSERT(c.LastExamineValue() == 050);
    TEST_ASSERT(c.NextExamineAddress() == 01014);
    TEST_ASSERT(c.NextDepositAddress() == 01012);

    DriveController(c, p);
    TEST_ASSERT(c.IsReadyForCommand());
    TEST_ASSERT(c.LastCommand() == 'S');
    TEST_ASSERT(c.LastAddress() == M93xxController::kUnknownAddress);
    TEST_ASSERT(c.LastExamineValue() == 0);
    TEST_ASSERT(c.NextExamineAddress() == M93xxController::kUnknownAddress);
    TEST_ASSERT(c.NextDepositAddress() == M93xxController::kUnknownAddress);

    TEST_ASSERT(*p == 0);

    printf("PASS\n");

    return true;
}

/** Test 2 -- M9301 Output Parsing */
bool Test2(void)
{
    static const char * ConsoleOutput =
        "000000 000000 000000 000000\r"
        "$L 1000\r"
        "$E 001000 000001 \r"
        "$E 001002 000002 \r"
        "$E 001004 000003 \r"
        "$E 001006 000004 \r"
        "$E 001010 000005 \r"
        "$D 42\r"
        "$E 001010 000042 \r"
        "$D 44\r"
        "$E 001010 000044 \r"
        "$D 46\r"
        "$D 000050\r"
        "$E 001012 000050 \r"
        "$S\r"
        "$";

    M93xxController c;
    const char *p = ConsoleOutput;

    printf("TEST2 ................. ");
    fflush(stdout);

    InitController(c);
    TEST_ASSERT(c.IsReadyForCommand());

    DriveController(c, p);
    TEST_ASSERT(c.IsReadyForCommand());
    TEST_ASSERT(c.LastCommand() == 0);
    TEST_ASSERT(c.LastAddress() == M93xxController::kUnknownAddress);

    DriveController(c, p);
    TEST_ASSERT(c.IsReadyForCommand());
    TEST_ASSERT(c.LastCommand() == 'L');
    TEST_ASSERT(c.LastAddress() == 01000);
    TEST_ASSERT(c.NextExamineAddress() == 01000);
    TEST_ASSERT(c.NextDepositAddress() == 01000);
    
    DriveController(c, p);
    TEST_ASSERT(c.IsReadyForCommand());
    TEST_ASSERT(c.LastCommand() == 'E');
    TEST_ASSERT(c.LastAddress() == 01000);
    TEST_ASSERT(c.LastExamineValue() == 1);
    TEST_ASSERT(c.NextExamineAddress() == 01002);
    TEST_ASSERT(c.NextDepositAddress() == 01000);

    DriveController(c, p);
    TEST_ASSERT(c.IsReadyForCommand());
    TEST_ASSERT(c.LastCommand() == 'E');
    TEST_ASSERT(c.LastAddress() == 01002);
    TEST_ASSERT(c.LastExamineValue() == 2);
    TEST_ASSERT(c.NextExamineAddress() == 01004);
    TEST_ASSERT(c.NextDepositAddress() == 01002);

    DriveController(c, p);
    TEST_ASSERT(c.IsReadyForCommand());
    TEST_ASSERT(c.LastCommand() == 'E');
    TEST_ASSERT(c.LastAddress() == 01004);
    TEST_ASSERT(c.LastExamineValue() == 3);
    TEST_ASSERT(c.NextExamineAddress() == 01006);
    TEST_ASSERT(c.NextDepositAddress() == 01004);

    DriveController(c, p);
    TEST_ASSERT(c.IsReadyForCommand());
    TEST_ASSERT(c.LastCommand() == 'E');
    TEST_ASSERT(c.LastAddress() == 01006);
    TEST_ASSERT(c.LastExamineValue() == 4);
    TEST_ASSERT(c.NextExamineAddress() == 01010);
    TEST_ASSERT(c.NextDepositAddress() == 01006);

    DriveController(c, p);
    TEST_ASSERT(c.IsReadyForCommand());
    TEST_ASSERT(c.LastCommand() == 'E');
    TEST_ASSERT(c.LastAddress() == 01010);
    TEST_ASSERT(c.LastExamineValue() == 5);
    TEST_ASSERT(c.NextExamineAddress() == 01012);
    TEST_ASSERT(c.NextDepositAddress() == 01010);

    DriveController(c, p);
    TEST_ASSERT(c.IsReadyForCommand());
    TEST_ASSERT(c.LastCommand() == 'D');
    TEST_ASSERT(c.LastAddress() == 01010);
    TEST_ASSERT(c.LastExamineValue() == 042);
    TEST_ASSERT(c.NextExamineAddress() == 01010);
    TEST_ASSERT(c.NextDepositAddress() == 01012);

    DriveController(c, p);
    TEST_ASSERT(c.IsReadyForCommand());
    TEST_ASSERT(c.LastCommand() == 'E');
    TEST_ASSERT(c.LastAddress() == 01010);
    TEST_ASSERT(c.LastExamineValue() == 042);
    TEST_ASSERT(c.NextExamineAddress() == 01012);
    TEST_ASSERT(c.NextDepositAddress() == 01010);

    DriveController(c, p);
    TEST_ASSERT(c.IsReadyForCommand());
    TEST_ASSERT(c.LastCommand() == 'D');
    TEST_ASSERT(c.LastAddress() == 01010);
    TEST_ASSERT(c.LastExamineValue() == 044);
    TEST_ASSERT(c.NextExamineAddress() == 01010);
    TEST_ASSERT(c.NextDepositAddress() == 01012);

    DriveController(c, p);
    TEST_ASSERT(c.IsReadyForCommand());
    TEST_ASSERT(c.LastCommand() == 'E');
    TEST_ASSERT(c.LastAddress() == 01010);
    TEST_ASSERT(c.LastExamineValue() == 044);
    TEST_ASSERT(c.NextExamineAddress() == 01012);
    TEST_ASSERT(c.NextDepositAddress() == 01010);

    DriveController(c, p);
    TEST_ASSERT(c.IsReadyForCommand());
    TEST_ASSERT(c.LastCommand() == 'D');
    TEST_ASSERT(c.LastAddress() == 01010);
    TEST_ASSERT(c.LastExamineValue() == 046);
    TEST_ASSERT(c.NextExamineAddress() == 01010);
    TEST_ASSERT(c.NextDepositAddress() == 01012);

    DriveController(c, p);
    TEST_ASSERT(c.IsReadyForCommand());
    TEST_ASSERT(c.LastCommand() == 'D');
    TEST_ASSERT(c.LastAddress() == 01012);
    TEST_ASSERT(c.LastExamineValue() == 050);
    TEST_ASSERT(c.NextExamineAddress() == 01012);
    TEST_ASSERT(c.NextDepositAddress() == 01014);

    DriveController(c, p);
    TEST_ASSERT(c.IsReadyForCommand());
    TEST_ASSERT(c.LastCommand() == 'E');
    TEST_ASSERT(c.LastAddress() == 01012);
    TEST_ASSERT(c.LastExamineValue() == 050);
    TEST_ASSERT(c.NextExamineAddress() == 01014);
    TEST_ASSERT(c.NextDepositAddress() == 01012);

    DriveController(c, p);
    TEST_ASSERT(c.IsReadyForCommand());
    TEST_ASSERT(c.LastCommand() == 'S');
    TEST_ASSERT(c.LastAddress() == M93xxController::kUnknownAddress);
    TEST_ASSERT(c.LastExamineValue() == 0);
    TEST_ASSERT(c.NextExamineAddress() == M93xxController::kUnknownAddress);
    TEST_ASSERT(c.NextDepositAddress() == M93xxController::kUnknownAddress);

    TEST_ASSERT(*p == 0);

    printf("PASS\n");

    return true;
}

/** Test 3 -- Command Output */
bool Test3(void)
{
    M93xxController c;

    printf("TEST3 ................. ");
    fflush(stdout);

    InitController(c);
    gSCLPort.Output.clear();
    c.SetAddress(010101);
    TEST_ASSERT(gSCLPort.Output.compare("L 010101\r") == 0);
    TEST_ASSERT(!c.IsReadyForCommand());

    InitController(c);
    gSCLPort.Output.clear();
    c.SetAddress(0);
    TEST_ASSERT(gSCLPort.Output.compare("L 000000\r") == 0);
    TEST_ASSERT(!c.IsReadyForCommand());

    InitController(c);
    gSCLPort.Output.clear();
    c.Examine();
    TEST_ASSERT(gSCLPort.Output.compare("E ") == 0);
    TEST_ASSERT(!c.IsReadyForCommand());

    InitController(c);
    gSCLPort.Output.clear();
    c.Deposit(042);
    TEST_ASSERT(gSCLPort.Output.compare("D 000042\r") == 0);
    TEST_ASSERT(!c.IsReadyForCommand());

    InitController(c);
    gSCLPort.Output.clear();
    c.Start();
    TEST_ASSERT(gSCLPort.Output.compare("S\r") == 0);
    TEST_ASSERT(!c.IsReadyForCommand());

    InitController(c);
    gSCLPort.Output.clear();
    c.SendCR();
    TEST_ASSERT(gSCLPort.Output.compare("\r") == 0);

    printf("PASS\n");

    return true;
}

int
main(int argc, char *argv[])
{
    int failures = 0;

    if (!Test1()) failures++;
    if (!Test2()) failures++;
    if (!Test3()) failures++;

    printf("%d failure%s\n", failures, failures != 1 ? "s" : "");

    return failures;
}

#endif // UNIT_TEST
