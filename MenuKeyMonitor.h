
#ifndef MENU_KEY_MONITOR_H
#define MENU_KEY_MONITOR_H

#include <string.h>

class MenuKeyMonitor
{
public:
    enum MatchResult {
        NO_MATCH,
        PARTIAL_MATCH,
        MATCH
    };

    static MatchResult Match(char ch, const char * & queuedChars, size_t & numQueuedChars);
    static bool CheckTimeout(uint32_t timeoutUS, const char * & queuedChars, size_t & numQueuedChars);
};

#endif // MENU_KEY_MONITOR_H