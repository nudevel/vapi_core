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
#include "vapi_core_sub.h"

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>


//=============================================================================
// Local Macro/Type/Enumeration/Structure Definitions
//=============================================================================
#define DBG_MSG(fmt,args...)
#define LOG_MSG(fmt,args...) fprintf(stdout, "[EX_SUB][LOG][%s] " fmt, __FUNCTION__, ##args)
#define ERR_MSG(fmt,args...) fprintf(stderr, "[EX_SUB][ERR][%s] " fmt, __FUNCTION__, ##args)
#define NOT_IMPLEMENTED ERR_MSG("Not Implemented: %s:%04d\n", __FILE__, __LINE__);

typedef int (*api_handler_t)(void* p_arg, uint32_t arg_len);


//=============================================================================
// Local Function/Variable Implementations
//=============================================================================

//------------------------------------------------------------
// API Implementations
//------------------------------------------------------------
static int test01(uint32_t set_val, uint32_t *p_get_val)
{
    *p_get_val = set_val;
    return 0;
}

static int test02(uint8_t *p_buff, uint32_t len)
{
    int i;
    for(i=0; i<len; ++i)
      p_buff[i] = (uint8_t)i;
    return 0;
}

//------------------------------------------------------------
// API Handler Implementations
//------------------------------------------------------------
static api_handler_t api_id_test01_handler(void* p_arg, uint32_t arg_len)
{
    int err_code = 0, line = 0;
    struct any_structure_01_t *p_struct;

    if( arg_len != sizeof(struct any_structure_01_t) ){ line = __LINE__; goto _err_end_; }

    p_struct = (struct any_structure_01_t*)p_arg;
    err_code = test01( p_struct->set_val, &p_struct->get_val);
    return (api_handler_t)err_code;


  _err_end_:
    if( line ) ERR_MSG("line=%d\n", line);
    if( err_code ) ERR_MSG("err_code=%d\n", err_code);

    return (api_handler_t)-1;
}

static api_handler_t api_id_test02_handler(void* p_arg, uint32_t arg_len)
{
    int err_code = 0;
    err_code = test02( (uint8_t*)p_arg, arg_len );
    return (api_handler_t)err_code;
}

//------------------------------------------------------------
// Root Handler Implementations
//------------------------------------------------------------
static api_handler_t api_handler_table[api_id_max] = {
    NULL,
    (api_handler_t)api_id_test01_handler,
    (api_handler_t)api_id_test02_handler,
};

static vapi_core_sub_handler_t root_handler(int32_t api_id, void* p_arg, uint32_t arg_len, void *p_cookie)
{
    DBG_MSG("api_id=%d\n", api_id);

    if( api_id > api_id_min  &&  api_id < api_id_max  &&  api_handler_table[api_id] ){
        api_handler_table[api_id](p_arg, arg_len);
        return (vapi_core_sub_handler_t)0;
    } else {
        return (vapi_core_sub_handler_t)-1;
    }
}

//------------------------------------------------------------
// Test Function Implementations
//------------------------------------------------------------
static int vapi_sub_test_start(void)
{
    int err_code = 0, line = 0;
    int fd;

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

//=============================================================================
// Global Function/Variable Implementations
//=============================================================================
int main(int argc, char *argv[])
{
    return vapi_sub_test_start();
}

