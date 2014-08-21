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

#include "pygdi_common.h"

#define MAX_GDI_CTX_ARRAY_SIZE 1024

static pthread_mutex_t sge_gdi_ctx_mutex = PTHREAD_MUTEX_INITIALIZER;
static sge_gdi_ctx_class_t* sge_gdi_ctx_array[MAX_GDI_CTX_ARRAY_SIZE];

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
