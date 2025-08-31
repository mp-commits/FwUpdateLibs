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
 * default_types.h
 *
 * @brief {Short description of the source file}
*/

#ifndef DEFAULT_TYPES_H_
#define DEFAULT_TYPES_H_

#ifdef __cplusplus
extern "C" {
#endif

/*----------------------------------------------------------------------------*/
/* INCLUDE DIRECTIVES                                                         */
/*----------------------------------------------------------------------------*/

#include <stdint.h>

/*----------------------------------------------------------------------------*/
/* PUBLIC TYPE DEFINITIONS                                                    */
/*----------------------------------------------------------------------------*/

#ifndef METADATA_TYPE_DEFINED
#define METADATA_TYPE_DEFINED
struct __attribute__((packed)) Metadata_s
{
    char        magic[16];              /* Magic metadata identifier */
    uint32_t    type;                   /* Firmware type */
    uint32_t    version;                /* Firmware version */
    uint32_t    rollbackNumber;         /* Anti rollback number */
    uint32_t    firmwareId;             /* Unique ID for this firmware */
    uint32_t    startAddress;           /* Jump address of the firmware */
    uint32_t    firmwareSize;           /* Bytes following startAddress */
    char        name[32];               /* Firmware name string */
    uint8_t     firmwareSignature[64];  /* Firmware data signature */
    uint8_t     metadataSignature[64];  /* Metadata signature */
};
#endif

#ifndef FRAGMENT_TYPE_DEFINED
#define FRAGMENT_TYPE_DEFINED
struct __attribute__((packed)) Fragment_s
{
    uint32_t    firmwareId;     /* Unique ID for this firmware (same in metadata) */
    uint32_t    number;         /* Fragment number */
    uint32_t    startAddress;   /* Fragment start address */
    uint32_t    size;           /* Fragment size */
    uint8_t     content[4016];  /* Fragment data */
    uint8_t     signature[64];  /* Fragment signature */
};
#endif

#ifndef ADDRESS_TYPE_DEFINED
#define ADDRESS_TYPE_DEFINED
typedef uint32_t AddressIntType;
#endif

#ifdef __cplusplus
} /* extern C */
#endif

/* EoF default_types.h */

#endif /* DEFAULT_TYPES_H_ */
