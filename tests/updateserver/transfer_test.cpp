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
// transfer_test.cpp
//
// {Short description of the source file}
//

// -----------------------------------------------------------------------------
// INCLUDE DIRECTIVES
// -----------------------------------------------------------------------------

#define CATCH_CONFIG_MAIN
#include <catch2/catch_all.hpp>
#include <cstring>

extern "C" {
#include "updateserver/transfer.h"

// Mock this
#include "updateserver/server.h"
}

// -----------------------------------------------------------------------------
// MACRO DEFINITIONS
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// VARIABLE DEFINITIONS
// -----------------------------------------------------------------------------

static size_t test_ProcessReqCallCount;
static uint8_t test_buffer[1024];
static uint8_t test_packet[32];
static UpdateServer_t test_server;

// -----------------------------------------------------------------------------
// PRIVATE FUNCTION DEFINITIONS
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// MOCK FUNCTION DEFINITIONS
// -----------------------------------------------------------------------------

size_t US_ProcessRequest(
    const UpdateServer_t* server,
    const uint8_t* request,
    size_t requestLength,
    uint8_t* response,
    size_t)
{
    uint8_t mirror[requestLength];

    memcpy(mirror, request, requestLength);

    for (size_t i = 0; i < requestLength; i++)
    {
        mirror[i] = ~mirror[i];
    }

    memcpy(response, mirror, requestLength);

    test_ProcessReqCallCount++;
    return requestLength;
}

// -----------------------------------------------------------------------------
// TEST SUITE DEFINITION
// -----------------------------------------------------------------------------

static void InitTestSuite(TransferBuffer_t* tb = nullptr)
{
    test_ProcessReqCallCount = 0;
    memset(test_buffer, 0, sizeof(test_buffer));
    memset(test_packet, 0, sizeof(test_packet));

    if (tb)
    {
        REQUIRE(TRANSFER_Init(tb, &test_server, test_buffer, sizeof(test_buffer)));
    }
}

static bool ExpectResponse(uint8_t code)
{
    return (test_packet[0] == 0) && (test_packet[1] == 0) && (test_packet[2] == code);
}

// -----------------------------------------------------------------------------
// TEST CASE DEFINITIONS
// -----------------------------------------------------------------------------

TEST_CASE("Init structure")
{
    InitTestSuite();

    TransferBuffer_t tb;

    REQUIRE_FALSE(TRANSFER_Init(nullptr, &test_server, test_buffer, sizeof(test_buffer)));
    REQUIRE_FALSE(TRANSFER_Init(&tb, nullptr, test_buffer, sizeof(test_buffer)));
    REQUIRE_FALSE(TRANSFER_Init(&tb, &test_server, nullptr, sizeof(test_buffer)));
    REQUIRE_FALSE(TRANSFER_Init(&tb, &test_server, test_buffer, 0U));
}

TEST_CASE("Single packet transfer")
{
    TransferBuffer_t tb;
    InitTestSuite(&tb);

    test_packet[0] = TRANSFER_SINGLE_PACKET;
    test_packet[1] = 0x1;
    test_packet[2] = 0x2;
    test_packet[3] = 0x3;
    test_packet[4] = 0x4;
    test_packet[5] = 0x5;

    WHEN("Invalid packet")
    {
        const size_t resSize = TRANSFER_Process(&tb, test_packet, 1U, sizeof(test_packet));
        REQUIRE(resSize == 3U);
        REQUIRE(ExpectResponse(PROTOCOL_NACK_INVALID_REQUEST));
    }
    WHEN("Valid packet")
    {
        const size_t resSize = TRANSFER_Process(&tb, test_packet, 6U, sizeof(test_packet));
        REQUIRE(resSize == 6U);
        REQUIRE(test_ProcessReqCallCount == 1U);
    
        REQUIRE(test_packet[0] == TRANSFER_SINGLE_PACKET);
        REQUIRE(test_packet[1] == (uint8_t)(~0x1));
        REQUIRE(test_packet[2] == (uint8_t)(~0x2));
        REQUIRE(test_packet[3] == (uint8_t)(~0x3));
        REQUIRE(test_packet[4] == (uint8_t)(~0x4));
        REQUIRE(test_packet[5] == (uint8_t)(~0x5));
    }
}

