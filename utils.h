//******************************************************************************
// (c)2014 BlueBolt Ltd.  All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
// * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
// * Neither the name of BlueBolt nor the names of
// its contributors may be used to endorse or promote products derived
// from this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// 
// Author:Ashley Retallack - ashley.retallack@gmail.com
// 
// File:utils.h
// 
// 
//******************************************************************************


#ifndef INCLUDE_pygdi_utils_h
#define INCLUDE_pygdi_utils_h


#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif
#ifndef MAX
#define MAX(a,b) ((a)<(b)?(b):(a))
#endif

#define GETTER(type, attr)\
  {         #attr,\
   (getter) type ## _ ## attr ## __get__,\
            NULL,\
            type ## _ ## attr ## __doc__,\
            NULL}

#define GETSET(type, attr)\
  {         #attr,\
   (getter) type ## _ ## attr ## __get__,\
   (setter) type ## _ ## attr ## __set__,\
            type ## _ ## attr ## __doc__,\
            NULL}

#define PYTRY_ENTER() try {
#define PYTRY_EXIT(ret) } catch(...) { Python_Handle_Exception(); return ret; }

void Python_Handle_Exception();

#endif
