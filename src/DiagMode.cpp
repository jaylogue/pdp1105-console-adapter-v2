#include "ConsoleAdapter.h"


void DiagMode_BasicIOTest(Port& uiPort)
{
    char ch, lastSent = 0, lastRcvd = 0;
    uint32_t mismatchCount = 0;
    uint64_t nextSendTime = 0;
    bool paused = false;
    constexpr uint64_t kMaxSendIntervalUS = 500000;

    uiPort.Write(
        "*** BASIC I/O TEST\r\n"
        "\r\n"
        "Repeatedly send characters and verify that each character is echoed.\r\n"
        "\r\n"
        "Test with PDP-11/05 console echo program:\r\n"
        "\r\n"
        "  001000: 012700  start:   mov #CRSR,r0    ; get Console RSR address in r0\r\n"
        "  001002: 177560\r\n"
        "  001004: 105710  waitrx:  tstb (r0)       ; if no input character available...\r\n"
        "  001006: 002376           bge waitrx      ;    continue waiting\r\n"
        "  001010: 116001           movb 2(r0), r1  ; read input character\r\n"
        "  001012: 000002\r\n"
        "  001014: 110160           movb r1, 6(r0)  ; echo character\r\n"
        "  001016: 000006\r\n"
        "  001020: 000771           br waitrx       ; loop\r\n"
        "\r\n"
        "SPACE to pause/resume test\r\n"
        "Ctrl+C to exit test\r\n"
        "\r\n"
    );

    while (true) {
        bool updateStatus = false;

        // Update the state of the activity LEDs
        ActivityLED::UpdateState();

        // Listen for control keys
        if (TryReadHostAuxPorts(ch)) {
            if (ch == CTRL_C) {
                uiPort.Write("\r\n*** EXIT\r\n");
                break;
            }
            if (ch == ' ') {
                paused = !paused;
                updateStatus = true;
            }
        }

        // Send character to PDP-11
        if (time_us_64() >= nextSendTime && !paused) {
            if (nextSendTime != 0 && lastRcvd != lastSent) {
                mismatchCount++;
            }
            lastSent++;
            gSCLPort.Write(lastSent);
            nextSendTime = time_us_64() + kMaxSendIntervalUS;
            updateStatus = true;
        }

        // Receive character from PDP-11 and check against last sent character
        if (gSCLPort.TryRead(lastRcvd)) {
            if (lastRcvd == lastSent) {
                nextSendTime = 0; // send next character immediately
            }
            else {
                mismatchCount++;
            }
            updateStatus = true;
        }

        if (updateStatus) {
            uiPort.Printf("\rLast Sent: %3d, Last Rcvd: %3d, Mismaches: %d", 
                lastSent, lastRcvd, mismatchCount);
        }
    }
}

void DiagMode_ReaderRunTest(Port& uiPort)
{
    char ch;
    int lastRRState = -1;
    uint8_t localRRCount = 0;
    uint8_t pdp11RRCount = 0;
    bool paused = false;

    uiPort.Write(
        "*** READER RUN INTERFACE TEST\r\n"
        "\r\n"
        "Send characters in response to RR signals, count RR signals seen and compare\r\n"
        "to counts received from the PDP-11.\r\n"
        "\r\n"
        "Test with PDP-11/05 READER RUN TEST program:\r\n"
        "\r\n"
        "  001000: 012700  start:   mov #CRSR,r0    ; get Console RSR address in r0\r\n"
        "  001002: 177560\r\n"
        "  001004: 005001           clr r1          ; clear counter register\r\n"
        "  001006: 105210  loop:    incb (r0)       ; enable READER RUN\r\n"
        "  001010: 105710  waitrx:  tstb (r0)       ; if no input character available...\r\n"
        "  001012: 002376           bge waitrx      ;    continue waiting\r\n"
        "  001014: 116002           movb 2(r0), r2  ; read input character\r\n"
        "  001016: 000002\r\n"
        "  001020: 105201           incb r1         ; increment counter\r\n"
        "  001022: 110160           movb r1, 6(r0)  ; output counter value\r\n"
        "  001024: 000006\r\n"
        "  001026: 000767           br loop         ; continue receiving\r\n"
        "\r\n"
        "SPACE to pause/resume test\r\n"
        "Ctrl+C to exit test\r\n"
        "\r\n"
    );

    while (true) {
        bool updateStatus = false;

        // Update the state of the activity LEDs
        ActivityLED::UpdateState();

        if (TryReadHostAuxPorts(ch)) {
            if (ch == CTRL_C) {
                uiPort.Write("\r\n*** EXIT\r\n");
                break;
            }
            if (ch == ' ') {
                paused = !paused;
                updateStatus = true;
            }
        }

        // Get RR count from PDP-11
        if (gSCLPort.TryRead(ch)) {
            pdp11RRCount = (uint8_t)ch;
            updateStatus = true;
        }

        int rrState = (int)gpio_get(READER_RUN_PIN);
        if (rrState != lastRRState) {
            lastRRState = rrState;
            updateStatus = true;
        }

        if (gSCLPort.ReaderRunRequested() && !paused) {
            gSCLPort.ClearReaderRunRequested();
            localRRCount++;
            gSCLPort.Write('.');
            updateStatus = true;
        }

        if (updateStatus) {
            bool mismatch = (((int)localRRCount - (int)pdp11RRCount) > 1);

            uiPort.Printf("\rREADER RUN: %s, local RR count: %3u, PDP-11 RR count: %3u %s", 
                rrState ? "HIGH" : " LOW",
                localRRCount,
                pdp11RRCount,
                (mismatch) ? "[MISMATCH]" : "          ");
        }
    }
}
