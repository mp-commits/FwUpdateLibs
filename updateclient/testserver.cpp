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

extern "C" {
    #include "ed25519_extra.h"
    #include "ed25519.h"
    #include "sha512.h"
}

#include "keyfile/openSSH_key.hpp"
#include "fragmentstore/fragmentstore.h" // Default types
#include "udpsocket.hpp"
#include "updateserver/server.h"
#include "updateserver/protocol.h"
#include "updateserver/transfer.h"

#include <stdio.h>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <iostream>
#include <csignal>
#include <map>

/*----------------------------------------------------------------------------*/
/* PRIVATE TYPE DEFINITIONS                                                   */
/*----------------------------------------------------------------------------*/

typedef struct
{
    Metadata_t recvMetadata;
    std::map<uint32_t, Fragment_t> recvFragments;
    KeyPair keys;
} TestServer_t;

/*----------------------------------------------------------------------------*/
/* MACRO DEFINITIONS                                                          */
/*----------------------------------------------------------------------------*/

#define REQUIRE(x) \
if(!(x)) \
{ \
std::cout << #x << " failed!" << std::endl; \
return -1; \
}

#define APP_METADATA_ADDRESS  0x08010000U
#define FIRST_FLASH_ADDRESS   (APP_METADATA_ADDRESS + sizeof(Metadata_t))
#define LAST_FLASH_ADDRESS    (0x82000000U)

/*----------------------------------------------------------------------------*/
/* VARIABLE DEFINITIONS                                                       */
/*----------------------------------------------------------------------------*/

static TestServer_t f_self;

/*----------------------------------------------------------------------------*/
/* PRIVATE FUNCTION DEFINITIONS                                               */
/*----------------------------------------------------------------------------*/

static void PrintBytes(const uint8_t* buf, size_t len, const char* header = nullptr)
{
    if (header)
    {
        printf("%s", header);
    }

    for (size_t i = 0; i < len; i++)
    {
        printf("%2X ", buf[i]);
    }
    printf("\r\n");
}

static bool VerifyMetadata(const Metadata_t* meta)
{
    const uint8_t* msg = (const uint8_t*)(meta);
    const size_t msgLen = sizeof(Metadata_t)-sizeof(meta->metadataSignature);
    return 1 == ed25519_verify(meta->metadataSignature, msg, msgLen, f_self.keys.GetPublicKey().data());
}

static bool VerifyFragment(const Fragment_t* frag)
{
    const uint8_t* msg = (const uint8_t*)(frag);
    const size_t msgLen = sizeof(Fragment_t)-sizeof(frag->signature);

    if (0U == frag->verifyMethod)
    {
        printf("Verifying fragment with ed25519\r\n");
        return 1 == ed25519_verify(frag->signature, msg, msgLen, f_self.keys.GetPublicKey().data());
    }
    else if (1U == frag->verifyMethod)
    {
        uint8_t inHash[64];
        sha512_context ctx;
        sha512_init(&ctx);

        printf("Verifying fragment with sha512\r\n");

        if (0U == frag->number)
        {
            sha512_update(&ctx, f_self.recvMetadata.metadataSignature, 64U);
        }
        else
        {
            const uint32_t pIdx = frag->number - 1U;
            if (f_self.recvFragments.find(pIdx) == f_self.recvFragments.end())
            {
                return false;
            }
            sha512_update(&ctx, f_self.recvFragments.at(pIdx).sha512, 64U);
        }

        sha512_update(&ctx, msg, msgLen);
        sha512_final(&ctx, inHash);

        return 0 == memcmp(inHash, frag->sha512, 64U);
    }
    else
    {
        return false;
    }
}

