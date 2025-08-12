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
 * protocol.h
 *
 * @brief Protocol definitions for update server
*/

#ifndef PROTOCOL_H_
#define PROTOCOL_H_

#ifdef __cplusplus
extern "C" {
#endif

/*----------------------------------------------------------------------------*/
/* PUBLIC MACRO DEFINITIONS                                                   */
/*----------------------------------------------------------------------------*/

#define TRANSFER_SINGLE_PACKET          (0x00)
#define TRANSFER_MULTI_PACKET_INIT      (0x01)
#define TRANSFER_MULTI_PACKET_TRANSFER  (0x02)
#define TRANSFER_MULTI_PACKET_END       (0x03)

#define PROTOCOL_SID_PING               (0x01U)
#define PROTOCOL_SID_READ_DATA_BY_ID    (0x02U)
#define PROTOCOL_SID_WRITE_DATA_BY_ID   (0x03U)
#define PROTOCOL_SID_PUT_METADATA       (0x10U)
#define PROTOCOL_SID_PUT_FRAGMENT       (0x11U)

#define PROTOCOL_ACK_OK                     (0x00U)
#define PROTOCOL_NACK_REQUEST_OUT_OF_RANGE  (0xE0U)
#define PROTOCOL_NACK_INVALID_REQUEST       (0xE1U)
#define PROTOCOL_NACK_BUSY_REPEAT_REQUEST   (0xE2U)
#define PROTOCOL_NACK_REQUEST_FAILED        (0xE3U)
#define PROTOCOL_NACK_INTERNAL_ERROR        (0xE4U)

#define PROTOCOL_DATA_ID_FIRMWARE_VERSION   (0x01U)
#define PROTOCOL_DATA_ID_FIRMWARE_TYPE      (0x02U)
#define PROTOCOL_DATA_ID_FIRMWARE_NAME      (0x03U)
#define PROTOCOL_DATA_ID_FIRMWARE_UPDATE    (0x10U)
#define PROTOCOL_DATA_ID_FIRMWARE_ROLLBACK  (0x11U)

#ifdef __cplusplus
} /* extern C */
#endif

/* EoF protocol.h */

#endif /* PROTOCOL_H_ */
