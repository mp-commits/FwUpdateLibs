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
 * transfer.c
 *
 * @brief Simple multi packet transport layer for update server protocol
*/

/*----------------------------------------------------------------------------*/
/* INCLUDE DIRECTIVES                                                         */
/*----------------------------------------------------------------------------*/

#include "updateserver/transfer.h"
#include <string.h>

/*----------------------------------------------------------------------------*/
/* MACRO DEFINITIONS                                                          */
/*----------------------------------------------------------------------------*/

#define IS_NULL(ptr) (ptr == NULL)

/*----------------------------------------------------------------------------*/
/* PRIVATE FUNCTION DEFINITIONS                                               */
/*----------------------------------------------------------------------------*/

static inline size_t TransferResponse(uint8_t* buf, uint8_t code)
{
    buf[0] = TRANSFER_SINGLE_PACKET;    // transfer layer
    buf[1] = 0U;                        // SID
    buf[2] = code;                      // arg
    return 3;
}

static inline uint32_t DecodeU32Be(const uint8_t* buf)
{
    uint32_t val = (uint32_t)(buf[0]) << 24U;
    val += (uint32_t)(buf[1]) << 16U;
    val += (uint32_t)(buf[2]) << 8U;
    val += (uint32_t)(buf[3]);
    return val;
}

static size_t HandleSinglePacket(
    TransferBuffer_t* tb,
    uint8_t* packet,
    size_t packetSize,
    size_t maxPacketSize)
{
    tb->state = TRANSFER_IDLE;
    tb->msgSize = packetSize - 1U;
    tb->transferSize = 0U;
    
    memcpy(tb->buf, &packet[1], packetSize - 1U);

    return 1U +  US_ProcessRequest(
        tb->server,
        tb->buf,
        tb->msgSize,
        &packet[1],
        maxPacketSize - 1U
    );
}

static size_t HandleTransferStart(
    TransferBuffer_t* tb,
    uint8_t* packet,
    size_t packetSize)
{
    if (packetSize != 5U)
    {
        return TransferResponse(packet, PROTOCOL_NACK_INVALID_REQUEST);
    }

    const uint32_t transferSize = DecodeU32Be(&packet[1]);

    if (transferSize > tb->bufSize)
    {
        return TransferResponse(packet, PROTOCOL_NACK_REQUEST_OUT_OF_RANGE);
    }

    tb->state = TRANSFER_RX;
    tb->msgSize = 0U;
    tb->transferSize = transferSize;

    return TransferResponse(packet, PROTOCOL_ACK_OK);
}

static size_t HandleTransferData(
    TransferBuffer_t* tb,
    uint8_t* packet,
    size_t packetSize)
{
    // Wrong transfer order
    if (tb->state != TRANSFER_RX)
    {
        return TransferResponse(packet, PROTOCOL_NACK_REQUEST_FAILED);
    }

    const size_t dataSize = packetSize - 1U;
    const size_t spaceRemaining = tb->bufSize - tb->msgSize;

    // Too large data size
    if (dataSize > spaceRemaining)
    {
        return TransferResponse(packet, PROTOCOL_NACK_REQUEST_OUT_OF_RANGE);
    }

    // Larger than initialized transfer
    if ((tb->msgSize + dataSize) > tb->transferSize)
    {
        return TransferResponse(packet, PROTOCOL_NACK_INVALID_REQUEST);
    }

    memcpy(&tb->buf[tb->msgSize], &packet[1], dataSize);
    tb->msgSize += dataSize;

    return TransferResponse(packet, PROTOCOL_ACK_OK);
}

static size_t HandleTransferEnd(
    TransferBuffer_t* tb,
    uint8_t* packet,
    size_t packetSize,
    size_t maxPacketSize)
{
    if (packetSize != 1U)
    {
        return 0U;
    }

    // Wrong transfer order
    if (tb->state != TRANSFER_RX)
    {
        return TransferResponse(packet, PROTOCOL_NACK_REQUEST_FAILED);
    }
    
    // Not complete
    if (tb->msgSize != tb->transferSize)
    {
        return TransferResponse(packet, PROTOCOL_NACK_REQUEST_OUT_OF_RANGE);
    }

    packet[0] = TRANSFER_SINGLE_PACKET;
    return 1U +  US_ProcessRequest(
        tb->server,
        tb->buf,
        tb->msgSize,
        &packet[1],
        maxPacketSize - 1U
    );
}

/*----------------------------------------------------------------------------*/
/* PUBLIC FUNCTION DEFINITIONS                                                */
/*----------------------------------------------------------------------------*/

bool TRANSFER_Init(
    TransferBuffer_t* tb, 
    const UpdateServer_t* server, 
    uint8_t* buf, 
    size_t bufSize)
{
    if (IS_NULL(tb) ||
        IS_NULL(server) ||
        IS_NULL(buf) ||
        (bufSize < 2U))
    {
        return false;
    }

    tb->buf = buf;
    tb->bufSize = bufSize;
    tb->server = server;
    tb->msgSize = 0U;
    tb->transferSize = 0U;
    tb->state = TRANSFER_IDLE;

    return true;
}

extern size_t TRANSFER_Process(
    TransferBuffer_t* tb,
    uint8_t* packet,
    size_t packetSize,
    size_t maxPacketSize)
{
    if (IS_NULL(tb) ||
        IS_NULL(packet) ||
        (packetSize < 2U) ||
        (packetSize > tb->bufSize) ||
        (maxPacketSize < 6U))
    {
        return 0U;
    }

    size_t resSize = 0U;

    switch (packet[0])
    {
    case TRANSFER_SINGLE_PACKET:
        resSize = HandleSinglePacket(tb, packet, packetSize, maxPacketSize);
        break;
    case TRANSFER_MULTI_PACKET_INIT:
        resSize = HandleTransferStart(tb, packet, packetSize);
        break;
    case TRANSFER_MULTI_PACKET_TRANSFER:
        resSize = HandleTransferData(tb, packet, packetSize);
        break;
    case TRANSFER_MULTI_PACKET_END:
        resSize = HandleTransferEnd(tb, packet, packetSize, maxPacketSize);
        break;
    default:
        return 0U;
    }

    if (resSize > maxPacketSize)
    {
        return 0U;
    }

    return resSize;
}

/* EoF transfer.c */
