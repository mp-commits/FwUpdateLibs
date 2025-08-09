/* MIT License
 * 
 * Copyright (c) 2025 Mikael Penttinen
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * -----------------------------------------------------------------------------
 *
 * command.c
 *
 * @brief Module for storing firmware update commands and install steps
 *        to user defined memory.
*/

/*----------------------------------------------------------------------------*/
/* INCLUDE DIRECTIVES                                                         */
/*----------------------------------------------------------------------------*/

#include "fragmentstore/command.h"
#include <string.h>

/*----------------------------------------------------------------------------*/
/* PRIVATE TYPE DEFINITIONS                                                   */
/*----------------------------------------------------------------------------*/

typedef struct
{
    uint32_t    command;
    Metadata_t  metadata;
    uint32_t    crc32;
} InstallMemory_t;

typedef struct
{
    Metadata_t metadata;
    uint32_t   crc32;
} HistoryMemory_t;

typedef struct
{
    uint32_t states[8U];
} StateMemory_t;

/*----------------------------------------------------------------------------*/
/* MACRO DEFINITIONS                                                          */
/*----------------------------------------------------------------------------*/

#define IS_NULL(ptr) (ptr == NULL)
#define ARRAY_SIZE(arr) (sizeof(arr)/sizeof(arr[0]))

#define MAGIC_HISTORY_WRITTEN   (0xA1A1A1A1)
#define MAGIC_FIRMWARE_WRITTEN  (0xB2B2B2B2)
#define MAGIC_FAILED            (0xEEEEEEEE)

/*----------------------------------------------------------------------------*/
/* VARIABLE DEFINITIONS                                                       */
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/* PRIVATE FUNCTION DEFINITIONS                                               */
/*----------------------------------------------------------------------------*/

static inline size_t GetRequiredSectors(
    const CommandArea_t* const ca,
    size_t size)
{
    return ((ca->memoryConfig->sectorSize + size) - 1U) 
            / ca->memoryConfig->sectorSize;
} 

static inline bool IsEmptyVal(
    const CommandArea_t* const ca,
    const uint8_t* buf, 
    size_t len
)
{
    for (size_t i = 0; i < len; i++)
    {
        if (buf[i] != ca->memoryConfig->eraseValue)
        {
            return false;
        }
    }

    return true;
}

static inline bool EntryExists(const StateMemory_t* mem, uint32_t magic)
{
    for (size_t i = 0; i < ARRAY_SIZE(mem->states); i++)
    {
        if (mem->states[i] == magic)
        {
            return true;
        }
    }

    return false;
}

static inline uint32_t GetMagic(CommandStatus_t status)
{
    switch (status)
    {
    case COMMAND_STATE_NONE:
        return 0xFFFFFFFFU;
    case COMMAND_STATE_HISTORY_WRITTEN:
        return MAGIC_HISTORY_WRITTEN;
    case COMMAND_STATE_FIRMWARE_WRITTEN:
        return MAGIC_FIRMWARE_WRITTEN;
    case COMMAND_STATE_FAILED:
        return MAGIC_FAILED;
    case COMMAND_STATE_COUNT:
        return 0xFFFFFFFFU;
    default:
        return 0xFFFFFFFFU;
    }
}

static bool EraseInstallMemory(const CommandArea_t* const ca)
{
    const Address_t addr = ca->commandAddress;
    const size_t    size = ca->commandSectors * ca->memoryConfig->sectorSize;

    return ca->memoryConfig->Eraser(addr, size);
}

static bool EraseHistoryMemory(const CommandArea_t* const ca)
{
    const Address_t addr = ca->historyAddress;
    const size_t    size = ca->historySectors * ca->memoryConfig->sectorSize;

    return ca->memoryConfig->Eraser(addr, size);
}

static bool EraseStateMemory(const CommandArea_t* const ca)
{
    const Address_t addr = ca->stateAddress;
    const size_t    size = ca->stateSectors * ca->memoryConfig->sectorSize;

    return ca->memoryConfig->Eraser(addr, size);
}

