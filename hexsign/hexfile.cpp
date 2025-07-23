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
 * hexfile.cpp
 *
 * @brief {Short description of the source file}
*/

/*----------------------------------------------------------------------------*/
/* INCLUDE DIRECTIVES                                                         */
/*----------------------------------------------------------------------------*/

#include "hexfile.hpp"
#include <exception>
#include <sstream>
#include <iomanip>

/*----------------------------------------------------------------------------*/
/* PRIVATE TYPE DEFINITIONS                                                   */
/*----------------------------------------------------------------------------*/

typedef struct HexRecord
{
    enum Type
    {
        HEX_DATA,
        HEX_EOF,
        HEX_EXTENDED_SEGMENT_ADDRESS,
        HEX_START_SEGMENT_ADDRESS,
        HEX_EXTENDED_LINEAR_ADDRESS,
        HEX_START_LINEAR_ADDRESS,
        HEX_ERROR
    };
    
    Type type;
    uint16_t address;
    std::vector<uint8_t> data;

    HexRecord() : type(HEX_ERROR), address(0U), data() {}
} HexRecord_t;

class HexFileLineError: public std::exception
{
public:
    HexFileLineError(size_t lineNumber, std::string msg = "")
    {
        m_number = lineNumber;
        m_msg = msg;
    }

    const char* what() const throw() override
    {
        std::stringstream ss;
        ss << "Invalid record on line " << m_number << ": " << m_msg;
        return ss.str().c_str();
    }

private:
    size_t m_number;
    std::string m_msg;
};

/*----------------------------------------------------------------------------*/
/* MACRO DEFINITIONS                                                          */
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/* VARIABLE DEFINITIONS                                                       */
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/* PRIVATE FUNCTION DEFINITIONS                                               */
/*----------------------------------------------------------------------------*/

static int CharToInt(char c, size_t lineNumber = 0)
{
    if (c >= '0' && c <= '9')
    {
        return c - '0';
    } 
    else if (c >= 'A' && c <= 'F')
    {
        return c - 'A' + 10;
    } 
    else
    {
        throw HexFileLineError(lineNumber, "Non HEX character");
    }
}

static bool VerifyChecksum(const std::vector<uint8_t> data)
{
    uint8_t checksum = 0U;

    for (const uint8_t byte: data)
    {
        checksum += byte;
    }

    return checksum == 0U;
}

static bool VerifySize(const std::vector<uint8_t> data)
{
    return (data.size() >= 5U) && (data.size() == (5U + (size_t)data.at(0U)));
}

static uint16_t GetU16BE(const uint8_t* buf)
{
    uint16_t val = ((uint16_t)(buf[0]) << 8U) + (uint16_t)(buf[1]);
    return val;
}

static uint32_t GetU32BE(const uint8_t* buf)
{
    uint32_t val = (uint32_t)(buf[3]);

    val += ((uint32_t)(buf[2]) << 8U);
    val += ((uint32_t)(buf[1]) << 16U);
    val += ((uint32_t)(buf[0]) << 24U);

    return val;
}

static HexRecord_t ReadLine(const std::string& line, size_t lineNumber)
{
    if (line.at(0) != ':')
    {
        throw HexFileLineError(lineNumber, "Does not begin with ':'");
    }
    if ((line.size() & 1U) != 1U)
    {
        throw HexFileLineError(lineNumber, "Odd number of characters following ':'");
    }

    std::string dataLine = line.substr(1);
    std::vector<uint8_t> data = {};

    // 1. Convert hex file character string to byte array

    std::string::const_iterator i = line.begin() + 1;

    while(i < line.end())
    {
        const char hi = *i++;
        const char lo = *i++;
        const uint8_t dataByte = (16 * CharToInt(hi)) + CharToInt(lo);
        data.push_back(dataByte);
    }

    // 2. Validate line

    if (!VerifyChecksum(data))
    {
        throw HexFileLineError(lineNumber, "Invalid checksum");
    }

    if (!VerifySize(data))
    {
        throw HexFileLineError(lineNumber, "Invalid size or byte count");
    }

    // 3. Convert to HexRecord_t

    HexRecord_t record;

    record.address = GetU16BE(&data.data()[1]);
    record.type = (HexRecord::Type)(data.at(3));
    record.data = std::vector<uint8_t>(data.begin() + 4, data.end() - 1);

    return record;
}

static std::vector<uint8_t> U16BE(uint16_t val)
{
    std::vector<uint8_t> bytes;
    bytes.push_back(val >> 8U);
    bytes.push_back(val & 0xFFU);
    return bytes;
}

static std::vector<uint8_t> U32BE(uint32_t val)
{
    std::vector<uint8_t> bytes;
    bytes.push_back(val >> 24U);
    bytes.push_back(val >> 16U);
    bytes.push_back(val >> 8U);
    bytes.push_back(val & 0xFFU);
    return bytes;
}

