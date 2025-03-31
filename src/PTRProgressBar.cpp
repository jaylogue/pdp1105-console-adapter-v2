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
        while ((size_t)sState < pos) {
            sPort->Write('=');
            sState++;
        }
        while ((size_t)sState > pos) {
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
        for (auto i = (size_t)sState + strlen(PROGRESS_BAR_PREFIX); i; i--) {
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
