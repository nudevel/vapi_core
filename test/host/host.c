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

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>


//=============================================================================
// Local Macro/Type/Enumeration/Structure Definitions
//=============================================================================
#define DBG_MSG(fmt,args...)
#define LOG_MSG(fmt,args...) fprintf(stdout, "[EX_HOST][LOG][%s] " fmt, __FUNCTION__, ##args)
#define ERR_MSG(fmt,args...) fprintf(stderr, "[EX_HOST][ERR][%s] " fmt, __FUNCTION__, ##args)
#define NOT_IMPLEMENTED ERR_MSG("Not Implemented: %s:%04d\n", __FILE__, __LINE__);

typedef struct
{
    int alive;
    int mode;
} vapi_test_thread_t;

//=============================================================================
// Local Function/Variable Implementations
//=============================================================================

//------------------------------------------------------------
// Virtual API Implementations
//------------------------------------------------------------
static int vapi_test01(int fd, uint32_t set_val /* in */, uint32_t *p_get_val /* out */)
{
    struct any_structure_01_t arg = { set_val, 0 };
    int err_code;

    err_code = vapi_core_invoke( fd, api_id_test01, &arg, sizeof(arg) );
    if( err_code == 0 ) *p_get_val = arg.get_val;
    return err_code;
}

static int vapi_test02(int fd, uint8_t *p_buff /* in/out */, uint32_t len /* in */)
{
    int err_code;
    err_code = vapi_core_invoke( fd, api_id_test02, p_buff, len );
    return err_code;
}

//------------------------------------------------------------
// Test Function Implementations
//------------------------------------------------------------
static void* vapi_test_thread(void* p_arg)
{
    int err_code = 0, line = 0;
    vapi_test_thread_t *p_info = (vapi_test_thread_t*)p_arg;
    int cnt = 0;
    int fd = 0;

    fd = vapi_core_open(TEST_PORT);
    if( fd == -1 ){ line = __LINE__; goto _err_end_; }

    while( p_info->alive ){

        if( p_info->mode & 0x01 ){
            uint32_t set_val, get_val = 0;
            set_val = cnt;
            err_code = vapi_test01(fd, set_val, &get_val);
            if( err_code != 0 ){ line = __LINE__; goto _err_end_; }
            if( set_val != get_val ){ line = __LINE__; goto _err_end_; }
            LOG_MSG("[%5d] vapi_test01() is OK.\n", cnt);
        }

        if( p_info->mode & 0x02 ){
            uint32_t len = 1024*1024*16;  // 16MB
            uint8_t *p_buff = calloc(1, len);
            if( p_buff ){
                int i;
                err_code = vapi_test02(fd, p_buff, len);
                if( err_code != 0 ){ line = __LINE__; goto _err_end_; }
                
                for(i=0; i<len; ++i)
                  if( p_buff[i] != (uint8_t)i ){ line = __LINE__; goto _err_end_; }
                free(p_buff);
                LOG_MSG("[%5d] vapi_test02() is OK.\n", cnt);
            } else {
                ERR_MSG("failed to calloc.\n");
            }
        }
        
        //usleep(10*1000);
        cnt++;
    }

    err_code = vapi_core_close(fd);
    if( err_code!=0 ){ line = __LINE__; goto _err_end_; }

    return NULL;

  _err_end_:
    if( line ) ERR_MSG("line=%d\n", line);
    if( err_code ) ERR_MSG("err_code=%d\n", err_code);

    vapi_core_close(fd);

    return NULL;
}

static int vapi_test_start(int mode)
{
    int err_code = 0, line = 0;
    pthread_t       thrd;
    pthread_attr_t  thrd_attr;
    vapi_test_thread_t arg = { 1, mode };

    err_code = pthread_attr_init( &thrd_attr );
    if( err_code!=0 ){ line = __LINE__; goto _err_end_; }
    err_code = pthread_create( &thrd, &thrd_attr, (void*)vapi_test_thread, (void*)&arg );
    if( err_code!=0 ){ line = __LINE__; goto _err_end_; }
    err_code = pthread_attr_destroy( &thrd_attr );
    if( err_code!=0 ){ line = __LINE__; goto _err_end_; }
    
    LOG_MSG("Please type 'x' to exit.\n");
    while( getchar() != 'x' );
    arg.alive = 0;

    err_code = pthread_join( thrd, NULL );
    if( err_code!=0 ){ line = __LINE__; goto _err_end_; }

    return 0;

  _err_end_:
    if( line ) ERR_MSG("line=%d\n", line);
    if( err_code ) ERR_MSG("err_code=%d\n", err_code);

    return -1;
    
}

//=============================================================================
// Global Function/Variable Implementations
//=============================================================================
int main(int argc, char *argv[])
{
    return vapi_test_start( argc == 2 ? atoi(argv[1]) : 0x03 );
}