static std::string HexLine(uint16_t address, HexRecord::Type recordType, std::vector<uint8_t> data)
{
    std::vector<uint8_t> bytes;

    auto Checksum = [&bytes]() -> uint8_t
    {
        uint8_t checksum = 0;

        for (auto b: bytes)
        {
            checksum += b;
        }

        return (~checksum) + 1U;
    };

    bytes.push_back(data.size());
    bytes.push_back(address >> 8U);
    bytes.push_back(address & 0xFFU);
    bytes.push_back(recordType);
    bytes.insert(bytes.end(), data.begin(), data.end());
    bytes.push_back(Checksum());

    std::stringstream hexBytes;
    hexBytes << std::hex << std::uppercase;;
    for (auto b: bytes)
    {
        hexBytes << std::setfill('0') << std::setw(2) << (uint16_t)(b);
    }
    
    std::string line = ":";
    line += hexBytes.str();
    line += '\n';

    return line;
}

static void WriteExtendedLinearSegment(std::ostream& out, uint32_t startAddress, const std::vector<uint8_t>& data)
{
    const uint16_t extendedLinearAddress = (uint16_t)(startAddress >> 16U);
    uint32_t lineAddress = startAddress & 0xFFFFU;

    if ((lineAddress + data.size()) > 0x10000U)
    {
        throw std::runtime_error("Extended segment contains too much data");
    }

    out << HexLine(0U, HexRecord::HEX_EXTENDED_LINEAR_ADDRESS, U16BE(extendedLinearAddress));

    for (size_t i = 0; i < data.size(); i += 16)
    {
        const size_t dataLeft = data.size() - i;
        const size_t subVecSize = (dataLeft < 16U) ? dataLeft : 16U;

        std::vector<uint8_t> lineBytes(data.begin() + i, data.begin() + i + subVecSize);
        out << HexLine(i, HexRecord::HEX_DATA, lineBytes);
    }
}

HexFile::Section* HexFile::FindSection(uint32_t nextAddress)
{
    for (Section& sec: m_sections)
    {
        const uint32_t nextSecAddress = sec.startAddress + sec.data.size();
        if (nextSecAddress == nextAddress)
        {
            return &sec;
        }
    }

    return nullptr;
}

/*----------------------------------------------------------------------------*/
/* PUBLIC FUNCTION DEFINITIONS                                                */
/*----------------------------------------------------------------------------*/

void HexFile::FromStream(std::istream& input)
{
    std::string line;
    bool hexEof = false;
    uint32_t addressOffset = 0U;
    uint32_t nextAddress = UINT32_MAX;

    size_t lineNumber = 0U;
    while (!input.eof() && !hexEof)
    {
        input >> line;
        lineNumber++;

        HexRecord_t record = ReadLine(line, lineNumber);

        if (record.type == HexRecord::HEX_DATA)
        {
            const uint32_t fullAddress = addressOffset + record.address;

            Section* sec = FindSection(fullAddress);
            if (sec == nullptr)
            {
                Section newSec;
                newSec.startAddress = fullAddress;
                newSec.data.insert(newSec.data.end(), record.data.begin(), record.data.end());
                m_sections.push_back(newSec);
            }
            else
            {
                sec->data.insert(sec->data.end(), record.data.begin(), record.data.end());
            }
        }
        else if (record.type == HexRecord::HEX_EOF)
        {
            hexEof = true;
        }
        else if (record.type == HexRecord::HEX_EXTENDED_SEGMENT_ADDRESS)
        {
            if (record.data.size() != 2U)
            {
                throw HexFileLineError(lineNumber, "Extended linear address record not 2 bytes");
            }

            const uint16_t extendedSegmentAddress = GetU16BE(record.data.data());
            addressOffset = (uint32_t)(extendedSegmentAddress) * 16U;
        }
        else if (record.type == HexRecord::HEX_EXTENDED_LINEAR_ADDRESS)
        {
            if (record.data.size() != 2U)
            {
                throw HexFileLineError(lineNumber, "Extended linear address record not 2 bytes");
            }

            const uint16_t extendedLinearAddress = GetU16BE(record.data.data());
            addressOffset = (uint32_t)(extendedLinearAddress) << 16U;
        }
        else if (record.type == HexRecord::HEX_START_LINEAR_ADDRESS)
        {
            if (record.data.size() != 4U)
            {
                throw HexFileLineError(lineNumber, "Start linear address record not 4 bytes");
            }

            m_startLinearAddressSet = true;
            m_startLinearAddress = GetU32BE(record.data.data());
        }
        else
        {
            throw HexFileLineError(lineNumber, "Unsupported record type");
        }
    }
    
}

void HexFile::ToStream(std::ostream& output)
{
    constexpr uint32_t extendedSegmentSize = 0x10000U;

    for (const auto& sec: m_sections)
    {
        size_t writePos = 0U;

        while (writePos < sec.data.size())
        {
            const uint32_t segmentAddress = sec.startAddress + writePos;
            const uint32_t maxLen = extendedSegmentSize - (segmentAddress & 0xFFFFU);

            const size_t subVecLen = (writePos + maxLen) > sec.data.size() ? (sec.data.size() - writePos) : maxLen;
            const std::vector<uint8_t> segmentData(sec.data.begin() + writePos, sec.data.begin() + writePos + subVecLen);

            WriteExtendedLinearSegment(output, segmentAddress, segmentData);

            writePos += subVecLen;
        }
    }

    if (m_startLinearAddressSet)
    {
        output << HexLine(0U, HexRecord::HEX_START_LINEAR_ADDRESS, U32BE(m_startLinearAddress));
    }

    output << HexLine(0U, HexRecord::HEX_EOF, {});
}

/* EoF hexfile.cpp */
