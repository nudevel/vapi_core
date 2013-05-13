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


#ifndef _VAPI_CORE_SUB_H_
#define _VAPI_CORE_SUB_H_

//=============================================================================
// Includes
//=============================================================================
#include <stdint.h>

//=============================================================================
// Macro/Type/Enumeration/Structure Definitions
//=============================================================================

/*!
  \brief
  "vapi_core_sub_handler_t" is the type of handler function to be
  called when an invoked request is received from the host side.

  \param[in] api_id
  The API function ID to be executed.

  \param[in,out] p_arg
  The pointer to the arguments.

  \param[in] arg_len
  The length of the arguments.

  \param[in,out] p_cookie
  The pointer to the user data.

  \return
  0 for success, and the other values for handling error.
  If not 0, vapi_core_invoke() of the host side will return error.
*/
typedef int (*vapi_core_sub_handler_t)(int32_t api_id, void* p_arg, uint32_t arg_len, void *p_cookie);


//=============================================================================
// Global Function/Variable Prototypes
//=============================================================================

/*!
  \brief
  "vapi_core_sub_open()" binds to the specified "port" and listen on it.
  Then it creates a thread of accepting from the host side.

  \param[in] port
  The port number to be listened. If 0, kernel will select an available port
  automatically, which can be gotten by vapi_core_sub_get_port() .

  \param[in] handler
  The handler function to be called when an invoked request is received from
  the host side.

  \param[in] p_cookie
  The pointer to the user data.

  \return
  It returns a descriptor. If error happened, -1 will return.
*/
int32_t vapi_core_sub_open(uint16_t port, vapi_core_sub_handler_t handler, const void *p_cookie);


/*!
  \brief
  "vapi_core_sub_close()" close the listened socket.
  However the accepted sockets would not be closed. This sockets should
  be closed by the host side.

  \param[in] fd
  The descriptor.

  \return
  0 for success, and -1 for error.
*/
int32_t vapi_core_sub_close(int32_t fd);


/*!
  \brief
  "vapi_core_sub_get_port()" gets the listened port number.

  \param[in] fd
  The descriptor.

  \param[out] p_port
  The pointer of port.

  \return
  0 for success, and -1 for error.
*/
int32_t vapi_core_sub_get_port(int32_t fd, uint16_t *p_port);

#endif // _VAPI_CORE_SUB_H_
