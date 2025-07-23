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
 * hexfile.hpp
 *
 * @brief {Short description of the source file}
*/

#ifndef HEXFILE_H_
#define HEXFILE_H_

/*----------------------------------------------------------------------------*/
/* INCLUDE DIRECTIVES                                                         */
/*----------------------------------------------------------------------------*/

#include <cstdint>
#include <istream>
#include <vector>

/*----------------------------------------------------------------------------*/
/* PUBLIC TYPE DEFINITIONS                                                    */
/*----------------------------------------------------------------------------*/

class HexFile 
{
public:
    struct Section
    {
        uint32_t startAddress;
        std::vector<uint8_t> data;

        Section(): startAddress(0U), data() {}
    };

    HexFile() : 
        m_sections({}), 
        m_startLinearAddress(0U), 
        m_startLinearAddressSet(false) 
        {}
    HexFile(std::istream& input) : 
        m_sections({}), 
        m_startLinearAddress(UINT32_MAX),
        m_startLinearAddressSet(false)
        {
            FromStream(input);
        }
    ~HexFile() {}

    void FromStream(std::istream& input);
    void ToStream(std::ostream& output);

    size_t GetSectionCount() const
    {
        return m_sections.size();
    }

    Section& GetSectionAt(size_t index)
    {
        return m_sections.at(index);
    }

private:
    Section* FindSection(uint32_t nextAddress);

    std::vector<Section> m_sections;
    uint32_t m_startLinearAddress;
    bool m_startLinearAddressSet;
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

/* EoF hexfile.hpp */

#endif /* HEXFILE_H_ */
