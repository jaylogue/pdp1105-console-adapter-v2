
#include "ConsoleAdapter.h"
#include "M93xxController.h"

void LoadDataMode(Port& uiPort, LoadDataSource& dataSrc)
{
    M93xxController m93xxCtr;
    bool startAddrLoaded = false;

    uiPort.Printf("*** LOADING: %s\r\n", dataSrc.Name());

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
            uiPort.Write("*** INTERRUPTED\r\n");
            break;
        }

        // Try to read a character from the M9301/M9312 console; if successful...
        if (gSCLPort.TryRead(ch)) {

            // Pass the character to the M93xxController for processing.
            m93xxCtr.ProcessOutput(ch);

            // If the M9301/M9312 is now ready for another command, but all data has been
            // loaded, inform the user that:
            //    a) the start address is being loaded (if the LDA has a start address), or
            //    b) the load command is complete.
            // This is done before the SCL character is echoed so that this message
            // appears ahead of the M9301/M9312 prompt.
            if (m93xxCtr.IsReadyForCommand() && dataSrc.AtEnd()) {
                if (dataSrc.GetStartAddress() != NO_ADDR && !startAddrLoaded) {
                    uiPort.Write("*** LOADING START ADDRESS\r\n");
                }
                else {
                    uiPort.Write("*** LOAD COMPLETE\r\n");
                }
            }

            // Echo the character from the M9301/M9312 console so the user sees the
            // prompts and commands as they are executed.
            WriteHostAuxPorts(ch);
        }

        // If the M9301/M9312 is still processing the current command, wait until
        // it's done.
        if (!m93xxCtr.IsReadyForCommand()) {
            continue;
        }

        // If the last data word has been deposited...
        if (dataSrc.AtEnd()) {

            // If the LDA specifies a start address, issue a final set address
            // command (L) so the user can start the program using the S command.
            if (dataSrc.GetStartAddress() != NO_ADDR && !startAddrLoaded) {
                m93xxCtr.SetAddress(dataSrc.GetStartAddress());
                startAddrLoaded = true;
                continue;
            }

            // Otherwise the loading process is complete, so return to terminal mode
            break;
        }

        // Get the next word to be loaded from the data source; if the next word
        // is not ready yet, wait until it is.
        if (!dataSrc.GetWord(data, addr)) {
            continue;
        }

        // If the next deposit address is not the load address for the next word,
        // issue a set address (L) command to M9301/M9312 console the and wait
        // for it to complete.
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
