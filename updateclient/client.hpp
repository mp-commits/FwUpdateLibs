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
 * client.hpp
 *
 * @brief {Short description of the source file}
*/

#ifndef CLIENT_H_
#define CLIENT_H_

/*----------------------------------------------------------------------------*/
/* INCLUDE DIRECTIVES                                                         */
/*----------------------------------------------------------------------------*/

#include "udpsocket.hpp"
#include "fragmentstore/fragmentstore.h"

#include <cstdint>

/*----------------------------------------------------------------------------*/
/* PUBLIC TYPE DEFINITIONS                                                    */
/*----------------------------------------------------------------------------*/

class UpdateClient
{
public:
    UpdateClient(UdpSocket& sock): m_sock(sock) {}

    bool Ping();

    std::vector<uint8_t> ReadDataById(uint8_t id);

    bool WriteDataById(uint8_t id, const std::vector<uint8_t>& data);

    bool PutMetadata(const Metadata_t& metadata);

    bool PutFragment(const Fragment_t& fragment);

private:
    std::vector<uint8_t> _SendRecv(const std::vector<uint8_t>& req);
    std::vector<uint8_t> _Request(const std::vector<uint8_t>& req);

    UdpSocket& m_sock;
};

/*----------------------------------------------------------------------------*/
/* PUBLIC MACRO DEFINITIONS                                                   */
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/* PUBLIC VARIABLE DEFINITIONS                                                */
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/* PUBLIC FUNCTION DECLARATIONS                                               */
/*----------------------------------------------------------------------------*/

/* EoF client.hpp */

#endif /* CLIENT_H_ */
