#include "ConsoleAdapter.h"
#include "PTRProgressBar.h"
#include "Settings.h"

int PTRProgressBar::sState = PROGRESS_BAR_INACTIVE;
Port * PTRProgressBar::sPort;

void PTRProgressBar::Update(Port * uiPort)
{
    // Only display progress bar when paper tape is mounted...
    if (PaperTapeReader::IsMounted()) {

        // If the target port has changed, clear any existing progress bar
        // on the old port.
        if (sState != PROGRESS_BAR_INACTIVE &&
            uiPort != sPort) {
            Clear();
        }

        // Initialize the progress bar if needed...
        if (sState == PROGRESS_BAR_INACTIVE) {

            // Only display progress bar if enabled in settings
            if (!Settings::ShouldShowPTRProgress(uiPort)) {
                return;
            }

            sState = 0;
            sPort = uiPort;

            // Display an empty progress bar
            sPort->Printf("%s%*s%s", 
                PROGRESS_BAR_PREFIX, PROGRESS_BAR_WIDTH, "", PROGRESS_BAR_SUFFIX);

            // Backspace to 0 position
            for (auto i = PROGRESS_BAR_WIDTH + strlen(PROGRESS_BAR_SUFFIX); i; i--) {
                sPort->Write(BS);
            }
        }

        // Compute the new progress bar position
        size_t pos = (PaperTapeReader::TapePosition() * PROGRESS_BAR_WIDTH) / 
            PaperTapeReader::TapeLength();
        pos = MIN(pos, PROGRESS_BAR_WIDTH);

        // Add or remove progress bar characters as needed.
        while (sState < pos) {
            sPort->Write('=');
            sState++;
        }
        while (sState > pos) {
            sPort->Write(RUBOUT);
            sState--;
        }
    }

    else {
        Clear();
    }
}

void PTRProgressBar::Clear(void)
{
    if (sState != PROGRESS_BAR_INACTIVE) {

        // Overwrite the progress bar with spaces and reposition back to the
        // original output point.
        for (auto i = sState + strlen(PROGRESS_BAR_PREFIX); i; i--) {
            sPort->Write(BS);
        }
        for (auto i = PROGRESS_BAR_TOTAL_WIDTH; i; i--) {
            sPort->Write(' ');
        }
        for (auto i = PROGRESS_BAR_TOTAL_WIDTH; i; i--) {
            sPort->Write(BS);
        }

        sState = PROGRESS_BAR_INACTIVE;
        sPort = NULL;
    }
}
