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
// command_test.cpp
//
// {Short description of the source file}
//

// -----------------------------------------------------------------------------
// INCLUDE DIRECTIVES
// -----------------------------------------------------------------------------

#define CATCH_CONFIG_MAIN
#include <catch2/catch_all.hpp>

extern "C" {
#include "imitation_flash.h"
#include "fragmentstore/command.h"
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

typedef struct {
    Metadata_t  metadata;
    uint8_t     binary[1 * MB];
} TestFirmware_t;

TestFirmware_t TEST_FIRMWARE_IMAGE;
uint8_t TEST_FLASH_MEMORY[16 * MB];     // 16 MB

// -----------------------------------------------------------------------------
// PRIVATE FUNCTION DEFINITIONS
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// MOCK FUNCTION DEFINITIONS
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// TEST SUITE DEFINITION
// -----------------------------------------------------------------------------

template <typename T>
T MakeRandom()
{
    T alloc;
    uint8_t* mem = (uint8_t*)(&alloc);
    for (size_t i = 0; i < sizeof(T); i++)
    {
        mem[i] = (uint8_t)rand();
    }
    return alloc;
}

template <typename T>
T MakeEmpty()
{
    T alloc;
    memset(&alloc, 0, sizeof(T));
    return alloc;
}

static bool MetadataEqual(const Metadata_t* a, const Metadata_t* b)
{
    return 0 == memcmp(a, b, sizeof(Metadata_t));
}

static uint32_t Crc32(const uint8_t* msg, size_t len)
{
    uint32_t r = ~0; const uint8_t *end = msg + len;
 
  while(msg < end)
  {
    r ^= *msg++;
 
    for(int i = 0; i < 8; i++)
    {
      uint32_t t = ~((r&1) - 1); r = (r>>1) ^ (0xEDB88320 & t);
    }
  }

  return ~r;
}

static void InitTestSuite(CommandArea_t& ca, MemoryConfig_t& memConf)
{
    FLASH_SetMemory(TEST_FLASH_MEMORY, sizeof(TEST_FLASH_MEMORY), SECTOR_SIZE);
    FLASH_Fill(0xFF);

    memConf.baseAddress = 0x0U;
    memConf.sectorSize = SECTOR_SIZE;
    memConf.memorySize = 12 * KB;
    memConf.eraseValue = 0xFFU;
    memConf.Reader = &FLASH_Read;
    memConf.Writer = &FLASH_Write;
    memConf.Eraser = &FLASH_Erase;

    REQUIRE(CA_InitStruct(&ca, &memConf, &Crc32));
}

static bool TestFlashEmpty()
{
    for (size_t i = 0; i < sizeof(TEST_FLASH_MEMORY); i++)
    {
        if (0xFF != TEST_FLASH_MEMORY[i])
        {
            return false;
        }
    }

    return true;
}

// -----------------------------------------------------------------------------
// TEST CASE DEFINITIONS
// -----------------------------------------------------------------------------

TEST_CASE("Structure memory configuration")
{
    CommandArea_t ca;
    MemoryConfig_t memConf;

    InitTestSuite(ca, memConf);

    const auto IsAligned = []
    (Address_t addr) -> bool
    {
        return (addr % SECTOR_SIZE) == 0U;
    };

    REQUIRE(IsAligned(ca.commandAddress));
    REQUIRE(IsAligned(ca.historyAddress));
    REQUIRE(IsAligned(ca.stateAddress));
    REQUIRE(ca.commandAddress != ca.historyAddress);
    REQUIRE(ca.historyAddress != ca.stateAddress);
}

TEST_CASE("Write-Read Install command")
{
    CommandArea_t ca;
    MemoryConfig_t memConf;

    CommandType_t readCommand = COMMAND_TYPE_ERROR;
    Metadata_t readMetadata = MakeEmpty<Metadata_t>();
    Metadata_t testMetadata = MakeRandom<Metadata_t>();

    InitTestSuite(ca, memConf);

    REQUIRE(CA_WriteInstallCommand(&ca, COMMAND_TYPE_INSTALL_FIRMWARE, &testMetadata));
    REQUIRE(CA_ReadInstallCommand(&ca, &readCommand, &readMetadata));

    REQUIRE(readCommand == COMMAND_TYPE_INSTALL_FIRMWARE);
    REQUIRE(MetadataEqual(&testMetadata, &readMetadata));
}

TEST_CASE("Write-Read history metadata")
{
    CommandArea_t ca;
    MemoryConfig_t memConf;

    InitTestSuite(ca, memConf);

    Metadata_t readMetadata = MakeEmpty<Metadata_t>();
    Metadata_t testMetadata = MakeRandom<Metadata_t>();

    REQUIRE(CA_WriteHistory(&ca, &testMetadata));
    REQUIRE(CA_ReadHistory(&ca, &readMetadata));
}

TEST_CASE("Write-Read install steps")
{
    CommandArea_t ca;
    MemoryConfig_t memConf;

    InitTestSuite(ca, memConf);

    REQUIRE(CA_GetStatus(&ca) == COMMAND_STATE_NONE);

    REQUIRE(CA_SetStatus(&ca, COMMAND_STATE_HISTORY_WRITTEN));
    REQUIRE(CA_GetStatus(&ca) == COMMAND_STATE_HISTORY_WRITTEN);

    REQUIRE(CA_SetStatus(&ca, COMMAND_STATE_FIRMWARE_WRITTEN));
    REQUIRE(CA_GetStatus(&ca) == COMMAND_STATE_FIRMWARE_WRITTEN);

    REQUIRE(CA_SetStatus(&ca, COMMAND_STATE_FAILED));
    REQUIRE(CA_GetStatus(&ca) == COMMAND_STATE_FAILED);
}

TEST_CASE("Installation procedure")
{
    CommandArea_t ca;
    MemoryConfig_t memConf;

    InitTestSuite(ca, memConf);

    const Metadata_t newFw = MakeRandom<Metadata_t>();
    const Metadata_t oldFw = MakeRandom<Metadata_t>();
    Metadata_t readFw = MakeEmpty<Metadata_t>();

    // Initialize known system state (write initial history)
    

    WHEN("Installation command is set")
    {
        REQUIRE(CA_WriteInstallCommand(&ca, COMMAND_TYPE_INSTALL_FIRMWARE, &newFw));
        REQUIRE(CA_GetStatus(&ca) == COMMAND_STATE_NONE);

        THEN("Old firmware is replaced with new one")
        {
            // Read command
            CommandType_t cmd = COMMAND_TYPE_NONE;
            REQUIRE(CA_ReadInstallCommand(&ca, &cmd, &readFw));
            REQUIRE(MetadataEqual(&readFw, &newFw));
            REQUIRE(cmd == COMMAND_TYPE_INSTALL_FIRMWARE);
            REQUIRE(CA_GetStatus(&ca) == COMMAND_STATE_NONE);

            // Write history
            REQUIRE(CA_WriteHistory(&ca, &oldFw));
            REQUIRE(CA_SetStatus(&ca, COMMAND_STATE_HISTORY_WRITTEN));
            REQUIRE(CA_GetStatus(&ca) == COMMAND_STATE_HISTORY_WRITTEN);

            WHEN("Installation is restarted")
            {
                REQUIRE(CA_ReadInstallCommand(&ca, &cmd, &readFw));
                REQUIRE(MetadataEqual(&readFw, &newFw));
                REQUIRE(cmd == COMMAND_TYPE_INSTALL_FIRMWARE);
                REQUIRE(CA_GetStatus(&ca) == COMMAND_STATE_HISTORY_WRITTEN);
            }

            // New firmware is installed
            REQUIRE(CA_SetStatus(&ca, COMMAND_STATE_FIRMWARE_WRITTEN));
            REQUIRE(CA_GetStatus(&ca) == COMMAND_STATE_FIRMWARE_WRITTEN);

            WHEN("Installation is restarted")
            {
                REQUIRE(CA_ReadInstallCommand(&ca, &cmd, &readFw));
                REQUIRE(MetadataEqual(&readFw, &newFw));
                REQUIRE(cmd == COMMAND_TYPE_INSTALL_FIRMWARE);
                REQUIRE(CA_GetStatus(&ca) == COMMAND_STATE_FIRMWARE_WRITTEN);
            }

            // Install command is erased
            REQUIRE(CA_EraseInstallCommand(&ca));
            REQUIRE_FALSE(CA_ReadInstallCommand(&ca, &cmd, &readFw));

            // Old firmware can be found from memory
            REQUIRE(CA_ReadHistory(&ca, &readFw));
            REQUIRE(MetadataEqual(&readFw, &oldFw));
        }
    }
}

TEST_CASE("Write-read rollback command")
{
    CommandArea_t ca;
    MemoryConfig_t memConf;

    InitTestSuite(ca, memConf);

    const Metadata_t random = MakeRandom<Metadata_t>();
    const Metadata_t empty = MakeEmpty<Metadata_t>();
    const Metadata_t* expected = nullptr;
    Metadata_t read = MakeRandom<Metadata_t>();

    REQUIRE(!MetadataEqual(&random, &read));

    SECTION("Specific rollback command")
    {
        REQUIRE(CA_WriteInstallCommand(&ca, COMMAND_TYPE_ROLLBACK, &random));
        expected = &random;
    }
    SECTION("Empty rollback command")
    {
        REQUIRE(CA_WriteInstallCommand(&ca, COMMAND_TYPE_ROLLBACK, nullptr));
        expected = &empty;
    }

    CommandType_t cmd = COMMAND_TYPE_ERROR;
    REQUIRE(CA_ReadInstallCommand(&ca, &cmd, &read));
    REQUIRE(cmd == COMMAND_TYPE_ROLLBACK);
    REQUIRE(MetadataEqual(&read, expected));
}

TEST_CASE("User status words")
{
    CommandArea_t ca;
    MemoryConfig_t memConf;

    InitTestSuite(ca, memConf);

    SECTION("Invalid status words")
    {
        REQUIRE_FALSE(CA_SetUserStatus(&ca, 0xA1A1A1A1));
        REQUIRE_FALSE(CA_SetUserStatus(&ca, 0xB2B2B2B2));
        REQUIRE_FALSE(CA_SetUserStatus(&ca, 0xEEEEEEEE));
        REQUIRE(TestFlashEmpty);
    }
    SECTION("Valid status words")
    {
        uint32_t userStatusWord;

        WHEN("0x01010101")
        {
            userStatusWord = 0x01010101;
        }
        WHEN("0xDEADBEED")
        {
            userStatusWord = 0xDEADBEED;
        }
        WHEN("0xABBA")
        {
            userStatusWord = 0xABBA;
        }

        REQUIRE(CA_SetUserStatus(&ca, userStatusWord));
        REQUIRE(CA_GetUserStatus(&ca, userStatusWord));
    }
    SECTION("Multiple user status words")
    {
        REQUIRE(CA_SetUserStatus(&ca, 0x01010101));
        REQUIRE(CA_GetUserStatus(&ca, 0x01010101));

        REQUIRE(CA_SetUserStatus(&ca, 0x02020202));

        REQUIRE(CA_GetUserStatus(&ca, 0x01010101));
        REQUIRE(CA_GetUserStatus(&ca, 0x02020202));

        REQUIRE(CA_SetUserStatus(&ca, 0x03030303));
        
        REQUIRE(CA_GetUserStatus(&ca, 0x01010101));
        REQUIRE(CA_GetUserStatus(&ca, 0x02020202));
        REQUIRE(CA_GetUserStatus(&ca, 0x03030303));
    }
}

// EoF command_test.cpp
