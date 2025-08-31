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
 * udpsocket.cpp
 *
 * @brief UDP socket wrapper for windows and linux builds
*/

/*----------------------------------------------------------------------------*/
/* INCLUDE DIRECTIVES                                                         */
/*----------------------------------------------------------------------------*/

#include "udpsocket.hpp"
#include "iostream"

#include <sstream>
#include <iomanip>

/*----------------------------------------------------------------------------*/
/* PUBLIC FUNCTION DEFINITIONS                                                */
/*----------------------------------------------------------------------------*/

#ifdef _WIN32

static void PrintLastError()
{
    int code = WSAGetLastError();

    std::cerr << "Failed with error code: " << code << "\n";

    char* msg = nullptr;

    FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
        nullptr, 
        code, 
        0, 
        (LPSTR)&msg, 
        0, 
        nullptr
    );

    if (msg)
    {
        std::cerr << "Reason: " << msg;
        LocalFree(msg);
    }
}

#else

#include <cerrno>

void PrintLastError()
{
    std::cerr << "Failed with errno: " << errno << " (" << std::strerror(errno) << ")\n";
}

#endif

static std::string Vec2Str(const std::vector<uint8_t>& vec)
{
    std::stringstream ss;
    ss << std::hex << std::uppercase;

    for (auto b: vec)
    {
        ss << std::setfill('0');
        ss << std::setw(2);
        ss << (uint32_t)b;
        ss << " ";
    }

    return ss.str();
}

UdpSocket::UdpSocket(uint16_t port)
{
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

    m_dbg = false;

    m_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (m_sock == INVALID_SOCKET)
    {
        std::cerr << "Socket creation failed.\n";
    }
    else
    {
        sockaddr_in localAddr {};
        localAddr.sin_family = AF_INET;
        localAddr.sin_port = htons(port);
        localAddr.sin_addr.s_addr = INADDR_ANY;
        bind(m_sock, (sockaddr*)&localAddr, sizeof(localAddr));

        m_remoteAddr.sin_family = AF_INET;
        m_remoteAddr.sin_addr.s_addr = INADDR_ANY;
    }
}

UdpSocket::~UdpSocket()
{
#ifdef _WIN32
    closesocket(m_sock);
    WSACleanup();
#else
    close(m_sock);
#endif
}

void UdpSocket::SetRemoteAddress(const char* ipv4, uint16_t port)
{
    m_remoteAddr.sin_family = AF_INET;
    m_remoteAddr.sin_port = htons(port);
    m_remoteAddr.sin_addr.s_addr = inet_addr(ipv4);
}

void UdpSocket::Send(const std::vector<uint8_t>& data)
{
    if (m_dbg)
    {
        std::cout << "Sending " << data.size() << " bytes:" << Vec2Str(data) << std::endl;
    }

    int sent = sendto(
        m_sock, 
        (const char*)data.data(), 
        data.size(), 
        0,
        (sockaddr*)&m_remoteAddr, 
        sizeof(m_remoteAddr)
    );

    if (sent == SOCKET_ERROR)
    {
        PrintLastError();
        std::cerr << "sendto failed.\n";
    }
}

std::vector<uint8_t> UdpSocket::Recv()
{
    std::vector<uint8_t> recv;
    recv.resize(1470);

    sockaddr_in from;
    int fromLen = sizeof(from);

    int recvLen = recvfrom(
        m_sock, 
        (char*)recv.data(), 
        recv.size(), 
        0, 
        (sockaddr*)&from, 
        &fromLen
    );

    if (recvLen < 0)
    {
        PrintLastError();
        std::cerr << "recvfrom failed.\n";
        return {};
    }

    // Set remote address automatically if not set
    if (m_remoteAddr.sin_addr.s_addr == INADDR_ANY)
    {
        m_remoteAddr = from;
    }

    if (from.sin_addr.s_addr != m_remoteAddr.sin_addr.s_addr)
    {
        std::cerr << "Received from wrong address.\n";
    }

    recv.resize(recvLen);

    if (m_dbg)
    {
        std::cout << "Received " << recv.size() << " bytes:" << Vec2Str(recv) << std::endl;
    }

    return recv;
}

/* EoF udpsocket.cpp */
