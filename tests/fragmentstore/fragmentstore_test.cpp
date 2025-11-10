// MIT License
// 
// Copyright (c) 2025 Mikael Penttinen
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
// -----------------------------------------------------------------------------
//
// fragmentstore_test.cpp
//
// {Short description of the source file}
//

// -----------------------------------------------------------------------------
// INCLUDE DIRECTIVES
// -----------------------------------------------------------------------------

#define CATCH_CONFIG_MAIN
#include <catch2/catch_all.hpp>

extern "C" {
#include "fragmentstore/fragmentstore.h"
#include "imitation_flash.h"
#include "ed25519.h"
}

// -----------------------------------------------------------------------------
// MACRO DEFINITIONS
// -----------------------------------------------------------------------------

#define KB (1024)
#define MB (1024 * KB)
#define SECTOR_SIZE (4 * KB)

#define TEST_FIRMWARE_ID (0xA5A50102U)
#define TEST_FIRMWARE_TYPE (0xC0FFEEU)
#define TEST_FIRMWARE_VERSION (0x00000100U)
#define TEST_FIRMWARE_NAME "unittest_firmware_image_ver_1.0"

#define member_size(type, member) (sizeof( ((type *)0)->member ))

// -----------------------------------------------------------------------------
// VARIABLE DEFINITIONS
// -----------------------------------------------------------------------------

uint8_t PUBLIC_KEY[32];
uint8_t PRIVATE_KEY[64];

typedef struct {
    Metadata_t  metadata;
    uint8_t     binary[1 * MB];
} TestFirmware_t;

TestFirmware_t TEST_FIRMWARE_IMAGE;
uint8_t TEST_FLASH_MEMORY[16 * MB];     // 16 MB

// -----------------------------------------------------------------------------
// PRIVATE FUNCTION DEFINITIONS
// -----------------------------------------------------------------------------

static bool TestValidateFragment(const Fragment_t* frag)
{
    const uint8_t* signature = &frag->signature[0];
    const uint8_t* data = (const uint8_t*)(frag);
    const size_t dataSize = sizeof(Fragment_t) - sizeof(frag->signature);

    if (ed25519_verify(signature, data, dataSize, PUBLIC_KEY) == 1)
    {
        return true;
    }

    return false;
}

static bool TestValidateMetadata(const Metadata_t* metadata)
{
    const uint8_t* signature = &metadata->metadataSignature[0];
    const uint8_t* data = (const uint8_t*)(metadata);
    const size_t dataSize = sizeof(Metadata_t) - sizeof(metadata->metadataSignature);

    if (ed25519_verify(signature, data, dataSize, PUBLIC_KEY) == 1)
    {
        return true;
    }

    return false;
}

// -----------------------------------------------------------------------------
// TEST SUITE DEFINITION
// -----------------------------------------------------------------------------

static size_t NumberOfFragmentsRequired()
{
    const size_t fragmentSize = member_size(Fragment_t, content);
    const size_t binarySize = member_size(TestFirmware_t, binary);
    return (fragmentSize + binarySize - 1U) / fragmentSize;
}

static void FillRandomBytes(uint8_t* buf, size_t len)
{
    for (size_t i = 0; i < len; i++)
    {
        buf[i] = (uint8_t)rand();
    }
}

static void InitTestSuite(FragmentArea_t& area, MemoryConfig_t& memConf)
{
    uint8_t seed[32];
    REQUIRE(ed25519_create_seed(seed) == 0);
    ed25519_create_keypair(PUBLIC_KEY, PRIVATE_KEY, seed);

    // Fill test firmware with random data
    FillRandomBytes(TEST_FIRMWARE_IMAGE.binary, sizeof(TEST_FIRMWARE_IMAGE.binary));

    TEST_FIRMWARE_IMAGE.metadata.type = TEST_FIRMWARE_TYPE;
    TEST_FIRMWARE_IMAGE.metadata.version = TEST_FIRMWARE_VERSION;
    TEST_FIRMWARE_IMAGE.metadata.rollbackNumber = 0U;
    TEST_FIRMWARE_IMAGE.metadata.firmwareId = TEST_FIRMWARE_ID;
    TEST_FIRMWARE_IMAGE.metadata.startAddress = 0U;
    TEST_FIRMWARE_IMAGE.metadata.firmwareSize = sizeof(TEST_FIRMWARE_IMAGE.binary);
    strcpy(TEST_FIRMWARE_IMAGE.metadata.name, TEST_FIRMWARE_NAME);
    ed25519_sign(
        TEST_FIRMWARE_IMAGE.metadata.firmwareSignature,
        TEST_FIRMWARE_IMAGE.binary,
        sizeof(TEST_FIRMWARE_IMAGE.binary),
        PUBLIC_KEY,
        PRIVATE_KEY
    );
    ed25519_sign(
        TEST_FIRMWARE_IMAGE.metadata.metadataSignature,
        (const uint8_t*)&TEST_FIRMWARE_IMAGE.metadata, 
        sizeof(Metadata_t) - sizeof(TEST_FIRMWARE_IMAGE.metadata.metadataSignature), 
        PUBLIC_KEY,
        PRIVATE_KEY
    );

    FLASH_SetMemory(TEST_FLASH_MEMORY, sizeof(TEST_FLASH_MEMORY), SECTOR_SIZE);
    FLASH_Fill(0xFF);

    memConf.baseAddress = 0x0U;
    memConf.sectorSize = SECTOR_SIZE;
    memConf.memorySize = 2 * MB;
    memConf.eraseValue = 0xFFU;
    memConf.Reader = &FLASH_Read;
    memConf.Writer = &FLASH_Write;
    memConf.Eraser = &FLASH_Erase;

    FA_ReturnCode_t res = FA_InitStruct(
        &area,
        &memConf,
        &TestValidateFragment,
        &TestValidateMetadata
    );

    REQUIRE(res == FA_ERR_OK);
}