static bool WriteInstallMemory(
    const CommandArea_t* const ca,
    const InstallMemory_t* mem
)
{
    return ca->memoryConfig->Writer(
        ca->commandAddress, 
        sizeof(InstallMemory_t), 
        (const uint8_t*)mem
    );
}

static bool WriteHistoryMemory(
    const CommandArea_t* const ca,
    const HistoryMemory_t* mem
)
{
    return ca->memoryConfig->Writer(
        ca->historyAddress, 
        sizeof(HistoryMemory_t), 
        (const uint8_t*)mem
    );
}

/*----------------------------------------------------------------------------*/
/* PUBLIC FUNCTION DEFINITIONS                                                */
/*----------------------------------------------------------------------------*/

bool CA_InitStruct(
    CommandArea_t* ca, 
    MemoryConfig_t* memConf,
    Crc32_t Crc32
)
{
    if (IS_NULL(ca) ||
        IS_NULL(memConf) ||
        IS_NULL(Crc32))
    {
        return false;
    }

    if (IS_NULL(memConf->Reader) ||
        IS_NULL(memConf->Writer) ||
        IS_NULL(memConf->Eraser))
    {
        return false;
    }

    if ((memConf->memorySize == 0U) ||
        (memConf->sectorSize == 0U) ||
        ((memConf->memorySize % memConf->sectorSize) != 0U))
    {
        return false;
    }

    

    const size_t insSectors = GetRequiredSectors(ca, sizeof(InstallMemory_t));
    const size_t histSectors = GetRequiredSectors(ca, sizeof(HistoryMemory_t));
    const size_t statSectors = GetRequiredSectors(ca, sizeof(StateMemory_t));
    const size_t totalSectos = insSectors + histSectors + statSectors;
    const size_t totalMemReq = totalSectos * memConf->sectorSize;

    if (memConf->memorySize < totalMemReq)
    {
        return false;
    }

    const Address_t ba = memConf->baseAddress;
    const size_t    ss = memConf->sectorSize;

    ca->memoryConfig = memConf;
    ca->Crc32 = Crc32;
    ca->commandAddress = ba;
    ca->historyAddress = ba + (insSectors * ss);
    ca->stateAddress = ca->historyAddress + (histSectors * ss);
    ca->commandSectors = insSectors;
    ca->historySectors = histSectors;
    ca->stateSectors = statSectors;

    return true;
}

CommandStatus_t CA_GetStatus(
    const CommandArea_t* ca
)
{
    if (IS_NULL(ca))
    {
        return COMMAND_STATE_FAILED;
    }

    CommandStatus_t cmd = COMMAND_STATE_NONE;

    StateMemory_t mem;
    const bool res = ca->memoryConfig->Reader(
        ca->stateAddress,
        sizeof(StateMemory_t),
        (uint8_t*)(&mem)
    );

    if (!res)
    {
        return COMMAND_STATE_FAILED;
    }

    // Verify in reverse order

    if (EntryExists(&mem, MAGIC_FAILED))
    {
        return COMMAND_STATE_FAILED;
    }

    if (EntryExists(&mem, MAGIC_FIRMWARE_WRITTEN))
    {
        return COMMAND_STATE_FIRMWARE_WRITTEN;
    }

    if (EntryExists(&mem, MAGIC_HISTORY_WRITTEN))
    {
        return COMMAND_STATE_HISTORY_WRITTEN;
    }

    return COMMAND_STATE_NONE;
}

bool CA_SetStatus(
    const CommandArea_t* ca,
    CommandStatus_t cmd
)
{
    if (IS_NULL(ca))
    {
        return false;
    }

    StateMemory_t mem;
    const bool res = ca->memoryConfig->Reader(
        ca->stateAddress,
        sizeof(StateMemory_t),
        (uint8_t*)(&mem)
    );

    if (!res)
    {
        return COMMAND_STATE_FAILED;
    }

    const uint32_t magic = GetMagic(cmd);

    if (magic == 0xFFFFFFFFU)
    {
        return false;
    }

    if (EntryExists(&mem, magic))
    {
        return true;
    }

    bool commandSet = false;

    for (size_t i = 0; i < ARRAY_SIZE(mem.states); i++)
    {
        const uint8_t* bytes = (const uint8_t*)&mem.states[i];
        if (IsEmptyVal(ca, bytes, sizeof(uint32_t)))
        {
            mem.states[i] = magic;
            commandSet = true;
            break;
        }
    }

    if (!commandSet)
    {
        return false;
    }

    return ca->memoryConfig->Writer(
        ca->stateAddress,
        sizeof(StateMemory_t),
        (const uint8_t*)(&mem)
    );
}

