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
 * imitation_flash.c
 *
 * @brief {Short description of the source file}
*/

/*----------------------------------------------------------------------------*/
/* INCLUDE DIRECTIVES                                                         */
/*----------------------------------------------------------------------------*/

#include "imitation_flash.h"
#include <string.h>

/*----------------------------------------------------------------------------*/
/* VARIABLE DEFINITIONS                                                       */
/*----------------------------------------------------------------------------*/

static uint8_t* f_mem = NULL;
static size_t f_size = 0U;
static size_t f_sectorSize = 0U;
static bool f_lock = false;

/*----------------------------------------------------------------------------*/
/* PRIVATE FUNCTION DEFINITIONS                                               */
/*----------------------------------------------------------------------------*/

static inline bool CheckAccess(uint32_t address, size_t size)
{
    return (address < f_size) && ((address + size) <= f_size);
}

/*----------------------------------------------------------------------------*/
/* PUBLIC FUNCTION DEFINITIONS                                                */
/*----------------------------------------------------------------------------*/

void FLASH_SetMemory(uint8_t* mem, size_t memorySize, size_t sectorSize)
{
    f_mem = mem;
    f_size = memorySize;
    f_sectorSize = sectorSize;
}

void FLASH_Fill(uint8_t value)
{
    memset(f_mem, value, f_size);
}

bool FLASH_Lock(void)
{
    if (f_lock)
    {
        return false;
    }

    f_lock = true;
    return true;
}

void FLASH_Unlock(void)
{
    f_lock = false;
}

bool FLASH_Read(uint32_t address, size_t size, uint8_t* out)
{
    if (CheckAccess(address, size) && FLASH_Lock())
    {
        memcpy(out, &f_mem[address], size);
        FLASH_Unlock();
        return true;
    }

    return false;
}

bool FLASH_Write(uint32_t address, size_t size, const uint8_t* in)
{
    if (CheckAccess(address, size) && FLASH_Lock())
    {
        /* NOR flash turns 1s to 0s only*/
        for (size_t i = 0; i < size; i++)
        {
            f_mem[address + i] &= in[i];
        }
        FLASH_Unlock();
        return true;
    }

    return false;
}

bool FLASH_Erase(uint32_t address, size_t size)
{
    if (((address % f_sectorSize) != 0U) ||
        ((size % f_sectorSize) != 0U))
    {
        return false;
    }

    if (CheckAccess(address, size) && FLASH_Lock())
    {
        memset(&f_mem[address], 0xFF, size);
        FLASH_Unlock();
        return true;
    }

    return false;
}

/* EoF imitation_flash.c */
