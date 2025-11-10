/* https://www.geeksforgeeks.org/dsa/decode-encoded-base-64-string-ascii-string/
 *
 * base64.cpp
 *
 * @brief {Short description of the source file}
*/

/*----------------------------------------------------------------------------*/
/* INCLUDE DIRECTIVES                                                         */
/*----------------------------------------------------------------------------*/

#include "base64.hpp"
#include <vector>

/*----------------------------------------------------------------------------*/
/* PUBLIC FUNCTION DEFINITIONS                                                */
/*----------------------------------------------------------------------------*/

std::string Base64::Decode(const std::string& encoded)
{
    std::string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string decoded;

    std::vector<int> T(256, -1);

    for (int i = 0; i < 64; i++) 
    {
        T[base64_chars[i]] = i;
    }

    int val = 0;
    int valb = -8;

    for (char c : encoded)
    {
        if (T[c] == -1)
        {
            break;
        }

        val = (val << 6) + T[c];

        valb += 6;

        if (valb >= 0)
        {
            decoded.push_back(char((val >> valb) & 0xFF));
            valb -= 8;
        }
    }

    return decoded;
}

/* EoF base64.cpp */
