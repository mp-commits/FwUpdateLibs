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
// updateserver_test.cpp
//
// Unit tests for firmware update server
//

// -----------------------------------------------------------------------------
// INCLUDE DIRECTIVES
// -----------------------------------------------------------------------------

#define CATCH_CONFIG_MAIN
#include <catch2/catch_all.hpp>
#include <vector>

extern "C" {
#include "updateserver/server.h"
#include "updateserver/protocol.h"
}

// -----------------------------------------------------------------------------
// MOCK FUNCTION DEFINITIONS
// -----------------------------------------------------------------------------

static uint8_t TestReadDataById(
    uint8_t id, 
    uint8_t* out, 
    size_t maxSize, 
    size_t* readSize)
{
    switch (id)
    {
    case PROTOCOL_DATA_ID_FIRMWARE_VERSION:
    {
        if (maxSize < 4U)
        {
            return PROTOCOL_NACK_INVALID_REQUEST;
        }
        out[0] = 0x11;
        out[1] = 0x22;
        out[2] = 0x33;
        out[3] = 0x44;
        *readSize = 4U;
        return PROTOCOL_ACK_OK;
    }
    case PROTOCOL_DATA_ID_FIRMWARE_TYPE:
        return PROTOCOL_NACK_BUSY_REPEAT_REQUEST;
    default:
        return PROTOCOL_NACK_REQUEST_OUT_OF_RANGE;
        break;
    }
}

static std::vector<uint8_t> f_writeData;
static uint8_t f_testReturnCode;

static uint8_t TestWriteDataById(
    uint8_t id, 
    const uint8_t* in, 
    size_t size)
{
    if (id == PROTOCOL_DATA_ID_FIRMWARE_UPDATE)
    {
        f_writeData = std::vector<uint8_t>(&in[0], &in[size]);
        return f_testReturnCode;
    }

    return PROTOCOL_NACK_REQUEST_OUT_OF_RANGE;
}

static uint8_t TestPutMetadata(
    const uint8_t* data, 
    size_t size)
{
    f_writeData = std::vector<uint8_t>(&data[0], &data[size]);
    return f_testReturnCode;
}

static uint8_t TestPutFragment(
    const uint8_t* data, 
    size_t size)
{
    f_writeData = std::vector<uint8_t>(&data[0], &data[size]);
    return f_testReturnCode;
}

// -----------------------------------------------------------------------------
// TEST SUITE DEFINITION
// -----------------------------------------------------------------------------

static void InitTestSuite(UpdateServer_t& server)
{
    f_testReturnCode = PROTOCOL_ACK_OK;
    f_writeData.clear();
    REQUIRE(US_InitServer(&server, &TestReadDataById, &TestWriteDataById, &TestPutMetadata, &TestPutFragment));
}

// -----------------------------------------------------------------------------
// TEST CASE DEFINITIONS
// -----------------------------------------------------------------------------

TEST_CASE("Invalid Calls")
{
    UpdateServer_t server;
    InitTestSuite(server);

    size_t len = 0U;
    std::vector<uint8_t> req = {0x00};
    std::vector<uint8_t> res = {0, 0};

    SECTION("Incorrect init")
    {
        REQUIRE_FALSE(US_InitServer(nullptr, &TestReadDataById, &TestWriteDataById, &TestPutMetadata, &TestPutFragment));
        REQUIRE_FALSE(US_InitServer(&server, nullptr, &TestWriteDataById, &TestPutMetadata, &TestPutFragment));
        REQUIRE_FALSE(US_InitServer(&server, &TestReadDataById, nullptr, &TestPutMetadata, &TestPutFragment));
        REQUIRE_FALSE(US_InitServer(&server, &TestReadDataById, &TestWriteDataById, nullptr, &TestPutFragment));
        REQUIRE_FALSE(US_InitServer(&server, &TestReadDataById, &TestWriteDataById, &TestPutMetadata, nullptr));
    }
    SECTION("Null arguments")
    {
        len = US_ProcessRequest(nullptr, nullptr, 0U, nullptr, 0U);
        REQUIRE(len == 0U);

        len = US_ProcessRequest(nullptr, req.data(), 0U, res.data(), 0U);
        REQUIRE(len == 0U);

        len = US_ProcessRequest(&server, nullptr, 0U, nullptr, 0U);
        REQUIRE(len == 0U);

        len = US_ProcessRequest(&server, req.data(), 0U, nullptr, 0U);
        REQUIRE(len == 0U);

        len = US_ProcessRequest(&server, nullptr, 0U, res.data(), 0U);
        REQUIRE(len == 0U);
    }
    SECTION("Incorrect buffers")
    {
        len = US_ProcessRequest(&server, req.data(), 0U, res.data(), 0U);
        REQUIRE(len == 0U);

        len = US_ProcessRequest(&server, req.data(), req.size(), res.data(), 0U);
        REQUIRE(len == 0U);

        len = US_ProcessRequest(&server, req.data(), 0U, res.data(), res.size());
        REQUIRE(len == 0U);
    }
    SECTION("Unknown request")
    {
        len = US_ProcessRequest(&server, req.data(), req.size(), res.data(), res.size());
        REQUIRE(len == 2U);
        REQUIRE(res.at(0) == 0x00);
        REQUIRE(res.at(1) == PROTOCOL_NACK_REQUEST_OUT_OF_RANGE);
    }
}