bool CA_WriteInstallCommand(
    const CommandArea_t* ca, 
    CommandType_t cmd,
    const Metadata_t* metadata
)
{
    if (IS_NULL(ca) ||
        IS_NULL(metadata))
    {
        return false;
    }

    if (!EraseInstallMemory(ca))
    {
        return false;
    }

    if (!EraseStateMemory(ca))
    {
        return false;
    }

    InstallMemory_t mem;
    memset(&mem, 0, sizeof(InstallMemory_t));

    mem.command = (uint32_t)cmd;
    memcpy(&mem.metadata, metadata, sizeof(Metadata_t));
    mem.crc32 = ca->Crc32(
        (const uint8_t*)(&mem), 
        sizeof(InstallMemory_t) - sizeof(uint32_t)
    );

    if (!WriteInstallMemory(ca, &mem))
    {
        return false;
    }

    return true;
}

bool CA_ReadInstallCommand(
    const CommandArea_t* ca, 
    CommandType_t* cmd,
    Metadata_t* metadata
)
{
    if (IS_NULL(ca) ||
        IS_NULL(cmd) ||
        IS_NULL(metadata))
    {
        return false;
    }

    InstallMemory_t mem;
    const bool res = ca->memoryConfig->Reader(
        ca->commandAddress,
        sizeof(InstallMemory_t),
        (uint8_t*)(&mem)
    );

    if (!res)
    {
        return false;
    }

    const uint32_t crc = ca->Crc32(
        (const uint8_t*)(&mem),
        sizeof(InstallMemory_t) - sizeof(uint32_t)
    );

    if (crc != mem.crc32)
    {
        return false;
    }

    if (IsEmptyVal(ca, (const uint8_t*)&mem.command, sizeof(mem.command)))
    {
        *cmd = COMMAND_TYPE_NONE;
    }
    else
    {
        switch (mem.command)
        {
        case (uint32_t)COMMAND_TYPE_INSTALL_FIRMWARE:
            *cmd = COMMAND_TYPE_INSTALL_FIRMWARE;
            break;
        default:
            *cmd = COMMAND_TYPE_ERROR;
            break;
        }
    }

    memcpy(metadata, &mem.metadata, sizeof(Metadata_t));

    return true;
}

bool CA_WriteHistory(
    const CommandArea_t* ca, 
    const Metadata_t* metadata
)
{
    if (IS_NULL(ca) ||
        IS_NULL(metadata))
    {
        return false;
    }

    if (!EraseHistoryMemory(ca))
    {
        return false;
    }

    HistoryMemory_t mem;
    memset(&mem, 0, sizeof(HistoryMemory_t));
    memcpy(&mem.metadata, metadata, sizeof(Metadata_t));
    mem.crc32 = ca->Crc32(
        (const uint8_t*)(&mem), 
        sizeof(HistoryMemory_t) - sizeof(uint32_t)
    );

    if (!WriteHistoryMemory(ca, &mem))
    {
        return false;
    }

    return true;
}

bool CA_ReadHistory(
    const CommandArea_t* ca, 
    Metadata_t* metadata
)
{
    if (IS_NULL(ca) ||
        IS_NULL(metadata))
    {
        return false;
    }

    HistoryMemory_t mem;
    const bool res = ca->memoryConfig->Reader(
        ca->historyAddress,
        sizeof(HistoryMemory_t),
        (uint8_t*)(&mem)
    );

    if (!res)
    {
        return false;
    }

    const uint32_t crc = ca->Crc32(
        (const uint8_t*)(&mem),
        sizeof(HistoryMemory_t) - sizeof(uint32_t)
    );

    if (crc != mem.crc32)
    {
        return false;
    }

    memcpy(metadata, &mem.metadata, sizeof(Metadata_t));

    return true;
}

/* EoF command.c */
