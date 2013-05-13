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

#include <pthread.h>
#include <sys/time.h>

//=============================================================================
// Local Macro/Type/Enumeration/Structure Definitions
//=============================================================================
#define DBG_MSG(fmt,args...)
#define LOG_MSG(fmt,args...) fprintf(stdout, "[SUB][LOG][%s] " fmt, __FUNCTION__, ##args)
#define ERR_MSG(fmt,args...) fprintf(stderr, "[SUB][ERR][%s] " fmt, __FUNCTION__, ##args)
#define NOT_IMPLEMENTED ERR_MSG("Not Implemented: %s:%04d\n", __FILE__, __LINE__);

typedef int (*api_handler_t)(void* p_arg, uint32_t arg_len);


//=============================================================================
// Local Function Prototypes
//=============================================================================
static callback_t vapi_test03_cb(int val, void *p_cookie);
static api_handler_t api_id_test01_handler(void* p_arg, uint32_t arg_len);
static api_handler_t api_id_test02_handler(void* p_arg, uint32_t arg_len);
static api_handler_t api_id_test03_handler(void* p_arg, uint32_t arg_len);
static vapi_core_sub_handler_t root_handler(int32_t api_id, void* p_arg, uint32_t arg_len, void *p_cookie);
static void* _test03_thread(uint32_t *p_arg);
static uint32_t _get_mtime(void);


//=============================================================================
// Function/Variable Implementations
//=============================================================================

//------------------------------------------------------------
// API Implementations
//------------------------------------------------------------
int32_t test01(uint32_t set_val, uint32_t *p_get_val)
{
    int err_code = 0;

    *p_get_val = 10;

    LOG_MSG("test01(%u, %u): err_code=%d .\n", set_val, *p_get_val, err_code);
    return err_code;
}

int32_t test02(uint8_t *p_buff, uint32_t len)
{
    int err_code = 0;
    int i;

    for(i=0; i<len; ++i)
      p_buff[i] = (uint8_t)i;

    LOG_MSG("test02(0x%08x, %u): err_code=%d .\n", (int)p_buff, len, err_code);
    return err_code;
}

int32_t test03(uint32_t set_val, callback_t cb, void *p_cookie)
{
    uint32_t *p_arg;
    int err_code, line;
    pthread_t       thrd;
    pthread_attr_t  thrd_attr;

    p_arg = malloc(8);
    p_arg[0] = (uint32_t)cb;
    p_arg[1] = (uint32_t)p_cookie;
    err_code = pthread_attr_init( &thrd_attr );
    if( err_code!=0 ){ line = __LINE__; goto _err_end_; }
    err_code = pthread_attr_setdetachstate(&thrd_attr , PTHREAD_CREATE_DETACHED);
    if( err_code!=0 ){ line = __LINE__; goto _err_end_; }
    err_code = pthread_create( &thrd, &thrd_attr, (void*)_test03_thread, (void*)p_arg);
    if( err_code!=0 ){ line = __LINE__; goto _err_end_; }
    err_code = pthread_attr_destroy( &thrd_attr );
    if( err_code!=0 ){ line = __LINE__; goto _err_end_; }

    LOG_MSG("test03(%u, 0x%08x, 0x%08x): err_code=%d .\n", set_val, (int)cb, (int)p_cookie, err_code);
    return 0;

  _err_end_:
    if( line ) ERR_MSG("line=%d\n", line);
    if( err_code ) ERR_MSG("err_code=%d\n", err_code);
    return -1;
}

void test04(void)
{
    LOG_MSG("test04(): .\n");
}

//------------------------------------------------------------
// Virtual Callback Function Implementations
//------------------------------------------------------------
static callback_t vapi_test03_cb(int val, void *p_cookie)
{
    int err_code;
    int fd_cb;
    struct _my_data_t {
        uint16_t port_cb;
        void*    p_cookie;
    };
    struct _my_data_t *p_my_data = (struct _my_data_t*)p_cookie;
    struct any_structure_cb_t arg = { val,  p_my_data->p_cookie};
    uint32_t stime = _get_mtime();

    fd_cb = vapi_core_open(p_my_data->port_cb);
    err_code = vapi_core_invoke(fd_cb, api_id_test03, &arg, sizeof(arg));
    LOG_MSG("vapi_test03_cb(%d, 0x%08x): err_code=%d:%d, %u msec.\n", arg.val, (int)arg.p_cookie, err_code, (int)arg.err_code, _get_mtime() - stime);
    vapi_core_close(fd_cb);
    free(p_my_data);

    return arg.err_code;
}

