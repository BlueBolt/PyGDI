#ifndef PYGDI_COMMON_H
#define PYGDI_COMMON_H

#include <Python.h>
#include <structmember.h>


typedef struct {
   PyObject_HEAD
   int ctxid;
} GDI;

#endif
