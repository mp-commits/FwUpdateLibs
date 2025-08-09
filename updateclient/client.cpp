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
 * client.cpp
 *
 * @brief Update client
*/

/*----------------------------------------------------------------------------*/
/* INCLUDE DIRECTIVES                                                         */
/*----------------------------------------------------------------------------*/

#include "client.hpp"
#include "updateserver/protocol.h"
#include <iostream>

/*----------------------------------------------------------------------------*/
/* MACRO DEFINITIONS                                                          */
/*----------------------------------------------------------------------------*/

constexpr size_t UDP_MAX_PAYLOAD_SIZE = 512;

/*----------------------------------------------------------------------------*/
/* PRIVATE FUNCTION DEFINITIONS                                               */
/*----------------------------------------------------------------------------*/

static void PutU32Be(std::vector<uint8_t>& vec, uint32_t val)
{
    vec.push_back((uint8_t)(val >> 24));
    vec.push_back((uint8_t)(val >> 16));
    vec.push_back((uint8_t)(val >> 8));
    vec.push_back((uint8_t)(val));
}

static bool IsPositiveTransferResponse(const std::vector<uint8_t>& res)
{
    if (res.size() != 3U)
    {
        return false;
    }

    if ((res.at(0) == 0x00U) &&
        (res.at(1) == 0x00U) &&
        (res.at(2) == PROTOCOL_ACK_OK))
    {
        return true;
    }

    return false;
}

static bool IsPositiveProtocolResponse(const std::vector<uint8_t>& res, uint8_t sid)
{
    if (res.size() < 2U)
    {
        return false;
    }

    if ((res.at(0) == sid) &&
        (res.at(1) == PROTOCOL_ACK_OK))
    {
        return true;
    }

    return false;
}

static std::vector<uint8_t> AnySubvector(const std::vector<uint8_t>& vec, size_t pos, size_t size)
{
    const size_t remaining = vec.size() - pos;
    const size_t subVecLen = (size < remaining) ? size : remaining;
    return std::vector<uint8_t>(vec.begin() + pos, vec.begin() + pos + subVecLen);
}

std::vector<uint8_t> UpdateClient::_SendRecv(const std::vector<uint8_t>& req)
{
    m_sock.Send(req);
    return m_sock.Recv();
}

std::vector<uint8_t> UpdateClient::_Request(const std::vector<uint8_t>& req)
{
    std::vector<uint8_t> transferResponse;

    constexpr uint32_t maxPayloadSize = (UDP_MAX_PAYLOAD_SIZE - 1);

    if (req.size() < maxPayloadSize)
    {
        // Transfer fits into a single UDP packet
        std::vector<uint8_t> transfer = {TRANSFER_SINGLE_PACKET};
        transfer.insert(transfer.end(), req.begin(), req.end());
        transferResponse = _SendRecv(transfer);
    }
    else
    {
        // Transfer has to divided into multiple packets
        std::vector<uint8_t> init = {TRANSFER_MULTI_PACKET_INIT};
        PutU32Be(init, req.size());

        if (IsPositiveTransferResponse(_SendRecv(init)))
        {
            for (size_t i = 0; i < req.size(); i += maxPayloadSize)
            {
                std::vector<uint8_t> payload = AnySubvector(req, i, maxPayloadSize);
                payload.insert(payload.begin(), TRANSFER_MULTI_PACKET_TRANSFER);
                if (!IsPositiveTransferResponse(_SendRecv(payload)))
                {
                    std::cerr << "Multi packet transfer init failed" << std::endl;
                    return {};
                }
            }

            std::vector<uint8_t> end = {TRANSFER_MULTI_PACKET_END};
            transferResponse = _SendRecv(end);
        }
        else
        {
            std::cerr << "Multi packet transfer init failed" << std::endl;
            return {};
        }
    }
    
    if (transferResponse.size() < 2)
    {
        std::cerr << "Invalid transfer response from server" << std::endl;
        return {};
    }

    // Remove transfer control byte
    return std::vector<uint8_t>(transferResponse.begin() + 1, transferResponse.end());
}

/*----------------------------------------------------------------------------*/
/* PUBLIC FUNCTION DEFINITIONS                                                */
/*----------------------------------------------------------------------------*/

bool UpdateClient::Ping()
{
    const auto response = _Request({PROTOCOL_SID_PING});
    return IsPositiveProtocolResponse(response, PROTOCOL_SID_PING);
}

std::vector<uint8_t> UpdateClient::ReadDataById(uint8_t id)
{
    const auto res = _Request({PROTOCOL_SID_READ_DATA_BY_ID, id});

    if (IsPositiveProtocolResponse(res, PROTOCOL_SID_READ_DATA_BY_ID))
    {
        if (res.size() < 3U)
        {
            std::cerr << "Invalid read data id response" << std::endl;
            return {};
        }
        return std::vector<uint8_t>(res.begin() + 2, res.end());
    }

    std::cerr << "Negative read data id response" << std::endl;
    return {};
}

bool UpdateClient::WriteDataById(uint8_t id, const std::vector<uint8_t>& data)
{
    std::vector<uint8_t> req = {PROTOCOL_SID_WRITE_DATA_BY_ID, id};
    req.insert(req.end(), data.begin(), data.end());
  
    if (IsPositiveProtocolResponse(_Request(req), PROTOCOL_SID_WRITE_DATA_BY_ID))
    {
        return true;
    }

    std::cerr << "Negative write data id response" << std::endl;
    return false;
}

bool UpdateClient::PutMetadata(const Metadata_t& metadata)
{
    std::vector<uint8_t> req = {PROTOCOL_SID_PUT_METADATA};

    const uint8_t* pMeta = (const uint8_t*)(&metadata);
    for (size_t i = 0; i < sizeof(metadata); i++)
    {
        req.push_back(pMeta[i]);
    }

    if (IsPositiveProtocolResponse(_Request(req), PROTOCOL_SID_PUT_METADATA))
    {
        return true;
    }

    std::cerr << "Negative put meatadata id response" << std::endl;
    return false;
}

bool UpdateClient::PutFragment(const Fragment_t& fragment)
{
    std::vector<uint8_t> req = {PROTOCOL_SID_PUT_FRAGMENT};

    const uint8_t* pFrag = (const uint8_t*)(&fragment);
    for (size_t i = 0; i < sizeof(fragment); i++)
    {
        req.push_back(pFrag[i]);
    }

    if (IsPositiveProtocolResponse(_Request(req), PROTOCOL_SID_PUT_FRAGMENT))
    {
        return true;
    }

    std::cerr << "Negative put fragment id response" << std::endl;
    return false;
}

/* EoF client.cpp */
