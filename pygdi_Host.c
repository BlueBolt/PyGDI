

#include "pygdi_common.h"
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

extern PyTypeObject Host_Type;

static void
Host_dealloc(Host* self)
{
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
Host_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    GDI *self;

    self = (GDI *)type->tp_alloc(type, 0);
    if (self != NULL) {
        // set up default attributes
    }

    return (PyObject *)self;
}

static int
Host_init(GDI *self, PyObject *args, PyObject *kwds)
{
    return 0;
}

static PyMemberDef Host_members[] = {
    {NULL}  /* Sentinel */
};

// Main methods

static PyObject *
GDI_initialise(GDI* self)
{
    PyObject *result = Py_None;  


    return result;
}

static PyMethodDef Host_methods[] = {
    {"initialise", (PyCFunction)GDI_initialise, METH_NOARGS,
     PyDoc_STR("initalise the GDI connection")},
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
    Py_TPFLAGS_DEFAULT,        /*tp_flags*/
    "GDI Host Object",             /* tp_doc */
    0,                     /* tp_traverse */
    0,                     /* tp_clear */
    0,                     /* tp_richcompare */
    0,                     /* tp_weaklistoffset */
    0,                     /* tp_iter */
    0,                     /* tp_iternext */
    Host_methods,             /* tp_methods */
    Host_members,             /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)Host_init,      /* tp_init */
    0,                         /* tp_alloc */
    Host_new,                 /* tp_new */
};
