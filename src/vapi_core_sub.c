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
#include "vapi_core_sub.h"

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
#define LOG_MSG(fmt,args...) fprintf(stdout, "[VAPI_CORE_SUB][LOG][%s] " fmt, __FUNCTION__, ##args)
#define ERR_MSG(fmt,args...) fprintf(stderr, "[VAPI_CORE_SUB][ERR][%s] " fmt, __FUNCTION__, ##args)
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
    int thrd_alive;
    vapi_core_sub_handler_t handler;
    void *p_cookie;
    pthread_t       thrd;
    uint16_t port;
} _vapi_core_sub_t;

typedef struct __vapi_core_sub_child_t
{
    int sock;
    vapi_core_sub_handler_t handler;
    void *p_cookie;
} _vapi_core_sub_child_t;


//=============================================================================
// Local Function/Variable Implementations
//=============================================================================
static void* _vapi_core_sub_child_thread(_vapi_core_sub_child_t *p_child)
{
    int err_code = 0, line = 0, errsv = 0;
    ssize_t size = -1;
    _vapi_core_hdr_t hdr;
    char *p_arg = NULL;
    struct timeval tv = { 0, 0 }; /* infinity. never timeout. */
    int opt;

    err_code = setsockopt(p_child->sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    if( err_code!=0 ){ line = __LINE__; errsv = errno;  goto _err_end_; }

    opt = 1;
    err_code = setsockopt( p_child->sock, IPPROTO_TCP, TCP_NODELAY, (char *)&opt, sizeof(opt) );
    if( err_code!=0 ){ line = __LINE__; errsv = errno;  goto _err_end_; }

    while( 1 ){
        // recv header
        size = _vapi_core_recv( p_child->sock, &hdr, sizeof(hdr), 0 );
        if( size < 0 ){ line = __LINE__; errsv = errno; goto _err_end_; }
        else if( size == 0 ){ break; }
        else if( size != sizeof(hdr) ){ line = __LINE__; goto _err_end_; }

        // recv data
        if( hdr.arg_len ){
            p_arg = malloc( hdr.arg_len );
            if( !p_arg ){ line = __LINE__; goto _err_end_; }

            size = _vapi_core_recv( p_child->sock, p_arg, hdr.arg_len, 0 );
            if( size < 0 ){ line = __LINE__; errsv = errno; goto _err_end_; }
            else if( size == 0 ){ break; }
            else if( size != hdr.arg_len ){ line = __LINE__; goto _err_end_; }
        }

        // call hander
        if( p_child->handler ) {
            hdr.err_code = p_child->handler(hdr.api_id, p_arg, hdr.arg_len, p_child->p_cookie);
            hdr.errsv = errno;
        } else {
            hdr.err_code = -99;
            hdr.errsv = ENXIO; /* No such device or address */
        }

        // send header
        size = _vapi_core_send( p_child->sock, &hdr, sizeof(hdr), MSG_NOSIGNAL );
        if( size < 0 ){ line = __LINE__; errsv = errno; goto _err_end_; }
        else if( size != sizeof(hdr) ){ line = __LINE__; goto _err_end_; }

        // send data
        if( hdr.arg_len ){
            size = _vapi_core_send( p_child->sock, p_arg, hdr.arg_len, MSG_NOSIGNAL );
            if( size < 0 ){ line = __LINE__; errsv = errno; goto _err_end_; }
            else if( size != hdr.arg_len ){ line = __LINE__; goto _err_end_; }
        }

        if( p_arg ){ free( p_arg ); p_arg = NULL; }
    }

    LOG_MSG("The peer(sock=0x%08x) side seems to be closed.\n", p_child->sock);

    if( p_arg ){ free( p_arg ); p_arg = NULL; }
    close(p_child->sock);
    free(p_child);

    return NULL;

  _err_end_:
    if( line ) ERR_MSG("line=%d\n", line);
    if( errsv ) ERR_MSG("errsv=%d\n", errsv);
    if( err_code ) ERR_MSG("err_code=%d\n", err_code);

    if( p_arg ){ free( p_arg ); p_arg = NULL; }
    close(p_child->sock);
    free(p_child);

    return NULL;
}

static void* _vapi_core_sub_accept_thread(_vapi_core_sub_t *p_fd)
{
    int err_code = 0, line = 0, errsv = 0;
    int sock;
    struct sockaddr_in addr;
    socklen_t len;
    pthread_t       thrd;
    pthread_attr_t  thrd_attr;
    _vapi_core_sub_child_t *p_child = NULL;

    p_fd->thrd_alive = 1;

    err_code = pthread_attr_init( &thrd_attr );
    if( err_code!=0 ){ line = __LINE__; goto _err_end_; }

    err_code = pthread_attr_setdetachstate(&thrd_attr , PTHREAD_CREATE_DETACHED);
    if( err_code!=0 ){ line = __LINE__; goto _err_end_; }

    while( p_fd->thrd_alive ){
        DBG_MSG("accepting...\n");
        p_child = NULL;
        sock = accept(p_fd->sock, (struct sockaddr*)&addr, &len);
        if( sock==-1 ){
            errsv = errno;
            switch(errsv){
              case EWOULDBLOCK /* Operation would block */:
                continue;
              default:
                ERR_MSG("failed to accept. errsv=%d\n", errsv);
                p_fd->thrd_alive = 0;
                line = __LINE__;
                goto _err_end_;
            }
        }

        p_child = calloc(1, sizeof(_vapi_core_sub_child_t));
        if( !p_child ){ line = __LINE__; goto _err_end_; }
        
        p_child->sock = sock;
        p_child->handler = p_fd->handler;
        p_child->p_cookie = p_fd->p_cookie;
        err_code = pthread_create( &thrd, &thrd_attr,
                                   (void*)_vapi_core_sub_child_thread, (void*)p_child);
        if( err_code!=0 ){ line = __LINE__; goto _err_end_; }
        LOG_MSG("The new connection(sock=0x%08x) was accepted. \n", p_child->sock);
    }

    err_code = pthread_attr_destroy( &thrd_attr );
    if( err_code!=0 ){ line = __LINE__; goto _err_end_; }

    return NULL;


  _err_end_:
    if( line ) ERR_MSG("line=%d\n", line);
    if( errsv ) ERR_MSG("errsv=%d\n", errsv);
    if( err_code ) ERR_MSG("err_code=%d\n", err_code);
    
    if( p_child ) free(p_child);
    p_fd->thrd_alive = 0;
    pthread_attr_destroy( &thrd_attr );

    return NULL;
}


//=============================================================================
// Global Function/Variable Implementations
//=============================================================================
int32_t vapi_core_sub_open(uint16_t port, vapi_core_sub_handler_t handler, const void *p_cookie)
{
    int err_code = 0, line = 0, errsv = 0;
    _vapi_core_sub_t *p_fd = NULL;
    struct sockaddr_in addr;
    pthread_attr_t  thrd_attr;
    struct timeval tv = { 2, 0 }; /* 2 sec */
    socklen_t socklen = sizeof(addr);

    p_fd = calloc( 1, sizeof(_vapi_core_sub_t) );
    if( !p_fd ){ line = __LINE__; goto _err_end_; }

    p_fd->handler = handler;

    p_fd->sock = socket(AF_INET, SOCK_STREAM, 0);
    if( p_fd->sock==-1 ){ line = __LINE__; errsv = errno; goto _err_end_; }

    err_code = setsockopt(p_fd->sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)); // for accept() timeout
    if( err_code!=0 ){ line = __LINE__; errsv = errno;  goto _err_end_; }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(port);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    err_code = bind(p_fd->sock, (struct sockaddr*)&addr, sizeof(addr));
    if( err_code!=0 ){ line = __LINE__; errsv = errno;  goto _err_end_; }

    err_code = listen(p_fd->sock, 5);
    if( err_code!=0 ){ line = __LINE__; errsv = errno;  goto _err_end_; }

    err_code = getsockname(p_fd->sock, (struct sockaddr*)&addr, &socklen );
    if( err_code!=0 ){ line = __LINE__; errsv = errno;  goto _err_end_; }
    p_fd->port = ntohs(addr.sin_port);

    err_code = pthread_attr_init( &thrd_attr );
    if( err_code!=0 ){ line = __LINE__; goto _err_end_; }
    err_code = pthread_create( &p_fd->thrd, &thrd_attr, (void*)_vapi_core_sub_accept_thread, (void*)p_fd);
    if( err_code!=0 ){ line = __LINE__; goto _err_end_; }
    err_code = pthread_attr_destroy( &thrd_attr );
    if( err_code!=0 ){ line = __LINE__; goto _err_end_; }

    return (int)p_fd;

  _err_end_:
    if( line ) ERR_MSG("line=%d\n", line);
    if( errsv ) ERR_MSG("errsv=%d\n", errsv);
    if( err_code ) ERR_MSG("err_code=%d\n", err_code);

    if( p_fd && (p_fd->sock > 0) ) close(p_fd->sock);
    if( p_fd ) free( p_fd );
    
    return -1;
}

