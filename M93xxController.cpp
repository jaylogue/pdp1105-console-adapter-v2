#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <ctype.h>

#ifndef UNIT_TEST

#include "ConsoleAdapter.h"

#else // UNIT_TEST

extern void WriteSCL(char ch);
extern void WriteSCL(const char * str);

#endif // UNIT_TEST

#include "M93xxController.h"
#include "MemFile.h"

void M93xxController::Reset(void)
{
    mState = kWaitingForPrompt;
    mLastCmd = 0;
    mLastAddr = kUnknownAddress;
    mLastVal = 0;
}

void M93xxController::ProcessOutput(char ch)
{
    switch (mState) {
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

void M93xxController::SetAddress(uint16_t addr)
{
    if (mState = kReadyForCommand) {
        char loadCmd[10];
        snprintf(loadCmd, sizeof(loadCmd), "L %06" PRIo16 "\r", addr);
        WriteSCL(loadCmd);
        mState = kWaitingForResponse;
    }
}

void M93xxController::Deposit(uint16_t val)
{
    if (mState = kReadyForCommand) {
        char depositCmd[10];
        snprintf(depositCmd, sizeof(depositCmd), "D %06" PRIo16 "\r", val);
        WriteSCL(depositCmd);
        mState = kWaitingForResponse;
    }
}

void M93xxController::Examine(void)
{
    if (mState = kReadyForCommand) {
        WriteSCL("E ");
        mState = kWaitingForResponse;
    }
}

void M93xxController::Start(void)
{
    if (mState = kReadyForCommand) {
        WriteSCL("S\r");
        mState = kWaitingForResponse;
    }
}

void M93xxController::SendCR(void)
{
    WriteSCL('\r');
}

void M93xxController::LoadFromFile(MemFile *f)
{
    uint16_t data, addr;

    // Wait until the M9301/M9312 is ready for another command
    if (!IsReadyForCommand()) {
        return;
    }

    // Get the next word to be loaded from the input file; if at EOF
    // or if the next word is not ready yet, do nothing
    if (!f->GetWord(data, addr)) {
        return;
    }

    // If the next deposit address is not the target address for the
    // file word, issue a set address (L) command and wait for it to
    // complete
    if (NextDepositAddress() != addr) {
        SetAddress(addr);
        return;
    }

    // Issue a deposit command (D) to load the word into memory at
    // the target address
    Deposit(data);

    // Advance the input file to the next word
    f->Advance();
}

#ifdef UNIT_TEST

#include <string>
#include <assert.h>

std::string sSCLOutput;

void WriteSCL(char ch)               { sSCLOutput += ch; }
void WriteSCL(const char * str)      { sSCLOutput += str; }

void DriveController(M93xxController &c, const char * &p)
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

    c.Reset();
    c.ProcessOutput('@');
    sSCLOutput.clear();
    c.SetAddress(010101);
    TEST_ASSERT(sSCLOutput.compare("L 010101\r") == 0);
    TEST_ASSERT(!c.IsReadyForCommand());

    c.Reset();
    c.ProcessOutput('@');
    sSCLOutput.clear();
    c.SetAddress(0);
    TEST_ASSERT(sSCLOutput.compare("L 000000\r") == 0);
    TEST_ASSERT(!c.IsReadyForCommand());

    c.Reset();
    c.ProcessOutput('@');
    sSCLOutput.clear();
    c.Examine();
    TEST_ASSERT(sSCLOutput.compare("E ") == 0);
    TEST_ASSERT(!c.IsReadyForCommand());

    c.Reset();
    c.ProcessOutput('@');
    sSCLOutput.clear();
    c.Deposit(042);
    TEST_ASSERT(sSCLOutput.compare("D 000042\r") == 0);
    TEST_ASSERT(!c.IsReadyForCommand());

    c.Reset();
    c.ProcessOutput('@');
    sSCLOutput.clear();
    c.Start();
    TEST_ASSERT(sSCLOutput.compare("S\r") == 0);
    TEST_ASSERT(!c.IsReadyForCommand());

    c.Reset();
    c.ProcessOutput('@');
    sSCLOutput.clear();
    c.SendCR();
    TEST_ASSERT(sSCLOutput.compare("\r") == 0);

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
