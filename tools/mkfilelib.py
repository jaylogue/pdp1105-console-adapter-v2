#!/usr/bin/env python3
"""
Tool for creating a file library image for the PDP-11/05 Console Adapter

This script takes an output file name, followed by a list of input files, and
creates an file library image file suitable for writing into the flash memory
of the PDP-11/05 Console Adapter.  The output file is in UF2 format, and thus
can be dragged directly onto the virtual drive created by the Raspberry Pi Pico
when in bootloader mode.
"""

import sys
import os
import struct
import zlib
import argparse

def errorExit(message):
    """Print error message to stderr and exit with code 1"""
    print(f"ERROR: {message}", file=sys.stderr)
    sys.exit(1)

# File library structure constants
# Note: these must align with the FileLib C++ code
MAX_FILE_NAME_LEN = 32
MAX_FILE_SIZE = 128 * 1024
HEADER_FORMAT = "<II{}s".format(MAX_FILE_NAME_LEN)  # uint32_t checksum, uint32_t length, char[MAX_FILE_NAME_LEN]
HEADER_SIZE = struct.calcsize(HEADER_FORMAT)
HEADER_ALIGNMENT = 4

# UF2 constants
UF2_MAGIC_START0 = 0x0A324655  # "UF2\n"
UF2_MAGIC_START1 = 0x9E5D5157  # UF2 file magic
UF2_MAGIC_END    = 0x0AB16F30  # UF2 file magic
UF2_FLAG_FAMILYID = 0x00002000  # Indicates the family ID field is present
UF2_BLOCK_SIZE = 512  # Size of a UF2 block
UF2_PAYLOAD_SIZE = 256  # Size of data payload in each block
UF2_DATA_SIZE = 476  # Size of data field in UF2 block (must be padded to this size)
UF2_FAMILY_ID_RP2040 = 0xE48BFF56  # Family ID for RP2040

# RPi Pico flash sector size in bytes
PICO_FLASH_SECTOR_SIZE = 4096

# Base address of Pico flash area
PICO_FLASH_BASE_ADDR = 0x10000000 # equal to XIP_BASE in Pico SDK

# Total Pico flash size 
PICO_TOTAL_FLASH_SIZE = 2 * 1024 * 1024

