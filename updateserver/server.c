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
 * server.c
 *
 * @brief Simple server to route request to perfrom remote firmware update
*/

/*----------------------------------------------------------------------------*/
/* INCLUDE DIRECTIVES                                                         */
/*----------------------------------------------------------------------------*/

#include "updateserver/server.h"
#include "updateserver/protocol.h"

/*----------------------------------------------------------------------------*/
/* PRIVATE TYPE DEFINITIONS                                                   */
/*----------------------------------------------------------------------------*/

typedef struct
{
    const uint8_t sid;
    const uint8_t* req;
    const size_t reqLen;
    uint8_t* res;
    const size_t maxResLen;
} ServiceArg_t;

/*----------------------------------------------------------------------------*/
/* MACRO DEFINITIONS                                                          */
/*----------------------------------------------------------------------------*/

#define IS_NULL(ptr) (ptr == NULL)

#define MINIMUM_RESPONSE_LENGTH (2U)

/*----------------------------------------------------------------------------*/
/* PRIVATE FUNCTION DEFINITIONS                                               */
/*----------------------------------------------------------------------------*/

static inline size_t MinSz(size_t a, size_t b)
{
    return (a < b) ? a : b;
}

static size_t BasicResponse(uint8_t sid, uint8_t code, uint8_t* buf)
{
    buf[0] = sid;
    buf[1] = code;
    return MINIMUM_RESPONSE_LENGTH;
}

static size_t HandlePing(
    const UpdateServer_t* server,
    const ServiceArg_t* arg)
{
    (void)server;

    if (arg->reqLen != 1U)
    {
        const uint8_t code = PROTOCOL_NACK_INVALID_REQUEST;
        return BasicResponse(arg->sid, code, arg->res);
    }

    return BasicResponse(arg->sid, PROTOCOL_ACK_OK, arg->res);
}

static size_t HandleReadDataById(
    const UpdateServer_t* server,
    const ServiceArg_t* arg)
{
    if (arg->reqLen != 2U)
    {
        const uint8_t code = PROTOCOL_NACK_INVALID_REQUEST;
        return BasicResponse(arg->sid, code, arg->res);
    }
    if (arg->maxResLen <= MINIMUM_RESPONSE_LENGTH)
    {
        // No space for any actual data
        const uint8_t code = PROTOCOL_NACK_INTERNAL_ERROR;
        return BasicResponse(arg->sid, code, arg->res);
    }

    const uint8_t   id      = arg->req[1];
    uint8_t*        outBuf  = &arg->res[MINIMUM_RESPONSE_LENGTH];
    const size_t    maxLen  = arg->maxResLen - 2U;
    size_t          readLen = 0U;

    const uint8_t result = server->ReadDid(id, outBuf, maxLen, &readLen);

    if (result == PROTOCOL_ACK_OK)
    {
        /* Encode positive response in front of read data */
        readLen += BasicResponse(arg->sid, result, arg->res);
        return readLen;
    }
    else
    {
        return BasicResponse(arg->sid, result, arg->res);
    }
}

static size_t HandleWriteDataById(
    const UpdateServer_t* server,
    const ServiceArg_t* arg)
{
    if (arg->reqLen < 3U)
    {
        /* Request must have sid + id + data */
        const uint8_t code = PROTOCOL_NACK_INVALID_REQUEST;
        return BasicResponse(arg->sid, code, arg->res);
    }

    const uint8_t   id      = arg->req[1];
    const uint8_t*  data    = &arg->req[2];
    const size_t    len     = arg->reqLen - 2U;

    const uint8_t result = server->WriteDid(id, data, len);

    return BasicResponse(arg->sid, result, arg->res);
}

static size_t HandlePutMetadata(
    const UpdateServer_t* server,
    const ServiceArg_t* arg)
{
    if (arg->reqLen < 2U)
    {
        /* Request must have sid + data */
        const uint8_t code = PROTOCOL_NACK_INVALID_REQUEST;
        return BasicResponse(arg->sid, code, arg->res);
    }

    const uint8_t*  data    = &arg->req[1];
    const size_t    len     = arg->reqLen - 1U;

    const uint8_t result = server->PutMetadata(data, len);

    return BasicResponse(arg->sid, result, arg->res);
}

static size_t HandlePutFragment(
    const UpdateServer_t* server,
    const ServiceArg_t* arg)
{
    if (arg->reqLen < 2U)
    {
        /* Request must have sid + data */
        const uint8_t code = PROTOCOL_NACK_INVALID_REQUEST;
        return BasicResponse(arg->sid, code, arg->res);
    }

    const uint8_t*  data    = &arg->req[1];
    const size_t    len     = arg->reqLen - 1U;

    const uint8_t result = server->PutFragment(data, len);

    return BasicResponse(arg->sid, result, arg->res);
}

/*----------------------------------------------------------------------------*/
/* PUBLIC FUNCTION DEFINITIONS                                                */
/*----------------------------------------------------------------------------*/

bool US_InitServer(
    UpdateServer_t* server,
    ReadDataById_t  readDid,
    WriteDataById_t writeDid,
    PutMetadata_t   putMetadata,
    PutFragment_t   putFragment)
{
    if (IS_NULL(server) ||
        IS_NULL(readDid) ||
        IS_NULL(writeDid) ||
        IS_NULL(putMetadata) ||
        IS_NULL(putFragment))
    {
        return false;
    }

    server->ReadDid = readDid;
    server->WriteDid = writeDid;
    server->PutMetadata = putMetadata;
    server->PutFragment = putFragment;

    return true;
}

size_t US_ProcessRequest(
    const UpdateServer_t* server,
    const uint8_t* request,
    size_t requestLength,
    uint8_t* response,
    size_t maxResponseLength)
{
    if (IS_NULL(server) ||
        IS_NULL(request) ||
        IS_NULL(response) ||
        (requestLength == 0U) ||
        (maxResponseLength < MINIMUM_RESPONSE_LENGTH))
    {
        return 0U;
    }

    const ServiceArg_t arg = {
        .sid = request[0U],
        .req = request,
        .reqLen = requestLength,
        .res = response,
        .maxResLen = maxResponseLength
    };

    size_t responseLength = 0U;

    switch (arg.sid)
    {
    case PROTOCOL_SID_PING:
        responseLength = HandlePing(server, &arg);
        break;
    case PROTOCOL_SID_READ_DATA_BY_ID:
        responseLength = HandleReadDataById(server, &arg);
        break;
    case PROTOCOL_SID_WRITE_DATA_BY_ID:
        responseLength = HandleWriteDataById(server, &arg);
        break;
    case PROTOCOL_SID_PUT_METADATA:
        responseLength = HandlePutMetadata(server, &arg);
        break;
    case PROTOCOL_SID_PUT_FRAGMENT:
        responseLength = HandlePutFragment(server, &arg);
        break;
    default:
        responseLength = BasicResponse(
            arg.sid, 
            PROTOCOL_NACK_REQUEST_OUT_OF_RANGE, 
            arg.res);
        break;
    }

    return responseLength;
}

/* EoF server.c */
