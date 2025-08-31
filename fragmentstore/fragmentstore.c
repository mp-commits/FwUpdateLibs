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
 * The above copyright notice and this permission notice shall be included in 
 * all copies or substantial portions of the Software.
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
 * fragmentstore.c
 *
 * @brief Fragment store source file
*/

/*----------------------------------------------------------------------------*/
/* INCLUDE DIRECTIVES                                                         */
/*----------------------------------------------------------------------------*/

#include "fragmentstore/fragmentstore.h"

#include <string.h>

/*----------------------------------------------------------------------------*/
/* MACRO DEFINITIONS                                                          */
/*----------------------------------------------------------------------------*/

#define IS_NULL(ptr) (ptr == NULL)

/*----------------------------------------------------------------------------*/
/* PRIVATE FUNCTION DEFINITIONS                                               */
/*----------------------------------------------------------------------------*/

static inline size_t GetRequiredSectors(
    const FragmentArea_t* const area,
    size_t size)
{
    return ((area->memoryConfig->sectorSize + size) - 1U) 
            / area->memoryConfig->sectorSize;
} 

static inline bool IsEmpty(
    const FragmentArea_t* const area,
    const uint8_t* mem,
    size_t size)
{
    const uint8_t emptyVal = area->memoryConfig->eraseValue;

    for (size_t i = 0U; i < size; i++)
    {
        if (mem[i] != emptyVal)
        {
            return false;
        }
    }

    return true;
}

static inline bool TryReadMetadata(
    const FragmentArea_t* const area, 
    Metadata_t* metadata)
{
    return area->memoryConfig->Reader(
        area->memoryConfig->baseAddress, 
        sizeof(Metadata_t), 
        (uint8_t*)metadata);
}

static inline bool ValidateMetadata(
    const FragmentArea_t* const area,
    const Metadata_t* metadata)
{
    return area->ValidateMetadata(metadata);
}

static inline Address_t GetFragmentAddress(
    const FragmentArea_t* const area, 
    size_t index)
{
    const size_t sectorIndex 
        = area->metadataSectors + (index * area->fragmentSectors);

    const Address_t startAddress = area->memoryConfig->baseAddress;
    const size_t sectorSize = area->memoryConfig->sectorSize;

    return startAddress + (sectorIndex * sectorSize);
}

static inline bool CheckAddressValidity(
    const FragmentArea_t* const area,
    Address_t address,
    size_t size)
{
    const Address_t startAddress = area->memoryConfig->baseAddress;
    const Address_t endAddress = startAddress + area->memoryConfig->memorySize;

    if ((address < startAddress) ||      /* Invalid address location */
        (address >= endAddress) ||       /* Invalid address location */
        ((address + size) > endAddress)) /* Invalid size */
    {
        return false;
    }

    return true;
}

static inline bool TryReadFragment(
    const FragmentArea_t* const area,
    Address_t address,
    Fragment_t* fragment)
{
    return area->memoryConfig->Reader(
        address, 
        sizeof(Fragment_t), 
        (uint8_t*)fragment);
}

static inline bool TryValidateFragment(
    const FragmentArea_t* const area,
    const Fragment_t* fragment)
{
    return area->ValidateFragment(fragment);
}

static inline bool TryEraseMetadataArea(
    const FragmentArea_t* const area)
{
    const size_t eraseSize = 
        area->metadataSectors * area->memoryConfig->sectorSize;
    const Address_t eraseAddress = area->memoryConfig->baseAddress;

    return area->memoryConfig->Eraser(eraseAddress, eraseSize);
}

static inline bool TryEraseFragmentArea(
    const FragmentArea_t* const area,
    Address_t address)
{
    const size_t eraseSize = 
        area->fragmentSectors * area->memoryConfig->sectorSize;

    return area->memoryConfig->Eraser(address, eraseSize);
}

static inline bool TryWriteMetadata(
    const FragmentArea_t* const area,
    const Metadata_t* metadata)
{
    return area->memoryConfig->Writer(
        area->memoryConfig->baseAddress,
        sizeof(Metadata_t), 
        (const uint8_t*)metadata);
}

static inline bool TryWriteFragment(
    const FragmentArea_t* const area,
    const Fragment_t* fragment,
    Address_t address)
{
    return area->memoryConfig->Writer(
        address,
        sizeof(Fragment_t),
        (const uint8_t*)fragment);
}

/*----------------------------------------------------------------------------*/
/* PUBLIC FUNCTION DEFINITIONS                                                */
/*----------------------------------------------------------------------------*/

FA_ReturnCode_t FA_InitStruct(
    FragmentArea_t* area,
    const MemoryConfig_t* memConf,
    ValidateFragment_t ValidateFragment,
    ValidateMetadata_t ValidateMetadata)
{
    if (IS_NULL(area) || 
        IS_NULL(memConf) || 
        IS_NULL(ValidateFragment) ||
        IS_NULL(ValidateMetadata))
    {
        return FA_ERR_PARAM;
    }

    if (IS_NULL(memConf->Reader) || 
        IS_NULL(memConf->Writer) || 
        IS_NULL(memConf->Eraser))
    {
        return FA_ERR_PARAM;
    }

    if ((memConf->memorySize == 0U) ||
        (memConf->sectorSize == 0U) ||
        ((memConf->memorySize % memConf->sectorSize) != 0U))
    {
        return FA_ERR_PARAM;
    }

    area->memoryConfig      = memConf;
    area->ValidateFragment  = ValidateFragment;
    area->ValidateMetadata  = ValidateMetadata;
    area->metadataSectors   = GetRequiredSectors(area, sizeof(Metadata_t));
    area->fragmentSectors   = GetRequiredSectors(area, sizeof(Fragment_t));

    return FA_ERR_OK;
}

