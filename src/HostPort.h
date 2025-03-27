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

#ifndef HOST_PORT_H
#define HOST_PORT_H

struct SerialConfig;

class HostPort final : public Port
{
public:
    HostPort(void) = default;
    virtual ~HostPort(void) = default;

    HostPort(const HostPort&) = delete;
    HostPort& operator=(const HostPort&) = delete;

    void Init(void);
    const SerialConfig& GetConfig(void);
    bool ConfigChanged(void);

    virtual char Read(void);
    virtual bool TryRead(char &ch);
    virtual void Write(char ch);
    virtual void Write(const char * str);
    virtual void Flush(void);
    virtual bool CanWrite(void);

private:
    static SerialConfig sSerialConfig;
};

extern HostPort gHostPort;

inline char HostPort::Read(void)
{
    char ch = stdio_getchar();
    ActivityLED::SysActive();
    return ch;
}

inline bool HostPort::TryRead(char& ch)
{
    if (stdio_get_until(&ch, 1, 0) == 1) {
        ActivityLED::SysActive();
        return true;
    }
    return false;
}

inline void HostPort::Write(char ch)
{
    stdio_putchar_raw(ch);
    ActivityLED::SysActive();
}

inline void HostPort::Write(const char * str)
{
    stdio_put_string(str, strlen(str), false, false);
    ActivityLED::SysActive();
}

inline void HostPort::Flush(void)
{
    stdio_flush();
}

inline bool HostPort::CanWrite(void)
{
    return true;
}

#endif // HOST_PORT_H