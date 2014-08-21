#ifndef PYGDI_GDI_H
#define PYGDI_GDI_H

#include <Python.h>
#include <structmember.h>


typedef struct {
   PyObject_HEAD
   int ctxid;
} GDI;

#endif
