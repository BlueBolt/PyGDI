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
// File:pygdi_GDI.c
// 
// 
//******************************************************************************



#include "types.h"
#include "pygdi_common.h"
#include "pygdi_GEObject.h"

// context type
/*
* connect
* shutdown
* get_admin_user
* get_sge_execd_port
* get_sge_qmaster_port
* get_spooling_method
* get_sge_root
* get_master
* get_cell_root
* get_username
*/

extern PyObject *GDIError;

extern PyTypeObject GDI_Type;
extern PyTypeObject GEObject_Type;
extern PyTypeObject Host_Type;
extern PyTypeObject ExecHost_Type;

static void
GDI_dealloc(GDI* self)
{
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
GDI_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    GDI *self;

    self = (GDI *)type->tp_alloc(type, 0);
    if (self != NULL) {
        self->ctxid = -1;
    }

    return (PyObject *)self;
}

static int
GDI_init(GDI *self, PyObject *args, PyObject *kwds)
{

    self->ctxid = -1;
    return 0;
}

static PyMemberDef GDI_members[] = {
    {"ctxid", T_INT, offsetof(GDI, ctxid), 0,
     "GDI Context id"},
    {NULL}  /* Sentinel */
};

// Main methods

/*
* Initalise the connection to the GDI
*/
static PyObject *
GDI_initialise(GDI* self)
{

    int ctx_idx = -1; 

    ctx_idx = nativeInit();

    self->ctxid = ctx_idx;

    Py_RETURN_NONE;
}

static PyObject *
GDI_close(GDI* self)
{
    sge_gdi_ctx_class_t *ctx = NULL;

    if (self->ctxid != -1)    
        ctx = getGDIContext(self->ctxid);

    if (ctx == NULL) 
    {
        PyErr_SetString(GDIError, "Failed to get context object, check the GDI class is initialiased");
        return NULL; 
    }

    closeGDIContext(ctx);

    self->ctxid = -1;

    Py_RETURN_NONE;
}

static PyObject *
GDI_getAdminUser(GDI* self)
{
    PyObject *result;  
    const char *sge_admin = NULL;
    sge_gdi_ctx_class_t *ctx = NULL;

    if (self->ctxid != -1)    
        ctx = getGDIContext(self->ctxid);

    if (ctx == NULL) 
    {
        PyErr_SetString(GDIError, "Failed to get context object, check the GDI class is initialiased");
        return NULL; 
    }

    sge_admin = ctx->get_admin_user(ctx);

    if (sge_admin != NULL) 
    {    
      result = PyString_FromString(sge_admin);
    } else {
        Py_INCREF(Py_None);
        result = Py_None;
    }
    
    return result;
}

static PyObject *
GDI_getSGERoot(GDI* self)
{

    sge_gdi_ctx_class_t *ctx = NULL;
    const char *sge_root = NULL;
    PyObject* result;


    if (self->ctxid != -1)    
    {
        ctx = getGDIContext(self->ctxid);
    }

    if (ctx == NULL) 
    {
        PyErr_SetString(GDIError, "Failed to get context object, check the GDI class is initialiased");
        return NULL;  
    }

    sge_root = ctx->get_sge_root(ctx);

    if (sge_root != NULL) 
    {    
        result = PyString_FromString(sge_root);
    } else {
        Py_INCREF(Py_None);
        result = Py_None;
    }
   
    return result;
}

static PyObject *
GDI_getSGECell(GDI* self)
{
    sge_gdi_ctx_class_t *ctx = NULL;
    const char *sge_cell = NULL;

    if (self->ctxid != -1)    
        ctx = getGDIContext(self->ctxid);

    if (ctx == NULL) 
    {
        PyErr_SetString(GDIError, "Failed to get context object, check the GDI class is initialiased");
        return NULL; 
    }

    sge_cell = ctx->get_cell_root(ctx);

    if (sge_cell != NULL) 
    {    
      return PyString_FromString(sge_cell);
    }
    
    Py_RETURN_NONE;
}

static PyObject *
GDI_getActQMaster(GDI* self)
{
    const char *sge_master = NULL;
    sge_gdi_ctx_class_t *ctx = NULL;

    if (self->ctxid != -1)    
        ctx = getGDIContext(self->ctxid);

    if (ctx == NULL) 
    {
        PyErr_SetString(GDIError, "Failed to get context object, check the GDI class is initialiased");
        return NULL; 
    }

    sge_master = ctx->get_master(ctx,true);

    if (sge_master != NULL) 
    {    
      return PyString_FromString(sge_master);
    }
    
    Py_RETURN_NONE;
}

