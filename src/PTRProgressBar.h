#ifndef PTR_PROGRESS_BAR_H
#define PTR_PROGRESS_BAR_H

/** Displays a text-based progress bar depicting the position
 *  of the virtual paper tape reader.
 */
class PTRProgressBar final
{
public:
    static void Update(Port * uiPort);
    static void Clear(void);

private:
    static constexpr int PROGRESS_BAR_INACTIVE = -1;

    static constexpr const char * PROGRESS_BAR_PREFIX = " PTR:[";
    static constexpr const char * PROGRESS_BAR_SUFFIX = "]";

    static constexpr size_t PROGRESS_BAR_TOTAL_WIDTH =
        strlen(PROGRESS_BAR_PREFIX) + PROGRESS_BAR_WIDTH + strlen(PROGRESS_BAR_SUFFIX);

    static int sState;
    static Port * sPort;
};

#endif // PTR_PROGRESS_BAR_H
