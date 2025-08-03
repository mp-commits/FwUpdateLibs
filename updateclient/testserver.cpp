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
 * testserver.cpp
 *
 * @brief Stub test server for function verification and debugging
*/

/*----------------------------------------------------------------------------*/
/* INCLUDE DIRECTIVES                                                         */
/*----------------------------------------------------------------------------*/

#include "crc32.hpp"
#include "udpsocket.hpp"
#include "updateserver/server.h"
#include "updateserver/protocol.h"
#include "updateserver/transfer.h"

#include <sstream>
#include <iomanip>
#include <iostream>
#include <csignal>

/*----------------------------------------------------------------------------*/
/* PRIVATE TYPE DEFINITIONS                                                   */
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/* MACRO DEFINITIONS                                                          */
/*----------------------------------------------------------------------------*/

#define REQUIRE(x) \
if(!(x)) \
{ \
std::cout << #x << " failed!" << std::endl; \
return -1; \
}

/*----------------------------------------------------------------------------*/
/* VARIABLE DEFINITIONS                                                       */
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/* PRIVATE FUNCTION DEFINITIONS                                               */
/*----------------------------------------------------------------------------*/

void SignalHandler( int signum )
{
   exit(2);
}

static uint8_t TEST_ReadDataById(
    uint8_t id, 
    uint8_t* out, 
    size_t maxSize, 
    size_t* readSize)
{
    if (maxSize < 16U)
    {
        return PROTOCOL_NACK_INTERNAL_ERROR;
    }
    switch (id)
    {
    case PROTOCOL_DATA_ID_FIRMWARE_VERSION:
        out[0] = 0;
        out[1] = 0;
        out[2] = 0;
        out[3] = 55;
        *readSize = 4U;
        return PROTOCOL_ACK_OK;
    case PROTOCOL_DATA_ID_FIRMWARE_TYPE:
        out[0] = 0;
        out[1] = 0;
        out[2] = 0;
        out[3] = 1;
        *readSize = 4U;
        return PROTOCOL_ACK_OK;
    case PROTOCOL_DATA_ID_FIRMWARE_NAME:
        strcpy((char*)out, "Testserver tool");
        *readSize = 16U;
        return PROTOCOL_ACK_OK;
    default:
        return PROTOCOL_NACK_REQUEST_OUT_OF_RANGE;
    }
}

static uint8_t TEST_WriteDataById(
    uint8_t id, 
    const uint8_t* in, 
    size_t size)
{
    std::stringstream ss;
    ss << std::hex;
    ss << "Wrote data id " << (uint32_t)(id);
    ss << " content";
    for (size_t i = 0; i < size; i++)
    {
        ss << " " << (uint32_t)(in[i]);
    }

    std::cout << ss.str() << std::endl;

    return PROTOCOL_ACK_OK;
}

static uint8_t TEST_PutMetadata(
    const uint8_t* data, 
    size_t size)
{
    std::stringstream ss;
    ss << std::hex;
    ss << "Wrote metadata " << InlineCrc32(data, size);

    std::cout << ss.str() << std::endl;

    return PROTOCOL_ACK_OK;
}

static uint8_t TEST_PutFragment(
    const uint8_t* data, 
    size_t size)
{
    std::stringstream ss;
    ss << std::hex;
    ss << "Wrote fragment " << InlineCrc32(data, size);

    std::cout << ss.str() << std::endl;

    return PROTOCOL_ACK_OK;
}

/*----------------------------------------------------------------------------*/
/* PUBLIC FUNCTION DEFINITIONS                                                */
/*----------------------------------------------------------------------------*/

int main(int argc, const char* argv[])
{
    signal(SIGINT, SignalHandler);
    
    static UdpSocket udp(8U);

    uint8_t packet[1472U];
    uint8_t transferBuffer[5 * 1024];

    UpdateServer_t us;
    REQUIRE(US_InitServer(&us, TEST_ReadDataById, TEST_WriteDataById, TEST_PutMetadata, TEST_PutFragment));

    TransferBuffer_t tb;
    REQUIRE(TRANSFER_Init(&tb, &us, transferBuffer, sizeof(transferBuffer)));

    std::cout << "Listening on port 8" << std::endl;

    while(true)
    {
        const auto rx = udp.Recv();

        if (rx.size() > sizeof(packet))
        {
            continue;
        }

        memcpy(packet, rx.data(), rx.size());
        const size_t resSize = TRANSFER_Process(&tb, packet, rx.size(), sizeof(packet));
        std::vector<uint8_t> tx(&packet[0], &packet[resSize]);

        udp.Send(tx);
    }

    return 0;
}

/* EoF testserver.cpp */