static PyObject *
GDI_getSgeQmasterPort(GDI* self)
{
    PyObject *result; 
    sge_gdi_ctx_class_t *ctx = NULL;

    if (self->ctxid != -1)    
        ctx = getGDIContext(self->ctxid);

    if (ctx == NULL) 
    {
        PyErr_SetString(GDIError, "Failed to get context object, check the GDI class is initialiased");
        return NULL; 
    }

    result = PyInt_FromLong(PyLong_AsLong(PyLong_FromUnsignedLong(ctx->get_sge_qmaster_port(ctx))));
    
    return result;
}

static PyObject *
GDI_getSgeExecdPort(GDI* self)
{
    PyObject *result;  
    sge_gdi_ctx_class_t *ctx = NULL;

    if (self->ctxid != -1)    
        ctx = getGDIContext(self->ctxid);

    if (ctx == NULL) 
    {
        PyErr_SetString(GDIError, "Failed to get context object, check the GDI class is initialiased");
        return NULL; 
    }
    
    result = PyInt_FromLong(PyLong_AsLong(PyLong_FromUnsignedLong(ctx->get_sge_execd_port(ctx))));

    return result;
}

PyObject *
GDI_get_object(GDI *self,PyObject *args)
{
    const lDescr *obj = NULL;
    lListElem *ep;
    // int err;
    int type_id;
    GEObject *py_obj = NULL;


    if (!PyArg_ParseTuple(args, "i", &type_id))
        return NULL;

    // if (type_id == -1 && PyErr_Occurred()) 
    //     return NULL; /* error */ 

    // err = py_typeid_to_gdi_type(type_id,&obj);
    const sge_object_type sge_type = type_id;

    obj = object_type_get_descr(sge_type);

    if (obj == NULL)
        return NULL;

    ep = lCreateElem(obj);

    if (object_has_type(ep,EH_Type))
            py_obj = PyObject_New(GEObject, &ExecHost_Type);

    if (py_obj) {
        py_obj->listElement = ep;
        py_obj->gdi = self;
        Py_INCREF(self);
        Py_INCREF(py_obj);

        return (PyObject *)py_obj;
    }

    // return wrap_listElem(ep,self);
    
    Py_RETURN_NONE;
}

PyObject *
GDI_get_execHosts(GDI *self)
{

    PyObject * list = PyList_New(0);
    
    return list;
}



static PyMethodDef GDI_methods[] = {
    {"initialise", (PyCFunction)GDI_initialise, METH_NOARGS,
     PyDoc_STR("initalise the GDI connection")},
    {"close", (PyCFunction)GDI_close, METH_NOARGS,
     PyDoc_STR("close the GDI connection")},
    {"getAdminUser", (PyCFunction)GDI_getAdminUser, METH_NOARGS,
     PyDoc_STR("retrieve the admin user name")},
    {"getActQMaster", (PyCFunction)GDI_getActQMaster, METH_NOARGS,
     PyDoc_STR("retrueve the hostname of the active QMaster node")},
    {"getSGECell", (PyCFunction)GDI_getSGECell, METH_NOARGS,
     PyDoc_STR("return the name of the current SGE Cell")},
    {"getSGERoot", (PyCFunction)GDI_getSGERoot, METH_NOARGS,
     PyDoc_STR("return the path to the current SGE root")},
    {"getSgeExecdPort", (PyCFunction)GDI_getSgeExecdPort, METH_NOARGS,
     PyDoc_STR("get the expexted port for excution deamons")},
    {"getSgeQmasterPort", (PyCFunction)GDI_getSgeQmasterPort, METH_NOARGS,
     PyDoc_STR("get the expexted port for the qmaster")},
    {"get_object", (PyCFunction)GDI_get_object, METH_VARARGS,
     PyDoc_STR("return an unpopulated object based on a type_id")},
    {"get_execHosts", (PyCFunction)GDI_get_execHosts, METH_NOARGS,
     PyDoc_STR("return a list of populated ExecHost objects")},
    {NULL, NULL, 0, NULL}  /* Sentinel */
};

PyTypeObject GDI_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "pygdi.GDI",               /*tp_name*/
    sizeof(GDI),               /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)GDI_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT,        /*tp_flags*/
    "GDI objects",             /* tp_doc */
    0,                         /* tp_traverse */
    0,                         /* tp_clear */
    0,                         /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    0,                          /* tp_iter */
    0,                          /* tp_iternext */
    GDI_methods,                /* tp_methods */
    GDI_members,                /* tp_members */
    0,                          /* tp_getset */
    0,                          /* tp_base */
    0,                          /* tp_dict */
    0,                          /* tp_descr_get */
    0,                          /* tp_descr_set */
    0,                          /* tp_dictoffset */
    (initproc)GDI_init,         /* tp_init */
    0,                          /* tp_alloc */
    GDI_new,                    /* tp_new */
};
