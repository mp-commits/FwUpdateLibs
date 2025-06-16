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
// imitation_flash_test.cpp
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
}

// -----------------------------------------------------------------------------
// PRIVATE FUNCTION DEFINITIONS
// -----------------------------------------------------------------------------

static bool IsAll(uint8_t* src, size_t size, uint8_t val)
{
    for (size_t i = 0; i < size; i++)
    {
        if (src[i] != val)
        {
            return false;
        }
    }

    return true;
}

static void PutStringToBuf(uint8_t* buf, const char* str)
{
    memcpy(buf, str, strlen(str));
}

static bool ContainsString(const uint8_t* buf, const char* str)
{
    return memcmp(buf, str, strlen(str)) == 0;
}

// -----------------------------------------------------------------------------
// MOCK FUNCTION DEFINITIONS
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// TEST SUITE DEFINITION
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// TEST CASE DEFINITIONS
// -----------------------------------------------------------------------------

TEST_CASE("Fill memory")
{
    uint8_t memory[1024];
    memset(memory, 0, sizeof(memory));

    FLASH_SetMemory(&memory[0], sizeof(memory), 128U);

    FLASH_Fill(0xFF);
    REQUIRE(IsAll(memory, sizeof(memory), 0xFF));

    FLASH_Fill(0xAA);
    REQUIRE(IsAll(memory, sizeof(memory), 0xAA));
}

TEST_CASE("Read")
{
    uint8_t memory[1024];
    memset(memory, 0, sizeof(memory));
    FLASH_SetMemory(&memory[0], sizeof(memory), 128U);
    FLASH_Fill(0xFF);

    uint8_t out[1024];
    memset(out, 0, sizeof(out));

    SECTION("Invalid access")
    {
        REQUIRE_FALSE(FLASH_Read(1024, 1, &out[0]));
        REQUIRE_FALSE(FLASH_Read(1023, 2, &out[0]));
        REQUIRE_FALSE(FLASH_Read(0, 1025, &out[0]));
    }
    SECTION("Valid access")
    {
        SECTION("Read small size")
        {
            PutStringToBuf(memory, "testString");
            FLASH_Read(0, 11, &out[0]);
            REQUIRE(ContainsString(out, "testString"));
        }
        SECTION("Read medium size")
        {
            PutStringToBuf(&memory[0x200], "Somewhat longer testing string in flash");
            FLASH_Read(0x200, 40, &out[0]);
            REQUIRE(ContainsString(out, "Somewhat longer testing string in flash"));
        }
        SECTION("Read full size")
        {
            memset(memory, 0xAA, sizeof(memory));
            FLASH_Read(0, sizeof(out), &out[0]);
            REQUIRE(IsAll(out, sizeof(out), 0xAA));
        }
    }
}

TEST_CASE("Write")
{
    uint8_t memory[1024];
    memset(memory, 0, sizeof(memory));
    FLASH_SetMemory(&memory[0], sizeof(memory), 128U);
    FLASH_Fill(0xFF);

    uint8_t in[1024];
    memset(in, 0, sizeof(in));

    SECTION("Invalid access")
    {
        REQUIRE_FALSE(FLASH_Write(1024, 1, &in[0]));
        REQUIRE_FALSE(FLASH_Write(1023, 2, &in[0]));
        REQUIRE_FALSE(FLASH_Write(0, 1025, &in[0]));
    }
    SECTION("Valid access")
    {
        SECTION("Write small size")
        {
            PutStringToBuf(in, "testString");
            FLASH_Write(0, 11, &in[0]);
            REQUIRE(ContainsString(memory, "testString"));
        }
        SECTION("Write medium size")
        {
            PutStringToBuf(&in[0], "Somewhat longer testing string in flash");
            FLASH_Write(0x200, 40, &in[0]);
            REQUIRE(ContainsString(&memory[0x200], "Somewhat longer testing string in flash"));
        }
        SECTION("Write full size")
        {
            memset(in, 0xAA, sizeof(in));
            FLASH_Write(0, sizeof(in), &in[0]);
            REQUIRE(IsAll(memory, sizeof(memory), 0xAA));
        }
    }
    SECTION("NOR characteristics")
    {
        const uint8_t oddBits = 0x55U;
        const uint8_t evenBits = 0xAA;

        REQUIRE(FLASH_Write(0U, 1U, &oddBits));
        REQUIRE(memory[0] == oddBits);
        REQUIRE(FLASH_Write(0U, 1U, &evenBits));
        REQUIRE(memory[0] == 0U);
    }
}

TEST_CASE("Erase")
{
    uint8_t memory[1024];
    memset(memory, 0, sizeof(memory));
    FLASH_SetMemory(&memory[0], sizeof(memory), 128U);
    
    SECTION("Invalid access")
    {
        REQUIRE_FALSE(FLASH_Erase(1U, 1U));
        REQUIRE_FALSE(FLASH_Erase(1U, 128U));
        REQUIRE_FALSE(FLASH_Erase(0U, 129U));
        REQUIRE_FALSE(FLASH_Erase(256U, 513U));
    }
    SECTION("Valid erase")
    {
        SECTION("One sector")
        {
            REQUIRE(FLASH_Erase(0U, 128));
            REQUIRE(IsAll(&memory[0], 128U, 0xFF));
        }
        SECTION("Multiple sectors")
        {
            REQUIRE(FLASH_Erase(128, 512U));
            REQUIRE(IsAll(&memory[128], 512U, 0xFF));
        }
        SECTION("All sectors")
        {
            REQUIRE(FLASH_Erase(0U, sizeof(memory)));
            REQUIRE(IsAll(&memory[0], sizeof(memory), 0xFF));
        }
    }
}

TEST_CASE("Flash lock")
{
    uint8_t memory[1024];
    memset(memory, 0, sizeof(memory));
    FLASH_SetMemory(&memory[0], sizeof(memory), 128U);
    FLASH_Fill(0xBB);
    FLASH_Unlock();

    uint8_t work[32];
    memset(work, 0xAA, sizeof(work));

    SECTION("Lock function")
    {
        REQUIRE(FLASH_Lock());
        REQUIRE_FALSE(FLASH_Lock());

        FLASH_Unlock();

        REQUIRE(FLASH_Lock());
        REQUIRE_FALSE(FLASH_Lock());
    }
    SECTION("Locked during operation")
    {
        REQUIRE(FLASH_Lock());

        WHEN("Writing")
        {
            REQUIRE_FALSE(FLASH_Write(0, 32U, &work[0]));
            REQUIRE(IsAll(memory, sizeof(memory), 0xBB));
        }
        WHEN("Reading")
        {
            REQUIRE_FALSE(FLASH_Read(0, 32U, &work[0]));
            REQUIRE(IsAll(work, 32, 0xAA));
        }
        WHEN("Erasing")
        {
            REQUIRE_FALSE(FLASH_Erase(0U, 128));
            REQUIRE(IsAll(memory, sizeof(memory), 0xBB));
        }
    }
}

// EoF imitation_flash_test.cpp
