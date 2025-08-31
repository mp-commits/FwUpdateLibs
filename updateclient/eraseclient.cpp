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
 * eraseclient.cpp
 *
 * @brief Simple client for erasing update slots on the target device
*/

/*----------------------------------------------------------------------------*/
/* INCLUDE DIRECTIVES                                                         */
/*----------------------------------------------------------------------------*/

#include "client.hpp"
#include "udpsocket.hpp"

#include "argparse/argparse.hpp"
#include "updateserver/protocol.h"

#include <fstream>
#include <string>
#include <vector>

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
    parser.add_argument("-a", "--address")
        .help("Destination IP address")
        .required();

    parser.add_argument("-p", "--port")
        .help("Destination IP port")
        .required();
    
    parser.add_argument("--localport")
        .help("Optional local IP port if different from remote port");

    parser.add_argument("slot")
        .help("Slot index to erase")
        .scan<'i', int>();
}

/*----------------------------------------------------------------------------*/
/* PUBLIC FUNCTION DEFINITIONS                                                */
/*----------------------------------------------------------------------------*/

int main(int argc, const char* argv[])
{
    argparse::ArgumentParser parser("Slot erase tool v0.1");
    AddArguments(parser);

    std::string ip;
    int localPort = 0;
    int remotePort = 0;
    int slot = 0;

    try
    {
        parser.parse_args(argc, argv);

        std::string localPortStr;
        std::string remotePortStr;

        ip = parser.get("-a");
        localPortStr = parser.get("-p");
        slot = parser.get<int>("slot");

        try
        {
            remotePortStr = parser.get("--localport");
        }
        catch(...)
        {
            remotePortStr = localPortStr;
        }

        localPort = std::stoi(localPortStr);
        remotePort = std::stoi(remotePortStr);
    }
    catch (const std::exception& err)
    {
        std::cerr << err.what() << std::endl;
        std::cerr << parser;
        return -1;
    }

    UdpSocket socket(localPort);
    socket.SetRemoteAddress(ip.c_str(), remotePort);
    UpdateClient client(socket);

    if ((slot < 0) || (slot > 255))
    {
        std::cerr << "Slot index must fit into one byte: " << slot << std::endl;
        return -2;
    }

    std::cout << "Writing slot erase request for slot " << slot << std::endl;
    client.WriteDataById(PROTOCOL_DATA_ID_ERASE_SLOT, {(uint8_t)slot});

    return 0;
}

/* EoF eraseclient.cpp */
