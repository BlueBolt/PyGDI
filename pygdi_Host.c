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
// File:pygdi_Host.c
// 
// 
//******************************************************************************


#include <Python.h>
#include <sstream>

#include "pygdi_common.h"
#include "types.h"
#include "utils.h"

#include "sgeobj/sge_host.h"

extern PyTypeObject GEObject_Type;
extern PyTypeObject Host_Type;
extern PyTypeObject ExecHost_Type;

static void
Host_dealloc(Host* self)
{
    Py_XDECREF(self->hostname);
    Py_XDECREF(&self->gdi);
    // free up the connection to the host element
    lFreeElem(&self->listElement);
    self->ob_type->tp_free((PyObject*)self);
}

Host *
Host_new(lListElem host)
{

    if (!host)
    {
        return NULL;
    }

    Host * pyobj = NULL;

    if (object_has_type(host,EH_Type))
    {
        pyobj = PyObject_New(Host,(PyTypeObject * ) &ExecHost_Type );
    }

    return pyobj;
}


PyObject * BuildPyHost(lListElem host)
{
    if (!host)
    {
        Py_RETURN_NONE;
    }
    
    Host * pyobj = Host_new(host);
    
    if(!pyobj)
    {
        std::ostringstream os;
        os << "Unknown transform type for BuildConstPyTransform.";
        throw Exception(os.str().c_str());
    }
    
    pyobj->listElement = new lListElem();    
    *pyobj->listElement = host;
    self->objtype = 5;

    const char * _eh_name;
    _eh_name = lGetHost(self->listElement, EH_name);

    Py_DECREF(self->hostname);
    Py_INCREF(value);
    self->hostname = PyString_FromString(_eh_name);
    
    return (PyObject *) pyobj;
}


static int
Host_init(Host *self, PyObject *args, PyObject *kwds)
{
    // if (GEObject_Type.tp_init((PyObject *)self, args, kwds) < 0)
    //     return -1;

    PyObject *hostname=NULL, *ipaddress=NULL, *tmp;
    // GDI *tmp_gdi, *gdi=NULL;

    static char *kwlist[] = {"hostname",NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "|S", kwlist, 
                                      &hostname, &ipaddress ))
        return -1; 

    if (hostname) {
        tmp = self->hostname;
        Py_INCREF(hostname);
        self->hostname = hostname;
        Py_XDECREF(tmp);
    }


    self->objtype = 5;

    return 0;
}

// getters/setters

PyDoc_STRVAR(Host_hostname__doc__,
  "The hosts hostname as it appears to grid");

static PyObject *
Host_hostname__get__(Host *self, void *closure)
{
    Py_INCREF(self->hostname);
    return self->hostname;
}

static int
Host_hostname__set__(Host *self, PyObject *value, void *closure)
{
    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError, "Cannot delete the hostname attribute");
        return -1;
    }

    if (! PyString_Check(value)) {
        PyErr_SetString(PyExc_TypeError, 
                        "The hostname attribute value must be a string");
        return -1;
    }
      
    // set the host object based on this hostname

    if (self->listElement == NULL)
    {
        lListElem *hep = NULL;
        hep = lCreateElem(EH_Type);
        self->listElement = hep;
    }

    lSetHost(self->listElement, EH_name, PyString_AsString(value));

    sge_resolve_host(self->listElement, EH_name); // TODO add check for success
    
    const char * _eh_name;
    _eh_name = lGetHost(self->listElement, EH_name);

    Py_DECREF(self->hostname);
    Py_INCREF(value);
    self->hostname = PyString_FromString(_eh_name);
    // self->hostname = value;

    return 0;
}

PyDoc_STRVAR(Host_cpus__doc__,
  "The hosts number of cpus");

static PyObject *
Host_cpus__get__(Host *self, void *closure)
{
    int cpus = 0;

    if (self->gdi == NULL)
        return NULL;

    lList *load_attr = NULL; 
    lListElem *ep_nproc = NULL;

    load_attr = lGetList(self->listElement, EH_load_list);

    if ((ep_nproc = lGetElemStr(load_attr, HL_name, LOAD_ATTR_NUM_PROC)))
    {
        const char *cp = lGetString(ep_nproc, HL_value);
        if (cp)
        {
            printf( "%s\n",cp );
            cpus = MAX(1, atoi(lGetString(ep_nproc, HL_value)));
       }
    }

    lFreeList(&load_attr);
    lFreeElem(&ep_nproc);

    return Py_BuildValue("i",cpus);
}


PyGetSetDef Host_getseters[] = {
    GETSET(Host, hostname),
    GETTER(Host, cpus),
    {NULL}
};


static PyMemberDef Host_members[] = {
    {NULL}  /* Sentinel */
};

// Main methods

static PyObject *
Host_getStats(Host* self)
{
    // PyObject *result;  

    if (lGetHost(self->listElement, EH_name))
    {
        const char * _eh_name;
        _eh_name = lGetHost(self->listElement, EH_name);
        return PyString_FromString(_eh_name);
        // Py_RETURN_NONE;
    }

    // return result;

    Py_RETURN_NONE;
}

static PyMethodDef Host_methods[] = {
    {"getStats", (PyCFunction)Host_getStats, METH_NOARGS,
     PyDoc_STR("get the running stats of this host")},
    {NULL}  /* Sentinel */
};

PyTypeObject Host_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "pygdi.Host",               /*tp_name*/
    sizeof(Host),               /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)Host_dealloc, /*tp_dealloc*/
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
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,        /*tp_flags*/
    "GDI Host Object",             /* tp_doc */
    0,                     /* tp_traverse */
    0,                     /* tp_clear */
    0,                     /* tp_richcompare */
    0,                     /* tp_weaklistoffset */
    0,                     /* tp_iter */
    0,                      /* tp_iternext */
    Host_methods,           /* tp_methods */
    0,                      /* tp_members */
    Host_getseters,         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)Host_init,      /* tp_init */
    0,                         /* tp_alloc */
    0,                 /* tp_new */
};

/*
* ExecHost
*/

static void
ExecHost_dealloc(ExecHost* self)
{
}

static int
ExecHost_init(ExecHost *self, PyObject *args, PyObject *kwds)
{
    if (Host_Type.tp_init((PyObject *)self, args, kwds) < 0)
        return -1;

    return 0;
}

static PyMemberDef ExecHost_members[] = {
    {NULL}  /* Sentinel */
};




// Main methods

static PyObject *
ExecHost_getStatus(ExecHost* self)
{

    // PyObject *result;  

    // return result;
    Py_RETURN_NONE;    
}

static PyMethodDef ExecHost_methods[] = {
    {"getStatus", (PyCFunction)ExecHost_getStatus, METH_NOARGS,
     PyDoc_STR("get the current status of this host")},
    {NULL}  /* Sentinel */
};

PyTypeObject ExecHost_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "pygdi.ExecHost",               /*tp_name*/
    sizeof(ExecHost),               /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)ExecHost_dealloc, /*tp_dealloc*/
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
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,        /*tp_flags*/
    "GDI ExecHost Object",             /* tp_doc */
    0,                     /* tp_traverse */
    0,                     /* tp_clear */
    0,                     /* tp_richcompare */
    0,                     /* tp_weaklistoffset */
    0,                     /* tp_iter */
    0,                     /* tp_iternext */
    ExecHost_methods,             /* tp_methods */
    ExecHost_members,             /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)ExecHost_init,      /* tp_init */
    0,                         /* tp_alloc */
    0,                 /* tp_new */
};
