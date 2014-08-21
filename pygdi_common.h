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


int nativeInit(void);

sge_gdi_ctx_class_t * getGDIContext( int ctx_index );
void closeGDIContext( sge_gdi_ctx_class_t *ctx );

// static PyObject * pygdi_fill(PyObject *self, PyObject *list, PyObject *filter, const char *classname, int target_list, lDescr *descr, PyObject *answers);

// static PyObject * pygdi_add(PyObject *self, PyObject *pygdi, PyObject *jobj, const char *classname, int target_list, lDescr *descr, PyObject *answers);

// static PyObject * pygdi_delete(PyObject *self, PyObject *pygdi, PyObject *jobj, const char* classname, int target_list, lDescr *descr, bool force, PyObject *answers);
// static PyObject * pygdi_delete_array(PyObject *self, PyObject *pygdi, PyObject *obj_list, const char *classname, int target_list, lDescr *descr, bool force, PyObject *options, PyObject *answers);

// static PyObject *  pygdi_update(PyObject *self, PyObject *pygdi, PyObject *jobj, , const char *classname, int target_list, lDescr *descr, PyObject *answers);

#endif
