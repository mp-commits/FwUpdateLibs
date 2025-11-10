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
 * generate_keyfile.cpp
 *
 * @brief Small too for generating header files containing OpenSSH public keys
*/

/*----------------------------------------------------------------------------*/
/* INCLUDE DIRECTIVES                                                         */
/*----------------------------------------------------------------------------*/

#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>

#include "argparse/argparse.hpp"
#include "keyfile/openSSH_key.hpp"

/*----------------------------------------------------------------------------*/
/* PRIVATE FUNCTION DEFINITIONS                                               */
/*----------------------------------------------------------------------------*/

static std::string MakeHexString(const std::vector<uint8_t>& vec, std::string delim = ", ")
{
    std::stringstream ss;

    ss << std::hex;
    bool first = true;
    for (const auto& byte: vec)
    {
        if (!first)
        {
            ss << delim;
        }
        first = false;
        ss << "0x" << std::setw(2) << std::setfill('0') << (int)(byte);
    }
    
    return ss.str();
}

/*----------------------------------------------------------------------------*/
/* PUBLIC FUNCTION DEFINITIONS                                                */
/*----------------------------------------------------------------------------*/

int main(int argc, const char* argv[])
{
    argparse::ArgumentParser parser("generate_keyfile", "v0.1");

    parser.add_argument("-i", "--input")
        .help("Input OpenSSH key pair file");

    parser.add_argument("-o", "--output")
        .help("Output header file");

    parser.parse_args(argc, argv);

    std::ifstream keyFile(parser.get("-i"));
    KeyPair keypair(keyFile);
    
    std::ofstream output(parser.get("-o"));

    output << "#ifndef __GENERATED_KEYFILE__\n";
    output << "#define __GENERATED_KEYFILE__\n";
    output << "const unsigned char generated_public_key[] = {" << MakeHexString(keypair.GetPublicKey()) << "};\n";
    output << "#endif\n";

    output.close();

    return 0;
}

/* EoF generate_keyfile.cpp */
