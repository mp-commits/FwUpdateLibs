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
 * command.h
 *
 * @brief Module for storing firmware update commands and install steps
 *        to user defined memory.
*/

#ifndef COMMAND_H_
#define COMMAND_H_

#ifdef __cplusplus
extern "C" {
#endif

/*----------------------------------------------------------------------------*/
/* INCLUDE DIRECTIVES                                                         */
/*----------------------------------------------------------------------------*/

#include "fragmentstore/fragmentstore.h"

/*----------------------------------------------------------------------------*/
/* PUBLIC TYPE DEFINITIONS                                                    */
/*----------------------------------------------------------------------------*/

typedef uint32_t (*Crc32_t)(const uint8_t* msg, size_t len);

typedef struct
{
    const MemoryConfig_t* memoryConfig;
    Crc32_t         Crc32;

    Address_t       commandAddress;
    Address_t       historyAddress;
    Address_t       stateAddress;
    size_t          commandSectors;
    size_t          historySectors;
    size_t          stateSectors;
} CommandArea_t;

typedef enum {
    COMMAND_TYPE_NONE,
    COMMAND_TYPE_ERROR,
    COMMAND_TYPE_INSTALL_FIRMWARE = 0xA5A5,
    COMMAND_TYPE_ROLLBACK = 0xD17D,
} CommandType_t;

typedef enum {
    COMMAND_STATE_NONE = 0,
    COMMAND_STATE_HISTORY_WRITTEN = 1,
    COMMAND_STATE_FIRMWARE_WRITTEN = 2,
    COMMAND_STATE_FAILED = 3,
    COMMAND_STATE_COUNT,
} CommandStatus_t;

/*----------------------------------------------------------------------------*/
/* PUBLIC FUNCTION DECLARATIONS                                               */
/*----------------------------------------------------------------------------*/

extern bool CA_InitStruct(
    CommandArea_t* ca, 
    const MemoryConfig_t* memConf,
    Crc32_t Crc32
);

extern CommandStatus_t CA_GetStatus(
    const CommandArea_t* ca
);

extern bool CA_SetStatus(
    const CommandArea_t* ca,
    CommandStatus_t cmd
);

extern bool CA_WriteInstallCommand(
    const CommandArea_t* ca, 
    CommandType_t cmd,
    const Metadata_t* metadata
);

extern bool CA_EraseInstallCommand(
    const CommandArea_t* ca
);

extern bool CA_ReadInstallCommand(
    const CommandArea_t* ca, 
    CommandType_t* cmd,
    Metadata_t* metadata
);

extern bool CA_WriteHistory(
    const CommandArea_t* ca, 
    const Metadata_t* metadata
);

extern bool CA_ReadHistory(
    const CommandArea_t* ca, 
    Metadata_t* metadata
);

#ifdef __cplusplus
} /* extern C */
#endif

/* EoF command.h */

#endif /* COMMAND_H_ */