TEST_CASE("Multi packet transfer")
{
    TransferBuffer_t tb;
    InitTestSuite(&tb);

    SECTION("Incorrect order")
    {
        WHEN("Calling transfer data before transfer init")
        {
            test_packet[0] = TRANSFER_MULTI_PACKET_TRANSFER;
            test_packet[1] = 0xdd;
            const size_t resSize = TRANSFER_Process(&tb, test_packet, 2U, sizeof(test_packet));
            REQUIRE(resSize == 3U);
            REQUIRE(ExpectResponse(PROTOCOL_NACK_REQUEST_FAILED));
        }
        WHEN("Calling transfer end before transfer init")
        {
            test_packet[0] = TRANSFER_MULTI_PACKET_END;
            const size_t resSize = TRANSFER_Process(&tb, test_packet, 1U, sizeof(test_packet));
            REQUIRE(resSize == 3U);
            REQUIRE(ExpectResponse(PROTOCOL_NACK_REQUEST_FAILED));
        }
    }
    SECTION("Invalid calls")
    {
        SECTION("Invalid start request")
        {
            test_packet[0] = TRANSFER_MULTI_PACKET_INIT;
            test_packet[1] = 0x00; // 0 bytes of data
            test_packet[2] = 0x00; // 0 bytes of data
            test_packet[3] = 0x00; // 0 bytes of data
            test_packet[4] = 0x00; // 0 bytes of data

            WHEN("Too short request")
            {
                const size_t resSize = TRANSFER_Process(&tb, test_packet, 3U, sizeof(test_packet));
                REQUIRE(resSize == 3U);
                REQUIRE(ExpectResponse(PROTOCOL_NACK_INVALID_REQUEST));
            }

            WHEN("Too long request")
            {
                const size_t resSize = TRANSFER_Process(&tb, test_packet, 6U, sizeof(test_packet));
                REQUIRE(resSize == 3U);
                REQUIRE(ExpectResponse(PROTOCOL_NACK_INVALID_REQUEST));
            }

            WHEN("Transfer length parameter 0")
            {
                const size_t resSize = TRANSFER_Process(&tb, test_packet, 5U, sizeof(test_packet));
                REQUIRE(resSize == 3U);
                REQUIRE(ExpectResponse(PROTOCOL_NACK_REQUEST_OUT_OF_RANGE));
            }
        }
        SECTION("Invalid transfer request")
        {
            WHEN("Transfer is initialized")
            {
                test_packet[0] = TRANSFER_MULTI_PACKET_INIT;
                test_packet[1] = 0x00; // 4 bytes of data
                test_packet[2] = 0x00; // 4 bytes of data
                test_packet[3] = 0x00; // 4 bytes of data
                test_packet[4] = 0x04; // 4 bytes of data
    
                const size_t resSize = TRANSFER_Process(&tb, test_packet, 5U, sizeof(test_packet));
                REQUIRE(resSize == 3U);
                REQUIRE(ExpectResponse(PROTOCOL_ACK_OK));

                THEN("Following transfer requests are bad")
                {
                    test_packet[0] = TRANSFER_MULTI_PACKET_TRANSFER;
                    test_packet[1] = 0x11; // 1st byte of data
                    test_packet[2] = 0x22; // 2nd byte of data
                    test_packet[3] = 0x33; // 3rd byte of data
                    test_packet[4] = 0x44; // 4th byte of data

                    WHEN("Transfer request is empty")
                    {
                        const size_t resSize = TRANSFER_Process(&tb, test_packet, 1U, sizeof(test_packet));
                        REQUIRE(resSize == 3U);
                        REQUIRE(ExpectResponse(PROTOCOL_NACK_INVALID_REQUEST));
                    }
                    WHEN("Transfer request contains more data than initialized")
                    {
                        const size_t resSize = TRANSFER_Process(&tb, test_packet, 6U, sizeof(test_packet));
                        REQUIRE(resSize == 3U);
                        REQUIRE(ExpectResponse(PROTOCOL_NACK_REQUEST_OUT_OF_RANGE));
                    }
                    WHEN("Transfer request contains more data than fits in response buffer")
                    {
                        const size_t resSize = TRANSFER_Process(&tb, test_packet, 6U, sizeof(test_packet));
                        REQUIRE(resSize == 3U);
                        REQUIRE(ExpectResponse(PROTOCOL_NACK_REQUEST_OUT_OF_RANGE));
                    }
                }
                THEN("Following transfer end request is bad")
                {
                    test_packet[0] = TRANSFER_MULTI_PACKET_END;
                    WHEN("Packet contains more than control byte")
                    {
                        const size_t resSize = TRANSFER_Process(&tb, test_packet, 2U, sizeof(test_packet));
                        REQUIRE(resSize == 3U);
                        REQUIRE(ExpectResponse(PROTOCOL_NACK_INVALID_REQUEST));
                    }
                    WHEN("End is called before all data has been transferred")
                    {
                        const size_t resSize = TRANSFER_Process(&tb, test_packet, 1U, sizeof(test_packet));
                        REQUIRE(resSize == 3U);
                        REQUIRE(ExpectResponse(PROTOCOL_NACK_REQUEST_OUT_OF_RANGE));
                    }
                }
            }
        }
    }
    SECTION("Correct order")
    {
        test_packet[0] = TRANSFER_MULTI_PACKET_INIT;
        test_packet[1] = 0x00; // 16 bytes of data
        test_packet[2] = 0x00; // 16 bytes of data
        test_packet[3] = 0x00; // 16 bytes of data
        test_packet[4] = 0x10; // 16 bytes of data

        size_t resSize = TRANSFER_Process(&tb, test_packet, 5U, sizeof(test_packet));
        REQUIRE(resSize == 3U);
        REQUIRE(ExpectResponse(PROTOCOL_ACK_OK));
        REQUIRE(tb.transferSize == 16U);

        test_packet[0] = TRANSFER_MULTI_PACKET_TRANSFER;
        test_packet[1] = 0x00; // First 4 bytes of data
        test_packet[2] = 0x11; // First 4 bytes of data
        test_packet[3] = 0x22; // First 4 bytes of data
        test_packet[4] = 0x33; // First 4 bytes of data

        resSize = TRANSFER_Process(&tb, test_packet, 5U, sizeof(test_packet));
        REQUIRE(resSize == 3U);
        REQUIRE(ExpectResponse(PROTOCOL_ACK_OK));
        REQUIRE(tb.msgSize == 4U);

        test_packet[0] = TRANSFER_MULTI_PACKET_TRANSFER;
        test_packet[1] = 0x44; // Second 4 bytes of data
        test_packet[2] = 0x55; // Second 4 bytes of data
        test_packet[3] = 0x66; // Second 4 bytes of data
        test_packet[4] = 0x77; // Second 4 bytes of data

        resSize = TRANSFER_Process(&tb, test_packet, 5U, sizeof(test_packet));
        REQUIRE(resSize == 3U);
        REQUIRE(ExpectResponse(PROTOCOL_ACK_OK));
        REQUIRE(tb.msgSize == 8U);

        test_packet[0] = TRANSFER_MULTI_PACKET_TRANSFER;
        test_packet[1] = 0x88; // Last 8 bytes of data
        test_packet[2] = 0x99; // Last 8 bytes of data
        test_packet[3] = 0xAA; // Last 8 bytes of data
        test_packet[4] = 0xBB; // Last 8 bytes of data
        test_packet[5] = 0xCC; // Last 8 bytes of data
        test_packet[6] = 0xDD; // Last 8 bytes of data
        test_packet[7] = 0xEE; // Last 8 bytes of data
        test_packet[8] = 0xFF; // Last 8 bytes of data

        resSize = TRANSFER_Process(&tb, test_packet, 9U, sizeof(test_packet));
        REQUIRE(resSize == 3U);
        REQUIRE(ExpectResponse(PROTOCOL_ACK_OK));
        REQUIRE(tb.msgSize == 16U);

        REQUIRE(test_buffer[0] == 0x00);
        REQUIRE(test_buffer[1] == 0x11);
        REQUIRE(test_buffer[2] == 0x22);
        REQUIRE(test_buffer[3] == 0x33);
        REQUIRE(test_buffer[4] == 0x44);
        REQUIRE(test_buffer[5] == 0x55);
        REQUIRE(test_buffer[6] == 0x66);
        REQUIRE(test_buffer[7] == 0x77);
        REQUIRE(test_buffer[8] == 0x88);
        REQUIRE(test_buffer[9] == 0x99);
        REQUIRE(test_buffer[10] == 0xAA);
        REQUIRE(test_buffer[11] == 0xBB);
        REQUIRE(test_buffer[12] == 0xCC);
        REQUIRE(test_buffer[13] == 0xDD);
        REQUIRE(test_buffer[14] == 0xEE);
        REQUIRE(test_buffer[15] == 0xFF);

        test_packet[0] = TRANSFER_MULTI_PACKET_END;
        resSize = TRANSFER_Process(&tb, test_packet, 1U, sizeof(test_packet));
        REQUIRE(resSize == 17U);
        REQUIRE(test_ProcessReqCallCount == 1U);

        // Processed request
        REQUIRE(test_packet[0] == 0x00);
        REQUIRE(test_packet[1] == (uint8_t)~0x00);
        REQUIRE(test_packet[2] == (uint8_t)~0x11);
        REQUIRE(test_packet[3] == (uint8_t)~0x22);
        REQUIRE(test_packet[4] == (uint8_t)~0x33);
        REQUIRE(test_packet[5] == (uint8_t)~0x44);
        REQUIRE(test_packet[6] == (uint8_t)~0x55);
        REQUIRE(test_packet[7] == (uint8_t)~0x66);
        REQUIRE(test_packet[8] == (uint8_t)~0x77);
        REQUIRE(test_packet[9] == (uint8_t)~0x88);
        REQUIRE(test_packet[10] == (uint8_t)~0x99);
        REQUIRE(test_packet[11] == (uint8_t)~0xAA);
        REQUIRE(test_packet[12] == (uint8_t)~0xBB);
        REQUIRE(test_packet[13] == (uint8_t)~0xCC);
        REQUIRE(test_packet[14] == (uint8_t)~0xDD);
        REQUIRE(test_packet[15] == (uint8_t)~0xEE);
        REQUIRE(test_packet[16] == (uint8_t)~0xFF);
    }
}

// EoF transfer_test.cpp
