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
 * imitation_flash.h
 *
 * @brief Imitation flash imitates standard NOR flash memory
*/

#ifndef IMITATION_FLASH_H_
#define IMITATION_FLASH_H_

#ifdef __cplusplus
extern "C" {
#endif

/*----------------------------------------------------------------------------*/
/* INCLUDE DIRECTIVES                                                         */
/*----------------------------------------------------------------------------*/

#include <stdint.h>
#include <stdbool.h>

/*----------------------------------------------------------------------------*/
/* PUBLIC FUNCTION DECLARATIONS                                               */
/*----------------------------------------------------------------------------*/

extern void FLASH_SetMemory(uint8_t* mem, size_t memorySize, size_t sectorSize);

extern void FLASH_Fill(uint8_t value);

extern bool FLASH_Lock(void);

extern void FLASH_Unlock(void);

extern bool FLASH_Read(uint32_t address, size_t size, uint8_t* out);

extern bool FLASH_Write(uint32_t address, size_t size, const uint8_t* in);

extern bool FLASH_Erase(uint32_t address, size_t size);

#ifdef __cplusplus
} /* extern C */
#endif

/* EoF imitation_flash.h */

#endif /* IMITATION_FLASH_H_ */
