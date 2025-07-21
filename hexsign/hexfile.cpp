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

static HexRecord_t ReadLine(const std::string line, size_t lineNumber)
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

    std::string::const_iterator i = line.begin();

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
    record.data = std::vector<uint8_t>(data.begin() + 4U, data.begin() + data.size() - 5U);

    return record;
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
            // Ignore execution start address for now
        }
        else
        {
            throw HexFileLineError(lineNumber, "Unsupported record type");
        }
    }
    
}

void HexFile::ToStream(std::ostream& output)
{
    (void)output;
}

/* EoF hexfile.cpp */
