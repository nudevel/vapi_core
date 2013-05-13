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

/*!
  \mainpage
  VAPI CORE is a lightweight IPC (Inter Process Communication) library to
  executed APIs of other processes on the Linux OS, which is written by C
  language and distributed under the BSD (Berkeley Standard Distribution)
  license.
*/

#ifndef _VAPI_CORE_H_
#define _VAPI_CORE_H_

//=============================================================================
// Includes
//=============================================================================
#include <stdint.h>

//=============================================================================
// Macro/Type/Enumeration/Structure Definitions
//=============================================================================

//=============================================================================
// Global Function/Variable Prototypes
//=============================================================================

/*!
  \brief
  "vapi_core_open()" establish a local TCP connection with the sub process
  listening on the port number specified by "dstport".
  It can be several times called with the same "dstport".
  
  \param[in] dstport
  The destination port number listened by the sub process.
  
  \return
  It returns a descriptor. If error happened, -1 will return.
*/
int32_t vapi_core_open(uint16_t dstport);


/*!
  \brief
  "vapi_core_close()" close the local TCP connection with the sub process.

  \param[in] fd
  The descriptor.

  \return
  0 for success, and -1 for error.
*/
int32_t vapi_core_close(int32_t fd);


/*!
  \brief
  "vapi_core_invoke()" requests executing a API function specified by the
  "api_id" to the sub module, and receives the acknowledgement from the sub
  module synchronously.

  \param[in] fd
  The descriptor.

  \param[in] api_id
  The API function ID to be executed.

  \param[in,out] p_arg
  The pointer to the arguments.

  \param[in] arg_len
  The length of the arguments.

  \return
  0 for success, and -1 for error.
*/
int32_t vapi_core_invoke(int32_t fd, int32_t api_id, void* p_arg, uint32_t arg_len);

#endif // _VAPI_CORE_H_
