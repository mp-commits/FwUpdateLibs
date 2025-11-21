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
 * flash_interface.h
 *
 * @brief Interface wrapper for the w25qxx driver
*/

#ifndef FLASH_INTERFACE_H_
#define FLASH_INTERFACE_H_

#ifdef __cplusplus
extern "C" {
#endif

/*----------------------------------------------------------------------------*/
/* INCLUDE DIRECTIVES                                                         */
/*----------------------------------------------------------------------------*/

#include "driver_w25qxx.h"
#include "fragmentstore/fragmentstore.h"    // Address_t

/*----------------------------------------------------------------------------*/
/* PUBLIC TYPE DEFINITIONS                                                    */
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/* PUBLIC MACRO DEFINITIONS                                                   */
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/* PUBLIC VARIABLE DEFINITIONS                                                */
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/* PUBLIC FUNCTION DECLARATIONS                                               */
/*----------------------------------------------------------------------------*/

/** Initialize interface wrapper for W25Qxx flash driver
 * 
 * @param device            Initialized w25qxx device handle
 * @param workBuffer        Memory buffer for verify operations
 * @param workBufferSize    Size of workBuffer
 * 
 * @return Init ok
 */
extern bool W25Qxx_INTERFACE_Init(
    w25qxx_handle_t* device, 
    uint8_t* workBuffer, 
    size_t workBufferSize
);

/** Read flash memory from the W25Qxx device
 * @note ReadMemory_t signature
 * @param address   Flash address
 * @param size      Read length
 * @param out       Destination Buffer
 */
extern bool W25Qxx_INTERFACE_ReadFlash(
    Address_t address, 
    size_t size, 
    uint8_t* out
);

/** Write to the W25Qxx device flash memory
 * @note WriteMemory_t signature
 * @param address   Flash address
 * @param size      Write length
 * @param in        Source buffer
 */
extern bool W25Qxx_INTERFACE_WriteFlash(
    Address_t address, 
    size_t size, 
    const uint8_t* in
);

/** Write to the W25Qxx device flash memory
 * @note WriteMemory_t signature
 * @param address   Flash address
 * @param size      Write length
 * @param in        Source buffer
 */
extern bool W25Qxx_INTERFACE_WriteAndVerifyFlash(
    Address_t address, 
    size_t size, 
    const uint8_t* in
);

/** Erase sectors in W25Qxx device flash memory
 * @note EraseSectors_t signature
 * @param address   Flash address
 * @param size      Erase size (Multiple of 4096)
 */
extern bool W25Qxx_INTERFACE_EraseFlash(
    Address_t address, 
    size_t size
);

#ifdef __cplusplus
} /* extern C */
#endif

/* EoF flash_interface.h */

#endif /* FLASH_INTERFACE_H_ */
