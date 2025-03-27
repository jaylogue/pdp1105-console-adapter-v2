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
