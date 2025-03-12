#!/usr/bin/env python3

"""
PDP-11 M9312 Console Interface Simulator

This script simulates the user interface of the PDP-11 M9312 console.
"""

import sys
import termios
import tty
import os
import argparse
import re
from datetime import datetime

class M9312Simulator:
    def __init__(self, device=None, serial_config=None, log_file=None):
        # Initialize 64KB of memory (32K words)
        self.memory = bytearray(64 * 1024)
        self.current_address = 0
        self.previous_command = None
        self.prompt = "\r\n@"
        self.last_direction = None
        
        # Set I/O device
        self.device = device
        self.in_fd = None
        self.out_fd = None
        self.device_opened = False
        self.serial_config = serial_config
        
        # Setup logging
        self.log_file = None
        if log_file:
            try:
                self.log_file = open(log_file, "a")
                timestamp = self.get_timestamp()
                self.log_file.write(f"\n--- M9312 Simulator Session Started at {timestamp} ---\n")
                self.log_file.flush()
            except OSError as e:
                print(f"Error opening log file {log_file}: {e}")
                # Continue without logging
                
        # Initialize I/O device
        if device:
            try:
                # Ensure device path is properly handled as a string
                device_path = str(device)
                self.in_fd = os.open(device_path, os.O_RDWR)
                self.out_fd = self.in_fd
                self.device_opened = True
            except OSError as e:
                print(f"Error opening device {device}: {e}")
                sys.exit(1)
        else:
            self.in_fd = sys.stdin.fileno()
            self.out_fd = sys.stdout.fileno()

        self.orig_in_fd_settings = termios.tcgetattr(self.in_fd)
        tty.setraw(self.in_fd)

        # Configure serial settings if serial configuration is provided
        if serial_config:
            baud_rate, data_bits, parity, stop_bits = serial_config
            self.configure_serial_port(baud_rate, data_bits, parity, stop_bits)

    def get_timestamp(self):
        """Get current timestamp for logging"""
        return datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    
    def log_char(self, char, direction):
        """Log a character to the log file with direction indicator"""
        if not self.log_file:
            return
            
        # Format non-printable characters as \xXX
        if not char.isprintable() or char.isspace() or ord(char) > 126:
            formatted = f"\\x{ord(char):02X}"
        else:
            formatted = char
            
        try:
            if self.last_direction == None or self.last_direction != direction:
                self.log_file.write(f"\n{direction}: ")
                self.last_direction = direction
            self.log_file.write(formatted)
            self.log_file.flush()
        except OSError:
            # Silently ignore logging errors
            pass
            
    def configure_serial_port(self, baud_rate, data_bits, parity, stop_bits):
        """Configure serial port parameters"""
        if not self.device_opened:
            return
            
        # Get current settings
        settings = termios.tcgetattr(self.in_fd)
        
        # Map baud rate to termios constant
        # These constants are defined in the termios module on Linux
        baud_map = {
            110: 0o003,    # B110
            300: 0o007,    # B300
            600: 0o010,    # B600
            1200: 0o011,   # B1200
            2400: 0o013,   # B2400
            4800: 0o014,   # B4800
            9600: 0o015,   # B9600
            19200: 0o016,  # B19200
            38400: 0o017   # B38400
        }

        # If baud rate not in map, use closest available
        if baud_rate not in baud_map:
            closest = min(baud_map.keys(), key=lambda x: abs(x - baud_rate))
            print(f"Warning: Baud rate {baud_rate} not supported, using {closest} instead")
            baud_rate = closest

        # Set input and output baud rate (directly in settings)
        # In termios.h: c_ispeed and c_ospeed are in positions 4 and 5 of the array
        settings[4] = baud_map[baud_rate]  # Input speed
        settings[5] = baud_map[baud_rate]  # Output speed
        
        # Linux termios constants
        CSIZE = 0o000060
        CS7 = 0o000040
        CS8 = 0o000060
        PARENB = 0o000400
        PARODD = 0o001000
        CSTOPB = 0o000100
        
        # Set data bits (clear bits 3-0 in c_cflag, then set)
        settings[2] &= ~CSIZE  # Clear data bits
        if data_bits == 7:
            settings[2] |= CS7
        else:  # 8 bits
            settings[2] |= CS8
            
        # Set parity
        if parity == 'N':  # No parity
            settings[2] &= ~PARENB
        else:
            settings[2] |= PARENB  # Enable parity
            if parity == 'O':  # Odd parity
                settings[2] |= PARODD
            else:  # Even parity (default for PARENB)
                settings[2] &= ~PARODD
                
        # Set stop bits
        if stop_bits == 2:
            settings[2] |= CSTOPB
        else:  # 1 stop bit
            settings[2] &= ~CSTOPB
            
        # Apply settings
        termios.tcsetattr(self.in_fd, termios.TCSANOW, settings)

    def read_char(self):
        """Read a single character from input device and echo it"""
        ch = os.read(self.in_fd, 1).decode('utf-8', errors='replace')
        if ch == '\x03' or ch == '\x04':  # Control-C or Control-D
            raise KeyboardInterrupt()
        self.log_char(ch, "IN")
        self.write(ch)
        return ch
    
    def write(self, text):
        """Write a string to the output device"""
        # Log each character separately
        for ch in text:
            self.log_char(ch, "OUT")
            
        os.write(self.out_fd, text.encode('utf-8'))

    def reset(self):
        """Reset state and print status line"""
        self.current_address = 0
        self.previous_command = None
        self.write("\r\n000000 173000 165212 000000")
    
    def is_octal_digit(self, char):
        """Check if a character is an octal digit (0-7)"""
        return char >= '0' and char <= '7'

    def parse_arg(self):
        val = 0
        while True:
            ch = self.read_char()
            if ch == '\r':
                return val
            elif not self.is_octal_digit(ch):
                raise ValueError()
            val = ((val << 3) & 0xFFFF) + int(ch, 8)

    def process_command(self):
        """Read and process a single command"""

        # Read command character and next character
        cmd = self.read_char() + self.read_char()

        # Process Load Address command
        if cmd == 'L ':

            # Read and parse the address argument
            try:
                self.current_address = self.parse_arg()
            except ValueError:
                return

        # Process Examine command
        elif cmd == 'E ':

            # Fail if the current address is odd
            if (self.current_address & 1) == 1:
                self.reset()
                return

            # Increment address if previous command was also Examine
            if self.previous_command == cmd:
                self.current_address += 2
                
            # Output the value at the current address (little-endian)
            value = self.memory[self.current_address] | (self.memory[self.current_address + 1] << 8)
            self.write(f"{self.current_address:06o} {value:06o}")
            
        # Process Deposit command
        elif cmd == 'D ':

            # Fail if the current address is odd
            if (self.current_address & 1) == 1:
                self.reset()
                return

            # Read and parse the value
            try:
                value = self.parse_arg()
            except ValueError:
                return

            # Store value in little-endian format
            self.memory[self.current_address] = value & 0xFF
            self.memory[self.current_address + 1] = (value >> 8) & 0xFF

        # Process Start command
        elif cmd == 'S\r':

            # no-op
            pass

        # Otherwise unknown command
        else:
            self.reset()
            return

        self.previous_command = cmd

    def run(self):
        """Main loop to process commands"""

        # Reset state
        self.reset()

        try:
            while True:
                self.write(self.prompt)
                self.process_command()
        except KeyboardInterrupt:
            print("\r\nExiting...\r\n")
            sys.exit(0)
        finally:
            # Restore original terminal settings for I/O device
            if self.orig_in_fd_settings:
                termios.tcsetattr(self.in_fd, termios.TCSADRAIN, self.orig_in_fd_settings)
            # Close I/O device
            if self.device_opened:
                if self.in_fd:
                    try:
                        os.close(self.in_fd)
                    except OSError:
                        pass
                if self.out_fd and self.out_fd != self.in_fd:
                    try:
                        os.close(self.out_fd)
                    except OSError:
                        pass
            # Close log file
            if self.log_file:
                try:
                    timestamp = self.get_timestamp()
                    self.log_file.write(f"\n--- M9312 Simulator Session Ended at {timestamp} ---\n\n")
                    self.log_file.close()
                except OSError:
                    pass