int32_t vapi_core_sub_close(int32_t fd)
{
    int err_code = 0, line = 0, errsv = 0;
    _vapi_core_sub_t *p_fd = NULL;

    if( fd == 0  ||  fd == -1 ){ line = __LINE__; goto _err_end_; }
    p_fd = (_vapi_core_sub_t*)fd;

    p_fd->thrd_alive = 0;
    err_code = pthread_join( p_fd->thrd, NULL );
    if( err_code!=0 ){ line = __LINE__; goto _err_end_; }

    err_code = close(p_fd->sock);
    if( err_code!=0 ){ line = __LINE__; errsv = errno;  goto _err_end_; }

    free(p_fd);

    return 0;

  _err_end_:
    if( line ) ERR_MSG("line=%d\n", line);
    if( errsv ) ERR_MSG("errsv=%d\n", errsv);
    if( err_code ) ERR_MSG("err_code=%d\n", err_code);

    return -1;
}

int32_t vapi_core_sub_get_port(int32_t fd, uint16_t *p_port)
{
    int line = 0, errsv = 0;
    _vapi_core_sub_t *p_fd = NULL;

    if( fd == 0  ||  fd == -1 ){ line = __LINE__; goto _err_end_; }
    p_fd = (_vapi_core_sub_t*)fd;

    *p_port = p_fd->port;

    return 0;

  _err_end_:
    if( line ) ERR_MSG("line=%d\n", line);
    if( errsv ) ERR_MSG("errsv=%d\n", errsv);

    return -1;
}
