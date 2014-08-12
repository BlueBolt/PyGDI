from distutils.core import setup, Extension
import os

basedir = os.path.dirname(__file__)

include_dirs = [    '.',
                    os.path.join(basedir,'../../libs'),
                    os.path.join(basedir,'../../libs/comm'),
                    os.path.join(basedir,'../../libs/comm/lists'),
                    os.path.join(basedir,'../../libs/cull'),
                    os.path.join(basedir,'../../libs/evc'),
                    os.path.join(basedir,'../../libs/evm'),
                    os.path.join(basedir,'../../libs/gdi'),
                    os.path.join(basedir,'../../libs/rmon'),
                    os.path.join(basedir,'../../libs/mir'),
                    os.path.join(basedir,'../../libs/sched'),
                    os.path.join(basedir,'../../libs/sgeobj'),
                    os.path.join(basedir,'../../libs/uti'),
                    os.path.join(basedir,'../../common')]

extra_objects = [os.path.join(basedir,'../../LINUXX64/libsge.a')]

pygdi_core = Extension('pygdi._pygdi',
                    sources = ['pygdi_common.c','pygdi_GDI.c','pygdi.c'],
                    include_dirs = include_dirs,
                    extra_objects = extra_objects,)

setup (name = 'PyGDI',
       version = '1.0',
       description = 'Basic Python Bindings to GDI API',
       packages=['pygdi'],
       ext_modules = [pygdi_core])