# Default file library base address
DEFAULT_BASE_ADDR = PICO_FLASH_BASE_ADDR + (PICO_TOTAL_FLASH_SIZE//2)

def createHeader(name, length):
    """
    Create a file header with the given name and length.
    Returns the header as bytes, without the checksum (which will be calculated later).
    """
    # Truncate name if necessary, and pad with zeros to MAX_FILE_NAME_LEN
    nameBytes = name.encode('utf-8')[:MAX_FILE_NAME_LEN]
    nameBytes = nameBytes + b'\0' * (MAX_FILE_NAME_LEN - len(nameBytes))
    
    # Create the header (without checksum for now)
    header = struct.pack("<I{}s".format(MAX_FILE_NAME_LEN), length, nameBytes)
    return header

def createUf2Block(addr, data, blockNo, totalBlocks, familyId=UF2_FAMILY_ID_RP2040):
    """
    Create a UF2 block with the given address, data, block number, and total blocks.
    """
    assert len(data) <= UF2_PAYLOAD_SIZE
    
    # Get the actual data size for the payload size field
    dataSize = len(data)
    
    # Construct the UF2 block header
    block = struct.pack("<IIIIIIII",
        UF2_MAGIC_START0, UF2_MAGIC_START1,
        UF2_FLAG_FAMILYID,  # flags (include family ID)
        addr,               # target address
        dataSize,           # payload size (actual data size)
        blockNo,            # block number
        totalBlocks,        # total number of blocks
        familyId            # family ID
    )
    
    # Add the data
    block += data
    
    # The data field must be 476 bytes according to the UF2 spec
    # Pad data field to UF2_DATA_SIZE
    dataPadding = UF2_DATA_SIZE - len(data)
    if dataPadding > 0:
        block += b'\0' * dataPadding
    
    # Add end magic
    block += struct.pack("<I", UF2_MAGIC_END)
    
    # Ensure we have the correct block size
    if len(block) != UF2_BLOCK_SIZE:
        errorExit(f"Block size {len(block)} != {UF2_BLOCK_SIZE}")
    
    return block

def buildLibImage(fileNames, maxLibSize):
    """
    Build a binary image of the file library as it will appear in memory on the Pico.
    """
    libImage = bytearray()
    
    # Process each input file
    for fileName in fileNames:
        # Get the base name of the input file, truncated to the max length
        baseName = os.path.basename(fileName)
        if len(baseName) > MAX_FILE_NAME_LEN:
            nameTruncated = True
            baseName = baseName[:MAX_FILE_NAME_LEN]
        else:
            nameTruncated = False

        # Read the input file
        try:
            with open(fileName, 'rb') as inputFile:
                fileData = inputFile.read()
        except Exception as e:
            errorExit(f"Unable to read {fileName}: {e}")

        # Get the length of the file
        fileLength = len(fileData)
        
        if fileLength > MAX_FILE_SIZE:
            errorExit(f"File '{fileName}' is too large (max size is { MAX_FILE_SIZE / 1024 }KiB)")

        print("Adding '{}' ({} bytes{})".format(baseName, fileLength, ", name truncated" if nameTruncated else ""))

        # Create the header (without checksum)
        headerWithoutChecksum = createHeader(baseName, fileLength)
        
        # Calculate the CRC32 checksum over the header (except checksum field) and file data
        crc = zlib.crc32(headerWithoutChecksum)
        crc = zlib.crc32(fileData, crc)
        
        # Prepend the header, including the checksum, and the file data to the file library image
        libImage += struct.pack("<I", crc)
        libImage += headerWithoutChecksum
        libImage += fileData
        
        # Pad to align the next header
        paddingSize = (HEADER_ALIGNMENT - (len(libImage) % HEADER_ALIGNMENT)) % HEADER_ALIGNMENT
        libImage += b'\0' * paddingSize

        if len(libImage) > maxLibSize:
            errorExit(f"File library is too large (max size is { maxLibSize / 1024 }KiB)")

    # Write an empty file header to mark the end of the file library
    libImage += b'\0' * HEADER_SIZE

    return libImage

def writeUf2File(outputFilePath, libImage, baseAddr):
    """
    Write a file library image into a UF2 file using the specified base address.
    """
    # Pad the image data to a multiple of the RPi Pico flash sector size
    paddingSize = (PICO_FLASH_SECTOR_SIZE - (len(libImage) % PICO_FLASH_SECTOR_SIZE)) % PICO_FLASH_SECTOR_SIZE
    if paddingSize > 0:
        libImage = libImage + bytearray(b'\0' * paddingSize)
    
    # Calculate number of blocks needed
    totalBlocks = (len(libImage) + UF2_PAYLOAD_SIZE - 1) // UF2_PAYLOAD_SIZE
    
    try:
        with open(outputFilePath, 'wb') as outputFile:
            # Write UF2 blocks
            for blockNo in range(totalBlocks):
                # Calculate data offset and size for this block
                dataOffset = blockNo * UF2_PAYLOAD_SIZE
                dataSize = min(UF2_PAYLOAD_SIZE, len(libImage) - dataOffset)
                
                # Get data for this block
                blockData = libImage[dataOffset:dataOffset + dataSize]
                
                # Calculate target address for this block
                targetAddr = baseAddr + dataOffset
                
                # Create and write UF2 block
                uf2Block = createUf2Block(targetAddr, blockData, blockNo, totalBlocks)
                outputFile.write(uf2Block)
    except IOError as e:
        errorExit(f"Failed to write to output file {outputFilePath}: {e}")

def main():
    """
    Main function to process command line arguments and build the file library image file.
    """
    try:
        # Parse command line arguments
        parser = argparse.ArgumentParser(description='Build a file library image file for the PDP-11/05 Console Adapter')
        parser.add_argument('outputFile', metavar='<output-file>', help='Name of the library image file to be created (UF2 format)')
        parser.add_argument('inputFiles', metavar='<input-file>', nargs='+', help='Name(s) of files to be included in the file library')
        parser.add_argument('--base-addr', type=lambda x: int(x, 0), default=DEFAULT_BASE_ADDR,
                            help='Base address for the file library in memory (default: 0x{:08X})'.format(DEFAULT_BASE_ADDR))
        args = parser.parse_args()

        maxLibSize = (PICO_FLASH_BASE_ADDR + PICO_TOTAL_FLASH_SIZE) - args.base_addr

        print(f"Creating file library image")

        # Build the file library memory image
        libImage = buildLibImage(args.inputFiles, maxLibSize)

        print(f"File library image created ({ len(args.inputFiles) } files, { len(libImage) } bytes)")

        # Create a UF2 file to load the file library into the target flash location.
        writeUf2File(args.outputFile, libImage, args.base_addr)

        print(f"File library image written to {args.outputFile} ({os.path.getsize(args.outputFile)} bytes)")

        print("Done")

    except KeyboardInterrupt:
        errorExit("Operation cancelled by user")
    except Exception as e:
        errorExit(f"Unexpected error: {e}")

if __name__ == "__main__":
    main()