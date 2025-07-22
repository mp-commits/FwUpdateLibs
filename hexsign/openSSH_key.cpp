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
 * openSSH_key.cpp
 *
 * @brief {Short description of the source file}
*/

/*----------------------------------------------------------------------------*/
/* INCLUDE DIRECTIVES                                                         */
/*----------------------------------------------------------------------------*/

#include "openSSH_key.hpp"
#include "base64.hpp"

#include <cstring>
#include <exception>
#include <stdexcept>
#include <string>

/*----------------------------------------------------------------------------*/
/* PRIVATE FUNCTION DEFINITIONS                                               */
/*----------------------------------------------------------------------------*/

static std::string ReadKeyFileContent(std::istream& input)
{
    std::string base64Content;
    std::string line;
    bool hasBegun = false;

    const std::string BEGIN_TAG = "-----BEGIN OPENSSH PRIVATE KEY-----";
    const std::string END_TAG = "-----END OPENSSH PRIVATE KEY-----";

    while(!input.eof())
    {
        std::getline(input, line);

        if (!hasBegun)
        {
            if (0 == line.compare(BEGIN_TAG))
            {
                hasBegun = true;
            }
        }
        else
        {
            if (0 == line.compare(END_TAG))
            {
                break;
            }
            else
            {
                base64Content += line;
            }
        }
    }

    return base64Content;
}

static uint32_t ReadUint32(const uint8_t* data) {
    uint32_t val = data[3];

    val += ((uint32_t)(data[2]) << 8U);
    val += ((uint32_t)(data[1]) << 16U);
    val += ((uint32_t)(data[0]) << 24);

    return val;
}

/*----------------------------------------------------------------------------*/
/* PUBLIC FUNCTION DEFINITIONS                                                */
/*----------------------------------------------------------------------------*/

void KeyPair::FromFile(std::istream& input)
{
    std::string fileData = ReadKeyFileContent(input);
    std::string binary = Base64::Decode(fileData);

    const uint8_t* p = reinterpret_cast<const uint8_t*>(binary.data());
    size_t len = binary.size();

    auto require = [&](size_t n) {
        if (p + n > reinterpret_cast<const uint8_t*>(binary.data() + len))
            throw std::runtime_error("Invalid key file");
    };

    // Check header "openssh-key-v1\0"
    const char header[] = "openssh-key-v1";
    require(sizeof(header));
    if (std::memcmp(p, header, sizeof(header)) != 0)
        throw std::runtime_error("Invalid OpenSSH header");
    p += sizeof(header);

    // Skip ciphername
    require(4); 
    uint32_t ciphername_len = ReadUint32(p); 
    p += 4;
    require(ciphername_len); 
    p += ciphername_len;

    // Skip kdfname
    require(4); 
    uint32_t kdfname_len = ReadUint32(p); 
    p += 4;
    require(kdfname_len); 
    p += kdfname_len;

    // Skip kdfoptions
    require(4); 
    uint32_t kdfoptions_len = ReadUint32(p); 
    p += 4;
    require(kdfoptions_len); 
    p += kdfoptions_len;

    // Key count
    require(4); uint32_t nkeys = ReadUint32(p); 
    p += 4;
    if (nkeys != 1)
        throw std::runtime_error("Expected exactly one key");

    // Skip public key blob
    require(4); 
    uint32_t pubkey_len = ReadUint32(p); 
    p += 4;
    require(pubkey_len); 
    p += pubkey_len;

    // Read private key block
    require(4); 
    uint32_t privkey_len = ReadUint32(p); 
    p += 4;
    require(privkey_len);
    const uint8_t* priv_start = p;
    const uint8_t* priv_end = p + privkey_len;

    // Sanity check: check two uint32 checksum fields (just skip)
    require(8); 
    p += 8;

    // Check "ssh-ed25519" string
    require(4); 
    uint32_t keytype_len = ReadUint32(p); 
    p += 4;
    require(keytype_len);

    std::string keytype(reinterpret_cast<const char*>(p), keytype_len);

    if (keytype != "ssh-ed25519")
        throw std::runtime_error("Unexpected key type: " + keytype);
    p += keytype_len;

    // Skip public key
    require(4); 
    uint32_t pub_len = ReadUint32(p); 
    p += 4;
    require(pub_len); 

    if (pub_len != 32)
        throw std::runtime_error("Expected 32-byte public key blob");

    m_publicKey = std::vector<uint8_t>(p, p + 32);

    p += pub_len;

    // Read private key
    require(4); 
    uint32_t priv_len = ReadUint32(p); 
    p += 4;
    require(priv_len);

    if (priv_len != 64)
        throw std::runtime_error("Expected 64-byte private key blob");

    m_privateKey = std::vector<uint8_t>(p, p + 64);
}

/* EoF openSSH_key.cpp */
