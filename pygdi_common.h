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
// File:pygdi_common.h
// 
// 
//******************************************************************************

#ifndef PYGDI_COMMON_H
#define PYGDI_COMMON_H

#include "Python.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "rmon/sgermon.h"

#include "uti/sge_prog.h"
#include "uti/sge_bootstrap.h"
#include "uti/sge_edit.h"
#include "uti/sge_log.h"
#include "uti/sge_error_class.h"

#include "cull/cull_list.h"
#include "cull/cull.h"

#include "commlib.h"
#include "cl_errors.h"

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

int listelem_to_obj(lListElem *ep,PyObject *obj,const lDescr* descr,lList **alpp);

int nativeInit(void);

sge_gdi_ctx_class_t * getGDIContext( int ctx_index );
void closeGDIContext( sge_gdi_ctx_class_t *ctx );

int py_typeid_to_gdi_type(int type_id,lDescr *obj);

void pygdi_fill(int ctxid, PyObject *list, PyObject *filter, int target_list, lDescr *descr);
// static PyObject * pygdi_add(PyObject *self, PyObject *pygdi, PyObject *jobj, const char *classname, int target_list, lDescr *descr, PyObject *answers);
// static PyObject * pygdi_delete(PyObject *self, PyObject *pygdi, PyObject *jobj, const char* classname, int target_list, lDescr *descr, bool force, PyObject *answers);
// static PyObject * pygdi_delete_array(PyObject *self, PyObject *pygdi, PyObject *obj_list, const char *classname, int target_list, lDescr *descr, bool force, PyObject *options, PyObject *answers);
// static PyObject *  pygdi_update(PyObject *self, PyObject *pygdi, PyObject *jobj, , const char *classname, int target_list, lDescr *descr, PyObject *answers);


int generic_fill_list(PyObject *list, lList *lp, lList **alpp); 

#endif
