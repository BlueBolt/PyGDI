

#include <Python.h>
#include <structmember.h>

#include "pygdi_common.h"
#include "types.h"

extern PyTypeObject GDI_Type;

static PyObject * getQueues( PyObject *self ) 
{
   sge_gdi_ctx_class_t *ctx = NULL;
   PyObject* result = PyList_New(0);
   
   ctx = getGDIContext();

   if (ctx == NULL) 
   {
      Py_DECREF(result);
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

    m = Py_InitModule3("_pygdi", module_methods,
                       "example pygdi binding module");

    Py_INCREF(&GDI_Type);
    PyModule_AddObject(m, "GDI", (PyObject *)&GDI_Type);
}

