#include <string.h>
#include "pico/stdlib.h"
#include "hardware/clocks.h"

#include "MenuKeyMonitor.h"

namespace {

size_t sKeySeq;
size_t sMatchLen;
absolute_time_t sLastMatchTime;
const char * const sKeySequences[] = {
    "\033Ox",       // vt100 F10
    "\033E[21~",    // ANSI, xterm, vt220, et al. F10
    "\0010\015",    // TVI-912 FUNC+0
    "\001I\015",    // TVI-920 F10
};
constexpr size_t kNumKeySequences = sizeof(sKeySequences) / sizeof(sKeySequences[0]);

} // unnamed namespace

MenuKeyMonitor::MatchResult MenuKeyMonitor::Match(char ch, const char * & delayedChars, size_t & numDelayedChars)
{
    size_t keySeq = sKeySeq;
    size_t matchLen = sMatchLen;

    delayedChars = "";
    numDelayedChars = 0;

    while (true) {
        const char * keySeqStr = sKeySequences[keySeq];

        if (keySeqStr[matchLen] == ch) {
            matchLen++;
            if (keySeqStr[matchLen] == 0) {
                sKeySeq = 0;
                sMatchLen = 0;
                return MATCH;
            }
            else {
                sLastMatchTime = get_absolute_time();
                sKeySeq = keySeq;
                sMatchLen = matchLen;
                return PARTIAL_MATCH;
            }
        }

        do {
            if (++keySeq == kNumKeySequences) {
                if (matchLen == 0) {
                    sKeySeq = 0;
                    sMatchLen = 0;
                    return NO_MATCH;
                }
                delayedChars = keySeqStr;
                numDelayedChars = matchLen;
                keySeq = 0;
                matchLen = 0;
            }
        } while (matchLen > 0 && strncmp(keySeqStr, sKeySequences[keySeq], matchLen) != 0);
    }
}

bool MenuKeyMonitor::CheckTimeout(uint32_t timeoutUS, const char * & delayedChars, size_t & numDelayedChars)
{
    if (sMatchLen == 0 || get_absolute_time() < (to_us_since_boot(sLastMatchTime) + timeoutUS)) {
        return false;
    }

    delayedChars = sKeySequences[sKeySeq];
    numDelayedChars = sMatchLen;
    sKeySeq = 0;
    sMatchLen = 0;
    return true;
}
