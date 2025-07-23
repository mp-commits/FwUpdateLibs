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
 * hexsign.cpp
 *
 * @brief {Short description of the source file}
*/

/*----------------------------------------------------------------------------*/
/* INCLUDE DIRECTIVES                                                         */
/*----------------------------------------------------------------------------*/

#include <iostream>
#include <fstream>
#include <cstdint>
#include <string>
#include <cstring>
#include <sstream>

#include "argparse/argparse.hpp"
#include "ed25519.h"
#include "hexfile.hpp"
#include "fragmentstore/fragmentstore.h"
#include "openSSH_key.hpp"

/*----------------------------------------------------------------------------*/
/* PRIVATE TYPE DEFINITIONS                                                   */
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/* MACRO DEFINITIONS                                                          */
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/* VARIABLE DEFINITIONS                                                       */
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/* PRIVATE FUNCTION DEFINITIONS                                               */
/*----------------------------------------------------------------------------*/

static void AddArguments(argparse::ArgumentParser& parser)
{
    parser.add_argument("-i", "--input")
        .help("Input HEX file")
        .required();

    parser.add_argument("-o", "--output")
        .help("Output HEX file")
        .required();

    parser.add_argument("-k", "--key")
        .help("Key file")
        .required();
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

static uint32_t Crc32(const uint8_t* data, size_t size)
{
  uint32_t r = ~0; const uint8_t *end = data + size;
 
  while(data < end)
  {
    r ^= *data++;
 
    for(int i = 0; i < 8; i++)
    {
      uint32_t t = ~((r&1) - 1); r = (r>>1) ^ (0xEDB88320 & t);
    }
  }
 
  return ~r;
}

static std::string Crc32Str(const uint8_t* data, size_t size)
{
    const uint32_t crc32 = Crc32(data, size);
    std::stringstream crcStr;
    crcStr << std::uppercase << std::hex << crc32;
    return crcStr.str();
}

static void OutputKeySignature(const KeyPair& keyPair)
{
    const auto pubKey = keyPair.GetPublicKey();

    std::cout << "Public key CRC32: " << Crc32Str(pubKey.data(), pubKey.size()) << std::endl;
}

static bool InRange(uint32_t val, uint32_t low, uint32_t high)
{
    return (val >= low) && (val <= high);
}

static bool CheckMetadataMem(const Metadata_t* const meta, const HexFile::Section& sec)
{
    static_assert(sizeof(meta->firmwareSignature) == 64);
    static_assert(sizeof(meta->metadataSignature) == 64);

    const char* magic = "_M_E_T_A_D_A_T_A";

    // Magic must be correct
    if (0 != memcmp(meta->magic, magic, sizeof(meta->magic)))
    {
        return false;
    }

    const uint32_t fwStart = sec.startAddress + sizeof(Metadata_t);
    const uint32_t fwEnd = sec.startAddress + sec.data.size();

    // Start address is somewhere ok
    if (!InRange(meta->startAddress, fwStart, fwEnd))
    {
        return false;
    }

    return true;
}

static void TrySignSection(HexFile::Section& sec, const uint8_t* seed)
{
    Metadata_t* const meta = (Metadata_t* const)sec.data.data();

    if (CheckMetadataMem(meta, sec))
    {
        const uint32_t fwStart = meta->startAddress;
        const uint32_t fwEnd = sec.startAddress + sec.data.size();
        const uint32_t fwSize = fwEnd - fwStart;

        // Set the actual HEX size to metadata
        meta->firmwareSize = fwSize;

        const uint32_t startOffset = fwStart - sec.startAddress;
        const uint8_t* secDataPtr = sec.data.data();
        const uint8_t* fwData = &secDataPtr[startOffset];
        
        uint8_t pubKey[32];
        uint8_t privKey[64];
        ed25519_create_keypair(pubKey, privKey, seed);

        // Sign both the firmware and the metadata
        ed25519_sign(meta->firmwareSignature, fwData, fwSize, pubKey, privKey);
        ed25519_sign(meta->metadataSignature, (const uint8_t*)(meta), sizeof(Metadata_t) - 64U, pubKey, privKey);
    }
    else
    {
        std::cout << "Metadata entry at 0x" << std::hex << sec.startAddress << std::dec << " not valid!" << std::endl;
    }
}

static void VerifySectionSignature(const HexFile::Section& sec, const uint8_t* pubKey)
{
    const Metadata_t* const meta = (const Metadata_t* const)sec.data.data();

    int metaOk = ed25519_verify(meta->metadataSignature, sec.data.data(), sizeof(Metadata_t) - 64U, pubKey);

    if (metaOk == 0)
    {
        std::cout << "Metadata signature check failed" << std::endl;
        return;
    }

    uint32_t fwOffset = meta->startAddress - sec.startAddress;
    int fwOk = ed25519_verify(meta->firmwareSignature, &sec.data.data()[fwOffset], meta->firmwareSize, pubKey);

    if (metaOk == 0)
    {
        std::cout << "Firmware signature check failed" << std::endl;
        return;
    }

    std::cout << "Firmware signature CRC32: " << Crc32Str(meta->firmwareSignature, 64U) << std::endl;
    std::cout << "Metadata signature CRC32: " << Crc32Str(meta->metadataSignature, 64U) << std::endl;
    std::cout << "Signed section at 0x" << std::hex << sec.startAddress << std::dec << std::endl;
}

/*----------------------------------------------------------------------------*/
/* PUBLIC FUNCTION DEFINITIONS                                                */
/*----------------------------------------------------------------------------*/

int main(int argc, char* argv[])
{
    std::cout << "hexsign v0.1" << std::endl;

    argparse::ArgumentParser parser("hexsign v0.1");
    AddArguments(parser);

    const char* arg0 = argv[0];
    const char* arg1 = "-i";
    const char* arg2 = "C:/Users/mikael/Desktop/git/reliable_fw_update/new_freertos_app/build/Debug/new_freertos_app.hex";
    const char* arg3 = "-o";
    const char* arg4 = "C:/Users/mikael/Desktop/git/reliable_fw_update/new_freertos_app/build/Debug/new_freertos_app_signed.hex";
    const char* arg5 = "-k";
    const char* arg6 = "C:/Users/mikael/.ssh/id_ed25519";

    const char* my_argv[] = {
        arg0,
        arg1,
        arg2,
        arg3,
        arg4,
        arg5,
        arg6,
    };

    int my_argc = sizeof(my_argv)/sizeof(my_argv[0]);

    try
    {
        parser.parse_args(my_argc, my_argv);
    }
    catch (const std::exception& err)
    {
        std::cerr << err.what() << std::endl;
        std::cerr << parser;
        return 1;
    }

    std::cout << "Input file: " << parser.get("-i") << std::endl;
    std::cout << "Output file: " << parser.get("-o") << std::endl;
    std::cout << "Key file: " << parser.get("-k") << std::endl;

    std::ifstream keyFile(parser.get("-k"));

    if (!keyFile.good())
    {
        std::cout << "Cannot open key file" << std::endl;
        return 1;
    }

    KeyPair keypair(keyFile);

    if (!VerifyKeys(keypair))
    {
        std::cout << "Invalid keys" << std::endl;
        return 1;
    }
    
    OutputKeySignature(keypair);

    std::ifstream inputFile(parser.get("-i"));

    if (!inputFile.good())
    {
        std::cout << "Cannot open input hex file" << std::endl;
        return 2;
    }

    HexFile hex(inputFile);

    for (size_t i = 0; i < hex.GetSectionCount(); i++)
    {
        HexFile::Section& sec = hex.GetSectionAt(i);

        std::cout << "Section" << i << ": start: 0x" << std::hex << sec.startAddress << " len: " << std::dec << sec.data.size() << std::endl;
        const uint8_t* seed = keypair.GetPrivateKey().data();
        const uint8_t* pubKey = keypair.GetPublicKey().data();
        TrySignSection(sec, seed);
        VerifySectionSignature(sec, pubKey);
    }

    std::ofstream outputFile(parser.get("-o"));

    if (!outputFile.good())
    {
        std::cout << "Cannot open output hex file" << std::endl;
        return 3;
    }

    hex.ToStream(outputFile);

    return 0;
}

/* EoF hexsign.cpp */
