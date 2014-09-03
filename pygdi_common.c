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
// File:pygdi_common.c
// 
// 
//******************************************************************************

/*___INFO__MARK_BEGIN__*/
/*************************************************************************
 *
 *  The Contents of this file are made available subject to the terms of
 *  the Sun Industry Standards Source License Version 1.2
 *
 *  Sun Microsystems Inc., March, 2001
 *
 *
 *  Sun Industry Standards Source License Version 1.2
 *  =================================================
 *  The contents of this file are subject to the Sun Industry Standards
 *  Source License Version 1.2 (the "License"); You may not use this file
 *  except in compliance with the License. You may obtain a copy of the
 *  License at http://gridengine.sunsource.net/Gridengine_SISSL_license.html
 *
 *  Software provided under this License is provided on an "AS IS" basis,
 *  WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING,
 *  WITHOUT LIMITATION, WARRANTIES THAT THE SOFTWARE IS FREE OF DEFECTS,
 *  MERCHANTABLE, FIT FOR A PARTICULAR PURPOSE, OR NON-INFRINGING.
 *  See the License for the specific provisions governing your rights and
 *  obligations concerning the Software.
 *
 *   The Initial Developer of the Original Code is: Sun Microsystems, Inc.
 *
 *   Copyright: 2001 by Sun Microsystems, Inc.
 *
 *   All Rights Reserved.
 *
 ************************************************************************/
/*___INFO__MARK_END__*/

#include <Python.h>
#include <structmember.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "rmon/sgermon.h"

#include "uti/sge_prog.h"
#include "uti/sge_bootstrap.h"
#include "uti/sge_edit.h"
#include "uti/sge_log.h"
#include "uti/sge_error_class.h"

#include "cull/cull_list.h"
#include "cull/cull.h"

#include "commlib.h"
#include "cl_errors.h"

#include "gdi/version.h"
#include "gdi/sge_gdi.h"
#include "gdi/sge_gdi2.h"
#include "gdi/sge_gdi_ctx.h"

#include "sgeobj/sge_all_listsL.h"
#include "sgeobj/sge_answer.h"
#include "sgeobj/sge_calendar.h"
#include "sgeobj/sge_qinstance_state.h"
#include "sgeobj/sge_cqueue.h"
#include "sgeobj/sge_ja_task.h"
#include "sgeobj/sge_sharetree.h"
#include "sgeobj/sge_utility.h"
#include "sgeobj/sge_event.h"
#include "sgeobj/sge_object.h"

#include "pygdi_common.h"
#include "types.h"

#define MAX_GDI_CTX_ARRAY_SIZE 1024

static pthread_mutex_t sge_gdi_ctx_mutex = PTHREAD_MUTEX_INITIALIZER;
static sge_gdi_ctx_class_t* sge_gdi_ctx_array[MAX_GDI_CTX_ARRAY_SIZE];

int listelem_to_obj(lListElem *ep,PyObject *obj,const lDescr* descr,lList **alpp)
{
   int ret = 0;
   if (object_has_type(ep,EH_Type))
   {
      obj = PyObject_New(Host,(PyTypeObject * ) &ExecHost_Type );
   }
   
   if (obj == NULL){
      ret = 1;
   }

   return(ret);
}

int nativeInit(void)
{
   lList *alp = NULL;
   int ret = -1;
   sge_gdi_ctx_class_t *ctx = NULL;
   int i;
   int ctx_index = -1;

   pthread_mutex_lock(&sge_gdi_ctx_mutex);
   i = 0;
   while(true) {
      if (i>=MAX_GDI_CTX_ARRAY_SIZE) {
         pthread_mutex_unlock(&sge_gdi_ctx_mutex);
         // THROW_ERROR((env, JGDI_ILLEGAL_STATE, "sge_gdi_ctx_array is full"));
         ret = -1;
         break;
      }
      if (sge_gdi_ctx_array[i] == NULL) {
         
         if (sge_gdi2_setup(&ctx, JGDI, MAIN_THREAD, &alp) != AE_OK) 
         {
            answer_list_output(&alp);
            lFreeList(&alp);  
            ret = -1;
            break;
         }

         /*
         ** TODO: find a more consistent solution for logging -> sge_log()
         **       to suppress any console log output
         */
         log_state_set_log_verbose(0);
         sge_gdi_set_thread_local_ctx(ctx);
   
         if (ctx == NULL) {
            pthread_mutex_unlock(&sge_gdi_ctx_mutex);
            ret = -1;
         } else {
            sge_gdi_ctx_array[i] = ctx;
            ctx_index = i;
            pthread_mutex_unlock(&sge_gdi_ctx_mutex);
            ret = 0;
            break;
         }
      }
      i++;
   }

   ret = ctx->connect(ctx);   
   if (ret != CL_RETVAL_OK) {
      ctx->get_errors(ctx, &alp, true);
      ret = -1;
   }
   lFreeList(&alp);

   sge_gdi_set_thread_local_ctx(NULL);
   if (ret < 0) {
      if (ctx_index >= 0) {
         pthread_mutex_lock(&sge_gdi_ctx_mutex);
         sge_gdi_ctx_array[ctx_index] = NULL;
         pthread_mutex_unlock(&sge_gdi_ctx_mutex);
      }
      sge_gdi_ctx_class_destroy(&ctx);
   } else {
      ret = ctx_index;
   }

   return ret;
}

