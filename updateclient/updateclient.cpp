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
 * updateclient.cpp
 *
 * @brief Simple client for uploading firmware to a supported device
*/

/*----------------------------------------------------------------------------*/
/* INCLUDE DIRECTIVES                                                         */
/*----------------------------------------------------------------------------*/

#include "client.hpp"
#include "udpsocket.hpp"

#include "argparse/argparse.hpp"
#include "crc32.hpp"
#include "fragmentstore/fragmentstore.h"
#include "hexfile.hpp"
#include "updateserver/protocol.h"

#include <fstream>
#include <string>
#include <vector>

/*----------------------------------------------------------------------------*/
/* PRIVATE TYPE DEFINITIONS                                                   */
/*----------------------------------------------------------------------------*/

typedef struct
{
    Metadata_t metadata;
    std::vector<Fragment_t> fragments;
} FirmwareSections_t;

/*----------------------------------------------------------------------------*/
/* MACRO DEFINITIONS                                                          */
/*----------------------------------------------------------------------------*/

#define member_size(type, member) (sizeof( ((type *)0)->member ))

/*----------------------------------------------------------------------------*/
/* VARIABLE DEFINITIONS                                                       */
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/* PRIVATE FUNCTION DEFINITIONS                                               */
/*----------------------------------------------------------------------------*/

static void MakeFirmwareSections(std::string hexFileName, FirmwareSections_t& sec)
{
    std::ifstream inputFile(hexFileName);
    HexFile hexFile(inputFile);

    if (hexFile.GetSectionCount() != 1)
    {
        std::cerr << "Invalid HEX file" << std::endl;
        return;
    }

    auto hexContent = hexFile.GetSectionAt(0);
    uint8_t* data = hexContent.data.data();

    memcpy(&sec.metadata, data, sizeof(Metadata_t));

    const size_t fragContentSize = member_size(Fragment_t, content);

    for (size_t pos = sizeof(Metadata_t), num = 0; pos < hexContent.data.size(); pos += fragContentSize, num++)
    {
        const size_t remaining = hexContent.data.size() - pos;
        const size_t fragSize = (remaining < fragContentSize) ? remaining : fragContentSize;

        Fragment_t frag;
        memset(&frag, 0, sizeof(frag));

        frag.firmwareId = sec.metadata.firmwareId;
        frag.number = num;
        frag.startAddress = hexContent.startAddress + pos;
        frag.size = fragSize;
        memcpy(frag.content, &data[pos], fragSize);
        // frag.signature TODO: Implement signing

        sec.fragments.push_back(frag);
    }
}

static inline uint32_t DecodeU32Be(const std::vector<uint8_t>& vec)
{
    uint32_t val = vec.at(3);
    val += ((uint32_t)vec.at(2)) << 8U;
    val += ((uint32_t)vec.at(1)) << 16U;
    val += ((uint32_t)vec.at(0)) << 24U;

    return val;
}

static void ReadFirmwareVersion(UpdateClient& client)
{
    const auto data = client.ReadDataById(PROTOCOL_DATA_ID_FIRMWARE_VERSION);

    if (data.size() != 4U)
    {
        std::cout << "Invalid firmware version size: " << data.size() << std::endl;
        return;
    }

    std::cout << "Firmware version: " << DecodeU32Be(data) << std::endl;
}

static void ReadFirmwareType(UpdateClient& client)
{
    const auto data = client.ReadDataById(PROTOCOL_DATA_ID_FIRMWARE_TYPE);

    if (data.size() != 4U)
    {
        std::cout << "Invalid firmware type size: " << data.size() << std::endl;
        return;
    }

    std::cout << "Firmware type: " << DecodeU32Be(data) << std::endl;
}

static void ReadFirmwareName(UpdateClient& client)
{
    const auto data = client.ReadDataById(PROTOCOL_DATA_ID_FIRMWARE_NAME);
    std::cout << "Firmware name: " << std::string(data.begin(), data.end()) << std::endl;
}

/*----------------------------------------------------------------------------*/
/* PUBLIC FUNCTION DEFINITIONS                                                */
/*----------------------------------------------------------------------------*/

int main(int argc, const char* argv[])
{

    const char* ip = "127.0.0.1";
    const uint16_t myPort = 7;
    const uint16_t remotePort = 8;

    UdpSocket socket(myPort);
    socket.SetRemoteAddress(ip, remotePort);
    UpdateClient client(socket);

    FirmwareSections_t sec;
    MakeFirmwareSections("C:/Users/mikael/Desktop/git/reliable_fw_update/new_freertos_app/build/Debug/new_freertos_app_signed.hex", sec);

    ReadFirmwareVersion(client);
    ReadFirmwareType(client);
    ReadFirmwareName(client);

    if (client.PutMetadata(sec.metadata))
    {
        std::cout << "Successfully uploaded metadata: " << std::hex << InlineCrc32((const uint8_t*)&sec.metadata, sizeof(sec.metadata)) << std::endl;
    }
    else
    {
        std::cout << "Metadata upload fail!" << std::endl;
        return 1U;
    }

    for (const auto& frag: sec.fragments)
    {
        if (client.PutFragment(frag))
        {
            std::cout << "Successfully uploaded fragment at " << frag.startAddress << ": " << std::hex << InlineCrc32((const uint8_t*)&frag, sizeof(frag)) << std::endl;
        }
        else
        {
            std::cout << "Fragment upload fail!" << std::endl;
            return 2U;
        }
    }

    return 0;
}

/* EoF updateclient.cpp */
