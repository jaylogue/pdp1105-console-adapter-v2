
#include "ConsoleAdapter.h"
#include "M93xxController.h"

void LoadDataMode(Port& uiPort, LoadDataSource& dataSrc)
{
    M93xxController m93xxCtr;
    uint16_t startAddr = dataSrc.GetStartAddress();

    uiPort.Write("*** LOADING FILE ***\r\n");

    // Send a couple returns to get the M9301/M9312 console to issue a prompt
    m93xxCtr.SendCR();
    m93xxCtr.SendCR();

    // TODO: add timeouts

    while (true) {
        char ch;
        uint16_t data, addr;

        // Update the state of the activity LEDs
        ActivityLED::UpdateState();

        // Update the connection status of the SCL port
        gSCLPort.CheckConnected();

        // Check for an interrupt character from the UI port.
        if (uiPort.TryRead(ch) && ch == '\x03') {
            uiPort.Write("*** INTERRUPTED ***\r\n");
            break;
        }

        // Wait for a character from the M9301/M9312 console.
        if (!gSCLPort.TryRead(ch)) {
            continue;
        }

        // Process the character
        m93xxCtr.ProcessOutput(ch);

        // If all data has been loaded, and the M9301/M9312 is ready for another
        // command, inform the user that we're loading the start address or the
        // load command it complete.
        if (dataSrc.AtEOF() && m93xxCtr.IsReadyForCommand()) {
            if (startAddr != LoadDataSource::NO_ADDR) {
                uiPort.Write("*** LOADING START ADDRESS ***\r\n");
            }
            else {
                uiPort.Write("*** LOAD COMPLETE ***\r\n");
            }
        }

        // Echo the character from the M9301/M9312 console so the user can see the
        // prompts and commands as they are executed.
        WriteHostAuxPorts(ch);

        // Wait until the M9301/M9312 is finished processing the current command
        if (!m93xxCtr.IsReadyForCommand()) {
            continue;
        }

        // If the last deposit command has completed...
        if (dataSrc.AtEOF()) {

            // If a start address has been specified, issue a set address command for
            // the address so the user start the program by simply typing 'S'.
            if (startAddr != LoadDataSource::NO_ADDR) {
                m93xxCtr.SetAddress(startAddr);
                startAddr = LoadDataSource::NO_ADDR;
                continue;
            }

            // Otherwise the loading process is complete
            break;
        }

        // Get the next word to be loaded from the data source; if the next word
        // is not ready yet, wait until it is.
        if (!dataSrc.GetWord(data, addr)) {
            continue;
        }

        // If the next deposit address is not the target address for the
        // file word, issue a set address (L) command to M9301/M9312 console the
        // and wait for it to complete.
        if (m93xxCtr.NextDepositAddress() != addr) {
            m93xxCtr.SetAddress(addr);
            continue;
        }

        // Issue a deposit command (D) to load the word into memory at the
        // target address
        m93xxCtr.Deposit(data);

        // Advance the data source to the next word
        dataSrc.Advance();
    }
}