sge_gdi_ctx_class_t * getGDIContext( int ctx_index )
{
   // lList *alp = NULL;
   sge_gdi_ctx_class_t *ctx = NULL;

   // if (sge_gdi2_setup(&ctx, QSTAT, MAIN_THREAD, &alp) != AE_OK) 
   // {
   //    answer_list_output(&alp);
   //    lFreeList(&alp);  
   //    return NULL; 
   // }

   if (ctx_index != -1)
      ctx = sge_gdi_ctx_array[ctx_index];

   return ctx;
}

/*
* Close/shutdown the connection and destroy the GDI Context
*/

void closeGDIContext( sge_gdi_ctx_class_t *ctx )
{
   cl_com_handle_t *handle = cl_com_get_handle(ctx->get_component_name(ctx), 0);
   cl_commlib_shutdown_handle(handle, CL_FALSE);
   sge_gdi_ctx_class_destroy(&ctx);
}

int py_typeid_to_gdi_type( int type_id, lDescr *obj )
{

   const sge_object_type sge_type = type_id;

   obj = object_type_get_descr(sge_type);

   if (obj == NULL)
      return -1;

   return 0;
}


// void generic_fill_list(PyObject pygdi, PyObject list, lList *lp, lList **alpp) 
// {
// }                   

void pygdi_fill(int ctxid, PyObject *list, PyObject *filter, int target_list, lDescr *descr) 
{

   /* receive Cull Object */
   lList *lp = NULL;
   lList *alp = NULL;
   lCondition *where = NULL;
   lEnumeration *what  = NULL;
   sge_gdi_ctx_class_t *ctx = NULL;
   int ret = -1;

   DENTER(TOP_LAYER, "pygdi_fill");
      
   // if (filter != NULL && target_list != SGE_STN_LIST) { 
   //   ret=build_filter(env, filter, &where, &alp);
   //   if (ret != JGDI_SUCCESS) {
   //      goto error;
   //   }
   // }
   
   /* get context */
   ctx = getGDIContext(ctxid);
   
   if (ctx == NULL) 
   {
      ret = 1;
   }

   sge_gdi_set_thread_local_ctx(ctx);

   /* create what and where */
   what = lWhat("%T(ALL)", descr);
   where = lWhere(""); // for now no filters
   
   /* get list */
   alp = ctx->gdi(ctx, target_list, SGE_GDI_GET, &lp, where, what);

   if (answer_list_has_error(&alp)) {
      ret = 1;
      goto error;
   } else {
      lFreeList(&alp);
   }   

   if (target_list == SGE_STN_LIST) {
      if (answer_list_has_error(&alp)) {
         ret = 1;
         goto error;
      } else {
         lFreeList(&alp);
      }   
   }

   // populate a list
   ret = generic_fill_list(list, lp, &alp);
  
error:
   
   /*
   ** this must be called before throw_error_from_answer_list, otherwise there is a pending 
   ** exception in the way
   */
   sge_gdi_set_thread_local_ctx(NULL);

   /* if error throw exception */
   if (ret != 0) {
      // set error string
   }
   
   lFreeWhat(&what);
   lFreeWhere(&where);
   lFreeList(&lp);
   lFreeList(&alp);
   DRETURN_VOID;
}

int generic_fill_list(PyObject *list, lList *lp, lList **alpp) 
{

   const lDescr *listdescr = NULL;
   lListElem *ep = NULL;
   PyObject* obj;
   int ret = 0;
   int count = 0;

   DENTER(TOP_LAYER, "generic_fill_list");

   list = PyList_New(lGetNumberOfElem(lp));

   listdescr = lGetListDescr(lp);
   for_each(ep, lp) {
      if ((ret = listelem_to_obj(ep, &obj, listdescr, alpp)) != 0) {
         return(ret);
      }
      // add to list object
      PyList_Append(list,obj);
      count++;
   }

   return(ret);

}
