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
// File:types.h
// 
// 
//******************************************************************************


#ifndef INCLUDE_pygdi_objects_h
#define INCLUDE_pygdi_objects_h


#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <structmember.h>

#include "gdi/version.h"
#include "gdi/sge_gdi.h"
#include "gdi/sge_gdi2.h"
#include "gdi/sge_gdi_ctx.h"

#include "sgeobj/sge_all_listsL.h"
#include "sgeobj/sge_answer.h"
#include "sgeobj/sge_calendar.h"
#include "sgeobj/sge_qinstance_state.h"
#include "sgeobj/sge_cqueue.h"
#include "sgeobj/sge_ja_task.h"
#include "sgeobj/sge_sharetree.h"
#include "sgeobj/sge_utility.h"
#include "sgeobj/sge_event.h"
#include "sgeobj/sge_object.h"


#define SIMPLE_TYPE(_name, _ptr_type, _ptr_name) \
        typedef struct {\
            PyObject_HEAD\
            _ptr_type *_ptr_name;\
        } _name;


// Main GDI class
typedef struct {
   PyObject_HEAD
   int ctxid;
} GDI;

// Root PyGEObject
typedef struct {
   PyObject_HEAD
   int objtype;
   GDI *gdi;
   lListElem *listElement;
} GEObject;

// Job
typedef struct {
   PyObject_HEAD
      int objtype;
      GDI *gdi;
      lListElem *listElement;
} Job;

// SchedConf

// Complex

// Queue
typedef struct {
   PyObject_HEAD
      int objtype;
      GDI *gdi;
   lListElem *listElement;
} Queue;

// QueueInstance
typedef struct {
   PyObject_HEAD
      int objtype;
      GDI *gdi;
   lListElem *listElement;
} QueueInstance;

// Host
typedef struct {
   PyObject_HEAD
      int objtype;
      GDI *gdi;
   lListElem *listElement;
   PyObject *hostname;
} Host;

// ExecHost
typedef struct {
   PyObject_HEAD
      int objtype;
      GDI *gdi;
      lListElem *listElement;
      PyObject *hostname; 
} ExecHost;

// HostGroup

// User

// UserSet

// ParallelEnvironment

// Project

// Calandar

extern PyTypeObject GDI_Type;
extern PyTypeObject GEObject_Type;
extern PyTypeObject Job_Type;
extern PyTypeObject Queue_Type;
extern PyTypeObject QueueInstance_Type;
extern PyTypeObject Host_Type;
extern PyTypeObject ExecHost_Type;
extern PyTypeObject AdminHost_Type;
extern PyTypeObject SubmitHost_Type;

#endif
