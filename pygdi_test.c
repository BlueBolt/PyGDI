/*___INFO__MARK_BEGIN__*/
/*************************************************************************
 *
 *  The Contents of this file are made available subject to the terms of
 *  the Sun Industry Standards Source License Version 1.2
 *
 *  Sun Microsystems Inc., March, 2001
 *
 *
 *  Sun Industry Standards Source License Version 1.2
 *  =================================================
 *  The contents of this file are subject to the Sun Industry Standards
 *  Source License Version 1.2 (the "License"); You may not use this file
 *  except in compliance with the License. You may obtain a copy of the
 *  License at http://gridengine.sunsource.net/Gridengine_SISSL_license.html
 *
 *  Software provided under this License is provided on an "AS IS" basis,
 *  WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING,
 *  WITHOUT LIMITATION, WARRANTIES THAT THE SOFTWARE IS FREE OF DEFECTS,
 *  MERCHANTABLE, FIT FOR A PARTICULAR PURPOSE, OR NON-INFRINGING.
 *  See the License for the specific provisions governing your rights and
 *  obligations concerning the Software.
 *
 *   The Initial Developer of the Original Code is: Sun Microsystems, Inc.
 *
 *   Copyright: 2001 by Sun Microsystems, Inc.
 *
 *   All Rights Reserved.
 *
 ************************************************************************/
/*___INFO__MARK_END__*/

#include <Python.h>
#include <structmember.h>
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



static sge_gdi_ctx_class_t * getGDIContext(  ) 
{
   lList *alp = NULL;
   sge_gdi_ctx_class_t *ctx = NULL;

   if (sge_gdi2_setup(&ctx, QSTAT, MAIN_THREAD, &alp) != AE_OK) 
   {
      answer_list_output(&alp);
      lFreeList(&alp);  
      return NULL; 
   }

   return ctx;
}

static void closeGDIContext( sge_gdi_ctx_class_t *ctx )
{
   cl_com_handle_t *handle = cl_com_get_handle(ctx->get_component_name(ctx), 0);
   cl_commlib_shutdown_handle(handle, CL_FALSE);
   sge_gdi_ctx_class_destroy(&ctx);
}

static PyObject * getSGERoot( PyObject *self ) 
{
   sge_gdi_ctx_class_t *ctx = NULL;
   const char *sge_root = NULL;
   PyObject* result = Py_None;
   
   ctx = getGDIContext();

   if (ctx == NULL) 
   {
      return NULL; 
   }

   sge_gdi_set_thread_local_ctx(ctx);

   sge_root = ctx->get_sge_root(ctx);

   if (sge_root != NULL) 
   {    
      result = PyString_FromString(sge_root);
   }

   sge_gdi_set_thread_local_ctx(NULL);
   // destroy the ctx object
   closeGDIContext(ctx);
   
   return result;
}

static PyObject * getQueues( PyObject *self ) 
{
   sge_gdi_ctx_class_t *ctx = NULL;
   PyObject* result = PyList_New(0);
   
   ctx = getGDIContext();

   if (ctx == NULL) 
   {
      return NULL; 
   }

   sge_gdi_set_thread_local_ctx(ctx);

   // get the queues 
   lList *qlp = NULL,*alp = NULL;
   lListElem *cq = NULL;
   int num_queues = 0;
   lCondition *where = NULL;
   lEnumeration *q_all = NULL;

   q_all = lWhat("%T(ALL)", QU_Type);
   where = lWhere("");

   alp = ctx->gdi(ctx, SGE_CQ_LIST, SGE_GDI_GET, &qlp, where, q_all);

   num_queues = lGetNumberOfElem(qlp);

   if (num_queues > 0) 
   {    
      // loop through the queues and added them to the main list
      for_each(cq, qlp) {
         const char *queue_name = lGetString(cq, CQ_name);

         PyList_Append(result,PyString_FromString( queue_name ));

      }
   }   

   lFreeList(&alp);
   lFreeList(&qlp);
   lFreeWhat(&q_all);
   lFreeWhere(&where);

   sge_gdi_set_thread_local_ctx(NULL);
   // destroy the ctx object
   closeGDIContext(ctx);
   
   return result;
}

static PyObject * getHosts( PyObject *self ) 
{
   sge_gdi_ctx_class_t *ctx = NULL;
   PyObject* result = PyList_New(0);
   
   ctx = getGDIContext();

   if (ctx == NULL) 
   {
      return NULL; 
   }

   sge_gdi_set_thread_local_ctx(ctx);

   // get the hosts 
   lList *hlp = NULL,*alp = NULL;
   lListElem *eh = NULL;
   int num_hosts = 0;
   lCondition *where = NULL;
   lEnumeration *h_all = NULL;

   h_all = lWhat("%T(ALL)", EH_Type);
   where = lWhere("%T(!(%Ic=%s || %Ic=%s))",
         EH_Type, EH_name, SGE_TEMPLATE_NAME, 
               EH_name, SGE_GLOBAL_NAME );

   alp = ctx->gdi(ctx, SGE_EH_LIST, SGE_GDI_GET, &hlp, where, h_all);

   num_hosts = lGetNumberOfElem(hlp);

   if (num_hosts > 0) 
   {    
      // loop through the queues and added them to the main list
      for_each(eh, hlp) {
         const char *eh_name = lGetHost(eh, EH_name);

         PyList_Append(result,PyString_FromString( eh_name ));

      }
   }   

   lFreeList(&alp);
   lFreeList(&hlp);
   lFreeWhat(&h_all);
   lFreeWhere(&where);

   sge_gdi_set_thread_local_ctx(NULL);
   // destroy the ctx object
   closeGDIContext(ctx);
   
   return result;
}

static PyMethodDef module_methods[] = {
   {"getSGERoot",  getSGERoot, METH_NOARGS,
     "return the path to SGE_ROOT"},
   {"getQueues",  getQueues, METH_NOARGS,
     "Return a list of names of queues"},
   {"getHosts",  getHosts, METH_NOARGS,
     "Return a list of names of execution hosts"},
    {NULL}  /* Sentinel */
};

#ifndef PyMODINIT_FUNC  /* declarations for DLL import/export */
#define PyMODINIT_FUNC void
#endif
PyMODINIT_FUNC
initpygdi(void) 
{
    PyObject* m;

    GDI_Type.tp_new = PyType_GenericNew;
    if (PyType_Ready(&GDI_Type) < 0)
        return;

    m = Py_InitModule3("pygdi", module_methods,
                       "example pygdi binding module");

    Py_INCREF(&GDI_Type);
    PyModule_AddObject(m, "GDI", (PyObject *)&GDI_Type);
}

