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
#include "argparse/argparse.hpp"
#include "hexfile.hpp"
#include "fragmentstore/fragmentstore.h"

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
    const char* arg6 = "keyfile";

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

    std::ifstream inputFile(parser.get("-i"));

    HexFile input;
    input.FromStream(inputFile);

    for (size_t i = 0; i < input.GetSectionCount(); i++)
    {
        const HexFile::Section& sec = input.GetSectionAt(i);

        std::cout << "Section" << i << ": start: " << std::hex << sec.startAddress << " len: " << sec.data.size() << std::dec << std::endl;
    }

    return 0;
}

/* EoF hexsign.cpp */
