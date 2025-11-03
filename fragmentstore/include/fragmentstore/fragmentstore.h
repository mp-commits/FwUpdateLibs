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
 * fragmentstore.h
 *
 * @brief Interface for fragment store library
*/

#ifndef FRAGMENTSTORE_H_
#define FRAGMENTSTORE_H_

#ifdef __cplusplus
extern "C" {
#endif

/*----------------------------------------------------------------------------*/
/* INCLUDE DIRECTIVES                                                         */
/*----------------------------------------------------------------------------*/

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#if (!defined METADATA_TYPE_DEFINED || \
     !defined FRAGMENT_TYPE_DEFINED || \
     !defined ADDRESS_TYPE_DEFINED)
#include "fragmentstore/default_types.h"
#endif

/*----------------------------------------------------------------------------*/
/* PUBLIC TYPE DEFINITIONS                                                    */
/*----------------------------------------------------------------------------*/

/* Formward declaration for configurability */
typedef struct Fragment_s Fragment_t;

/* Formward declaration for configurability */
typedef struct Metadata_s Metadata_t;

/** User defines AddressType, some width uint recommended.
 *  Arithmetic with AddressType and size_t is performed
 *  when calculating addresses. Width must be enought for full
 *  possible address range in MemoryConfig_t!
*/
typedef AddressIntType Address_t;

/** Read fragment memory
 * 
 * @param address Read start address
 * @param size Read size
 * @param out Read data area
 * @return Read successful
 */
typedef bool (*ReadMemory_t)(Address_t address, size_t size, uint8_t* out);

/** Write fragment memory
 * 
 * @param address Write start address
 * @param size Write size
 * @param out Write data area
 * @return Write successful
 */
typedef bool (*WriteMemory_t)(Address_t address, size_t size, const uint8_t* in);

/** Erase fragment memory sectors
 * 
 * @param address Erase start address (even sector address)
 * @param size Erase size (multiple of sector size)
 * @return Erase successful
 */
typedef bool (*EraseSectors_t)(Address_t address, size_t size);

/** Validate one fragment
 * 
 * @param frag Pointer to fragment structure
 * @return fragment is valid
 */
typedef bool (*ValidateFragment_t)(const Fragment_t* frag);

/** Validate one fragment
 * 
 * @param metadata Pointer to metadata structure
 * @return metadata is valid
 */
typedef bool (*ValidateMetadata_t)(const Metadata_t* metadata);

typedef struct
{
    Address_t       baseAddress;    /* Base address used for memory access */
    size_t          sectorSize;     /* Sector size. (smallest erasable sie) */
    size_t          memorySize;     /* Memory area size */
    uint8_t         eraseValue;     /* Erase value of the memory */

    ReadMemory_t    Reader;         /* Read memory */
    WriteMemory_t   Writer;         /* Write memory */
    EraseSectors_t  Eraser;         /* Erase memory */
} MemoryConfig_t;

typedef struct
{
    const MemoryConfig_t*   memoryConfig;
    size_t                  metadataSectors;
    size_t                  fragmentSectors;
    ValidateMetadata_t      ValidateMetadata;
    ValidateFragment_t      ValidateFragment;
} FragmentArea_t;

typedef enum
{
    FA_ERR_OK,
    FA_ERR_EMPTY,
    FA_ERR_INVALID,
    FA_ERR_BUSY,
    FA_ERR_PARAM
} FA_ReturnCode_t;

/*----------------------------------------------------------------------------*/
/* PUBLIC FUNCTION DECLARATIONS                                               */
/*----------------------------------------------------------------------------*/

/** Initialize data structure for one fragment area (FW slot)
 * 
 * @param area Allocated area structure
 * @param memConf Allocated and configured memory configuration
 * @param ValidateFragment Fragment validator function
 * @param ValidateMetadata Firmware Metadata validator function
 * 
 * @return OK when config successful
 * @return PARAM when parameters are invalid
 */
extern FA_ReturnCode_t FA_InitStruct(
    FragmentArea_t* area,
    const MemoryConfig_t* memConf,
    ValidateFragment_t ValidateFragment,
    ValidateMetadata_t ValidateMetadata);

/** Get the highest possible fragment index of this area
 * 
 * Calculation is based on the sector count requirements of metadata, fragments
 * and the memory size.
 * 
 * @param area Fragment area handle
 * @return Highest possible index
 */
extern size_t FA_GetMaxFragmentIndex(
    const FragmentArea_t* const area);

/** Erase the whole fragment area
 * 
 * @return OK when erase is successful
 * @return BUSY if memory is busy
 * @return PARAM if parameters are invalid
 */
extern FA_ReturnCode_t FA_EraseArea(
    const FragmentArea_t* const area);

