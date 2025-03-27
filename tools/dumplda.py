#!/usr/bin/env python3
"""
Dump PDP-11 LDA files

This script reads an LDA file (PDP-11 Paper Tape Binary Format) and
prints the contents of each block, including start marker, byte count,
load address, data, and checksum.
"""

import sys
import argparse

def errorExit(message):
    """Print error message to stderr and exit with code 1"""
    print(f"ERROR: {message}", file=sys.stderr)
    sys.exit(1)

def parseLdaFile(fileData):
    """
    Parse an LDA file and return the blocks.
    
    Each block consists of:
    - Leader (0x00 bytes, ignored)
    - Start marker (1 byte, value 0x01)
    - Pad (1 byte, value 0x00)
    - Byte Count (2 bytes, little-endian) - includes the 6 bytes of the header
    - Load Address (2 bytes, little-endian)
    - Block Data (byte count - 6 bytes length)
    - Checksum (1 byte)
    - Trailer (0x00 bytes, ignored)
    """
    blocks = []
    pos = 0
    
    # Skip initial leader bytes (0x00)
    while pos < len(fileData) and fileData[pos] == 0:
        pos += 1
    
    while pos < len(fileData):
        # Check for start marker
        if pos >= len(fileData) or fileData[pos] != 0x01:
            # Skip non-start bytes (could be trailer of previous block)
            pos += 1
            continue
            
        startPos = pos
        
        # Start marker found
        startMarker = fileData[pos]
        pos += 1
        
        # Check if we have enough bytes left for a complete block header
        if pos + 5 > len(fileData):
            break
            
        # Read pad byte (should be 0x00)
        pad = fileData[pos]
        pos += 1
        
        # Read byte count (2 bytes, little-endian)
        byteCount = fileData[pos] | (fileData[pos+1] << 8)
        pos += 2
        
        # Read load address (2 bytes, little-endian)
        loadAddress = fileData[pos] | (fileData[pos+1] << 8)
        pos += 2
        
        # Calculate actual data length (byte count minus 6 bytes for header)
        dataLength = byteCount - 6
        
        # Ensure the data length is non-negative
        if dataLength < 0:
            # Invalid block, move to next byte and continue
            pos = startPos + 1
            continue
        
        # Check if this is a special block with Byte Count = 6 (just header, no data, no checksum)
        isEndMarkerBlock = (byteCount == 6)
        
        if isEndMarkerBlock:
            # This is a special "End Marker" block with no data and no checksum
            blockData = bytearray()
            checksum = None
            checksumValid = True  # No checksum to validate
        else:
            # Normal block with data and checksum
            # Ensure we have enough bytes for the data + checksum
            if pos + dataLength + 1 > len(fileData):
                # Incomplete block, move to next byte and continue
                pos = startPos + 1
                continue
                
            # Read block data (data length is byte count minus 6 bytes for header)
            blockData = fileData[pos:pos+dataLength]
            pos += dataLength
            
            # Read checksum
            checksum = fileData[pos]
            pos += 1
            
            # Validate checksum by adding all bytes including the checksum byte and checking if sum mod 256 = 0
            checksumSum = (startMarker + pad + (byteCount & 0xFF) + 
                          ((byteCount >> 8) & 0xFF) + (loadAddress & 0xFF) + 
                          ((loadAddress >> 8) & 0xFF) + sum(blockData) + checksum) & 0xFF
            checksumValid = checksumSum == 0
        
        # Store block information
        blocks.append({
            'offset': startPos,  # Store the file offset where this block starts
            'start_marker': startMarker,
            'pad': pad,
            'byte_count': byteCount,
            'load_address': loadAddress,
            'is_end_marker': isEndMarkerBlock,
            'data': blockData,
            'checksum': checksum,
            'checksum_valid': checksumValid
        })
        
        # Skip trailer bytes (0x00) after a block
        while pos < len(fileData) and fileData[pos] == 0:
            pos += 1
    
    return blocks

def printBlocks(blocks):
    """Print the contents of each block"""
    for i, block in enumerate(blocks):
        print(f"Block {i}: (offset 0x{block['offset']:04X})")
        print(f"  Start Marker: 0x{block['start_marker']:02X}")
        print(f"  Pad: 0x{block['pad']:02X}")
        print(f"  Byte Count: {block['byte_count']}/0x{block['byte_count']:X} (includes 6-byte header)")
        
        # For end marker blocks with Byte Count = 6, print Load Address as Start Address
        if block['is_end_marker']:
            # For end marker blocks, indicate if the address is for halt (odd) or run (even)
            if block['load_address'] % 2 == 1:  # Odd address
                print(f"  Start Address: {block['load_address']:06o} (halt)")
            else:  # Even address
                print(f"  Start Address: {block['load_address']:06o} (run)")
        else:
            print(f"  Load Address: {block['load_address']:06o}")  # Print in octal, padded to 6 chars
        
        # Print data length and data in hex format, 16 bytes per row
        dataLength = len(block['data'])
        print(f"  Data: ({dataLength}/0x{dataLength:X} bytes)")
        if not block['data']:
            print("    <empty>")
        else:
            for i in range(0, dataLength, 16):
                # Get up to 16 bytes for this row
                row_data = block['data'][i:i+16]
                # Format as hex
                dataHex = ' '.join(f"{b:02X}" for b in row_data)
                # Print with offset from the start of the data
                print(f"    +{i:04X}: {dataHex}")
        
        # Print checksum and validation on the same line (only for blocks with checksums)
        if block['checksum'] is not None:
            validityStr = "valid" if block['checksum_valid'] else "INVALID"
            print(f"  Checksum: 0x{block['checksum']:02X} ({validityStr})")
        else:
            print("  No checksum (End Marker block)")
        print()

def main():
    """Main function to process command line arguments and parse LDA file"""
    try:
        parser = argparse.ArgumentParser(description='Read and display contents of an LDA file')
        parser.add_argument('lda_file', metavar='LDA_FILE', help='LDA file to read')
        
        args = parser.parse_args()
        
        # Read the input file
        try:
            with open(args.lda_file, 'rb') as f:
                fileData = f.read()
        except Exception as e:
            errorExit(f"Failed to read input file: {e}")
        
        if not fileData:
            errorExit("Input file is empty")
        
        # Parse the LDA file
        blocks = parseLdaFile(fileData)
        
        if not blocks:
            print("No valid blocks found in the file.")
            return
        
        # Print the blocks
        printBlocks(blocks)
        
    except KeyboardInterrupt:
        errorExit("Operation cancelled by user")
    except Exception as e:
        errorExit(f"Unexpected error: {e}")

if __name__ == "__main__":
    main()