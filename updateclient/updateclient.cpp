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
#include "keyfile/openSSH_key.hpp"

extern "C"
{
    #include "sha512.h"
    #include "ed25519.h"
}

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

        sec.fragments.push_back(frag);
    }
}

static bool VerifyKeys(const KeyPair& keyPair)
{
    const char msg[] = "Test message to verify asymmetric keys";
    uint8_t signature[64];

    const auto seed = keyPair.GetPrivateKey();
    const auto pubFile = keyPair.GetPublicKey();

    // Verify key sizes
    if ((seed.size() < 32) || (pubFile.size() != 32))
    {
        return false;
    }

    // Generate signing keys from the keyPair
    uint8_t pubGen[32];
    uint8_t privGen[64];
    ed25519_create_keypair(pubGen, privGen, seed.data());

    // Verify that the derived public key matches the key pair
    if (memcmp(pubFile.data(), pubGen, sizeof(pubGen)))
    {
        return false;
    }

    // Perform quick sign-verify test
    ed25519_sign(signature, (const uint8_t*)msg, strlen(msg), pubFile.data(), privGen);
    return ed25519_verify(signature, (const uint8_t*)msg, strlen(msg), pubFile.data());
}

static void AddHashChain(FirmwareSections_t& sec)
{
    uint8_t lastHash[64];

    memcpy(lastHash, sec.metadata.metadataSignature, 64U);

    const auto CalculateNextHash = []
    (uint8_t* lastHash, const uint8_t* data, size_t len) -> int
    {
        sha512_context ctx;
        int ret;
        if ((ret = sha512_init(&ctx))) return ret;
        if ((ret = sha512_update(&ctx, lastHash, 64U))) return ret;
        if ((ret = sha512_update(&ctx, data, len))) return ret;
        if ((ret = sha512_final(&ctx, lastHash))) return ret;
        return 0;
    };

    for (auto& f: sec.fragments)
    {
        const uint8_t* msg = (const uint8_t*)(&f);
        const size_t msgLen = sizeof(f)-sizeof(f.sha512);

        f.verifyMethod = 1U;

        int res = CalculateNextHash(lastHash, msg, msgLen);
        if (0 != res)
        {
            throw std::runtime_error("fragment sha512 failed");
        }

        memcpy(f.sha512, lastHash, 64U);
    }
}

static void SignFragments(FirmwareSections_t& sec, std::string keyFileName)
{
    std::ifstream keyFile(keyFileName);
    if (!keyFile.good())
    {
        throw std::runtime_error("Failed to open: "+keyFileName);
    }

    KeyPair keypair(keyFile);
    
    if (!VerifyKeys(keypair))
    {
        throw std::runtime_error("Invalid openSSH ed25519 keyfile: "+keyFileName);
    }

    uint8_t pubKey[32];
    uint8_t privKey[64];
    ed25519_create_keypair(pubKey, privKey, keypair.GetPrivateKey().data());

    for (auto& f: sec.fragments)
    {
        const uint8_t* msg = (const uint8_t*)(&f);
        const size_t msgLen = sizeof(f)-sizeof(f.signature);

        f.verifyMethod = 0U;

        ed25519_sign(f.signature, msg, msgLen, pubKey, privKey);
        if (!ed25519_verify(f.signature, msg, msgLen, keypair.GetPublicKey().data()))
        {
            throw std::runtime_error("Fragment signing re-verification failed");
        }
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

static void AddArguments(argparse::ArgumentParser& parser)
{
    parser.add_argument("-a", "--address")
        .help("Destination IP address")
        .default_value("127.0.0.1");

    parser.add_argument("-p", "--port")
        .help("Destination IP port")
        .default_value("8");
    
    parser.add_argument("--localport")
        .help("Optional local IP port if different from remote port");

    parser.add_argument("-k", "--key")
        .help("Optional keypair for signing firmware fragments")
        .default_value("");

    parser.add_argument("command")
        .help("Client operation command")
        .required();

    parser.add_argument("command_arg")
        .help("argument for [command]")
        .default_value("");
}

static int ClientExecuteReset(UpdateClient& client)
{
    std::cout << "Writing reset request" << std::endl;
    client.WriteDataById(PROTOCOL_DATA_ID_RESET, {0});
    return 0;
}

static int ClientExecuteUpdate(UpdateClient& client, std::string& argStr, std::string& keyFile)
{
    if (argStr.empty())
    {
        std::cerr << "Argument string empty. Should contain .hex file path";
        return -1;
    }

    FirmwareSections_t sec;
    MakeFirmwareSections(argStr, sec);    

    if (keyFile.empty())
    {
        AddHashChain(sec);
    }
    else
    {
        SignFragments(sec, keyFile);
    }

    std::cout << "Fragment creation successful" << std::endl;
    
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
        return 1;
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
            return 2;
        }
    }

    std::cout << "Writing update request" << std::endl;
    std::vector<uint8_t> metadataBuffer(sizeof(Metadata_t));
    mempcpy(metadataBuffer.data(), &sec.metadata, metadataBuffer.size());
    client.WriteDataById(PROTOCOL_DATA_ID_FIRMWARE_UPDATE, metadataBuffer);

    return ClientExecuteReset(client);
}

