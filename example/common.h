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


#ifndef _COMMON_H_
#define _COMMON_H_

#include <stdint.h>

#define TEST_PORT (60000)

enum api_id_e
{
    api_id_min    = 0x00000000,
    api_id_test01 = 0x00000001,
    api_id_test02 = 0x00000002,
    api_id_test03 = 0x00000003,
    api_id_test04 = 0x00000004,
    api_id_max
};

struct any_structure_01_t
{
    uint32_t set_val;
    uint32_t get_val;
    int32_t err_code;
};

struct any_structure_02_t
{
    int not_used;
};

typedef int (*callback_t)(int val, void *p_cookie);
struct any_structure_03_t
{
    uint32_t set_val;
    callback_t cb;
    void *p_cookie;
    int32_t err_code;
};

struct any_structure_cb_t
{
    uint32_t val;
    void *p_cookie;
    callback_t err_code;
};

#endif // _COMMON_H_
