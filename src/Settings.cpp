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

#include <inttypes.h>

#include "ConsoleAdapter.h"
#include "Settings.h"
#include "Menu.h"

#include "hardware/flash.h"
#include "hardware/sync.h"

#include "crc32.h"

struct alignas(uint64_t) SettingsRecord
{
    uint32_t Marker;
    uint16_t RecordSize;
    uint16_t RecordVersion;
    uint32_t DataRevision;

    uint32_t GetSize(void) const;
    bool IsValid(void) const;
    const SettingsRecord * NextInSector(void) const;
    uint32_t ComputeCheckSum(void) const;

    static constexpr uint32_t ACTIVE_MARKER = 0x5044500B;
    static constexpr uint32_t EMPTY_MARKER  = 0xFFFFFFFF;
};

struct alignas(uint64_t) SettingsRecord_V1 final : public SettingsRecord
{
    SerialConfig SCLConfig;
    bool SCLConfigFollowsUSB;
    SerialConfig AuxConfig;
    bool AuxConfigFollowsUSB;
    uint32_t ShowPTRProgress;
    uint32_t CheckSum;

    static constexpr uint32_t VERSION = 1;
};

struct alignas(uint64_t) SettingsRecord_V2 final : public SettingsRecord
{
    SerialConfig SCLConfig;
    bool SCLConfigFollowsUSB;
    SerialConfig AuxConfig;
    bool AuxConfigFollowsUSB;
    uint32_t ShowPTRProgress;
    uint32_t UppercaseMode;
    uint32_t CheckSum;

    static constexpr uint32_t VERSION = 2;
};

typedef struct SettingsRecord_V2 SettingsRecord_Latest;

SerialConfig Settings::SCLConfig = { SCL_DEFAULT_BAUD_RATE, 8, 1, SerialConfig::PARITY_NONE };
bool Settings::SCLConfigFollowsUSB = true;

SerialConfig Settings::AuxConfig = { AUX_DEFAULT_BAUD_RATE, 8, 1, SerialConfig::PARITY_NONE };
bool Settings::AuxConfigFollowsUSB;

Settings::ShowPTRProgress_t Settings::ShowPTRProgress = Settings::ShowPTRProgress_Enabled;

bool Settings::UppercaseMode;

const SettingsRecord * Settings::sActiveRec;
uint32_t Settings::sEraseCount;

// Start and end of adapter settings storage area in flash.
// Defined in linker script.
extern "C" uint8_t __SettingsStorageStart;
extern "C" uint8_t __SettingsStorageEnd;

static inline const uint8_t * GetFlashSector(const void * ptr)
{
    auto sector = ((uintptr_t)ptr) & ~(FLASH_SECTOR_SIZE - 1);
    return (const uint8_t *)sector;
}

static inline const uint8_t * GetFlashPage(const void * ptr)
{
    auto page = ((uintptr_t)ptr) & ~(FLASH_PAGE_SIZE - 1);
    return (const uint8_t *)page;
}

void Settings::Init(void)
{
    sActiveRec = NULL;

    // Scan each flash sector looking for the (active, valid and supported) settings
    // record with the latest (i.e. highest) data revision.
    for (auto sec = &__SettingsStorageStart; sec < &__SettingsStorageEnd; sec += FLASH_SECTOR_SIZE) {
        for (auto rec = (const SettingsRecord *)sec; rec != NULL; rec = rec->NextInSector()) {
            if (rec->IsValid() &&
                rec->Marker == SettingsRecord::ACTIVE_MARKER &&
                IsSupportedRecord(rec->RecordVersion)) {
                if (sActiveRec == NULL || rec->DataRevision >= sActiveRec->DataRevision) {
                    sActiveRec = rec;
                }
            }
        }
    }

    // If an active settings record was found, extract its contents
    if (sActiveRec != NULL) {
        if (sActiveRec->RecordVersion == SettingsRecord_V1::VERSION) {
            auto recV1 = (const SettingsRecord_V1 *)sActiveRec;
            SCLConfig = recV1->SCLConfig;
            SCLConfigFollowsUSB = recV1->SCLConfigFollowsUSB;
            AuxConfig = recV1->AuxConfig;
            AuxConfigFollowsUSB = recV1->AuxConfigFollowsUSB;
            ShowPTRProgress  = (ShowPTRProgress_t)recV1->ShowPTRProgress;
            UppercaseMode = false;
        }
        else if (sActiveRec->RecordVersion == SettingsRecord_V2::VERSION) {
            auto recV2 = (const SettingsRecord_V2 *)sActiveRec;
            SCLConfig = recV2->SCLConfig;
            SCLConfigFollowsUSB = recV2->SCLConfigFollowsUSB;
            AuxConfig = recV2->AuxConfig;
            AuxConfigFollowsUSB = recV2->AuxConfigFollowsUSB;
            ShowPTRProgress  = (ShowPTRProgress_t)recV2->ShowPTRProgress;
            UppercaseMode = (recV2->UppercaseMode != 0);
        }
    }
}