size_t FA_GetMaxFragmentIndex(
    const FragmentArea_t* const area)
{
    if (IS_NULL(area))
    {
        return 0U;
    }

    const size_t totalSectors = 
        area->memoryConfig->memorySize / area->memoryConfig->sectorSize;
    const size_t totalFragSec = totalSectors - area->metadataSectors;
    return totalFragSec / area->fragmentSectors;
}

FA_ReturnCode_t FA_EraseArea(
    const FragmentArea_t* const area)
{
    if (IS_NULL(area))
    {
        return FA_ERR_PARAM;
    }

    if (!area->memoryConfig->Eraser(
            area->memoryConfig->baseAddress,
            area->memoryConfig->memorySize)
        )
    {
        return FA_ERR_BUSY;
    }

    return FA_ERR_OK;
}

FA_ReturnCode_t FA_ReadMetadata(
    const FragmentArea_t* const area,
    Metadata_t* metadata)
{
    if (IS_NULL(area) ||
        IS_NULL(metadata))
    {
        return FA_ERR_PARAM;
    }

    if (!TryReadMetadata(area, metadata))
    {
        return FA_ERR_BUSY;
    }

    if (IsEmpty(area, (const uint8_t*)metadata, sizeof(Metadata_t)))
    {
        return FA_ERR_EMPTY;
    }

    if (!ValidateMetadata(area, metadata))
    {
        return FA_ERR_INVALID;
    }

    return FA_ERR_OK;
}

FA_ReturnCode_t FA_ReadFragment(
    const FragmentArea_t* const area,
    size_t index,
    Fragment_t* fragment)
{
    if (IS_NULL(area) ||
        IS_NULL(fragment))
    {
        return FA_ERR_PARAM;
    }

    const Address_t address = GetFragmentAddress(area, index);

    if (!CheckAddressValidity(area, address, sizeof(Fragment_t)))
    {
        return FA_ERR_PARAM;
    }

    if (!TryReadFragment(area, address, fragment))
    {
        return FA_ERR_BUSY;
    }

    if (IsEmpty(area, (const uint8_t*)fragment, sizeof(Fragment_t)))
    {
        return FA_ERR_EMPTY;
    }

    if (!TryValidateFragment(area, fragment))
    {
        return FA_ERR_INVALID;
    }

    return FA_ERR_OK;
}

FA_ReturnCode_t FA_WriteMetadata(
    const FragmentArea_t* const area,
    const Metadata_t* metadata)
{
    if (IS_NULL(area) ||
        IS_NULL(metadata))
    {
        return FA_ERR_PARAM;
    }

    if (!ValidateMetadata(area, metadata))
    {
        return FA_ERR_INVALID;
    }

    if (!TryEraseMetadataArea(area))
    {
        return FA_ERR_BUSY;
    }

    if (!TryWriteMetadata(area, metadata))
    {
        return FA_ERR_BUSY;
    }
    
    return FA_ERR_OK;
}

FA_ReturnCode_t FA_WriteFragment(
    const FragmentArea_t* const area,
    size_t index,
    const Fragment_t* fragment)
{
    if (IS_NULL(area) ||
        IS_NULL(fragment))
    {
        return FA_ERR_PARAM;
    }

    const Address_t address = GetFragmentAddress(area, index);

    if (!CheckAddressValidity(area, address, sizeof(Fragment_t)))
    {
        return FA_ERR_PARAM;
    }

    if (!TryValidateFragment(area, fragment))
    {
        return FA_ERR_INVALID;
    }

    if (!TryWriteFragment(area, fragment, address))
    {
        return FA_ERR_BUSY;
    }

    return FA_ERR_OK;
}

FA_ReturnCode_t FA_EraseFragmentSlot(
    const FragmentArea_t* const area,
    size_t index)
{
    if (IS_NULL(area))
    {
        return FA_ERR_PARAM;
    }

    const Address_t address = GetFragmentAddress(area, index);

    if (!CheckAddressValidity(area, address, sizeof(Fragment_t)))
    {
        return FA_ERR_PARAM;
    }

    if (!TryEraseFragmentArea(area, address))
    {
        return FA_ERR_BUSY;
    }

    return FA_ERR_OK;
}

FA_ReturnCode_t FA_FindLastFragment(
    const FragmentArea_t* const area,
    Fragment_t* fragment,
    size_t* index)
{
    if (IS_NULL(area) ||
        IS_NULL(fragment) ||
        IS_NULL(index))
    {
        return FA_ERR_PARAM;
    }

    size_t left = 0U;
    size_t right = FA_GetMaxFragmentIndex(area);
    
    while (left <= right)
    {
        const size_t middle = left + ((right - left) / 2U);
        const Address_t address = GetFragmentAddress(area, middle);

        if (!TryReadFragment(area, address, fragment))
        {
            /* Memory busy. Stop */
            return FA_ERR_BUSY;
        }

        if (IsEmpty(area, (const uint8_t*)fragment, sizeof(Fragment_t)))
        {
            /* Empty fragment, continue search */
            if (middle == 0U)
            {
                /* First possible index */
                return FA_ERR_EMPTY;
            }
            right = middle - 1U;
        }
        else if (!TryValidateFragment(area, fragment))
        {
            /* Invalid fragment. Stop and report index */
            *index = middle;
            return FA_ERR_INVALID;
        }
        else
        {
            /* Valid non-empty fragment, continue search */
            if (middle == SIZE_MAX)
            {
                /* Last possible index */
                break;
            }
            *index = middle;
            left = middle + 1U;
        }
    }

    return FA_ERR_OK;
}

/* EoF fragmentstore.c */
