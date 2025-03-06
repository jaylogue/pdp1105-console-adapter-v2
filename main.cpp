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

/*
 * A Raspberry Pi Pico based USB-to-serial adapter for interfacing with
 * the console of a PDP-11/05. 
 */

#include "ConsoleAdapter.h"
 
int main()
{
    // Initialize the host interface
    HostPort::Init();

    // Initialize the SCL port
    SCLPort::Init();

    // Initialize the Aux Terminal port
    AuxPort::Init();

    // Initialize the activity LEDs
    ActivityLED::Init();

    // Enter Terminal mode
    TerminalMode();
}
