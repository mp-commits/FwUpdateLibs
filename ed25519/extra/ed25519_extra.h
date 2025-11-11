/* @brief This software is a wrapper for an ed25519 library by Orson Peters <orsonpeters@gmail.com>:
 * 
 * Copyright (c) 2015 Orson Peters <orsonpeters@gmail.com>
 *
 * This software is provided 'as-is', without any express or implied warranty. In no event will the
 * authors be held liable for any damages arising from the use of this software.
 * 
 * Permission is granted to anyone to use this software for any purpose, including commercial
 * applications, and to alter it and redistribute it freely, subject to the following restrictions:
 * 
 * 1. The origin of this software must not be misrepresented; you must not claim that you wrote the
 *    original software. If you use this software in a product, an acknowledgment in the product
 *    documentation would be appreciated but is not required.
 * 
 * 2. Altered source versions must be plainly marked as such, and must not be misrepresented as
 *    being the original software.
 * 
 * 3. This notice may not be removed or altered from any source distribution.
 * 
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