def validate_serial_config(config_str):
    """
    Validate serial configuration string in format:
    <baud-rate>-<data-bits>-<parity>-<stop-bits>
    
    Where:
    - baud-rate: 110 to 38400
    - data-bits: 7 or 8
    - parity: O, E, or N (odd, even, none)
    - stop-bits: 1 or 2
    
    Returns parsed configuration as a tuple (baud_rate, data_bits, parity, stop_bits)
    or raises argparse.ArgumentTypeError if invalid
    """
    pattern = r'^(\d+)-([78])-(O|E|N)-([12])$'
    match = re.match(pattern, config_str)
    
    if not match:
        raise argparse.ArgumentTypeError(
            "Serial config must be in format: <baud-rate>-<data-bits>-<parity>-<stop-bits>\n"
            "  <baud-rate>: 110 to 38400\n"
            "  <data-bits>: 7 or 8\n"
            "  <parity>: O, E, or N (odd, even, none)\n"
            "  <stop-bits>: 1 or 2"
        )
    
    baud_rate = int(match.group(1))
    data_bits = int(match.group(2))
    parity = match.group(3)
    stop_bits = int(match.group(4))
    
    # Validate baud rate
    if baud_rate < 110 or baud_rate > 38400:
        raise argparse.ArgumentTypeError(f"Baud rate must be between 110 and 38400, got {baud_rate}")
        
    return (baud_rate, data_bits, parity, stop_bits)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="PDP-11 M9312 Console Interface Simulator")
    parser.add_argument("--device", "-d", type=str,
                      help="Device to use for console I/O (default: stdin/stdout)")
    parser.add_argument("--serial-config", "-s", type=validate_serial_config, 
                      help="Set I/O device serial configuration. Format: <baud>-<data-bits>-<parity>-<stop-bits>")
    parser.add_argument("--log-file", "-l", type=str,
                      help="Log I/O activity to the specified file")
    args = parser.parse_args()
    
    simulator = M9312Simulator(args.device, args.serial_config, args.log_file)
    simulator.run()