static bool TryInstallFirmware(const Metadata_t* meta)
{
    int cmp = memcmp(meta, &f_self.recvMetadata, sizeof(Metadata_t));

    if (cmp != 0)
    {
        printf("Metadata arg not equal to uploaded firmware");
        return false;
    }

    ed25519_multipart_t ctx;
    int ed = ed25519_multipart_init(&ctx, f_self.recvMetadata.firmwareSignature, f_self.keys.GetPublicKey().data());

    if (ed != 1)
    {
        printf("ed25519_multipart_init failed");
        return false;
    }

    uint32_t nextStart = FIRST_FLASH_ADDRESS;
    uint32_t nextIdx = 0;

    for (const auto& entry: f_self.recvFragments)
    {
        if (entry.first != nextIdx)
        {
            printf("Fragment map key incorrect\r\n");
        }
        else
        {
            nextIdx++;
        }

        const Fragment_t* frag = &entry.second;
        
        if (frag->startAddress != nextStart)
        {
            printf("Fragment %u: unexpected start address: %lX, expected %lX\r\n", frag->number, frag->startAddress, nextStart);
            return false;
        }
        else
        {
            nextStart += frag->size;
        }

        uint32_t verifyOffset = 0U;
        size_t   verifyLen = frag->size;

        if (frag->startAddress < meta->startAddress)
        {
            verifyOffset = meta->startAddress - frag->startAddress;
        }

        if (verifyOffset < verifyLen)
        {
            verifyLen -= verifyOffset;
        }

        if (verifyLen > 0U)
        {
            ed = ed25519_multipart_continue(&ctx, &frag->content[verifyOffset], verifyLen);
            if (ed != 1U)
            {
                printf("ed25519_multipart_continue failed\r\n");
                return false;
            }
        }
    }

    ed = ed25519_multipart_end(&ctx);
    if (ed != 1U)
    {
        printf("ed25519_multipart_end failed\r\n");
        return false;
    }

    return true;
}

static void SignalHandler( int signum )
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
    ss << std::dec;
    ss << " content of " << size << " bytes: ";
    ss << std::hex;
    for (size_t i = 0; i < size; i++)
    {
        ss << " " << (uint32_t)(in[i]);
    }

    std::cout << ss.str() << std::endl;

    if ((id == PROTOCOL_DATA_ID_FIRMWARE_UPDATE) && 
        (size == sizeof(Metadata_t)))
    {
        const Metadata_t* meta = (const Metadata_t*)(in);
        if (TryInstallFirmware(meta))
        {
            printf("INSTALL OK!\r\n");
            return PROTOCOL_ACK_OK;
        }
        return PROTOCOL_NACK_REQUEST_FAILED;
    }

    return PROTOCOL_ACK_OK;
}

static uint8_t TEST_PutMetadata(
    const uint8_t* data, 
    size_t size)
{
    std::stringstream ss;
    ss << std::hex;
    ss << "Received metadata " << InlineCrc32(data, size);
    std::cout << ss.str() << std::endl;

    if (size == sizeof(Metadata_t))
    {
        // PrintBytes(data, size);
        const Metadata_t* meta = (const Metadata_t*)data;
        if (VerifyMetadata(meta))
        {
            std::cout << "Metadata OK" << std::endl;
            f_self.recvMetadata = *meta;
            return PROTOCOL_ACK_OK;
        }
        else
        {
            std::cout << "Metadata invalid" << std::endl;
        }
        return PROTOCOL_NACK_INVALID_REQUEST;
    }
    else
    {
        std::cout << "Metadata wrong size" << std::endl;
    }

    return PROTOCOL_NACK_REQUEST_OUT_OF_RANGE;
}

static uint8_t TEST_PutFragment(
    const uint8_t* data, 
    size_t size)
{
    std::stringstream ss;
    ss << std::hex;
    ss << "Received fragment " << InlineCrc32(data, size);
    std::cout << ss.str() << std::endl;

    if (size == sizeof(Fragment_t))
    {
        const Fragment_t* frag = (const Fragment_t*)data;
        if (VerifyFragment(frag))
        {
            f_self.recvFragments[frag->number] = *frag;
            return PROTOCOL_ACK_OK;
        }
        else
        {
            return PROTOCOL_NACK_INVALID_REQUEST;
        }
    }

    return PROTOCOL_NACK_REQUEST_OUT_OF_RANGE;
}

/*----------------------------------------------------------------------------*/
/* PUBLIC FUNCTION DEFINITIONS                                                */
/*----------------------------------------------------------------------------*/

int main(int argc, const char* argv[])
{
    signal(SIGINT, SignalHandler);
    f_self = (TestServer_t){0};

    if (argc != 2)
    {
        std::cout << "Required args: testserver ./path/to/id_ed25519" << std::endl;
        return -1;
    }

    {
        std::ifstream keyFile(argv[1]);
        f_self.keys.FromFile(keyFile);
        std::cout << "Loaded keys from " << argv[1] << std::endl;
        PrintBytes(f_self.keys.GetPrivateKey().data(), f_self.keys.GetPrivateKey().size(), "Private key: ");
        PrintBytes(f_self.keys.GetPublicKey().data(), f_self.keys.GetPublicKey().size(), "Public key: ");
    }

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