TEST_CASE("Ping")
{
    UpdateServer_t server;
    InitTestSuite(server);

    size_t len = 0U;
    std::vector<uint8_t> req = {PROTOCOL_SID_PING};
    std::vector<uint8_t> res = {0, 0};

    SECTION("Incorrect request")
    {
        req = {PROTOCOL_SID_PING, 0x20};
        len = US_ProcessRequest(&server, req.data(), req.size(), res.data(), res.size());
        REQUIRE(len == 2);
        REQUIRE(res.at(0) == PROTOCOL_SID_PING);
        REQUIRE(res.at(1) == PROTOCOL_NACK_INVALID_REQUEST);
    }
    SECTION("Correct request")
    {
        len = US_ProcessRequest(&server, req.data(), req.size(), res.data(), res.size());
        REQUIRE(len == 2);
        REQUIRE(res.at(0) == PROTOCOL_SID_PING);
        REQUIRE(res.at(1) == PROTOCOL_ACK_OK);
    }
}

TEST_CASE("Read Data by ID")
{
    UpdateServer_t server;
    InitTestSuite(server);

    size_t len = 0U;
    std::vector<uint8_t> req = {PROTOCOL_SID_READ_DATA_BY_ID, 0x00};
    std::vector<uint8_t> res(128);

    SECTION("Invalid request message")
    {
        WHEN("Request too short")
        {
            req = {PROTOCOL_SID_READ_DATA_BY_ID};
        }
        WHEN("Request too long")
        {
            req = {PROTOCOL_SID_READ_DATA_BY_ID, PROTOCOL_DATA_ID_FIRMWARE_VERSION, 0x00};
        }

        len = US_ProcessRequest(&server, req.data(), req.size(), res.data(), res.size());
        REQUIRE(len == 2);
        REQUIRE(res.at(0) == PROTOCOL_SID_READ_DATA_BY_ID);
        REQUIRE(res.at(1) == PROTOCOL_NACK_INVALID_REQUEST);
    }
    SECTION("Invalid Identifier")
    {
        len = US_ProcessRequest(&server, req.data(), req.size(), res.data(), res.size());
        REQUIRE(len == 2);
        REQUIRE(res.at(0) == PROTOCOL_SID_READ_DATA_BY_ID);
        REQUIRE(res.at(1) == PROTOCOL_NACK_REQUEST_OUT_OF_RANGE);
    }
    SECTION("Read Function Busy")
    {
        req = {PROTOCOL_SID_READ_DATA_BY_ID, PROTOCOL_DATA_ID_FIRMWARE_TYPE};
        len = US_ProcessRequest(&server, req.data(), req.size(), res.data(), res.size());
        REQUIRE(len == 2);
        REQUIRE(res.at(0) == PROTOCOL_SID_READ_DATA_BY_ID);
        REQUIRE(res.at(1) == PROTOCOL_NACK_BUSY_REPEAT_REQUEST);
    }
    SECTION("Read ok")
    {
        req = {PROTOCOL_SID_READ_DATA_BY_ID, PROTOCOL_DATA_ID_FIRMWARE_VERSION};
        len = US_ProcessRequest(&server, req.data(), req.size(), res.data(), res.size());
        REQUIRE(len == 6);
        REQUIRE(res.at(0) == PROTOCOL_SID_READ_DATA_BY_ID);
        REQUIRE(res.at(1) == PROTOCOL_ACK_OK);
        REQUIRE(res.at(2) == 0x11);
        REQUIRE(res.at(3) == 0x22);
        REQUIRE(res.at(4) == 0x33);
        REQUIRE(res.at(5) == 0x44);
    }
}

