
#include "ConsoleAdapter.h"
#include "M93xxController.h"

void LoadDataMode(LoadDataSource * dataSrc)
{
    M93xxController m93xxCtr;

    WriteHostAuxPorts("*** LOAD DATA\r\n");

    // Send a couple returns to get the M9301/M9312 console to issue a prompt
    m93xxCtr.SendCR();
    m93xxCtr.SendCR();

    // TODO: add timeouts

    while (true) {
        char ch;
        uint16_t data, addr;

        // Wait for a character from the M9301/M9312 console. While waiting,
        // check for an interrupt character from the Host or Aux Terminal ports.
        while (!SCLPort::TryRead(ch)) {
            if (TryReadHostAuxPorts(ch) && ch == '\x03') {
                WriteHostAuxPorts("*** LOAD DATA INTERRUPTED\r\n");
                return;
            }
        }

        // Process the character
        m93xxCtr.ProcessOutput(ch);

        // If the last deposit command has been issued, and the M9301/M9312 has
        // completed the command, exit load file mode.
        if (dataSrc->AtEOF() && m93xxCtr.IsReadyForCommand()) {
            WriteHostAuxPorts("*** LOAD DATA COMPLETE\r\n");
            WriteHostAuxPorts(ch);
            return;
        }

        // Echo the character from the M9301/M9312 console so the user can see the
        // commands as they are executed.
        WriteHostAuxPorts(ch);

        // Wait until the M9301/M9312 is finished processing the current command
        if (!m93xxCtr.IsReadyForCommand()) {
            continue;
        }

        // Get the next word to be loaded from the data source; if the next word
        // is not ready yet, wait until it is.
        if (!dataSrc->GetWord(data, addr)) {
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
        dataSrc->Advance();
    }
}
