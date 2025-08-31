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
// ed25519_tests.cpp
//
// {Short description of the source file}
//

// -----------------------------------------------------------------------------
// INCLUDE DIRECTIVES
// -----------------------------------------------------------------------------

#define CATCH_CONFIG_MAIN
#include <catch2/catch_all.hpp>

extern "C" {
#include "ed25519.h"
#include "ed25519_extra.h"
}

// -----------------------------------------------------------------------------
// TEST SUITE DEFINITION
// -----------------------------------------------------------------------------

uint8_t PUBLIC_KEY[32];
uint8_t PRIVATE_KEY[64];

static void InitTestSuite()
{
    uint8_t seed[32];
    REQUIRE(ed25519_create_seed(seed) == 0);
    ed25519_create_keypair(PUBLIC_KEY, PRIVATE_KEY, seed);
}

static void FillRandomBytes(uint8_t* buf, size_t len)
{
    for (size_t i = 0; i < len; i++)
    {
        buf[i] = (uint8_t)rand();
    }
}

// -----------------------------------------------------------------------------
// TEST CASE DEFINITIONS
// -----------------------------------------------------------------------------

TEST_CASE("ed25519")
{
    InitTestSuite();

    uint8_t buf[4096];
    FillRandomBytes(buf, sizeof(buf));

    const uint8_t* msg = buf;
    const size_t msgLen = sizeof(buf);

    uint8_t signature[64];
    ed25519_sign(signature, msg, msgLen, PUBLIC_KEY, PRIVATE_KEY);

    SECTION("Normal ed25519 verification")
    {
        REQUIRE(1 == ed25519_verify(signature, msg, msgLen, PUBLIC_KEY));
    }
    SECTION("Multi part ed25519 verification")
    {
        ed25519_multipart_t ctx;
        REQUIRE(1 == ed25519_multipart_init(&ctx, signature, PUBLIC_KEY));

        WHEN("In single chunk")
        {
            REQUIRE(1 == ed25519_multipart_continue(&ctx, msg, msgLen));
        }
        WHEN("In multiple chunks")
        {
            for (size_t i = 0; i < msgLen; i += 128U)
            {
                REQUIRE(1 == ed25519_multipart_continue(&ctx, &msg[i], 128U));
            }
        }
        WHEN("In 1 byte chuncks")
        {
            for (size_t i = 0; i < msgLen; i++)
            {
                REQUIRE(1 == ed25519_multipart_continue(&ctx, &msg[i], 1U));
            }
        }

        REQUIRE(1 == ed25519_multipart_end(&ctx));
    }
}

// EoF ed25519_tests.cpp