TEST_CASE("Write Data by ID")
{
    UpdateServer_t server;
    InitTestSuite(server);

    size_t len = 0U;
    std::vector<uint8_t> req;
    std::vector<uint8_t> res(128);

    SECTION("Invalid request")
    {
        WHEN("Request has no identifier")
        {
            req = {PROTOCOL_SID_WRITE_DATA_BY_ID, 0x00};
        }
        WHEN("Request has no data")
        {
            req = {PROTOCOL_SID_WRITE_DATA_BY_ID, 0x00};
        }

        len = US_ProcessRequest(&server, req.data(), req.size(), res.data(), res.size());
        REQUIRE(len == 2);
        REQUIRE(res.at(0) == PROTOCOL_SID_WRITE_DATA_BY_ID);
        REQUIRE(res.at(1) == PROTOCOL_NACK_INVALID_REQUEST);
    }
    SECTION("Invalid Identifier")
    {
        req = {PROTOCOL_SID_WRITE_DATA_BY_ID, 0x00, 0x11};
        len = US_ProcessRequest(&server, req.data(), req.size(), res.data(), res.size());
        REQUIRE(len == 2);
        REQUIRE(res.at(0) == PROTOCOL_SID_WRITE_DATA_BY_ID);
        REQUIRE(res.at(1) == PROTOCOL_NACK_REQUEST_OUT_OF_RANGE);
    }
    SECTION("Valid identifier")
    {
        req = {PROTOCOL_SID_WRITE_DATA_BY_ID, PROTOCOL_DATA_ID_FIRMWARE_UPDATE, 0xAA, 0xBB, 0xCC};
        len = US_ProcessRequest(&server, req.data(), req.size(), res.data(), res.size());
        REQUIRE(len == 2);
        REQUIRE(res.at(0) == PROTOCOL_SID_WRITE_DATA_BY_ID);
        REQUIRE(res.at(1) == PROTOCOL_ACK_OK);

        REQUIRE(f_writeData.size() == 3U);
        REQUIRE(f_writeData.at(0) == 0xAA);
        REQUIRE(f_writeData.at(1) == 0xBB);
        REQUIRE(f_writeData.at(2) == 0xCC);
    }
}

TEST_CASE("Put Metadata")
{
    UpdateServer_t server;
    InitTestSuite(server);

    size_t len = 0U;
    std::vector<uint8_t> req;
    std::vector<uint8_t> res(128);

    SECTION("Invalid request message")
    {
        WHEN("Request has no data")
        {
            req = {PROTOCOL_SID_PUT_METADATA};
        }

        len = US_ProcessRequest(&server, req.data(), req.size(), res.data(), res.size());
        REQUIRE(len == 2);
        REQUIRE(res.at(0) == PROTOCOL_SID_PUT_METADATA);
        REQUIRE(res.at(1) == PROTOCOL_NACK_INVALID_REQUEST);
    }
    SECTION("Valid request message")
    {
        req = {PROTOCOL_SID_PUT_METADATA, 0xAA, 0xBB, 0xCC};

        WHEN("Operation fails")
        {
            f_testReturnCode = PROTOCOL_NACK_REQUEST_FAILED;

            len = US_ProcessRequest(&server, req.data(), req.size(), res.data(), res.size());
            REQUIRE(len == 2);
            REQUIRE(res.at(0) == PROTOCOL_SID_PUT_METADATA);
            REQUIRE(res.at(1) == PROTOCOL_NACK_REQUEST_FAILED);
        }
        WHEN("Operation succeeds")
        {
            len = US_ProcessRequest(&server, req.data(), req.size(), res.data(), res.size());
            REQUIRE(len == 2);
            REQUIRE(res.at(0) == PROTOCOL_SID_PUT_METADATA);
            REQUIRE(res.at(1) == PROTOCOL_ACK_OK);

            REQUIRE(f_writeData.size() == 3U);
            REQUIRE(f_writeData.at(0) == 0xAA);
            REQUIRE(f_writeData.at(1) == 0xBB);
            REQUIRE(f_writeData.at(2) == 0xCC);
        }
    }
}

TEST_CASE("Put Fragment")
{
    UpdateServer_t server;
    InitTestSuite(server);

    size_t len = 0U;
    std::vector<uint8_t> req;
    std::vector<uint8_t> res(128);

    SECTION("Invalid request message")
    {
        WHEN("Request has no data")
        {
            req = {PROTOCOL_SID_PUT_FRAGMENT};
        }

        len = US_ProcessRequest(&server, req.data(), req.size(), res.data(), res.size());
        REQUIRE(len == 2);
        REQUIRE(res.at(0) == PROTOCOL_SID_PUT_FRAGMENT);
        REQUIRE(res.at(1) == PROTOCOL_NACK_INVALID_REQUEST);
    }
    SECTION("Valid request message")
    {
        req = {PROTOCOL_SID_PUT_FRAGMENT, 0xAA, 0xBB, 0xCC};

        WHEN("Operation fails")
        {
            f_testReturnCode = PROTOCOL_NACK_REQUEST_FAILED;

            len = US_ProcessRequest(&server, req.data(), req.size(), res.data(), res.size());
            REQUIRE(len == 2);
            REQUIRE(res.at(0) == PROTOCOL_SID_PUT_FRAGMENT);
            REQUIRE(res.at(1) == PROTOCOL_NACK_REQUEST_FAILED);
        }
        WHEN("Operation succeeds")
        {
            len = US_ProcessRequest(&server, req.data(), req.size(), res.data(), res.size());
            REQUIRE(len == 2);
            REQUIRE(res.at(0) == PROTOCOL_SID_PUT_FRAGMENT);
            REQUIRE(res.at(1) == PROTOCOL_ACK_OK);

            REQUIRE(f_writeData.size() == 3U);
            REQUIRE(f_writeData.at(0) == 0xAA);
            REQUIRE(f_writeData.at(1) == 0xBB);
            REQUIRE(f_writeData.at(2) == 0xCC);
        }
    }
}

// EoF updateserver_test.cpp