static void CreateFragment(Fragment_t& frag, size_t offset)
{
    const size_t maxFragmentSize = sizeof(frag.content);
    const size_t dataLeft = (sizeof(TEST_FIRMWARE_IMAGE.binary) - offset);

    const uint32_t fragmentNumber = offset / maxFragmentSize;
    const uint32_t fragmentSize = 
        (dataLeft > maxFragmentSize)
            ? maxFragmentSize
            : dataLeft;
            
    // Clear the struct first to make sure any padding is zero
    memset(&frag, 0, sizeof(Fragment_t));
    frag.firmwareId = TEST_FIRMWARE_ID;
    frag.number = fragmentNumber;
    frag.startAddress = offset;
    frag.size = fragmentSize;
    memcpy(frag.content, &TEST_FIRMWARE_IMAGE.binary[offset], fragmentSize);

    frag.verifyMethod = 0U;
    const uint8_t* data = (const uint8_t*)&frag;
    const size_t dataSize = sizeof(Fragment_t) - sizeof(frag.signature);
    ed25519_sign(frag.signature, data, dataSize, PUBLIC_KEY, PRIVATE_KEY);
}

static void CopyFragmentToFirmware(const Fragment_t& fragment, TestFirmware_t& firmware)
{
    const size_t offset = fragment.startAddress;
    const size_t size = fragment.size;
    
    REQUIRE((offset + size) <= member_size(TestFirmware_t, binary));

    memcpy(&firmware.binary[offset], fragment.content, size);
}

// -----------------------------------------------------------------------------
// TEST CASE DEFINITIONS
// -----------------------------------------------------------------------------

TEST_CASE("Empty flash")
{
    MemoryConfig_t memoryConfig;
    FragmentArea_t testArea;

    InitTestSuite(testArea, memoryConfig);

    Metadata_t metadata;
    Fragment_t fragment;

    REQUIRE(FA_ReadMetadata(&testArea, &metadata) == FA_ERR_EMPTY);
    for (size_t i = 0; i < FA_GetMaxFragmentIndex(&testArea); i++)
    {
        REQUIRE(FA_ReadFragment(&testArea, i, &fragment) == FA_ERR_EMPTY);
    }

    size_t lastFragmentIndex = 0U;
    FA_ReturnCode_t res = FA_FindLastFragment(&testArea, &fragment, &lastFragmentIndex);
    REQUIRE(res == FA_ERR_EMPTY);
    res = FA_FindLastFragmentLinear(&testArea, &fragment, &lastFragmentIndex);
    REQUIRE(res == FA_ERR_EMPTY);

    WHEN("One fragment is written")
    {
        CreateFragment(fragment, 0U);
        REQUIRE(FA_WriteFragment(&testArea, 0U, &fragment) == FA_ERR_OK);

        lastFragmentIndex = SIZE_MAX;
        WHEN("Binary search")
        {
            res = FA_FindLastFragment(&testArea, &fragment, &lastFragmentIndex);
        }
        WHEN("Linear search")
        {
            res = FA_FindLastFragmentLinear(&testArea, &fragment, &lastFragmentIndex);
        }
        REQUIRE(res == FA_ERR_OK);
        REQUIRE(lastFragmentIndex == 0U);
    }
}

