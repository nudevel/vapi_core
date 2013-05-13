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
#include "common.h"
#include "vapi_core.h"
#include "vapi_core_sub.h"

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include <errno.h>
#include <pthread.h>
#include <sys/time.h>


//=============================================================================
// Local Macro/Type/Enumeration/Structure Definitions
//=============================================================================
#define DBG_MSG(fmt,args...)
#define LOG_MSG(fmt,args...) fprintf(stdout, "[HOST][LOG][%s] " fmt, __FUNCTION__, ##args)
#define ERR_MSG(fmt,args...) fprintf(stderr, "[HOST][ERR][%s] " fmt, __FUNCTION__, ##args)
#define NOT_IMPLEMENTED ERR_MSG("Not Implemented: %s:%04d\n", __FILE__, __LINE__);

typedef int (*cb_handler_t)(void* p_arg, uint32_t arg_len);


//=============================================================================
// Local Function/Variable Prototypes
//=============================================================================
static callback_t test03_cb(int val, void *p_cookie);
static cb_handler_t test03_cb_handler(void* p_arg, uint32_t arg_len);
static vapi_core_sub_handler_t root_cb_handler(int32_t api_id, void* p_arg, uint32_t arg_len, void *p_cookie);
static uint32_t _get_mtime(void);
static callback_t test03_cb_handler_table[1];
static cb_handler_t cb_handler_table[api_id_max];


//=============================================================================
// Function/Variable Implementations
//=============================================================================

//------------------------------------------------------------
// Virtual API Implementations
//------------------------------------------------------------
int32_t vapi_test01(int fd, uint32_t set_val /* in */, uint32_t *p_get_val /* out */)
{
    struct any_structure_01_t arg = { set_val, 0, 0 };
    int err_code;
    uint32_t stime = _get_mtime();

    err_code = vapi_core_invoke( fd, api_id_test01, &arg, sizeof(arg) );
    if( err_code == 0 ) *p_get_val = arg.get_val;
    LOG_MSG("vapi_test01(%d, %u, %u): err_code=%d:%d, %u msec.\n", fd, set_val, *p_get_val, err_code, arg.err_code, _get_mtime() - stime);

    return arg.err_code;
}

int32_t vapi_test02(int fd, uint8_t *p_buff /* in/out */, uint32_t len /* in */)
{
    int err_code;
    uint32_t stime = _get_mtime();

    err_code = vapi_core_invoke( fd, api_id_test02, p_buff, len );
    LOG_MSG("vapi_test02(%d, 0x%08x, %u): err_code=%d, %u msec.\n", fd, (int)p_buff, len, err_code, _get_mtime() - stime);

    return err_code;
}

int32_t vapi_test03(int fd, int fd_cb, uint32_t set_val /* in */, callback_t cb /* in */, void *p_cookie /* in */)
{
    struct any_structure_03_t arg = { set_val, NULL, p_cookie, 0 };
    int err_code;
    uint32_t stime = _get_mtime();
    uint16_t port_cb;

    vapi_core_sub_get_port(fd_cb, &port_cb);
    test03_cb_handler_table[0] = (callback_t)cb;
    arg.cb = (callback_t)(port_cb & 0x0000ffff); // technique
    err_code = vapi_core_invoke( fd, api_id_test03, &arg, sizeof(arg) );
    LOG_MSG("vapi_test03(%d, %u, 0x%08x, 0x%08x): err_code=%d:%d, %u msec.\n", fd, set_val, (int)cb, (int)p_cookie, err_code, arg.err_code, _get_mtime() - stime);

    return arg.err_code;
}

void vapi_test04(int fd)
{
    int err_code;
    uint32_t stime = _get_mtime();

    err_code = vapi_core_invoke( fd, api_id_test04, NULL, 0 );
    LOG_MSG("vapi_test04(%d): err_code=%d, %u msec.\n", fd, err_code, _get_mtime() - stime);
}