/** Read the metadata from the area
 * 
 * @param area Fragment area handle
 * @param metadata Allocated memory for metadata output
 * 
 * @return OK when read is successful (valid metadata)
 * @return EMPTY if metadata is empty
 * @return INVALID if metadata is invalid
 * @return BUSY if memory is busy
 * @return PARAM if parameters are invalid
 */
extern FA_ReturnCode_t FA_ReadMetadata(
    const FragmentArea_t* const area,
    Metadata_t* metadata);

/** Read fragment from area
 * 
 * @param area Fragment area
 * @param index Fragment slot index between 0 and FA_GetMaxFragmentIndex()
 * @param fragment Allocated memory for fragment output
 * 
 * @return OK when read is successful (valid fragment)
 * @return EMPTY if fragment is empty
 * @return INVALID if fragment is invalid
 * @return BUSY if memory is busy
 * @return PARAM if parameters are invalid
 */
extern FA_ReturnCode_t FA_ReadFragment(
    const FragmentArea_t* const area,
    size_t index,
    Fragment_t* fragment);

/** Read fragment from area and ignore invalid content
 * 
 * @param area Fragment area
 * @param index Fragment slot index between 0 and FA_GetMaxFragmentIndex()
 * @param fragment Allocated memory for fragment output
 * 
 * @return OK when read is successful (valid or invalid fragment)
 * @return EMPTY if fragment is empty
 * @return BUSY if memory is busy
 * @return PARAM if parameters are invalid
 */
extern FA_ReturnCode_t FA_ReadFragmentForce(
    const FragmentArea_t* const area,
    size_t index,
    Fragment_t* fragment);

/** Write firmware metadata to fragment area
 * 
 * @param area Fragment area handle
 * @param metadata Allocated memory for metadata output
 * 
 * @note Required whole sectors are erased before metadata is written
 * 
 * @return OK when write  is successful
 * @return INVALID if metadata is invalid
 * @return BUSY if memory is busy
 * @return PARAM if parameters are invalid
 */
extern FA_ReturnCode_t FA_WriteMetadata(
    const FragmentArea_t* const area,
    const Metadata_t* metadata);

/** Write firmware fragment to fragment area
 * 
 * @param area Fragment area handle
 * @param index Fragment slot index between 0 and FA_GetMaxFragmentIndex()
 * @param fragment Allocated memory for fragment output
 * 
 * @note Required whole sectors are erased before fragment is written
 * 
 * @return OK when write  is successful
 * @return INVALID if metadata is invalid
 * @return BUSY if memory is busy
 * @return PARAM if parameters are invalid
 */
extern FA_ReturnCode_t FA_WriteFragment(
    const FragmentArea_t* const area,
    size_t index,
    const Fragment_t* fragment);

/** Erase one fragment slot
 * 
 * @param area Fragment area handle
 * @param index Fragment slot index between 0 and FA_GetMaxFragmentIndex()
 * 
 * @note Required whole sectors are erased before fragment is written
 * 
 * @return OK when erase  is successful
 * @return BUSY if memory is busy
 * @return PARAM if parameters are invalid
 */
extern FA_ReturnCode_t FA_EraseFragmentSlot(
    const FragmentArea_t* const area,
    size_t index);

/** Search fragment area slots using binary search until either:
 *  1. No more empty slots are found (last fragment)
 *  2. Any invalid fragment is found (invalid fragment at some index)
 * 
 * @param area Fragment area handle
 * @param fragment Working memory buffer for one fragment at a time
 * @param index Search result index at the time of return
 * 
 * @return OK=Last fragment index found
 * @return INVALID=Invalid fragment index found
 * @return BUSY=Memory was busy
 * @return PARAM=Function params invalid
 */
extern FA_ReturnCode_t FA_FindLastFragment(
    const FragmentArea_t* const area,
    Fragment_t* fragment,
    size_t* index);

/** Search fragment area slots using linear search until either:
 *  1. No more empty slots are found (last fragment)
 *  2. Any invalid fragment is found (invalid fragment at some index)
 * 
 * @param area Fragment area handle
 * @param fragment Working memory buffer for one fragment at a time
 * @param index Search result index at the time of return
 * 
 * @return OK=Last fragment index found
 * @return INVALID=Invalid fragment index found
 * @return BUSY=Memory was busy
 * @return PARAM=Function params invalid
 */
extern FA_ReturnCode_t FA_FindLastFragmentLinear(
    const FragmentArea_t* const area,
    Fragment_t* fragment,
    size_t* index);

#ifdef __cplusplus
} /* extern C */
#endif

/* EoF fragmentstore.h */

#endif /* FRAGMENTSTORE_H_ */