TEST_CASE("Write-read test firmware")
{
    MemoryConfig_t memoryConfig;
    FragmentArea_t testArea;

    InitTestSuite(testArea, memoryConfig);

    REQUIRE(FA_EraseArea(&testArea) == FA_ERR_OK);
    REQUIRE(FA_WriteMetadata(&testArea, &TEST_FIRMWARE_IMAGE.metadata) == FA_ERR_OK);
    
    REQUIRE(FA_GetMaxFragmentIndex(&testArea) >= NumberOfFragmentsRequired());

    for (size_t off = 0, i = 0; off < member_size(TestFirmware_t, binary); off += member_size(Fragment_t, content), i++)
    {
        Fragment_t frag;
        CreateFragment(frag, off);
        REQUIRE(FA_WriteFragment(&testArea, i, &frag) == FA_ERR_OK);
    }

    size_t lastFragmentIndex = 0;
    Fragment_t readFrag;
    
    WHEN("Binary search")
    {
        REQUIRE(FA_FindLastFragment(&testArea, &readFrag, &lastFragmentIndex) == FA_ERR_OK);
    }
    WHEN("Linear search")
    {
        REQUIRE(FA_FindLastFragmentLinear(&testArea, &readFrag, &lastFragmentIndex) == FA_ERR_OK);
    }

    REQUIRE(lastFragmentIndex == (NumberOfFragmentsRequired() - 1U));

    TestFirmware_t readFirmware;

    REQUIRE(FA_ReadMetadata(&testArea, &readFirmware.metadata) == FA_ERR_OK);
    REQUIRE(memcmp(&readFirmware.metadata, &TEST_FIRMWARE_IMAGE.metadata, sizeof(Metadata_t)) == 0);

    for (size_t i = 0U; i < NumberOfFragmentsRequired(); i++)
    {
        REQUIRE(FA_ReadFragment(&testArea, i, &readFrag) == FA_ERR_OK);
        CopyFragmentToFirmware(readFrag, readFirmware);
    }

    REQUIRE(memcmp(&readFirmware, &TEST_FIRMWARE_IMAGE, sizeof(TestFirmware_t)) == 0);
}

TEST_CASE("Invalid data cannot be written")
{
    MemoryConfig_t memoryConfig;
    FragmentArea_t testArea;

    InitTestSuite(testArea, memoryConfig);

    WHEN("Writing invalid metadata")
    {
        TEST_FIRMWARE_IMAGE.metadata.rollbackNumber = 1U; // Change content without changing signature
        REQUIRE(FA_WriteMetadata(&testArea, &TEST_FIRMWARE_IMAGE.metadata) != FA_ERR_OK);
    }
    WHEN("Writing invalid fragment")
    {
        Fragment_t frag;
        CreateFragment(frag, 0U);
        frag.content[45] = ~frag.content[45];
        REQUIRE(FA_WriteFragment(&testArea, 0U, &frag) != FA_ERR_OK);
    }

    // Ensure memory was not written
    for (size_t i = 0; i < (sizeof(TEST_FLASH_MEMORY) / sizeof(uint64_t)); i++)
    {
        const uint64_t* buf = (const uint64_t*)(TEST_FLASH_MEMORY);
        if (buf[i] != UINT64_MAX)
        {
            REQUIRE_FALSE(true);
        }
    }
}

TEST_CASE("Invalid data in flash")
{
    MemoryConfig_t memoryConfig;
    FragmentArea_t testArea;

    InitTestSuite(testArea, memoryConfig);

    REQUIRE(FA_WriteMetadata(&testArea, &TEST_FIRMWARE_IMAGE.metadata) == FA_ERR_OK);
    Fragment_t frag;
    CreateFragment(frag, 0U);
    REQUIRE(FA_WriteFragment(&testArea, 0U, &frag) == FA_ERR_OK);

    WHEN("Metadata is corrupted")
    {
        const size_t injectionAddress = sizeof(Metadata_t) / 2U;
        TEST_FLASH_MEMORY[injectionAddress] = ~TEST_FLASH_MEMORY[injectionAddress];
        Metadata_t metadata;
        REQUIRE(FA_ReadMetadata(&testArea, &metadata) == FA_ERR_INVALID);
    }
    WHEN("Fragment is corrupted")
    {
        const size_t injectionAddress = (testArea.metadataSectors * SECTOR_SIZE) + (sizeof(Fragment_t) / 2U);
        TEST_FLASH_MEMORY[injectionAddress] = ~TEST_FLASH_MEMORY[injectionAddress];
        Fragment_t fragment;
        REQUIRE(FA_ReadFragment(&testArea, 0U, &fragment) == FA_ERR_INVALID);
    }
}

// EoF fragmentstore_test.cpp
