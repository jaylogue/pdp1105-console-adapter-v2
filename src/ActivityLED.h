#ifndef ACTIVITY_LED_H
#define ACTIVITY_LED_H

class ActivityLED final
{
public:
    static void Init(void);
    static void TxActive(void);
    static void RxActive(void);
    static void SysActive(void);
    static void UpdateState(void);

private:
    struct LEDState {
        unsigned GPIO;
        bool IsActive;
        uint64_t LastUpdateTime;

        void UpdateState(void);
    };

    static LEDState sTxLED;
    static LEDState sRxLED;
    static LEDState sSysLED;
};

inline void ActivityLED::TxActive(void)
{
    sTxLED.IsActive = true;
}

inline void ActivityLED::RxActive(void)
{
    sRxLED.IsActive = true;
}

inline void ActivityLED::SysActive(void)
{
    sSysLED.IsActive = true;
}

#endif // ACTIVITY_LED_H
