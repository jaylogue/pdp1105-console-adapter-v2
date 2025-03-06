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
};

#endif // ACTIVITY_LED_H