void Settings::Save(void)
{
    // Determine the data revision number to be stored in the new
    // settings record
    uint32_t nextDataRev = (sActiveRec != NULL) ? sActiveRec->DataRevision + 1 : 1;

    // Build a local copy of the data to be stored in the new settings
    // record.
    SettingsRecord_Latest newRecData = { };
    newRecData.Marker = SettingsRecord::ACTIVE_MARKER;
    newRecData.RecordSize = sizeof(newRecData);
    newRecData.RecordVersion = SettingsRecord_Latest::VERSION;
    newRecData.DataRevision = nextDataRev;
    newRecData.SCLConfig = SCLConfig;
    newRecData.SCLConfigFollowsUSB = SCLConfigFollowsUSB;
    newRecData.AuxConfig = AuxConfig;
    newRecData.AuxConfigFollowsUSB = AuxConfigFollowsUSB;
    newRecData.ShowPTRProgress = (uint8_t)ShowPTRProgress;
    newRecData.UppercaseMode = UppercaseMode;
    newRecData.CheckSum = newRecData.ComputeCheckSum();

    // Find the place in flash at which the new settings record should
    // be written.  At the same time, determine if the containing flash
    // sector needs to be erased first.
    bool eraseSector = false;
    const SettingsRecord * newRec = FindEmptyRecord(eraseSector);

    // Compute the base address of the page(s) in flash that need to
    // be written
    const uint8_t * page = GetFlashPage(newRec);

    // Compute the offset into page at which the record will be written
    size_t recOffsetInPage = (size_t)(((const uint8_t *)newRec) - page);

    // Compute the number of flash pages to be writen
    size_t pageCount = 
        (recOffsetInPage + sizeof(newRecData) + (FLASH_PAGE_SIZE - 1)) /
            FLASH_PAGE_SIZE;

    // Make a copy of the data currently contained in the target flash pages
    uint8_t writeBuf[pageCount * FLASH_PAGE_SIZE];
    if (!eraseSector) {
        memcpy(writeBuf, page, pageCount * FLASH_PAGE_SIZE);
    }
    else {
        memset(writeBuf, 0xFF, pageCount * FLASH_PAGE_SIZE);
    }

    // Update the copy with the new record data.
    memcpy(writeBuf + recOffsetInPage, (const uint8_t *)&newRecData, sizeof(newRecData));

    // Disable interrupts while manipulating flash
    uint32_t intState = save_and_disable_interrupts();

    // If needed, erase the sector before writing the new data
    if (eraseSector) {
        uint32_t sectorOffsetInFlash = (uint32_t)GetFlashSector(newRec) - XIP_BASE;
        flash_range_erase(sectorOffsetInFlash, FLASH_SECTOR_SIZE);
        sEraseCount++;
    }

    // Write the updated page data to flash
    uint32_t pageOffsetInFlash = ((uint32_t)page) - XIP_BASE;
    flash_range_program(pageOffsetInFlash, writeBuf, pageCount * FLASH_PAGE_SIZE);

    // Restore interrupts.
    restore_interrupts(intState);

    // Keep track of the most recently written settings record.
    sActiveRec = newRec;
}

