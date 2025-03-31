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
    mState = kStart;
    mLastCmd = 0;
    mLastAddr = kUnknownAddress;
    mLastVal = 0;
    CancelPromptTimeout();
    CancelIdleTimeout();
}

bool M93xxController::ProcessOutput(char ch)
{
    // Fail if the character is not something we expect the M9301/M9312
    // console to output.  This may indicate that we're not talking to a
    // console or that the serial configuration is wrong.
    if (!IsValidOutputChar(ch)) {
        return false;
    }

    switch (mState) {
    case kWaitingForSyncChar:

        // Ignore all characters from the console until we see the sync
        // character. Once the sync character is seen, start looking for
        // the initial prompt character. Arm the idle timeout in case we
        // need to send another sync character.
        if (ch == kSyncChar) {
            mState = kWaitingForInitialPrompt;
            ArmIdleTimeout();
        }

        break;

    case kWaitingForInitialPrompt:

        // If we get any valid character while waiting for the initial prompt
        // then cancel the idle timeout.
        CancelIdleTimeout();

        /* fall thru */

    case kWaitingForPrompt:

        // Once we get a prompt character the console is ready for a command.
        if (ch == '@' || ch == '$') {
            mState = kReadyForCommand;
            CancelPromptTimeout();
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

    case kStart:
        break;
    }

    return true;
}

bool M93xxController::ProcessTimeouts(void)
{
    switch (mState) {
    case kStart:

        // Start the interaction with the console by sending a special
        // sync character and waiting for it to be echoed. Use a
        // character that will not be interpreted in any meaningful
        // way by the console code (a '.' in this case).
        gSCLPort.Write(kSyncChar);
        mState = kWaitingForSyncChar;

        // Arm the prompt timeout to limit the total amount of time
        // spent waiting for the initial prompt.
        ArmPromptTimeout();

        return false;

    case kWaitingForInitialPrompt:

        // If waiting for the initial prompt and no characters are
        // received within the idle timeout, then send another sync
        // character and continue waiting.
        //
        // This handles the case where the console was already waiting
        // at a prompt when the first sync character was sent.  Because
        // the console always waits for 2 input characters before
        // reacting, an additional character is necessary to get the
        // console to issue a prompt.
        if (IdleTimeoutExpired()) {
            gSCLPort.Write(kSyncChar);
        }

        /* fall thru */

    default:

        // Fail if the console doesn't issue a prompt within an appropriate
        // amount of time.
        if (PromptTimeoutExpired()) {
            return true;
        }

        return false;

    case kReadyForCommand:

        // No timeouts while waiting for a command from the application
        return false;
    }
}

void M93xxController::SetAddress(uint16_t addr)
{
    if (mState == kReadyForCommand) {
        char loadCmd[10];
        snprintf(loadCmd, sizeof(loadCmd), "L %06" PRIo16 "\r", addr);
        gSCLPort.Write(loadCmd);
        mState = kWaitingForResponse;
    }
}

void M93xxController::Deposit(uint16_t val)
{
    if (mState == kReadyForCommand) {
        char depositCmd[10];
        snprintf(depositCmd, sizeof(depositCmd), "D %06" PRIo16 "\r", val);
        gSCLPort.Write(depositCmd);
        mState = kWaitingForResponse;
    }
}

void M93xxController::Examine(void)
{
    if (mState == kReadyForCommand) {
        gSCLPort.Write("E ");
        mState = kWaitingForResponse;
    }
}

void M93xxController::Start(void)
{
    if (mState == kReadyForCommand) {
        gSCLPort.Write("S\r");
        mState = kWaitingForResponse;
    }
}

void M93xxController::SendCR(void)
{
    gSCLPort.Write('\r');
}

bool M93xxController::IsValidOutputChar(char ch)
{
    static const char kValidChars[] = { 
        '@', '$', ' ', '\r', '\n',
        'L', 'E', 'D', 'S',
        '0', '1', '2', '3', '4', '5', '6', '7'
    };

    if (memchr(kValidChars, ch, sizeof(kValidChars)) != NULL) {
        return true;
    }

    if (ch == kSyncChar && (mState == kWaitingForSyncChar || mState == kWaitingForInitialPrompt)) {
        return true;
    }

    return false;
}

#ifdef UNIT_TEST

#include <string>
#include <assert.h>

void InitController(M93xxController& c)
{
    c.Reset();
    c.ProcessTimeouts();
    c.ProcessOutput('.');
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
