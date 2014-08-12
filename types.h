
#ifndef INCLUDE_pygdi_objects_h
#define INCLUDE_pygdi_objects_h


#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <structmember.h>

#define SIMPLE_TYPE(_name, _ptr_type, _ptr_name) \
        typedef struct {\
            PyObject_HEAD\
            Repository *repo;\
            _ptr_type *_ptr_name;\
        } _name;



typedef struct {
   PyObject_HEAD
   int ctxid;
} GDI;

#endif