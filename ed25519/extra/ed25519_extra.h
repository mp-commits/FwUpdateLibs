/* MIT License
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
 * ed25519_extra.h
 *
 * @brief Extra ED25519 functions
*/

#ifndef ED25519_EXTRA_H_
#define ED25519_EXTRA_H_

#ifdef __cplusplus
extern "C" {
#endif

/*----------------------------------------------------------------------------*/
/* INCLUDE DIRECTIVES                                                         */
/*----------------------------------------------------------------------------*/

#include <stddef.h>
#include <stdint.h>

#include "sha512.h"
#include "ge.h"

/*----------------------------------------------------------------------------*/
/* PUBLIC TYPE DEFINITIONS                                                    */
/*----------------------------------------------------------------------------*/

typedef struct
{
    unsigned char signature[64];
    ge_p3 A;
    sha512_context hash;
} ed25519_multipart_t;

/*----------------------------------------------------------------------------*/
/* PUBLIC FUNCTION DECLARATIONS                                               */
/*----------------------------------------------------------------------------*/

extern int ed25519_multipart_init(ed25519_multipart_t* ctx, const unsigned char *signature, const unsigned char *public_key);
extern int ed25519_multipart_continue(ed25519_multipart_t* ctx, const unsigned char *message, size_t message_len);
extern int ed25519_multipart_end(ed25519_multipart_t* ctx);

#ifdef __cplusplus
} /* extern C */
#endif

/* EoF ed25519_extra.h */

#endif /* ED25519_EXTRA_H_ */
