
#ifndef INCLUDE_pygdi_objects_h
#define INCLUDE_pygdi_objects_h


#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <structmember.h>

#define SIMPLE_TYPE(_name, _ptr_type, _ptr_name) \
        typedef struct {\
            PyObject_HEAD\
            _ptr_type *_ptr_name;\
        } _name;


// Main GDI class

typedef struct {
   PyObject_HEAD
   int ctxid;
} GDI;

// Root PyGEObject

typedef struct {
   PyObject_HEAD
   int objtype;
} GEObject;

// Job

typedef struct {
   PyObject_HEAD
} Job;

// SchedConf

// Complex

// Queue

typedef struct {
   PyObject_HEAD
} Queue;

// Host

typedef struct {
   PyObject_HEAD
} Host;

// HostGroup

// User

// UserSet

// ParallelEnvironment

// Project

// Calandar




#endif
