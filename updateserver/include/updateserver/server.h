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
 * server.h
 *
 * @brief Interface for update server
*/

#ifndef UPDATESERVER_SERVER_H_
#define UPDATESERVER_SERVER_H_

#ifdef __cplusplus
extern "C" {
#endif

/*----------------------------------------------------------------------------*/
/* INCLUDE DIRECTIVES                                                         */
/*----------------------------------------------------------------------------*/

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

/*----------------------------------------------------------------------------*/
/* PUBLIC TYPE DEFINITIONS                                                    */
/*----------------------------------------------------------------------------*/

/** Read data using an identifier
 * 
 * @param id Data ID
 * @param out Read buffer
 * @param maxSize Maximum read size
 * @param readSize Actual read size
 * 
 * @return Result code
 */
typedef uint8_t (*ReadDataById_t)(
    uint8_t id, 
    uint8_t* out, 
    size_t maxSize, 
    size_t* readSize
);

/** Write data using an identifier
 * 
 * @param id Data ID
 * @param in Write buffer
 * @param size Write size
 * 
 * @return Result code
 */
typedef uint8_t (*WriteDataById_t)(
    uint8_t id, 
    const uint8_t* in, 
    size_t size
);

/** Put new metadata
 * 
 * @param data Data buffer
 * @param size Data size
 * 
 * @return Result code
 */
typedef uint8_t (*PutMetadata_t)(
    const uint8_t* data, 
    size_t size
);

/** Put new fragment
 * 
 * @param data Data buffer
 * @param size Data size
 * 
 * @return Result code
 */
typedef uint8_t (*PutFragment_t)(
    const uint8_t* data, 
    size_t size
);

typedef struct 
{
    ReadDataById_t  ReadDid;
    WriteDataById_t WriteDid;
    PutMetadata_t   PutMetadata;
    PutFragment_t   PutFragment;
} UpdateServer_t;

/*----------------------------------------------------------------------------*/
/* PUBLIC FUNCTION DECLARATIONS                                               */
/*----------------------------------------------------------------------------*/

/** Initialize server instance
 * 
 * @param server Server instance
 * @param readDid Read service
 * @param writeDid Write service
 * @param putMetadata Put new metadata service
 * @param putFragment Put new fragment service
 * 
 * @return Initialization successful
 */
extern bool US_InitServer(
    UpdateServer_t* server,
    ReadDataById_t  readDid,
    WriteDataById_t writeDid,
    PutMetadata_t   putMetadata,
    PutFragment_t   putFragment);

/** Process incoming update server request
 * 
 * @param request Request buffer
 * @param requestLength Request length
 * @param response Response buffer
 * @param maxResponseLength Maximum number of bytes to put to response buffer
 * 
 * @return Number of bytes in response
 */
extern size_t US_ProcessRequest(
    const UpdateServer_t* server,
    const uint8_t* request,
    size_t requestLength,
    uint8_t* response,
    size_t maxResponseLength);

#ifdef __cplusplus
} /* extern C */
#endif

/* EoF server.h */

#endif /* UPDATESERVER_SERVER_H_ */