const SettingsRecord * Settings::FindEmptyRecord(bool& eraseSector)
{
    eraseSector = false;

    // Search for empty space to store a new record within the same flash sector
    // as the most recently record written. Use that space if found.
    if (sActiveRec != NULL) {
        for (auto newRec = sActiveRec->NextInSector();
             newRec != NULL && newRec->IsValid();
             newRec = newRec->NextInSector()) {
            if (newRec->Marker == SettingsRecord::EMPTY_MARKER) {
                return newRec;
            }
        }
    }

    // If no room in the current sector, arrange to write the record in the next
    // logical sector.
    const uint8_t * newSector;
    if (sActiveRec != NULL) {
        newSector = GetFlashSector(sActiveRec) + FLASH_SECTOR_SIZE;
        if (newSector >= &__SettingsStorageEnd) {
            newSector = &__SettingsStorageStart;
        }
    }
    else {
        newSector = &__SettingsStorageStart;
    }

    // Check if the new sector needs to be erased first
    for (size_t i = 0; i < FLASH_SECTOR_SIZE; i++) {
        if (newSector[i] != 0xFF) {
            eraseSector = true;
            break;
        }
    }

    // Arrange to write the new record at the beginning of the sector.
    return (const SettingsRecord * )newSector;
}

bool Settings::IsSupportedRecord(uint16_t recVer)
{
    return (recVer == SettingsRecord_V1::VERSION ||
            recVer == SettingsRecord_V2::VERSION);
}

void Settings::PrintStats(Port& uiPort)
{
    size_t storageSize = (size_t)(&__SettingsStorageEnd - &__SettingsStorageStart);

    uiPort.Printf(
        TITLE_PREFIX "SETTINGS STATS:\r\n"
        "  Storage area start: 0x%08x\r\n"
        "  Storage area end: 0x%08x\r\n"
        "  Storage area size: %u (%u sectors)\r\n"
        "  Latest SettingsRecord version: %u\r\n"
        "  Latest SettingsRecord size: %u\r\n"
        "  Sector erase count since boot: %u\r\n",
        (uintptr_t)&__SettingsStorageStart,
        (uintptr_t)&__SettingsStorageEnd,
        storageSize, storageSize / FLASH_SECTOR_SIZE,
        SettingsRecord_Latest::VERSION,
        sizeof(SettingsRecord_Latest),
        sEraseCount
    );
    if (sActiveRec != NULL) {
        size_t curSector = 
            (size_t)(GetFlashSector(sActiveRec) - &__SettingsStorageStart) / FLASH_SECTOR_SIZE;
        size_t offset =
            (size_t)(((const uint8_t *)sActiveRec) - GetFlashSector(sActiveRec));
        size_t remainingSpace =
            (size_t)((GetFlashSector(sActiveRec) + FLASH_SECTOR_SIZE) - 
                ((const uint8_t *)sActiveRec + sActiveRec->GetSize()));
        uiPort.Printf(
            "  Active record flash sector: %zu\r\n"
            "  Active record offset in sector: %zu\r\n"
            "  Remaining space in active record sector: %zu\r\n"
            "  Active record version: %" PRIu32 "\r\n"
            "  Active record size: %" PRIu32 "\r\n"
            "  Active record data revision: %" PRIu32 "\r\n",
            curSector,
            offset,
            remainingSpace,
            sActiveRec->RecordVersion,
            sActiveRec->RecordSize,
            sActiveRec->DataRevision
        );
    }
    else {
        uiPort.Write(
            "  Active record flash sector: (n/a)\r\n"
            "  Active record offset in sector: (n/a)\r\n"
            "  Remaining space in active record sector: (n/a)\r\n"
            "  Active record version: (n/a)\r\n"
            "  Active record size: (n/a)\r\n"
            "  Active record data revision: (n/a)\r\n"
        );
    }
}

uint32_t SettingsRecord::GetSize(void) const
{
    return (Marker == ACTIVE_MARKER) ? RecordSize : sizeof(SettingsRecord_Latest);
}

