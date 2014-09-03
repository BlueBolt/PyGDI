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
// File:pygdi_GEObject.c
// 
// 
//******************************************************************************


#include <Python.h>
#include <structmember.h>

#include "pygdi_common.h"
#include "pygdi_GEObject.h"
#include "types.h"



extern PyObject *GDIError;
extern PyTypeObject GEObject_Type;
extern PyTypeObject Host_Type;
extern PyTypeObject ExecHost_Type;

static void
GEObject_dealloc(GEObject* self)
{
    Py_CLEAR(self->gdi);
    if(self->listElement != NULL) lFreeElem(&self->listElement);
    Py_TYPE(self)->tp_free(self);
}

static PyObject *
GEObject_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    GEObject *self;

    self = (GEObject *)type->tp_alloc(type, 0);
    if (self != NULL) {
        self->objtype = -1;
    }

    return (PyObject *)self;
}

static int
GEObject_init(GEObject *self, PyObject *args, PyObject *kwds)
{

    // PyObject *gdi=NULL, *tmp_gdi;

    // static char *kwlist[] = {"gdi",NULL};

    // if (! PyArg_ParseTupleAndKeywords(args, kwds, "|O", kwlist, 
    //                                   &gdi))
    //     return -1; 


    // if (gdi) {
    //     tmp_gdi = self->gdi;
    //     Py_INCREF(gdi);
    //     self->gdi = gdi;
    //     Py_XDECREF(tmp_gdi);
    // }

    self->objtype = -1;
    return 0;
}

static PyMemberDef GEObject_members[] = {
    {"objtype", T_INT, offsetof(GEObject, objtype), 0,
     "type of object"},
    {NULL}  /* Sentinel */
};

// Main methods

static PyMethodDef GEObject_methods[] = {   
    {NULL}  /* Sentinel */
};

// setters and getters

PyTypeObject GEObject_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "pygdi.GEObject",               /*tp_name*/
    sizeof(GEObject),               /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)GEObject_dealloc, /*tp_dealloc*/
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
    "GEObject base",             /* tp_doc */
    0,                     /* tp_traverse */
    0,                     /* tp_clear */
    0,                     /* tp_richcompare */
    0,                     /* tp_weaklistoffset */
    0,                     /* tp_iter */
    0,                     /* tp_iternext */
    GEObject_methods,             /* tp_methods */
    GEObject_members,             /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)GEObject_init,      /* tp_init */
    0,                         /* tp_alloc */
    GEObject_new,                 /* tp_new */
};

PyObject *
wrap_listElem(lListElem *ep, GDI *gdi)
{
    GEObject *py_obj = NULL;
    // lDescr *tmpDescr;

    // tmpDescr = ep->descr;

    if (object_has_type(ep,EH_Type))
            py_obj = PyObject_New(GEObject, &ExecHost_Type);

    if (py_obj) {
        py_obj->listElement = ep;
        if (gdi) {
            py_obj->gdi = gdi;
            Py_INCREF(gdi);
        }
    }

    return (PyObject *)py_obj;
}