//------------------------------------------------------------
// API Handler Implementations
//------------------------------------------------------------
static api_handler_t api_id_test01_handler(void* p_arg, uint32_t arg_len)
{
    int err_code = 0;
    struct any_structure_01_t *p_struct;

    p_struct = (struct any_structure_01_t*)p_arg;
    p_struct->err_code = test01( p_struct->set_val, &p_struct->get_val);

    return (api_handler_t)err_code;
}

static api_handler_t api_id_test02_handler(void* p_arg, uint32_t arg_len)
{
    return (api_handler_t)test02( (uint8_t*)p_arg, arg_len );
}

static api_handler_t api_id_test03_handler(void* p_arg, uint32_t arg_len)
{
    int err_code = 0;
    struct any_structure_03_t *p_struct;
    struct _my_data_t {
        uint16_t port_cb;
        void*    p_cookie;
    };
    struct _my_data_t *p_my_data = malloc( sizeof(struct _my_data_t) );

    p_struct = (struct any_structure_03_t*)p_arg;
    p_my_data->port_cb = (uint16_t)( (int)p_struct->cb & 0x0000ffff ); /* technique */
    p_my_data->p_cookie = p_struct->p_cookie;
    p_struct->err_code = test03( p_struct->set_val, (callback_t)vapi_test03_cb /* virtual cb */, (void*)p_my_data );

    return (api_handler_t)err_code;
}

static api_handler_t api_id_test04_handler(void* p_arg, uint32_t arg_len)
{
    test04();
    return NULL;
}

//------------------------------------------------------------
// Root Handler Implementations
//------------------------------------------------------------
static api_handler_t api_handler_table[api_id_max] = {
    NULL,
    (api_handler_t)api_id_test01_handler,
    (api_handler_t)api_id_test02_handler,
    (api_handler_t)api_id_test03_handler,
    (api_handler_t)api_id_test04_handler,
};

static vapi_core_sub_handler_t root_handler(int32_t api_id, void* p_arg, uint32_t arg_len, void *p_cookie)
{
    if( api_id > api_id_min  &&  api_id < api_id_max  &&  api_handler_table[api_id] ){
        return (vapi_core_sub_handler_t)api_handler_table[api_id](p_arg, arg_len);
    } else {
        return (vapi_core_sub_handler_t)-1;
    }
}

//------------------------------------------------------------
// Main Function Implementations
//------------------------------------------------------------
int main(int argc, char *argv[])
{
    int err_code = 0, line = 0;
    int fd = 0;

    fd = vapi_core_sub_open(TEST_PORT, (vapi_core_sub_handler_t)root_handler, NULL);
    if( fd == -1 ){ line = __LINE__; goto _err_end_; }

    LOG_MSG("Please type 'x' to exit.\n");
    while( getchar() != 'x' );

    err_code = vapi_core_sub_close(fd);
    if( err_code == -1 ){ line = __LINE__; goto _err_end_; }

    return 0;

    
  _err_end_:
    if( line ) ERR_MSG("line=%d\n", line);
    if( err_code ) ERR_MSG("err_code=%d\n", err_code);

    vapi_core_sub_close(fd);
    
    return -1;
}

//------------------------------------------------------------
// Others
//------------------------------------------------------------
static void* _test03_thread(uint32_t *p_arg)
{
    callback_t cb = (callback_t)p_arg[0];
    void *p_cookie = (void*)p_arg[1];

    cb(1, p_cookie);
    free(p_arg);

    return NULL;
}

static uint32_t _get_mtime(void)
{
    struct timeval tv;
    if( gettimeofday(&tv, NULL) == 0 )
      return (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
    else
      return 0;
}

