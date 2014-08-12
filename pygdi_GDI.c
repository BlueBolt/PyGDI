

#include "types.h"

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

extern PyTypeObject GDI_Type;

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
        self->ctxid = 0;
    }

    return (PyObject *)self;
}

static int
GDI_init(GDI *self, PyObject *args, PyObject *kwds)
{
    return 0;
}

static PyMemberDef GDI_members[] = {
    {"ctxid", T_INT, offsetof(GDI, ctxid), 0,
     "GDI Context id"},
    {NULL}  /* Sentinel */
};

// Main methods

// static PyObject *
// GDI_initialise(GDI* self)
// {
//     PyObject *result;  


//     return result;
// }

// static PyObject *
// GDI_close(GDI* self)
// {
//     PyObject *result;  

    
//     return result;
// }

// static PyObject *
// GDI_getAdminUser(GDI* self)
// {
//     PyObject *result;  

    
//     return result;
// }

// static PyObject *
// GDI_getSGERoot(GDI* self)
// {
//     PyObject *result;  

    
//     return result;
// }

// static PyObject *
// GDI_getSGECell(GDI* self)
// {
//     PyObject *result;  

    
//     return result;
// }

// static PyObject *
// GDI_getActQMaster(GDI* self)
// {
//     PyObject *result;  

    
//     return result;
// }

// static int 
// GDI_getSgeQmasterPort(GDI* self)
// {
//     int result = 0;  

    
//     return result;
// }

// static int 
// GDI_getSgeExecdPort(GDI* self)
// {
//     int result = 0;  

    
//     return result;
// }

// static PyObject *
// GDI_getRealExecHostList(GDI* self)
// {
//     PyObject *result;  

    
//     return result;
// }

static PyMethodDef GDI_methods[] = {
    {NULL}  /* Sentinel */
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
    0,                     /* tp_traverse */
    0,                     /* tp_clear */
    0,                     /* tp_richcompare */
    0,                     /* tp_weaklistoffset */
    0,                     /* tp_iter */
    0,                     /* tp_iternext */
    GDI_methods,             /* tp_methods */
    GDI_members,             /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)GDI_init,      /* tp_init */
    0,                         /* tp_alloc */
    GDI_new,                 /* tp_new */
};