static int ClientExecuteRollback(UpdateClient& client, std::string& argStr)
{
    std::vector<uint8_t> rollbackArg = {0};

    if (!argStr.empty())
    {
        FirmwareSections_t sec;
        MakeFirmwareSections(argStr, sec);
        rollbackArg.resize(sizeof(sec.metadata));
        mempcpy(rollbackArg.data(), &sec.metadata, rollbackArg.size());
        std::cout << "Set rollback request with file " << argStr << std::endl;
    }
    
    ReadFirmwareVersion(client);
    ReadFirmwareType(client);
    ReadFirmwareName(client);

    std::cout << "Writing rollback request" << std::endl;
    client.WriteDataById(PROTOCOL_DATA_ID_FIRMWARE_ROLLBACK, rollbackArg);

    return ClientExecuteReset(client);
}

static int ClientExecuteSlotErase(UpdateClient& client, std::string& arg)
{
    if (arg.empty())
    {
        std::cerr << "Erase argument must be an integer in range 0-255" << std::endl;
        return -1;
    }

    int slot = std::stoi(arg);

    if ((slot < 0) || (slot > 255))
    {
        std::cerr << "Erase slot index must fit into one byte: " << slot << std::endl;
        return -2;
    }

    std::cout << "Writing slot erase request for slot " << slot << std::endl;
    client.WriteDataById(PROTOCOL_DATA_ID_ERASE_SLOT, {(uint8_t)slot});

    return 0;
}

/*----------------------------------------------------------------------------*/
/* PUBLIC FUNCTION DEFINITIONS                                                */
/*----------------------------------------------------------------------------*/

int main(int argc, const char* argv[])
{
    argparse::ArgumentParser parser("Upload tool v0.2");
    AddArguments(parser);

    std::string serverIp;
    int serverPort = 0;
    int clientPort = 0;
    std::string keyFileName;
    std::string command;
    std::string commandArg;

    try
    {
        parser.parse_args(argc, argv);

        std::string serverPortStr;
        std::string clientPort;

        command = parser.get("command");
        commandArg = parser.get("command_arg");
        serverIp = parser.get("-a");
        serverPortStr = parser.get("-p");
        keyFileName = parser.get("-k");
        
        try {clientPort = parser.get("--localport");} catch(...){clientPort = serverPortStr;}

        serverPort = std::stoi(serverPortStr);
        clientPort = std::stoi(clientPort);
    }
    catch (const std::exception& err)
    {
        std::cerr << err.what() << std::endl;
        std::cerr << parser;
        return -1;
    }

    UdpSocket socket(clientPort);
    socket.SetRemoteAddress(serverIp.c_str(), serverPort);
    UpdateClient client(socket);

    if (command.empty())
    {
        std::cerr << "Command empty" << std::endl;
    }
    else if (command == "upload")
    {
        return ClientExecuteUpdate(client, commandArg, keyFileName);
    }
    else if (command == "reset")
    {
        return ClientExecuteReset(client);
    }
    else if (command == "rollback")
    {
        return ClientExecuteRollback(client, commandArg);
    }
    else if (command == "erase")
    {
        return ClientExecuteSlotErase(client, commandArg);
    }
    else if (command == "version")
    {
        ReadFirmwareVersion(client);
        ReadFirmwareType(client);
        ReadFirmwareName(client);
        return 0;
    }
    else
    {
        std::cerr << "Invalid command: " << command;
        std::cerr << "\n Must be one of the following:";
        std::cerr << "\n    upload ./path/to/binary.hex";
        std::cerr << "\n    reset";
        std::cerr << "\n    rollback [hexfile]";
        std::cerr << "\n    erase 0-255";
        std::cerr << "\n    version";
        std::cerr << std::endl;
    }

    return -10;
}

/* EoF updateclient.cpp */