bool SettingsRecord::IsValid(void) const
{
    // Shouldn't happen, but check anyway
    if (this == NULL) {
        return false;
    }

    const uint8_t * recStart = (const uint8_t *)this;

    // Verify that the record falls within the settings flash area
    if (recStart < &__SettingsStorageStart || recStart >= &__SettingsStorageEnd) {
        return false;
    }

    const uint8_t * sector = GetFlashSector(this);
    const uint8_t * sectorEnd = sector + FLASH_SECTOR_SIZE;

    // Verify that the header portion of the record does not overlap
    // the end of the sector
    const uint8_t * recHeaderEnd = recStart + sizeof(SettingsRecord);
    if (recHeaderEnd > sectorEnd) {
        return false;
    }

    // If the record is marked active...
    if (Marker == ACTIVE_MARKER) {

        // Verify that the record version and size are correct
        if (RecordVersion == 0) {
            return false;
        }
        else if (RecordVersion == SettingsRecord_V1::VERSION) {
            if (RecordSize != sizeof(SettingsRecord_V1)) {
                return false;
            }
        }
        else if (RecordVersion == SettingsRecord_V2::VERSION) {
            if (RecordSize != sizeof(SettingsRecord_V2)) {
                return false;
            }
        }

        // Verify that the full record does not overlap the end of the sector
        const uint8_t * recEnd = recStart + RecordSize;
        if (recEnd > sectorEnd) {
            return false;
        }

        // Verify the checksum value
        if (RecordVersion == SettingsRecord_V1::VERSION) {
            if (((const SettingsRecord_V1 *)this)->CheckSum != ComputeCheckSum()) {
                return false;
            }
        }
        else if (RecordVersion == SettingsRecord_V2::VERSION) {
            if (((const SettingsRecord_V2 *)this)->CheckSum != ComputeCheckSum()) {
                return false;
            }
        }
    }

    // Otherwise, the record must be an empty record...
    else {
        if (Marker != EMPTY_MARKER) {
            return false;
        }

        // Verify the empty record does not overlap the end of the sector
        const uint8_t * recEnd = recStart + sizeof(SettingsRecord_Latest);
        if (recEnd > sectorEnd) {
            return false;
        }

        // Verify that the record is truly empty
        for (size_t i = 0; i < sizeof(SettingsRecord_Latest); i++) {
            if (recStart[i] != 0xFF) {
                return false;
            }
        }
    }

    return true;
}

const SettingsRecord * SettingsRecord::NextInSector(void) const
{
    // Get a pointer to the record immediately following the current record.
    const uint8_t * nextRecStart = ((const uint8_t *)this) + GetSize();

    // Verify the next record falls entirely within the current
    // flash sector.  Return NULL if not.
    const uint8_t * const sectorStart = GetFlashSector(this);
    const uint8_t * const sectorEnd = sectorStart + FLASH_SECTOR_SIZE;
    if (nextRecStart < sectorStart || nextRecStart >= sectorEnd) {
        return NULL;
    }
    if ((nextRecStart + sizeof(SettingsRecord)) > sectorEnd) {
        return NULL;
    }
    const SettingsRecord * nextRec = (const SettingsRecord *)nextRecStart;
    const uint8_t * nextRecEnd = nextRecStart + nextRec->GetSize();
    if (nextRecEnd > sectorEnd) {
        return NULL;
    }

    return nextRec;
}

uint32_t SettingsRecord::ComputeCheckSum(void) const
{
    if (Marker == ACTIVE_MARKER) {
        if (RecordVersion == SettingsRecord_V1::VERSION) {
            auto recV1 = (const SettingsRecord_V1 *)this;
            return crc32((const uint8_t *)recV1,
                         ((const uint8_t *)&recV1->CheckSum) - (const uint8_t *)recV1);
        }
        else if (RecordVersion == SettingsRecord_V2::VERSION) {
            auto recV2 = (const SettingsRecord_V2 *)this;
            return crc32((const uint8_t *)recV2,
                         ((const uint8_t *)&recV2->CheckSum) - (const uint8_t *)recV2);
        }
    }
    return UINT32_MAX;
}
