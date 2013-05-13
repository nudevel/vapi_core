/*=============================================================================

Copyright (c) 2013, Naoto Uegaki
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=============================================================================*/


#ifndef _VAPI_CORE_LOCAL_H_
#define _VAPI_CORE_LOCAL_H_

//=============================================================================
// Includes
//=============================================================================
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>

//=============================================================================
// Macro/Type/Enumeration/Structure Definitions
//=============================================================================

//=============================================================================
// Local Function/Variable Prototypes
//=============================================================================

//=============================================================================
// Local Inline Function Implementations
//=============================================================================
static inline ssize_t _vapi_core_send(int sockfd, const void *buf, size_t len, int flags)
{
    ssize_t size, sum=0;
    uint8_t *p_pos = (uint8_t*)buf;

    for(sum=0; sum<len; sum+=size){
        size = send(sockfd, (void*)(p_pos + sum), len - sum, flags);
        if( size < 0 ) return size;
    }

    return sum;
}

static inline ssize_t _vapi_core_recv(int sockfd, void *buf, size_t len, int flags)
{
    ssize_t size, sum=0;
    uint8_t *p_pos = (uint8_t*)buf;

    for(sum=0; sum<len; sum+=size){
        size = recv(sockfd, (void*)(p_pos + sum), len - sum, flags);
        if( size < 0  ||  size == 0 ) return size;
    }

    return sum;
}

#endif // _VAPI_CORE_LOCAL_H_
