#!/usr/bin/env python3
"""
Paper Tape File Set Builder for PDP-11/05 Console Adapter

This script builds a paper tape file set for the PDP-11/05 Console Adapter. It takes
a list of input files and creates a UF2 file that can be directly flashed onto the
Console Adapter.
"""

import sys
import os
import struct
import zlib
import binascii
import argparse

def errorExit(message):
    """Print error message to stderr and exit with code 1"""
    print(f"ERROR: {message}", file=sys.stderr)
    sys.exit(1)

# File set structure constants
# Note: these must align with the FileSet C++ code
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

# Default base address (extracted from FileSet.cpp)
DEFAULT_BASE_ADDR = 0x10100000  # XIP_BASE (0x10000000) + 1 * 1024 * 1024

# RPi Pico flash page size in bytes
PICO_FLASH_PAGE_SIZE = 4096

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

def buildFilesetImage(fileNames):
    """
    Build a binary file set image from the input files.
    """
    filesetImage = bytearray()
    
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
        
        # Prepend the header, including the checksum, and the file data to the file set image
        filesetImage += struct.pack("<I", crc)
        filesetImage += headerWithoutChecksum
        filesetImage += fileData
        
        # Pad to align the next header
        paddingSize = (HEADER_ALIGNMENT - (len(filesetImage) % HEADER_ALIGNMENT)) % HEADER_ALIGNMENT
        filesetImage += b'\0' * paddingSize

    # Write an empty file header to mark the end of the file set
    filesetImage += b'\0' * HEADER_SIZE

    return filesetImage

def writeUf2File(outputFilePath, filesetImage, baseAddr):
    """
    Write the fileset image into a UF2 file using the specified base address.
    """
    # Pad the image data to a multiple of the RPi Pico flash page size
    paddingSize = (PICO_FLASH_PAGE_SIZE - (len(filesetImage) % PICO_FLASH_PAGE_SIZE)) % PICO_FLASH_PAGE_SIZE
    if paddingSize > 0:
        filesetImage = filesetImage + bytearray(b'\0' * paddingSize)
    
    # Calculate number of blocks needed
    totalBlocks = (len(filesetImage) + UF2_PAYLOAD_SIZE - 1) // UF2_PAYLOAD_SIZE
    
    try:
        with open(outputFilePath, 'wb') as outputFile:
            # Write UF2 blocks
            for blockNo in range(totalBlocks):
                # Calculate data offset and size for this block
                dataOffset = blockNo * UF2_PAYLOAD_SIZE
                dataSize = min(UF2_PAYLOAD_SIZE, len(filesetImage) - dataOffset)
                
                # Get data for this block
                blockData = filesetImage[dataOffset:dataOffset + dataSize]
                
                # Calculate target address for this block
                targetAddr = baseAddr + dataOffset
                
                # Create and write UF2 block
                uf2Block = createUf2Block(targetAddr, blockData, blockNo, totalBlocks)
                outputFile.write(uf2Block)
    except IOError as e:
        errorExit(f"Failed to write to output file {outputFilePath}: {e}")

def main():
    """
    Main function to process command line arguments and build the file set.
    """
    try:
        # Parse command line arguments
        parser = argparse.ArgumentParser(description='Build a paper-tape file set that can be flashed onto the PDP-11/05 Console Adapter')
        parser.add_argument('outputFile', metavar='<output-file>', help='Output flash image file (UF2 format)')
        parser.add_argument('inputFiles', metavar='<input-file>', nargs='+', help='Input paper tape files to include in the file set')
        parser.add_argument('--base-addr', type=lambda x: int(x, 0), default=DEFAULT_BASE_ADDR,
                            help='Base address in flash (default: 0x{:08X})'.format(DEFAULT_BASE_ADDR))
        
        args = parser.parse_args()

        # Build the fileset binary
        filesetImage = buildFilesetImage(args.inputFiles)
        print(f"File set created ({ len(args.inputFiles) } files, { len(filesetImage) } bytes)")

        writeUf2File(args.outputFile, filesetImage, args.base_addr)

        print(f"Flash image file created: {args.outputFile} ({os.path.getsize(args.outputFile)} bytes)")

        print("Done")

    except KeyboardInterrupt:
        errorExit("Operation cancelled by user")
    except Exception as e:
        errorExit(f"Unexpected error: {e}")

if __name__ == "__main__":
    main()