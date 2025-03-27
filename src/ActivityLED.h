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
