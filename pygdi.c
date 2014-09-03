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
// File:pygdi.c
// 
// 
//******************************************************************************



#include <Python.h>
#include <structmember.h>

#include "pygdi_common.h"
#include "types.h"

PyObject *GDIError;

extern PyTypeObject GDI_Type;
extern PyTypeObject GEObject_Type;
extern PyTypeObject Host_Type;
extern PyTypeObject ExecHost_Type;
extern PyTypeObject Queue_Type;

static PyObject * getQueues( PyObject *self ) 
{
   lList *qlp = NULL,*alp = NULL;
   sge_gdi_ctx_class_t *ctx = NULL;
   PyObject* result = PyList_New(0);
   
   if (sge_gdi2_setup(&ctx, QSTAT, MAIN_THREAD, &alp) != AE_OK) 
   {
      answer_list_output(&alp);
      lFreeList(&alp);  
      return NULL; 
   }

   if (ctx == NULL) 
   {
      Py_DECREF(result);
      return NULL; 
   }

   sge_gdi_set_thread_local_ctx(ctx);

   // get the queues 
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
   lList *hlp = NULL,*alp = NULL;
   sge_gdi_ctx_class_t *ctx = NULL;
   PyObject* result = PyList_New(0);
   
   if (sge_gdi2_setup(&ctx, QSTAT, MAIN_THREAD, &alp) != AE_OK) 
   {
      answer_list_output(&alp);
      lFreeList(&alp);  
      return NULL; 
   }

   if (ctx == NULL) 
   {
      return NULL; 
   }

   sge_gdi_set_thread_local_ctx(ctx);

   // get the hosts 
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
init_pygdi(void) 
{
    PyObject* m;

    GDI_Type.tp_new = PyType_GenericNew;
    if (PyType_Ready(&GDI_Type) < 0)
        return;

    GEObject_Type.tp_new = PyType_GenericNew;
    if (PyType_Ready(&GEObject_Type) < 0)
        return;

    Host_Type.tp_base = &GEObject_Type;
    if (PyType_Ready(&Host_Type) < 0)
        return;

    ExecHost_Type.tp_base = &Host_Type;
    if (PyType_Ready(&ExecHost_Type) < 0)
        return;

    m = Py_InitModule3("_pygdi", module_methods,
                       "example pygdi binding module");

    GDIError = PyErr_NewException("pygdi.GDIError", NULL, NULL);
    Py_INCREF(GDIError);
    PyModule_AddObject(m, "error", GDIError);

    Py_INCREF(&GDI_Type);
    PyModule_AddObject(m, "GDI", (PyObject *)&GDI_Type);
    Py_INCREF(&GEObject_Type);
    PyModule_AddObject(m, "GEObject", (PyObject *)&GEObject_Type);
    Py_INCREF(&Host_Type);
    PyModule_AddObject(m, "Host", (PyObject *)&Host_Type);
    Py_INCREF(&ExecHost_Type);
    PyModule_AddObject(m, "ExecHost", (PyObject *)&ExecHost_Type);
}

