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
 * transfer.h
 *
 * @brief {Short description of the source file}
*/

#ifndef UPDATESERVER_TRANSFER_H_
#define UPDATESERVER_TRANSFER_H_

#ifdef __cplusplus
extern "C" {
#endif

/*----------------------------------------------------------------------------*/
/* INCLUDE DIRECTIVES                                                         */
/*----------------------------------------------------------------------------*/

#include "updateserver/protocol.h"
#include "updateserver/server.h"

#include <stdint.h>
#include <stdbool.h>

/*----------------------------------------------------------------------------*/
/* PUBLIC TYPE DEFINITIONS                                                    */
/*----------------------------------------------------------------------------*/

typedef enum
{
    TRANSFER_IDLE,
    TRANSFER_RX
} TransferState_t;

typedef struct
{
    uint8_t*                buf;
    size_t                  bufSize;
    size_t                  msgSize;
    size_t                  transferSize;
    TransferState_t         state;
    const UpdateServer_t*   server;
} TransferBuffer_t;

/*----------------------------------------------------------------------------*/
/* PUBLIC FUNCTION DECLARATIONS                                               */
/*----------------------------------------------------------------------------*/

/** Initialize Transfer layer
 * 
 * @param tb Transfer buffer instance
 * @param server Update server instance for the transfer buffer
 * @param buf Transfer buffer memory
 * @param bufSize Size of buf* area
 * 
 * @return Init successful
 */
extern bool TRANSFER_Init(
    TransferBuffer_t* tb, 
    const UpdateServer_t* server, 
    uint8_t* buf, 
    size_t bufSize);

/** Process incoming packet
 * 
 * @param tb Transfer buffer instance
 * @param packet Packet buffer
 * @param packetSize Size of actual packet in packet* area
 * @param maximum  Maximum size of packet* area for response encoding
 * 
 * @return Num bytes encoded in packet* as a response packet
 * 
 * @note Responses are always forced to be singe packet transfer currently
 */
extern size_t TRANSFER_Process(
    TransferBuffer_t* tb,
    uint8_t* packet,
    size_t packetSize,
    size_t maxPacketSize);

#ifdef __cplusplus
} /* extern C */
#endif

/* EoF transfer.h */

#endif /* UPDATESERVER_TRANSFER_H_ */
