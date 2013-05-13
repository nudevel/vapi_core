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


//=============================================================================
// Includes
//=============================================================================
#include "vapi_core_local.h"
#include "vapi_core.h"

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <errno.h>
#include <pthread.h>


//=============================================================================
// Local Macro/Type/Enumeration/Structure Definitions
//=============================================================================
#define DBG_MSG(fmt,args...)
#define LOG_MSG(fmt,args...) fprintf(stdout, "[VAPI_CORE][LOG][%s] " fmt, __FUNCTION__, ##args)
#define ERR_MSG(fmt,args...) fprintf(stderr, "[VAPI_CORE][ERR][%s] " fmt, __FUNCTION__, ##args)
#define NOT_IMPLEMENTED ERR_MSG("Not Implemented: %s:%04d\n", __FILE__, __LINE__);

typedef struct
{
    int32_t api_id;
    uint32_t arg_len;
    int err_code, errsv;
} _vapi_core_hdr_t;

typedef struct
{
    int sock;
} _vapi_core_t;


//=============================================================================
// Local Function/Variable Implementations
//=============================================================================


//=============================================================================
// Global Function/Variable Implementations
//=============================================================================
int32_t vapi_core_open(uint16_t dstport)
{
    int err_code = 0, line = 0, errsv = 0;
    _vapi_core_t *p_fd = NULL;
    struct sockaddr_in addr;
    int opt;

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(dstport);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    p_fd = calloc( 1, sizeof(_vapi_core_t) );
    if( !p_fd ){ line = __LINE__; goto _err_end_; }

    p_fd->sock = socket(AF_INET, SOCK_STREAM, 0);
    if( p_fd->sock==-1 ){ line = __LINE__; errsv = errno; goto _err_end_; }

    opt = 1;
    err_code = setsockopt( p_fd->sock, IPPROTO_TCP, TCP_NODELAY, (char *)&opt, sizeof(opt) );
    if( err_code!=0 ){ line = __LINE__; errsv = errno;  goto _err_end_; }

    while( (err_code = connect(p_fd->sock, (struct sockaddr*)&addr, sizeof(addr)) ) == -1 ){
        LOG_MSG("connecting ...\n");
        sleep(1);
    }

    return (int32_t)p_fd;

  _err_end_:
    if( line ) ERR_MSG("line=%d\n", line);
    if( errsv ) ERR_MSG("errsv=%d\n", errsv);
    if( err_code ) ERR_MSG("err_code=%d\n", err_code);

    if( p_fd && (p_fd->sock > 0) ) close(p_fd->sock);
    if( p_fd ) free( p_fd );
    
    return -1;
}

int32_t vapi_core_close(int32_t fd)
{
    int err_code = 0, line = 0, errsv = 0;
    _vapi_core_t *p_fd = NULL;

    if( fd == 0  ||  fd == -1 ){ line = __LINE__; goto _err_end_; }
    p_fd = (_vapi_core_t*)fd;

    err_code = close( p_fd->sock );
    if( err_code!=0 ){ line = __LINE__; errsv = errno;  goto _err_end_; }

    free( p_fd );

    return 0;

  _err_end_:
    if( line ) ERR_MSG("line=%d\n", line);
    if( errsv ) ERR_MSG("errsv=%d\n", errsv);
    if( err_code ) ERR_MSG("err_code=%d\n", err_code);

    return -1;
}

int32_t vapi_core_invoke(int32_t fd, int32_t api_id, void* p_arg, uint32_t arg_len)
{
    int err_code = 0, line = 0, errsv = 0;
    _vapi_core_t *p_fd = NULL;
    ssize_t size = -1;
    _vapi_core_hdr_t hdr;
    

    if( fd == 0  ||  fd == -1 ){ line = __LINE__; goto _err_end_; }
    p_fd = (_vapi_core_t*)fd;

    // send header
    memset(&hdr, 0, sizeof(hdr));
    hdr.api_id = api_id;
    hdr.arg_len = arg_len;
    size = _vapi_core_send( p_fd->sock, &hdr, sizeof(hdr), MSG_NOSIGNAL );
    if( size < 0 ){ line = __LINE__; errsv = errno; goto _err_end_; }
    else if( size != sizeof(hdr) ){ line = __LINE__; goto _err_end_; }

    // send data
    if( hdr.arg_len ){
        size = _vapi_core_send( p_fd->sock, p_arg, hdr.arg_len, MSG_NOSIGNAL );
        if( size < 0 ){ line = __LINE__; errsv = errno; goto _err_end_; }
        else if( size != hdr.arg_len ){ line = __LINE__; goto _err_end_; }
    }

    // recv header
    size = _vapi_core_recv( p_fd->sock, &hdr, sizeof(hdr), 0 );
    if( size < 0 ){ line = __LINE__; errsv = errno; goto _err_end_; }
    else if( size == 0 ){ line = __LINE__; goto _err_end_; }
    else if( size != sizeof(hdr) ){ line = __LINE__; goto _err_end_; }

    // recv data
    if( hdr.arg_len ){
        size = _vapi_core_recv( p_fd->sock, p_arg, hdr.arg_len, 0 );
        if( size < 0 ){ line = __LINE__; errsv = errno; goto _err_end_; }
        else if( size == 0 ){ line = __LINE__; goto _err_end_; }
        else if( size != hdr.arg_len ){ line = __LINE__; goto _err_end_; }
    }

    if( hdr.err_code != 0 ){ line = __LINE__; errsv = hdr.errsv; goto _err_end_; }

    return 0;

  _err_end_:
    if( line ) ERR_MSG("line=%d\n", line);
    if( errsv ) ERR_MSG("errsv=%d\n", errsv);
    if( err_code ) ERR_MSG("err_code=%d\n", err_code);

    return -1;
}