//------------------------------------------------------------
// Callback Function Implementations
//------------------------------------------------------------
static callback_t test03_cb(int val, void *p_cookie)
{
    callback_t err_code = NULL;
    LOG_MSG("test03_cb(%d, 0x%08x): err_code=%d.\n", val, (int)p_cookie, (int)err_code);
    return err_code;
}

//------------------------------------------------------------
// Callback Handler Function Implementations
//------------------------------------------------------------
static cb_handler_t test03_cb_handler(void* p_arg, uint32_t arg_len)
{
    struct any_structure_cb_t *p_struct = (struct any_structure_cb_t *)p_arg;
    if( test03_cb_handler_table[0] ){
        p_struct->err_code = (callback_t)test03_cb_handler_table[0](p_struct->val, p_struct->p_cookie);
        return NULL;
    } else {
        return (cb_handler_t)-1;
    }
}

//------------------------------------------------------------
// Root Handler Function Implementations
//------------------------------------------------------------
static vapi_core_sub_handler_t root_cb_handler(int32_t api_id, void* p_arg, uint32_t arg_len, void *p_cookie)
{
    if( api_id > api_id_min  &&  api_id < api_id_max  &&  cb_handler_table[api_id] )
      return (vapi_core_sub_handler_t)cb_handler_table[api_id](p_arg, arg_len);
    else
      return (vapi_core_sub_handler_t)-1;
}

//------------------------------------------------------------
// Main Function Implementations
//------------------------------------------------------------
int main(int argc, char *argv[])
{
    int err_code = 0, line = 0;
    int fd = 0, fd_cb = 0;
    int mode = argc == 2 ? atoi(argv[1]) : 0x0F ;
    char ch;

    // open
    fd = vapi_core_open(TEST_PORT);
    if( fd == -1 ){ line = __LINE__; goto _err_end_; }

    if( mode & 0x04 ){
        fd_cb = vapi_core_sub_open(0 /* auto */, (vapi_core_sub_handler_t)root_cb_handler, NULL);
        if( fd_cb == -1 ){ line = __LINE__; goto _err_end_; }
        cb_handler_table[ api_id_test03 ] = (cb_handler_t)test03_cb_handler;
    }

  _retry_label_:
    
    // vapi_test01
    if( mode & 0x01 ){
        uint32_t set_val = 2, get_val = 0;
        err_code = vapi_test01(fd, set_val, &get_val);
    }
    
    // vapi_test02
    if( mode & 0x02 ){
        uint32_t len = 1024*1024*16;  // 16MB
        uint8_t *p_buff = calloc(1, len);
        if( p_buff ){
            err_code = vapi_test02(fd, p_buff, len);
            free(p_buff);
        } else {
            ERR_MSG("failed to calloc.\n");
        }
    }

    // vapi_test03
    if( mode & 0x04 ){
        uint32_t my_data = 0xbeafbeaf;
        err_code = vapi_test03(fd, fd_cb, 12345, (callback_t)test03_cb, (void*)&my_data);
    }

    // vapi_test04
    if( mode & 0x08 ){
        vapi_test04(fd);
    }

    do {
        LOG_MSG("Please type 'x' to exit, or 'r' to retry.\n");
        ch = getchar();
        if( ch == 'x' ) break;
        else if( ch == 'r' ) goto _retry_label_;
    } while(1);

    // close
    err_code = vapi_core_close(fd);
    if( err_code!=0 ){ line = __LINE__; goto _err_end_; }

    if( mode & 0x04 ){
        vapi_core_sub_close(fd_cb);
    }
    
    return 0;

  _err_end_:
    if( line ) ERR_MSG("line=%d\n", line);
    if( err_code ) ERR_MSG("err_code=%d\n", err_code);

    vapi_core_close(fd);

    return -1;
}

//------------------------------------------------------------
// Others
//------------------------------------------------------------
static uint32_t _get_mtime(void)
{
    struct timeval tv;
    if( gettimeofday(&tv, NULL) == 0 )
      return (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
    else
      return 0;
}

