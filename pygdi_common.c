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


sge_gdi_ctx_class_t * getGDIContext( void )
{
   lList *alp = NULL;
   sge_gdi_ctx_class_t *ctx = NULL;

   if (sge_gdi2_setup(&ctx, QSTAT, MAIN_THREAD, &alp) != AE_OK) 
   {
      answer_list_output(&alp);
      lFreeList(&alp);  
      return NULL; 
   }

   return ctx;
}

void closeGDIContext( sge_gdi_ctx_class_t *ctx )
{
   cl_com_handle_t *handle = cl_com_get_handle(ctx->get_component_name(ctx), 0);
   cl_commlib_shutdown_handle(handle, CL_FALSE);
   sge_gdi_ctx_class_destroy(&ctx);
}
