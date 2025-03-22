#include "ConsoleAdapter.h"
#include "Settings.h"

static void HandleHostSerialConfigChange(void);
static void UpdateProgressBar(Port *lastUIPort);
static void ClearProgressBar(void);

static constexpr size_t PROGRESS_BAR_WIDTH = 20;
static constexpr const char * PROGRESS_BAR_PREFIX = " PTR:[";
static constexpr const char * PROGRESS_BAR_SUFFIX = "]";
static constexpr size_t PROGRESS_BAR_TOTAL_WIDTH =
    strlen(PROGRESS_BAR_PREFIX) + PROGRESS_BAR_WIDTH + strlen(PROGRESS_BAR_SUFFIX);

static constexpr int PROGRESS_BAR_INACTIVE = -1;

static int sProgressBarState = PROGRESS_BAR_INACTIVE;
static Port * sProgressBarPort = NULL;

void TerminalMode(void)
{
    char ch;
    Port *uiPort, *lastUIPort = &gHostPort;
    
    while (true) {

        // Update the connection status of the SCL port
        gSCLPort.CheckConnected();

        // Handle requests from the USB host to change the serial configuration.
        if (gHostPort.SerialConfigChanged()) {
            HandleHostSerialConfigChange();
        }

        // If the PDP-11 has triggered the READER RUN line and there's data
        // available to be read from a paper tape file, then deliver a single
        // byte from the paper tape to the SCL port.
        if (gSCLPort.ReaderRunRequested() && PaperTapeReader::TryRead(ch)) {
            gSCLPort.ClearReaderRunRequested();
            gSCLPort.Write(ch);

            // Display/update the paper taper reader progress bar
            UpdateProgressBar(lastUIPort);
        }

        // Process characters received from either the USB host or the auxiliary terminal.
        if (gSCLPort.CanWrite() && TryReadHostAuxPorts(ch, uiPort)) {

            // If the menu key was pressed enter menu mode
            if (ch == MENU_KEY) {
                ClearProgressBar();
                MenuMode(*uiPort);
            }

            // Otherwise forward the character to the SCL port.
            else {
                gSCLPort.Write(ch);
            }

            lastUIPort = uiPort;
        }

        // Forward characters received from the SCL port to both the Host and Aux terminals
        if (gSCLPort.TryRead(ch)) {
            ClearProgressBar();
            WriteHostAuxPorts(ch);
        }

        // Update the state of the activity LEDs
        ActivityLED::UpdateState();
    }
}

void HandleHostSerialConfigChange(void)
{
    SerialConfig newConfig;

    gHostPort.GetSerialConfig(newConfig);

    if (Settings::SCLConfigFollowsHost) {
        gSCLPort.SetConfig(newConfig);
    }

#if defined(AUX_TERM_UART)
    if (Settings::AuxConfigFollowsHost) {
        gAuxPort.SetConfig(newConfig);
    }
#endif
}

void UpdateProgressBar(Port * lastUIPort)
{
    if (PaperTapeReader::IsMounted()) {

        // If the target port has changed, clear any existing progress bar
        if (sProgressBarState != PROGRESS_BAR_INACTIVE &&
            lastUIPort != sProgressBarPort) {
            ClearProgressBar();
        }

        // Initialize the progress bar if needed...
        if (sProgressBarState == PROGRESS_BAR_INACTIVE) {

            // Only display progress bar if enabled
            if (!Settings::ShouldShowPTRProgress(lastUIPort)) {
                return;
            }

            sProgressBarState = 0;
            sProgressBarPort = lastUIPort;

            // Display an empty progress bar
            sProgressBarPort->Printf("%s%*s%s", 
                PROGRESS_BAR_PREFIX, PROGRESS_BAR_WIDTH, "", PROGRESS_BAR_SUFFIX);

            // Backspace to 0 position
            for (auto i = PROGRESS_BAR_WIDTH + strlen(PROGRESS_BAR_SUFFIX); i; i--) {
                sProgressBarPort->Write(BS);
            }
        }

        // Get the new progress bar position
        size_t pos = (PaperTapeReader::TapePosition() * PROGRESS_BAR_WIDTH) / 
            PaperTapeReader::TapeLength();
        pos = MIN(pos, PROGRESS_BAR_WIDTH);

        // Add or remove progress bar characters as needed.
        while (sProgressBarState < pos) {
            sProgressBarPort->Write('=');
            sProgressBarState++;
        }
        while (sProgressBarState > pos) {
            sProgressBarPort->Write(RUBOUT);
            sProgressBarState--;
        }
    }

    else {
        ClearProgressBar();
    }
}

void ClearProgressBar(void)
{
    if (sProgressBarState != PROGRESS_BAR_INACTIVE) {

        // Overwrite the progress bar with spaces and reposition back to the
        // original output point.
        for (auto i = sProgressBarState + strlen(PROGRESS_BAR_PREFIX); i; i--) {
            sProgressBarPort->Write(BS);
        }
        for (auto i = PROGRESS_BAR_TOTAL_WIDTH; i; i--) {
            sProgressBarPort->Write(' ');
        }
        for (auto i = PROGRESS_BAR_TOTAL_WIDTH; i; i--) {
            sProgressBarPort->Write(BS);
        }

        sProgressBarState = PROGRESS_BAR_INACTIVE;
        sProgressBarPort = NULL;
    }
}
