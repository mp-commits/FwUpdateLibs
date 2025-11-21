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
 * interface_w25qxx.c
 *
 * @brief Interface wrapper for the w25qxx driver
*/

/*----------------------------------------------------------------------------*/
/* INCLUDE DIRECTIVES                                                         */
/*----------------------------------------------------------------------------*/

#include "w25qxx/flash_interface.h"

/*----------------------------------------------------------------------------*/
/* PRIVATE TYPE DEFINITIONS                                                   */
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/* MACRO DEFINITIONS                                                          */
/*----------------------------------------------------------------------------*/

#define KB      (1024U)
#define _4KB    (4U*KB)
#define _32KB   (32U*KB)
#define _64KB   (64U*KB)

#define Aligned(val, alignment) (0 == ((val) % (alignment)))
#define Min(a,b) (((a) < (b)) ? (a) : (b))

/*----------------------------------------------------------------------------*/
/* VARIABLE DEFINITIONS                                                       */
/*----------------------------------------------------------------------------*/

static w25qxx_handle_t* f_w25qxx;
static uint8_t*         f_buf;
static size_t           f_bufSize;

/*----------------------------------------------------------------------------*/
/* PRIVATE FUNCTION DEFINITIONS                                               */
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/* PUBLIC FUNCTION DEFINITIONS                                                */
/*----------------------------------------------------------------------------*/

bool W25Qxx_INTERFACE_Init(
    w25qxx_handle_t* device, 
    uint8_t* workBuffer, 
    size_t workBufferSize
)
{
    if ((NULL != device) &&
        (NULL != workBuffer) &&
        (0U != workBufferSize))
    {
        f_w25qxx = device;
        f_buf = workBuffer;
        f_bufSize = workBufferSize;
        return true;
    }
    return false;
}

bool W25Qxx_INTERFACE_ReadFlash(
    Address_t address, 
    size_t size, 
    uint8_t* out
)
{
    return 0U == w25qxx_read(f_w25qxx, address, out, size);
}

bool W25Qxx_INTERFACE_WriteFlash(
    Address_t address, 
    size_t size, 
    const uint8_t* in
)
{
    return 0U == w25qxx_write(f_w25qxx, address, (uint8_t*)in, size);
}

bool W25Qxx_INTERFACE_WriteAndVerifyFlash(
    Address_t address, 
    size_t size, 
    const uint8_t* in
)
{
    if (!W25Qxx_INTERFACE_WriteFlash(address, size, in))
    {
        return false;
    }

    size_t pos = 0U;

    while (pos < size)
    {
        const size_t remaining = size - pos;
        const size_t blockSize = Min(remaining, f_bufSize);

        const Address_t readAddr = address + pos;
        if (0U != w25qxx_read(f_w25qxx, readAddr, f_buf, blockSize))
        {
            break;
        }

        if (0 != memcmp(f_buf, &in[pos], blockSize))
        {
            break;
        }

        pos += blockSize;
    }

    return pos == size;
}

bool W25Qxx_INTERFACE_EraseFlash(
    Address_t address, 
    size_t size
)
{
    if (!Aligned(address, _4KB) ||
        !Aligned(size, _4KB) ||
        (size == 0U))
    {
        return false;
    }

    const Address_t start = address;
    const Address_t end = address + size;
    Address_t pos = start;

    while (pos < end)
    {
        const size_t remaining = end - pos;
        
        if (Aligned(pos, _64KB) && (remaining > _64KB))
        {
            if (0U == w25qxx_block_erase_64k(f_w25qxx, pos))
            {
                pos += _64KB;
            }
            else
            {
                break;
            }
        }
        else if (Aligned(pos, _32KB) && (remaining > _32KB))
        {
            if (0U == w25qxx_block_erase_32k(f_w25qxx, pos))
            {
                pos += _32KB;
            }
            else
            {
                break;
            }
        }
        else
        {
            if (0U == w25qxx_sector_erase_4k(f_w25qxx, pos))
            {
                pos += _4KB;
            }
            else
            {
                break;
            }
        }
    }

    return pos == end;
}

/* EoF interface_w25qxx.c */
