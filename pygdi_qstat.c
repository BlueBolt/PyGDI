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

#include <ctype.h>
#include <string.h>
#include <jni.h>

#include "rmon/sgermon.h"

#include "uti/sge_log.h"
#include "uti/sge_error_class.h"
#include "uti/sge_prog.h"
#include "uti/sge_bootstrap.h"

#include "cull/cull.h"

#include "comm/commlib.h"
#include "cl_errors.h"

#include "sgeobj/sge_all_listsL.h"
#include "sgeobj/sge_answer.h"
#include "sgeobj/sge_qinstance_state.h"

#include "gdi/sge_gdi.h"
#include "gdi/sge_gdi_ctx.h"
#include "gdi/sge_gdi2.h"

#include "basis_types.h"
#include "sge_qstat.h"
#include "qstat_printing.h"
#include "parse.h"
#include "pygdi_common.h"
#include "pygdi.h"
#include "pygdi_wrapper.h"
#include "pygdi_logging.h"


typedef struct {
   
   jobject queue_filter;              /* qstat -q  option */
   jobject resource_filter;           /* qstat -l  option */
   jobject queue_state_filter;        /* qstat -qs option */
   jobject host_filter;               /* qhost -h  option */
   jobject queue_user_filter;         /* qstat -U option  */
   jobject job_user_filter;           /* qstat -u option */
   jobject resource_attribute_filter; /* qstat -F option */
   jobject pe_filter;                 /* qstat -pe option */
   jobject job_state_filter;          /* qstat -s option */

} pygdi_qstat_filter_t;

static pygdi_result_t build_queue_state_filter(JNIEnv *env, jobject queue_state_filter, 
                                 u_long32* queue_state, lList **alpp);
                                 
static pygdi_result_t build_resource_attribute_filter(JNIEnv *env, jobject resource_attribute_filter,
                                           lList**resource_attribute_list, u_long32 *full_listing, lList **alpp);
                                 

static pygdi_result_t pygdi_qstat_env_init(JNIEnv *env, sge_gdi_ctx_class_t *ctx, pygdi_qstat_filter_t *filter, 
                               qstat_env_t* qstat_env, lList **alpp);
                               
int pygdi_qstat_cqueue_summary(cqueue_summary_handler_t *thiz, const char* cqname, 
                              cqueue_summary_t *summary, lList **alpp);
                              

/*-------------------------------------------------------------------------*
 * NAME
 *   pygdi_qstat_env_init - initialize the qstat_env for a pygdi request
 * PARAMETER
 *  env       - the JNI env
 *  ctx       - pygdi context
 *  filter    - qstat filter element 
 *  qstat_env - the qstat_env which will be initialized
 *  alpp      - answer list for error reporting
 *
 * RETURN
 *
 *    PYGDI_SUCCESS - qstat_env successfully initialized
 *    else - Error, reason has been stored in answer list
 *
 * EXTERNAL
 *
 * DESCRIPTION
 *-------------------------------------------------------------------------*/
static pygdi_result_t pygdi_qstat_env_init(JNIEnv *env, sge_gdi_ctx_class_t *ctx, pygdi_qstat_filter_t *filter, 
                               qstat_env_t* qstat_env, lList **alpp) {
   pygdi_result_t ret = PYGDI_SUCCESS;
   
   DENTER(PYGDI_LAYER, "pygdi_qstat_env_init");
   
   qstat_env->ctx = ctx;
   qstat_env->full_listing = QSTAT_DISPLAY_ALL;
   qstat_env->explain_bits = QI_DEFAULT;
   qstat_env->need_queues = true;
   qstat_env->job_info = 0;
   qstat_env->group_opt = 0;
   qstat_env->queue_state = U_LONG32_MAX;
   qstat_env->longest_queue_length=30;
   
   /*
   ** add default what filter of qstat
   */
   qstat_filter_add_core_attributes(qstat_env);

   
   if (filter->queue_filter != NULL) {
      if((ret=get_string_list(env, filter->queue_filter, "getQueues", &(qstat_env->queueref_list), QR_Type, QR_name, alpp))) {
         DPRINTF(("get_string_list for queue_filter failed\n"));
         goto error;
      }
      
      pygdi_log_printf(env, PYGDI_QSTAT_LOGGER, FINE,
                      "BEGIN ------------- queue filter --------------");
      
      pygdi_log_list(env, PYGDI_QSTAT_LOGGER, FINE, qstat_env->queueref_list);
      
      pygdi_log_printf(env, PYGDI_QSTAT_LOGGER, FINE,
                      "END ------------- queue filter --------------");
                      
      qstat_filter_add_q_attributes(qstat_env);                    
   }
   
   if (filter->resource_filter != NULL) {
      if ((ret=build_resource_filter(env, filter->resource_filter, &(qstat_env->resource_list), alpp)) != PYGDI_SUCCESS) {
         goto error;
      }
      qstat_filter_add_l_attributes(qstat_env);
   }


   if ((ret=build_resource_attribute_filter(env, filter->resource_attribute_filter, &(qstat_env->qresource_list), &(qstat_env->full_listing), alpp)) != PYGDI_SUCCESS) {
      goto error;
   }
   
   if (filter->pe_filter != NULL) {
      if((ret=get_string_list(env, filter->pe_filter, "getPEList", &(qstat_env->pe_list), ST_Type, ST_name, alpp))) {
         DPRINTF(("get_string_list for queue_filter failed\n"));
         goto error;
      }
      
      pygdi_log_printf(env, PYGDI_QSTAT_LOGGER, FINE,
                      "BEGIN ------------- PE filter --------------");
      
      pygdi_log_list(env, PYGDI_QSTAT_LOGGER, FINE, qstat_env->pe_list);
      
      pygdi_log_printf(env, PYGDI_QSTAT_LOGGER, FINE,
                      "END ------------- PE filter --------------");
                      
      qstat_filter_add_pe_attributes(qstat_env);
   }

   if ((ret=build_queue_state_filter(env, filter->queue_state_filter, &(qstat_env->queue_state), alpp))) {
      goto error;
   }
   
   if (filter->queue_user_filter != NULL) {
      if((ret=get_string_list(env, filter->queue_user_filter, "getUsers", &(qstat_env->queue_user_list), ST_Type, ST_name, alpp))) {
         DPRINTF(("get_string_list for queue_user_list failed\n"));
         goto error;
      }
      
      pygdi_log_printf(env, PYGDI_QSTAT_LOGGER, FINE,
                      "BEGIN ------------- queue user filter --------------");
      
      pygdi_log_list(env, PYGDI_QSTAT_LOGGER, FINE, qstat_env->queue_user_list);
      
      pygdi_log_printf(env, PYGDI_QSTAT_LOGGER, FINE, 
                      "END ------------- queue user filter --------------");
                      
      qstat_filter_add_U_attributes(qstat_env);
   }
   
   if (filter->job_user_filter != NULL) {
      if((ret=get_string_list(env, filter->job_user_filter, "getUsers", &(qstat_env->user_list),  ST_Type, ST_name, alpp))) {
         DPRINTF(("get_string_list for job_user_filter failed\n"));
         goto error;
      }
      
      pygdi_log_printf(env, PYGDI_QSTAT_LOGGER, FINE,
                      "BEGIN ------------- job user filter --------------");
      
      pygdi_log_list(env, PYGDI_QSTAT_LOGGER, FINE, qstat_env->user_list);
      
      pygdi_log_printf(env, PYGDI_QSTAT_LOGGER, FINE,
                      "END ------------- job user filter --------------");
   }
   
   if (filter->job_state_filter != NULL) {
      jstring job_state_obj = NULL; 
      const char * job_state = NULL;
      int filter_res = 0;
      if ((ret=JobStateFilter_getStateString(env, filter->job_state_filter, &job_state_obj, alpp)) != PYGDI_SUCCESS) {
         goto error;
      }
      job_state = (*env)->GetStringUTFChars(env, job_state_obj, 0);
      if (job_state == NULL) {
         answer_list_add(alpp, "pygdi_qstat_env_init: GetStringUTFChars failed. Out of memory.", STATUS_EMALLOC, ANSWER_QUALITY_ERROR);
         DRETURN(PYGDI_ERROR);
      }
      filter_res = build_job_state_filter(qstat_env, job_state, alpp);
      (*env)->ReleaseStringUTFChars(env, job_state_obj, job_state);
      
      if(filter_res != 0) {
         ret = PYGDI_ERROR;
         goto error;
      }
   }
   
error:

  DRETURN(ret);
}


static pygdi_result_t build_queue_state_filter(JNIEnv *env, jobject queue_state_filter, u_long32* queue_state, lList **alpp)
{
   pygdi_result_t ret = PYGDI_SUCCESS;

   DENTER(PYGDI_LAYER, "build_queue_state_filter");
   
   if (queue_state_filter != NULL) {
      jstring options_obj = NULL;
      if ((ret = QueueStateFilter_getOptions(env, queue_state_filter, &options_obj, alpp)) != PYGDI_SUCCESS) {
         DRETURN(ret);
      } else {
         const char* options = NULL;
         u_long32 filter = 0xFFFFFFFF;
         if (options_obj == NULL) {     
            answer_list_add(alpp, "build_queue_state_filter: options_obj is NULL", STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
            DRETURN(PYGDI_ILLEGAL_STATE);
         }
         options = (*env)->GetStringUTFChars(env, options_obj, 0);
         if (options == NULL) {
            answer_list_add(alpp, "build_queue_state_filter: GetStringUTFChars failed. Out of memory.", STATUS_EMALLOC, ANSWER_QUALITY_ERROR);
            DRETURN(PYGDI_ERROR);
         }
         pygdi_log_printf(env, PYGDI_QSTAT_LOGGER, FINE,
                             "queue state filter is '%s'", options);
         *queue_state = qinstance_state_from_string(options, alpp, filter);
         (*env)->ReleaseStringUTFChars(env, options_obj, options);
      }
   } else {
      *queue_state = U_LONG32_MAX;
   }
   
   DRETURN(ret);
}


static pygdi_result_t build_resource_attribute_filter(JNIEnv *env, jobject resource_attribute_filter,
                                                     lList **resource_attribute_list, u_long32 *full_listing, lList **alpp) {
   jobject value_name_list = NULL;
   jobject iterator = NULL;
   pygdi_result_t ret = PYGDI_SUCCESS;
   u_long32 count = 0;
   
   DENTER(PYGDI_LAYER, "build_resource_attribute_filter");

   if (resource_attribute_filter == NULL) {
      DRETURN(PYGDI_SUCCESS);
   }
   
   *full_listing |= QSTAT_DISPLAY_QRESOURCES|QSTAT_DISPLAY_FULL;
   
   if ((ret = ResourceAttributeFilter_getValueNames(env, resource_attribute_filter, &value_name_list, alpp)) != PYGDI_SUCCESS) {
      DPRINTF(("ResourceAttributeFilter_getValueNames failed\n"));
      goto error;
   }
   
   /* we allow an empty list */
   if (value_name_list == NULL) {
      DRETURN(PYGDI_SUCCESS);
   }
   
   if ((ret = List_iterator(env, value_name_list, &iterator, alpp)) != PYGDI_SUCCESS) {
      goto error;
   }
   
   while (1) {
      jboolean has_next = false;
      jobject  name_obj = NULL;
      const char *name = NULL;
      
      if ((ret = Iterator_hasNext(env, iterator, &has_next, alpp)) != PYGDI_SUCCESS) {
         goto error;
      }
      if(!has_next) {
         break;
      }
      if ((ret = Iterator_next(env, iterator, &name_obj, alpp)) != PYGDI_SUCCESS) {
         goto error;
      } 
      if (name_obj == NULL) {
         answer_list_add(alpp, "build_resource_attribute_filter: name_obj is NULL.", STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
         ret = PYGDI_ILLEGAL_STATE;
         goto error;
      }
      name = (*env)->GetStringUTFChars(env, name_obj, 0);
      if (name == NULL) {
         answer_list_add(alpp, "build_resource_attribute_filter: GetStringUTFChars failed. Out of memory.", STATUS_EMALLOC, ANSWER_QUALITY_ERROR);
         ret = PYGDI_ERROR;
         goto error;
      }
      lAddElemStr(resource_attribute_list, CE_name, name, CE_Type);
      (*env)->ReleaseStringUTFChars(env, name_obj, name);
      count++;
   }
  
   if (count > 0) {
      pygdi_log_printf(env, PYGDI_QSTAT_LOGGER, FINE,
                      "BEGIN ------------- resource attribute filter --------------");
      
      pygdi_log_list(env, PYGDI_QSTAT_LOGGER, FINE, *resource_attribute_list);
      
      pygdi_log_printf(env, PYGDI_QSTAT_LOGGER, FINE,
                      "END ------------- resource attribute filter --------------");
   }
error:
   if (ret != PYGDI_SUCCESS) {
      lFreeList(resource_attribute_list);
   }
   DRETURN(ret);
}

/*-------------------------------------------------------------------------*
 * NAME
 *   build_resource_filter - build the resource filter for a qstat_env
 * PARAMETER
 *  env       - the JNI environment
 *  filter    - the qstat filter definition
 *  qstat_env - the qstat environment
 *  alpp      - answer list for error reporting
 *
 * RETURN
 *
 *  PYGDI_SUCCESS - filer successfully built
 *  else - error, reason has been reported in alpp
 *
 * EXTERNAL
 *
 * DESCRIPTION
 *-------------------------------------------------------------------------*/
pygdi_result_t build_resource_filter(JNIEnv *env, jobject resource_filter, 
                                 lList **resource_list, lList **alpp) {
   pygdi_result_t ret = PYGDI_SUCCESS;
   jobject resource_name_set = NULL;
   jobject iterator = NULL;
   u_long32 count = 0;
   
   DENTER(PYGDI_LAYER, "build_resource_filter");
   
   if (resource_list == NULL) {
      answer_list_add(alpp, "build_resource_filter: resource list is NULL",
                      STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
      DRETURN(PYGDI_ILLEGAL_ARGUMENT);
   }

   if (resource_filter == NULL) {
      DRETURN(PYGDI_SUCCESS);
   }
   if ((ret = ResourceFilter_getResourceNames(env, resource_filter, &resource_name_set, alpp)) != PYGDI_SUCCESS) {
      DPRINTF(("ResourceFilter_getResources failed\n"));
      goto error;
   }
   if (resource_name_set == NULL) {
      answer_list_add(alpp, "method getResourceNames return null",
                      STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
      ret = PYGDI_ILLEGAL_ARGUMENT;
      goto error;
   }
   
   if ((ret = Set_iterator(env, resource_name_set, &iterator, alpp)) != PYGDI_SUCCESS) {
      DPRINTF(("Set_iterator failed\n"));
      goto error;
   }
   
   while (1) {
      jboolean has_next = false;
      jobject  name_obj = NULL;
      jstring  value_obj = NULL;
      lListElem *complex_attribute = NULL;
      const char *name = NULL;
      
      if ((ret = Iterator_hasNext(env, iterator, &has_next, alpp)) != PYGDI_SUCCESS) {
         DPRINTF(("Iterator_hasNext failed\n"));
         goto error;
      }
      if(!has_next) {
         break;
      }
      if ((ret = Iterator_next(env, iterator, &name_obj, alpp)) != PYGDI_SUCCESS) {
         DPRINTF(("Iterator_next failed\n"));
         goto error;
      }
      if (name_obj == NULL) {
         answer_list_add(alpp, "build_resource_filter: name_obj is NULL.", STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
         ret = PYGDI_ILLEGAL_STATE;
         goto error;
      }
      name = (*env)->GetStringUTFChars(env, name_obj, 0);
      if (name == NULL) {
         answer_list_add(alpp, "build_resource_filter: GetStringUTFChars failed. Out of memory.", STATUS_EMALLOC, ANSWER_QUALITY_ERROR);
         ret = PYGDI_ERROR;
         goto error;
      }
      complex_attribute = lAddElemStr(resource_list, CE_name, name, CE_Type);
      (*env)->ReleaseStringUTFChars(env, name_obj, name);
      
      if ((ret = ResourceFilter_getResource(env, resource_filter, lGetString(complex_attribute, CE_name), 
                                     &value_obj, alpp)) != PYGDI_SUCCESS) {
         DPRINTF(("ResourceFilter_getResource failed\n"));
         goto error;
      } 
      if (value_obj != NULL) {
         const char *value = (*env)->GetStringUTFChars(env, value_obj, 0);
         if (value == NULL) {
            answer_list_add(alpp, "build_resource_filter: GetStringUTFChars failed. Out of memory.", STATUS_EMALLOC, ANSWER_QUALITY_ERROR);
            ret = PYGDI_ERROR;
            goto error;
         }
         lSetString(complex_attribute, CE_stringval, value);
         (*env)->ReleaseStringUTFChars(env, value_obj, value);
      } else {
         answer_list_add_sprintf(alpp, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR,
                                 "resource %s has null value", 
                                 lGetString(complex_attribute, CE_name));
         ret = PYGDI_ILLEGAL_ARGUMENT;
         goto error;
      }

      count++; 
   }
   
   if (count > 0) {
      pygdi_log_printf(env, PYGDI_QSTAT_LOGGER, FINE,
                      "BEGIN ------------- resource attribute filter --------------");
      
      pygdi_log_list(env, PYGDI_QSTAT_LOGGER, FINE, *resource_list);
      
      pygdi_log_printf(env, PYGDI_QSTAT_LOGGER, FINE,
                      "END ------------- resource attribute filter --------------");
   }


error:
   if (ret != PYGDI_SUCCESS) {
      lFreeList(resource_list);
   }
   DRETURN(ret);
}

typedef struct {
   JNIEnv   *jni_env;
   jobject   list;     /* list where the instances of ClusterQueueSummary are stored */
   pygdi_result_t result;
} pygdi_qstat_cqueue_summary_ctx_t;


/*-------------------------------------------------------------------------*
 * NAME
 *   pygdi_qstat_cqueue_summary - handler for qstat cluster queue summary algorithm
 * PARAMETER
 *  handler  - handler
 *  cqname   - name of the cluster queue
 *  summary  - container the cluster queue summary 
 *  alpp     - answer list for error reporting
 *
 * RETURN
 *    0   if the cluster queue summary has been handled correctly
 *    -1  error
 *
 * DESCRIPTION
 *-------------------------------------------------------------------------*/
int pygdi_qstat_cqueue_summary(cqueue_summary_handler_t *handler, const char* cqname, 
                              cqueue_summary_t *summary, lList **alpp) {
   pygdi_qstat_cqueue_summary_ctx_t *ctx = (pygdi_qstat_cqueue_summary_ctx_t*)handler->ctx;
   jobject cqueue_summary = NULL;
   pygdi_result_t ret = PYGDI_SUCCESS;
   
   DENTER(PYGDI_LAYER, "pygdi_qstat_cqueue_summary");

   if (ctx == NULL) {
      DPRINTF(("ctx is NULL\n"));
      abort();
   }
   if (ctx->jni_env == NULL) {
      DPRINTF(("ctx->jni_env is NULL\n"));
      abort();
   }
   if (ctx->list == NULL) {
      DPRINTF(("ctx->list is NULL\n"));
      abort();
   }
   
   ret = ClusterQueueSummary_init(ctx->jni_env, &cqueue_summary, alpp);
   
   if (ret != PYGDI_SUCCESS) {
      DPRINTF(("constructor for ClusterQueueSummary failed\n"));
      goto error;
   }
   
   if (summary->is_load_available) {
      ret = ClusterQueueSummary_setLoad(ctx->jni_env, cqueue_summary, summary->load, alpp);
      if( ret != PYGDI_SUCCESS) {
         goto error;
      }
   }
   
   if ((ret = ClusterQueueSummary_setName(ctx->jni_env, cqueue_summary, cqname, alpp)) != PYGDI_SUCCESS) {
      goto error;
   }
   if ((ret = ClusterQueueSummary_setReservedSlots(ctx->jni_env, cqueue_summary, summary->resv, alpp)) != PYGDI_SUCCESS) {
      goto error;
   }
   if ((ret = ClusterQueueSummary_setUsedSlots(ctx->jni_env, cqueue_summary, summary->used, alpp)) != PYGDI_SUCCESS) {
      goto error;
   }
   if ((ret = ClusterQueueSummary_setTotalSlots(ctx->jni_env, cqueue_summary, summary->total, alpp)) != PYGDI_SUCCESS) {
      goto error;
   }
   if ((ret = ClusterQueueSummary_setAvailableSlots(ctx->jni_env, cqueue_summary, summary->available, alpp)) != PYGDI_SUCCESS) {
      goto error;
   }
   if ((ret = ClusterQueueSummary_setTempDisabled(ctx->jni_env, cqueue_summary, summary->temp_disabled, alpp)) != PYGDI_SUCCESS) {
      goto error;
   }
   if ((ret = ClusterQueueSummary_setManualIntervention(ctx->jni_env, cqueue_summary, summary->manual_intervention, alpp)) != PYGDI_SUCCESS) {
      goto error;
   }
   if ((ret = ClusterQueueSummary_setSuspendManual(ctx->jni_env, cqueue_summary, summary->suspend_manual, alpp)) != PYGDI_SUCCESS) {
      goto error;
   }
   if ((ret = ClusterQueueSummary_setSuspendThreshold(ctx->jni_env, cqueue_summary, summary->suspend_threshold, alpp)) != PYGDI_SUCCESS) {
      goto error;
   }
   if ((ret = ClusterQueueSummary_setSuspendOnSubordinate(ctx->jni_env, cqueue_summary, summary->suspend_on_subordinate, alpp)) != PYGDI_SUCCESS) {
      goto error;
   }
   if ((ret = ClusterQueueSummary_setSuspendByCalendar(ctx->jni_env, cqueue_summary, summary->suspend_calendar, alpp)) != PYGDI_SUCCESS) {
      goto error;
   }
   if ((ret = ClusterQueueSummary_setUnknown(ctx->jni_env, cqueue_summary, summary->unknown, alpp)) != PYGDI_SUCCESS) {
      goto error;
   }
   if ((ret = ClusterQueueSummary_setLoadAlarm(ctx->jni_env, cqueue_summary, summary->load_alarm, alpp)) != PYGDI_SUCCESS) {
      goto error;
   }
   if ((ret = ClusterQueueSummary_setDisabledManual(ctx->jni_env, cqueue_summary, summary->disabled_manual, alpp)) != PYGDI_SUCCESS) {
      goto error;
   }
   if ((ret = ClusterQueueSummary_setDisabledByCalendar(ctx->jni_env, cqueue_summary, summary->disabled_calendar, alpp)) != PYGDI_SUCCESS) {
      goto error;
   }
   if ((ret = ClusterQueueSummary_setAmbiguous(ctx->jni_env, cqueue_summary, summary->ambiguous, alpp)) != PYGDI_SUCCESS) {
      goto error;
   }
   if ((ret = ClusterQueueSummary_setOrphaned(ctx->jni_env, cqueue_summary, summary->orphaned, alpp)) != PYGDI_SUCCESS) {
      goto error;
   }
   if ((ret = ClusterQueueSummary_setError(ctx->jni_env, cqueue_summary, summary->error, alpp)) != PYGDI_SUCCESS) {
      goto error;
   }
   {
      jboolean add_result = false;
      if ((ret = List_add(ctx->jni_env, ctx->list, cqueue_summary, &add_result, alpp)) != PYGDI_SUCCESS) {
         goto error;
      }
   }
   
error:
   ctx->result = ret;
   if (ret != PYGDI_SUCCESS) {
      DRETURN(-1);
   }
   DRETURN(0);
}

/* ---------------------------------------------------------------------------*/
/* ------------------ QSTAT HANDLER ------------------------------------------*/
/* ---------------------------------------------------------------------------*/
typedef struct {
   JNIEnv *jni_env;
   jobject job;     /* the current job object (instanceof com.sun.grid.pygdi.monitoring.JobSummary */
   jobject list;    /* list where jobs will be stored at the job_finished event */
   pygdi_result_t result;
} pygdi_job_ctx_t;

static pygdi_result_t pygdi_qstat_job_init(job_handler_t* handler, pygdi_job_ctx_t *ctx, JNIEnv *env, lList **alpp);
static int pygdi_qstat_job(job_handler_t* handler, u_long32 jid, job_summary_t *summary, lList **alpp);
static int pygdi_qstat_sub_task(job_handler_t* handler, task_summary_t *summary, lList **alpp);
static int pygdi_qstat_job_additional_info(job_handler_t *handler, job_additional_info_t name, const char* value, lList **alpp);
static int pygdi_qstat_job_requested_pe(job_handler_t *handler, const char* pe_name, const char* pe_range, lList **alpp);
static int pygdi_qstat_job_granted_pe(job_handler_t *handler, const char* pe_name, int pe_slots, lList **alpp);
static int pygdi_qstat_job_request(job_handler_t* handler, const char* name, const char* value, lList **alpp);
static int pygdi_qstat_job_hard_resource(job_handler_t *handler, const char* name, const char* value, double uc, lList **alpp);
static int pygdi_qstat_job_soft_resource(job_handler_t *handler, const char* name, const char* value, double uc, lList **alpp);
static int pygdi_qstat_job_hard_requested_queue(job_handler_t *handler, const char* name, lList **alpp);
static int pygdi_qstat_job_soft_requested_queue(job_handler_t *handler, const char* name, lList **alpp);
static int pygdi_qstat_job_master_hard_requested_queue(job_handler_t* handler, const char* name, lList **alpp);
static int pygdi_qstat_job_predecessor_requested(job_handler_t* handler, const char* name, lList **alpp);
static int pygdi_qstat_job_predecessor(job_handler_t* handler, u_long32 jid, lList **alpp);
static int pygdi_qstat_job_ad_predecessor_requested(job_handler_t* handler, const char* name, lList **alpp);
static int pygdi_qstat_job_ad_predecessor(job_handler_t* handler, u_long32 jid, lList **alpp);
static int pygdi_qstat_job_finished(job_handler_t* handler, u_long32 jid, lList **alpp);

   
   
static int pygdi_qstat_job(job_handler_t* handler, u_long32 jid, job_summary_t *summary, lList **alpp) {
   pygdi_job_ctx_t *ctx = (pygdi_job_ctx_t*)handler->ctx;
   pygdi_result_t ret = PYGDI_SUCCESS;
   
   DENTER(PYGDI_LAYER, "pygdi_qstat_job");
   
   if ((ret = JobSummaryImpl_init(ctx->jni_env, &(ctx->job), alpp)) != PYGDI_SUCCESS) {
      goto error;
   }
   if ((ret = JobInfoImpl_setId(ctx->jni_env, ctx->job, jid, alpp)) != PYGDI_SUCCESS) {
      goto error;
   }
   if ((ret = JobInfoImpl_setMasterQueue(ctx->jni_env, ctx->job, summary->master, alpp)) != PYGDI_SUCCESS) {
      goto error;
   }
   if ((ret = JobInfoImpl_setName(ctx->jni_env, ctx->job, summary->name, alpp)) != PYGDI_SUCCESS) {
      goto error;
   }
   if ((ret = JobSummaryImpl_setDepartment(ctx->jni_env, ctx->job, summary->department, alpp)) != PYGDI_SUCCESS) {
      goto error;
   }
   if ((ret = JobSummaryImpl_setProject(ctx->jni_env, ctx->job, summary->project, alpp)) != PYGDI_SUCCESS) {
      goto error;
   }
   if ((ret = JobInfoImpl_setUser(ctx->jni_env, ctx->job, summary->user, alpp)) != PYGDI_SUCCESS) {
      goto error;
   }
   if ((ret = JobInfoImpl_setState(ctx->jni_env, ctx->job, summary->state, alpp)) != PYGDI_SUCCESS) {
      goto error;
   }
   if ((ret = JobSummaryImpl_setQueueAssigned(ctx->jni_env, ctx->job, summary->is_queue_assigned, alpp)) != PYGDI_SUCCESS) {
      goto error;
   }
/*    if (summary->is_queue_assigned) { */
      if ((ret = JobInfoImpl_setQueue(ctx->jni_env, ctx->job, summary->queue, alpp)) != PYGDI_SUCCESS) {
         goto error;
      }
/*    } */
   if ((ret = JobInfoImpl_setPriority(ctx->jni_env, ctx->job, summary->priority, alpp)) != PYGDI_SUCCESS) {
      goto error;
   }
   if ((ret = JobSummaryImpl_setNormalizedRequestedPriority(ctx->jni_env, ctx->job, summary->nprior, alpp)) != PYGDI_SUCCESS) {
      goto error;
   }
   if ((ret = JobSummaryImpl_setNormalizedPriority(ctx->jni_env, ctx->job, summary->nprior, alpp)) != PYGDI_SUCCESS) {
      goto error;
   }
   if ((ret = JobSummaryImpl_setNormalizedTickets(ctx->jni_env, ctx->job, summary->ntckts, alpp)) != PYGDI_SUCCESS) {
      goto error;
   }
   if ((ret = JobSummaryImpl_setNormalizedUrgency(ctx->jni_env, ctx->job, summary->nurg, alpp)) != PYGDI_SUCCESS) {
      goto error;
   }
   if ((ret = JobSummaryImpl_setUrgency(ctx->jni_env, ctx->job, summary->urg, alpp)) != PYGDI_SUCCESS) {
      goto error;
   }
   if ((ret = JobSummaryImpl_setOverrideTickets(ctx->jni_env, ctx->job, summary->override_tickets, alpp)) != PYGDI_SUCCESS) {
      goto error;
   }
   if ((ret = JobSummaryImpl_setTickets(ctx->jni_env, ctx->job, summary->tickets, alpp)) != PYGDI_SUCCESS) {
      goto error;
   }
   if ((ret = JobSummaryImpl_setOtickets(ctx->jni_env, ctx->job, summary->otickets, alpp)) != PYGDI_SUCCESS) {
      goto error;
   }
   if ((ret = JobSummaryImpl_setFtickets(ctx->jni_env, ctx->job, summary->ftickets, alpp)) != PYGDI_SUCCESS) {
      goto error;
   }
   if ((ret = JobSummaryImpl_setStickets(ctx->jni_env, ctx->job, summary->stickets, alpp)) != PYGDI_SUCCESS) {
      goto error;
   }
   if ((ret = JobSummaryImpl_setRrcontr(ctx->jni_env, ctx->job, summary->rrcontr, alpp)) != PYGDI_SUCCESS) {
      goto error;
   }
   if ((ret = JobSummaryImpl_setWtcontr(ctx->jni_env, ctx->job, summary->wtcontr, alpp)) != PYGDI_SUCCESS) {
      goto error;
   }
   if ((ret = JobSummaryImpl_setShare(ctx->jni_env, ctx->job, summary->share, alpp)) != PYGDI_SUCCESS) {
      goto error;
   }
   if ((ret = JobSummaryImpl_setSlots(ctx->jni_env, ctx->job, summary->slots, alpp)) != PYGDI_SUCCESS) {
      goto error;
   }
   if ((ret = JobInfoImpl_setStartTime_0(ctx->jni_env, ctx->job, ((jlong)summary->start_time) * 1000, alpp)) != PYGDI_SUCCESS) {
      goto error;
   }
   if ((ret = JobInfoImpl_setSubmitTime_0(ctx->jni_env, ctx->job, ((jlong)summary->submit_time) * 1000, alpp)) != PYGDI_SUCCESS) {
      goto error;
   }
   
   if ((ret = JobSummaryImpl_setDeadline_0(ctx->jni_env, ctx->job, ((jlong)summary->deadline) * 1000, alpp)) != PYGDI_SUCCESS) {
      goto error;
   }
   
   if ((ret = JobSummaryImpl_setRunning(ctx->jni_env, ctx->job, summary->is_running, alpp)) != PYGDI_SUCCESS) {
      goto error;
   }
   
   if ((ret = JobSummaryImpl_setArray(ctx->jni_env, ctx->job, summary->is_array, alpp)) != PYGDI_SUCCESS) {
      goto error;
   }
   
   if (summary->is_array) {
      if ((ret = JobInfoImpl_setTaskId(ctx->jni_env, ctx->job, summary->task_id, alpp)) != PYGDI_SUCCESS) {
         goto error;
      }
   }
   if (summary->has_cpu_usage) {
      if ((ret = JobSummaryImpl_setCpuUsage(ctx->jni_env, ctx->job, summary->cpu_usage, alpp)) != PYGDI_SUCCESS) {
         goto error;
      }
   }
   if (summary->has_mem_usage) {
      if ((ret = JobSummaryImpl_setMemUsage(ctx->jni_env, ctx->job, summary->mem_usage, alpp)) != PYGDI_SUCCESS) {
         goto error;
      }
   }
   if (summary->has_io_usage) {
      if ((ret = JobSummaryImpl_setIoUsage(ctx->jni_env, ctx->job, summary->io_usage, alpp)) != PYGDI_SUCCESS) {
         goto error;
      }
   }

error:
   if (ret != PYGDI_SUCCESS) {
      ctx->result = ret;
      DRETURN(-1);
   }
   ctx->result = PYGDI_SUCCESS;
   DRETURN(0);
}

static int pygdi_qstat_sub_task(job_handler_t* handler, task_summary_t *summary, lList **alpp) {
   pygdi_job_ctx_t *ctx = (pygdi_job_ctx_t*)handler->ctx;
   jobject task = NULL;
   pygdi_result_t ret = PYGDI_SUCCESS;
   
   DENTER(PYGDI_LAYER, "pygdi_qstat_sub_task");
   
   if ((ret = TaskSummaryImpl_init(ctx->jni_env, &task, alpp)) != PYGDI_SUCCESS) {
      goto error;
   }
   
   if ((ret = TaskSummaryImpl_setTaskId(ctx->jni_env, task, summary->task_id, alpp)) != PYGDI_SUCCESS) {
      goto error;
   }
   
   if (summary->has_cpu_usage) {
      if ((ret = TaskSummaryImpl_setCpuUsage(ctx->jni_env, task, summary->cpu_usage, alpp)) != PYGDI_SUCCESS) {
         goto error;
      }
   }
   if (summary->has_io_usage) {
      if ((ret = TaskSummaryImpl_setIoUsage(ctx->jni_env, task, summary->io_usage, alpp)) != PYGDI_SUCCESS) {
         goto error;
      }
   }
   if (summary->has_mem_usage) {
      if ((ret = TaskSummaryImpl_setMemUsage(ctx->jni_env, task, summary->mem_usage, alpp)) != PYGDI_SUCCESS) {
         goto error;
      }
   }
   
   if ((ret = TaskSummaryImpl_setState(ctx->jni_env, task, summary->state, alpp)) != PYGDI_SUCCESS) {
      goto error;
   }
   if ((ret = TaskSummaryImpl_setRunning(ctx->jni_env, task, summary->is_running, alpp)) != PYGDI_SUCCESS) {
      goto error;
   }
   if (summary->has_exit_status) {
      if ((ret = TaskSummaryImpl_setExitStatus(ctx->jni_env, task, summary->exit_status, alpp)) != PYGDI_SUCCESS) {
         goto error;
      }
   }
   
   if((ret = JobSummaryImpl_addTask(ctx->jni_env, ctx->job, task, alpp)) != PYGDI_SUCCESS) {
      goto error;
   }

error:

   ctx->result = ret;
   if (ret != PYGDI_SUCCESS) {
      DRETURN(-1);
   }
   DRETURN(0);
}

static int pygdi_qstat_job_additional_info(job_handler_t *handler, job_additional_info_t name, const char* value, lList **alpp) { 
   pygdi_job_ctx_t *ctx = (pygdi_job_ctx_t*)handler->ctx;
   pygdi_result_t ret = PYGDI_SUCCESS;
   DENTER(PYGDI_LAYER, "pygdi_qstat_job_additional_info");

   switch (name) {
      case CHECKPOINT_ENV: {
        ret = JobSummaryImpl_setCheckpointEnv(ctx->jni_env, ctx->job, value, alpp);
        break;
      }
      case MASTER_QUEUE: {
        ret = JobInfoImpl_setMasterQueue(ctx->jni_env, ctx->job, value, alpp);
        break;
      }
      case FULL_JOB_NAME: { /* Ingore it */
        break;
      }
      default: {
         answer_list_add_sprintf(alpp, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR,
                                 "Unkown additional job info %s", name);
         ret = PYGDI_ILLEGAL_STATE;
      }
   }
   ctx->result = ret;
   if (ret != PYGDI_SUCCESS) {
      DRETURN(-1);
   }
   DRETURN(0);
}

static int pygdi_qstat_job_requested_pe(job_handler_t *handler, const char* pe_name, const char* pe_range, lList **alpp) {
   pygdi_job_ctx_t *ctx = (pygdi_job_ctx_t*)handler->ctx;
   pygdi_result_t ret = PYGDI_SUCCESS;
   
   DENTER(PYGDI_LAYER, "pygdi_qstat_job_requested_pe");

   if ((ret = JobSummaryImpl_setParallelEnvironmentName(ctx->jni_env, ctx->job, pe_name, alpp)) != PYGDI_SUCCESS) {
      goto error;
   }
   if ((ret = JobSummaryImpl_setParallelEnvironmentRange(ctx->jni_env, ctx->job, pe_range, alpp)) != PYGDI_SUCCESS) {
      goto error;
   }
   
error:
   ctx->result = ret;
   if (ret != PYGDI_SUCCESS) {
      DRETURN(-1);
   }
   DRETURN(0);
}

static int pygdi_qstat_job_granted_pe(job_handler_t *handler, const char* pe_name, int pe_slots, lList **alpp) {
   pygdi_job_ctx_t *ctx = (pygdi_job_ctx_t*)handler->ctx;
   pygdi_result_t ret = PYGDI_SUCCESS;
   
   DENTER(PYGDI_LAYER, "pygdi_qstat_job_granted_pe");
   
   if ((ret = JobSummaryImpl_setGrantedPEName(ctx->jni_env, ctx->job, pe_name, alpp)) != PYGDI_SUCCESS) {
      goto error;
   }
   if ((ret = JobSummaryImpl_setGrantedPESlots(ctx->jni_env, ctx->job, pe_slots, alpp)) != PYGDI_SUCCESS) {
      goto error;
   }
error:
   ctx->result = ret;
   if (ret != PYGDI_SUCCESS) {
      DRETURN(-1);
   }
   DRETURN(0);
}

static int pygdi_qstat_job_request(job_handler_t* handler, const char* name, const char* value, lList **alpp) {
   pygdi_job_ctx_t *ctx = (pygdi_job_ctx_t*)handler->ctx;
   pygdi_result_t ret = PYGDI_SUCCESS;
   DENTER(PYGDI_LAYER, "pygdi_qstat_job_request");
   
   ret = JobSummaryImpl_addRequest(ctx->jni_env, ctx->job, name, value, alpp);
   
   ctx->result = ret;
   if (ret != PYGDI_SUCCESS) {
      DRETURN(-1);
   }
   DRETURN(0);
}

static int pygdi_qstat_job_hard_resource(job_handler_t *handler, const char* name, const char* value, double uc, lList **alpp) {
   pygdi_job_ctx_t *ctx = (pygdi_job_ctx_t*)handler->ctx;
   pygdi_result_t ret = PYGDI_SUCCESS;
   DENTER(PYGDI_LAYER, "pygdi_qstat_job_hard_resource");

   ret = JobSummaryImpl_addHardRequest(ctx->jni_env, ctx->job, name, value, (jdouble)uc, alpp);
   
   ctx->result = ret;
   if (ret != PYGDI_SUCCESS) {
      DRETURN(-1);
   }
   DRETURN(0);
}

static int pygdi_qstat_job_soft_resource(job_handler_t *handler, const char* name, const char* value, double uc, lList **alpp) {
   pygdi_job_ctx_t *ctx = (pygdi_job_ctx_t*)handler->ctx;
   pygdi_result_t ret = PYGDI_SUCCESS;
   DENTER(PYGDI_LAYER, "pygdi_qstat_job_soft_resource");

   ret = JobSummaryImpl_addSoftRequest(ctx->jni_env, ctx->job, name, value, alpp);
   
   ctx->result = ret;
   if (ret != PYGDI_SUCCESS) {
      DRETURN(-1);
   }
   DRETURN(0);
}

static int pygdi_qstat_job_hard_requested_queue(job_handler_t *handler, const char* name, lList **alpp) {
   pygdi_job_ctx_t *ctx = (pygdi_job_ctx_t*)handler->ctx;
   pygdi_result_t ret = PYGDI_SUCCESS;
   DENTER(PYGDI_LAYER, "pygdi_qstat_job_hard_requested_queue");

   ret = JobSummaryImpl_addHardRequestedQueue(ctx->jni_env, ctx->job, name, alpp);
   
   ctx->result = ret;
   if (ret != PYGDI_SUCCESS) {
      DRETURN(-1);
   }
   DRETURN(0);
}

static int pygdi_qstat_job_soft_requested_queue(job_handler_t *handler, const char* name, lList **alpp) {
   pygdi_job_ctx_t *ctx = (pygdi_job_ctx_t*)handler->ctx;
   DENTER(PYGDI_LAYER, "pygdi_qstat_job_soft_requested_queue");

   ctx->result = JobSummaryImpl_addSoftRequestedQueue(ctx->jni_env, ctx->job, name, alpp);
   
   if (ctx->result != PYGDI_SUCCESS) {
      DRETURN(-1);
   }
   DRETURN(0);
}

static int pygdi_qstat_job_master_hard_requested_queue(job_handler_t* handler, const char* name, lList **alpp) {
   pygdi_job_ctx_t *ctx = (pygdi_job_ctx_t*)handler->ctx;
   DENTER(PYGDI_LAYER, "pygdi_qstat_job_master_hard_requested_queue");

   ctx->result = JobSummaryImpl_addHardRequestedMasterQueue(ctx->jni_env, ctx->job, name, alpp);
   
   if (ctx->result != PYGDI_SUCCESS) {
      DRETURN(-1);
   }
   DRETURN(0);
}

static int pygdi_qstat_job_predecessor_requested(job_handler_t* handler, const char* name, lList **alpp) {
   pygdi_job_ctx_t *ctx = (pygdi_job_ctx_t*)handler->ctx;
   DENTER(PYGDI_LAYER, "pygdi_qstat_job_predecessor_requested");

   ctx->result = JobSummaryImpl_addRequestedPredecessor(ctx->jni_env, ctx->job, name, alpp);
   
   if (ctx->result != PYGDI_SUCCESS) {
      DRETURN(-1);
   }
   DRETURN(0);
}

static int pygdi_qstat_job_predecessor(job_handler_t* handler, u_long32 jid, lList** alpp) {
   pygdi_job_ctx_t *ctx = (pygdi_job_ctx_t*)handler->ctx;
   DENTER(PYGDI_LAYER, "pygdi_qstat_job_predecessor");

   ctx->result = JobSummaryImpl_addPredecessor(ctx->jni_env, ctx->job, jid, alpp);
   
   if (ctx->result != PYGDI_SUCCESS) {
      DRETURN(-1);
   }
   DRETURN(0);
}

static int pygdi_qstat_job_ad_predecessor_requested(job_handler_t* handler, const char* name, lList **alpp) 
{
   pygdi_job_ctx_t *ctx = (pygdi_job_ctx_t*)handler->ctx;
   DENTER(PYGDI_LAYER, "pygdi_qstat_job_ad_predecessor_requested");

   ctx->result = JobSummaryImpl_addRequestedArrayPredecessor(ctx->jni_env, ctx->job, name, alpp);
   
   if (ctx->result != PYGDI_SUCCESS) {
      DRETURN(-1);
   }
   DRETURN(0);
}

static int pygdi_qstat_job_ad_predecessor(job_handler_t* handler, u_long32 jid, lList** alpp) 
{
   pygdi_job_ctx_t *ctx = (pygdi_job_ctx_t*)handler->ctx;
   DENTER(PYGDI_LAYER, "pygdi_qstat_job_ad_predecessor");

   ctx->result = JobSummaryImpl_addArrayPredecessor(ctx->jni_env, ctx->job, jid, alpp);
   
   if (ctx->result != PYGDI_SUCCESS) {
      DRETURN(-1);
   }
   DRETURN(0);
}

static int pygdi_qstat_job_finished(job_handler_t* handler, u_long32 jid, lList **alpp) {
   pygdi_job_ctx_t *ctx = (pygdi_job_ctx_t*)handler->ctx;
   jboolean add_result = false;
   DENTER(PYGDI_LAYER, "pygdi_qstat_job_finished");

   ctx->result = List_add(ctx->jni_env, ctx->list, ctx->job, &add_result, alpp);
   ctx->job = NULL;
   if (ctx->result != PYGDI_SUCCESS) {
      DRETURN(-1);
   }
   DRETURN(0);
}



static pygdi_result_t pygdi_qstat_job_init(job_handler_t* handler, pygdi_job_ctx_t *ctx, JNIEnv *env, lList **alpp) {
   pygdi_result_t ret = PYGDI_SUCCESS;
   DENTER(PYGDI_LAYER, "pygdi_qstat_job_init");
   
   memset(ctx, 0, sizeof(pygdi_job_ctx_t));
   ctx->jni_env = env;
   
   memset(handler, 0, sizeof(job_handler_t));
   handler->ctx = ctx;
   handler->report_job = pygdi_qstat_job;
   handler->report_sub_task = pygdi_qstat_sub_task;
   handler->report_additional_info = pygdi_qstat_job_additional_info;
   handler->report_requested_pe = pygdi_qstat_job_requested_pe;
   handler->report_granted_pe  = pygdi_qstat_job_granted_pe;
   handler->report_request = pygdi_qstat_job_request;
   handler->report_hard_resource = pygdi_qstat_job_hard_resource;
   handler->report_soft_resource = pygdi_qstat_job_soft_resource;
   handler->report_hard_requested_queue = pygdi_qstat_job_hard_requested_queue;
   handler->report_soft_requested_queue = pygdi_qstat_job_soft_requested_queue;
   handler->report_master_hard_requested_queue = pygdi_qstat_job_master_hard_requested_queue;
   handler->report_predecessor_requested = pygdi_qstat_job_predecessor_requested;
   handler->report_predecessor = pygdi_qstat_job_predecessor;
   handler->report_ad_predecessor_requested = pygdi_qstat_job_ad_predecessor_requested;
   handler->report_ad_predecessor = pygdi_qstat_job_ad_predecessor;
   handler->report_job_finished = pygdi_qstat_job_finished;
   
   ctx->result = PYGDI_SUCCESS;
   
   ret = ArrayList_init(env, &(ctx->list), alpp);
   
   DRETURN(ret);
}


static int pygdi_qstat_queue_started(qstat_handler_t *handler, const char* qname, lList **alpp);
static int pygdi_qstat_queue_summary(qstat_handler_t *handler, const char* qname,  queue_summary_t *summary, lList **alpp);
static int pygdi_qstat_queue_load_alarm(qstat_handler_t* handler, const char* qname, const char* reason, lList **alpp);
static int pygdi_qstat_queue_suspend_alarm(qstat_handler_t* handler, const char* qname, const char* reason, lList **alpp);
static int pygdi_qstat_queue_message(qstat_handler_t* handler, const char* qname, const char *message, lList **alpp);
static int pygdi_qstat_queue_resource(qstat_handler_t* handler, const char* dom, const char* name, const char* value, lList **alpp);
static int pygdi_qstat_queue_finished(qstat_handler_t* handler, const char* qname, lList **alpp);
static int pygdi_qstat_queue_jobs_finished(qstat_handler_t *handler, const char* qname, lList **alpp);

static int pygdi_qstat_pending_jobs_finished(qstat_handler_t *handler, lList **alpp);
static int pygdi_qstat_finished_jobs_finished(qstat_handler_t *handler, lList **alpp);
static int pygdi_qstat_error_jobs_finished(qstat_handler_t *handler, lList **alpp);
static int pygdi_qstat_zombie_jobs_finished(qstat_handler_t *handler, lList **alpp);


typedef struct {
   JNIEnv  *jni_env;
   jobject queue_instance_summary;
   jobject result_obj;
   jobject unassigned_job_summary;
   pygdi_job_ctx_t job_ctx;
   pygdi_result_t result;
} qstat_ctx_t;


static int pygdi_qstat_queue_started(qstat_handler_t *handler, const char* qname, lList **alpp) {
   qstat_ctx_t *ctx = (qstat_ctx_t*)handler->ctx;
   JNIEnv *env = ctx->jni_env;
   pygdi_result_t ret = PYGDI_SUCCESS;
   
   DENTER(PYGDI_LAYER, "pygdi_qstat_queue_started");
   
   pygdi_log_printf(env, PYGDI_QSTAT_LOGGER, FINEST,
                   "queue instance %s started", qname);
   
   ret = QueueInstanceSummaryImpl_init(env, &ctx->queue_instance_summary, alpp);
   if (ret != PYGDI_SUCCESS) {
      goto error;
   }
   
   if ((ret = QueueInstanceSummaryImpl_setName(env, ctx->queue_instance_summary, qname, alpp)) != PYGDI_SUCCESS) {
      goto error;
   }
   
error:
   ctx->result = ret;
   if(ret != PYGDI_SUCCESS) {
      DRETURN(-1);
   }
   DRETURN(0);
}

static int pygdi_qstat_queue_summary(qstat_handler_t *handler, const char* qname,  queue_summary_t *summary, lList **alpp) {
   qstat_ctx_t *ctx = (qstat_ctx_t*)handler->ctx;
   JNIEnv *env = ctx->jni_env;
   jobject qi_obj = NULL;   
   DENTER(PYGDI_LAYER, "pygdi_qstat_queue_summary");

   
   if (ctx->queue_instance_summary == NULL) {
      answer_list_add(alpp, "illegal state: have no queue_instance_summary object",
                      STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
      DRETURN(-1);
   }
   qi_obj = ctx->queue_instance_summary;
   
   if (QueueInstanceSummaryImpl_setArch(env, qi_obj, summary->arch, alpp) != PYGDI_SUCCESS) {
      DRETURN(-1);
   }
   if (QueueInstanceSummaryImpl_setReservedSlots(env, qi_obj, summary->resv_slots, alpp) != PYGDI_SUCCESS) {
      DRETURN(-1);
   }
   if (QueueInstanceSummaryImpl_setUsedSlots(env, qi_obj, summary->used_slots, alpp) != PYGDI_SUCCESS) {
      DRETURN(-1);
   }
   if (QueueInstanceSummaryImpl_setTotalSlots(env, qi_obj, summary->total_slots, alpp) != PYGDI_SUCCESS) {
      DRETURN(-1);
   }
   if (QueueInstanceSummaryImpl_setHasLoadValue(env, qi_obj, summary->has_load_value, alpp) != PYGDI_SUCCESS) {
      DRETURN(-1);
   }
   if (QueueInstanceSummaryImpl_setHasLoadValueFromObject(env, qi_obj, summary->has_load_value_from_object, alpp) != PYGDI_SUCCESS) {
      DRETURN(-1);
   }
   if (QueueInstanceSummaryImpl_setLoadAvg(env, qi_obj, summary->load_avg, alpp) != PYGDI_SUCCESS) {
      DRETURN(-1);
   }
   if (QueueInstanceSummaryImpl_setLoadAvgStr(env, qi_obj, summary->load_avg_str, alpp) != PYGDI_SUCCESS) {
      DRETURN(-1);
   }
   if (QueueInstanceSummaryImpl_setQueueType(env, qi_obj, summary->queue_type, alpp) != PYGDI_SUCCESS) {
      DRETURN(-1);
   }
   if (QueueInstanceSummaryImpl_setState(env, qi_obj, summary->state, alpp) != PYGDI_SUCCESS) {
      DRETURN(-1);
   }
   
   DRETURN(0);
}

static int pygdi_qstat_queue_load_alarm(qstat_handler_t* handler, const char* qname, const char* reason, lList **alpp) {
   qstat_ctx_t *ctx = (qstat_ctx_t*)handler->ctx;
   JNIEnv *env = ctx->jni_env;
   DENTER(PYGDI_LAYER, "pygdi_qstat_queue_load_alarm");
   
   if (ctx->queue_instance_summary == NULL) {
      answer_list_add(alpp, "illegal state: have no queue_instance_summary object",
                      STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
      DRETURN(-1);
   }
   
   if (QueueInstanceSummaryImpl_setLoadAlarmReason(env, ctx->queue_instance_summary, reason, alpp) != PYGDI_SUCCESS) {
      DRETURN(-1);
   }
   
   DRETURN(0);
}

static int pygdi_qstat_queue_suspend_alarm(qstat_handler_t* handler, const char* qname, const char* reason, lList **alpp) {
   qstat_ctx_t *ctx = (qstat_ctx_t*)handler->ctx;
   JNIEnv *env = ctx->jni_env;
   DENTER(PYGDI_LAYER, "pygdi_qstat_queue_suspend_alarm");
   
   if (ctx->queue_instance_summary == NULL) {
      answer_list_add(alpp, "illegal state: have no queue_instance_summary object",
                      STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
      DRETURN(-1);
   }
   
   if (QueueInstanceSummaryImpl_setSuspendAlarmReason(env, ctx->queue_instance_summary, reason, alpp) != PYGDI_SUCCESS) {
      DRETURN(-1);
   }
   
   DRETURN(0);
}

static int pygdi_qstat_queue_message(qstat_handler_t* handler, const char* qname, const char *message, lList **alpp) {
   qstat_ctx_t *ctx = (qstat_ctx_t*)handler->ctx;
   JNIEnv *env = ctx->jni_env;
   DENTER(PYGDI_LAYER, "pygdi_qstat_queue_message");
   
   if (ctx->queue_instance_summary == NULL) {
      answer_list_add(alpp, "illegal state: have no queue_instance_summary object",
                      STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
      DRETURN(-1);
   }
   
   if (QueueInstanceSummaryImpl_addExplainMessage(env, ctx->queue_instance_summary, message, alpp) != PYGDI_SUCCESS) {
      DRETURN(-1);
   }
   DRETURN(0);
}

static int pygdi_qstat_queue_resource(qstat_handler_t* handler, const char* dom, const char* name, const char* value, lList **alpp) {
   qstat_ctx_t *ctx = (qstat_ctx_t*)handler->ctx;
   JNIEnv *env = ctx->jni_env;
   DENTER(PYGDI_LAYER, "pygdi_qstat_queue_message");
   
   if (ctx->queue_instance_summary == NULL) {
      answer_list_add(alpp, "illegal state: have no queue_instance_summary object",
                      STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
      DRETURN(-1);
   }

   if (QueueInstanceSummaryImpl_addResource(env, ctx->queue_instance_summary, dom, name, value, alpp) != PYGDI_SUCCESS) {
      DRETURN(-1);
   }
   DRETURN(0);
}

static int pygdi_qstat_queue_finished(qstat_handler_t* handler, const char* qname, lList **alpp) {
   qstat_ctx_t *ctx = (qstat_ctx_t*)handler->ctx;
   JNIEnv *env = ctx->jni_env;
   pygdi_result_t ret = PYGDI_SUCCESS;
   
   DENTER(PYGDI_LAYER, "pygdi_qstat_queue_finished");

   ret = QueueInstanceSummaryResultImpl_addQueueInstanceSummary(env, ctx->result_obj, ctx->queue_instance_summary, alpp);
   if (ret != PYGDI_SUCCESS) {
      DRETURN(-1);
   }
   
   ctx->queue_instance_summary = NULL;

   pygdi_log_printf(env, PYGDI_QSTAT_LOGGER, FINER,
                   "queue instance %s finished", qname);
                        
   if (ret != PYGDI_SUCCESS) {
      DRETURN(-1);
   }
   
   DRETURN(0);
}

static int pygdi_qstat_queue_jobs_finished(qstat_handler_t *handler, const char* qname, lList **alpp) {
   qstat_ctx_t *ctx = (qstat_ctx_t*)handler->ctx;
   
   DENTER(PYGDI_LAYER, "pygdi_qstat_queue_jobs_finished");
   
   if (QueueInstanceSummaryImpl_addJobs(ctx->jni_env, ctx->queue_instance_summary, ctx->job_ctx.list, alpp) != PYGDI_SUCCESS) {
      DRETURN(-1);
   }
   
   if (List_clear(ctx->jni_env, ctx->job_ctx.list, alpp) != PYGDI_SUCCESS) {
      DRETURN(-1);
   }
   
   DRETURN(0);
}                   


static int pygdi_qstat_pending_jobs_finished(qstat_handler_t *handler, lList **alpp) {
   qstat_ctx_t *ctx = (qstat_ctx_t*)handler->ctx;
   
   DENTER(PYGDI_LAYER,"pygdi_qstat_pending_jobs_finished");
   
   if (QueueInstanceSummaryResultImpl_addPendingJobs(ctx->jni_env, ctx->result_obj, ctx->job_ctx.list, alpp) != PYGDI_SUCCESS) {
      DRETURN(-1);
   }
   
   if (List_clear(ctx->jni_env, ctx->job_ctx.list, alpp) != PYGDI_SUCCESS) {
      DRETURN(-1);
   }
   
   DRETURN(0);
}

static int pygdi_qstat_finished_jobs_finished(qstat_handler_t *handler, lList **alpp) {
   qstat_ctx_t *ctx = (qstat_ctx_t*)handler->ctx;
   
   DENTER(PYGDI_LAYER,"pygdi_qstat_finished_jobs_finished");
   
   if (QueueInstanceSummaryResultImpl_addFinishedJobs(ctx->jni_env, ctx->result_obj, ctx->job_ctx.list, alpp) != PYGDI_SUCCESS) {
      DRETURN(-1);
   }
   
   if (List_clear(ctx->jni_env, ctx->job_ctx.list, alpp) != PYGDI_SUCCESS) {
      DRETURN(-1);
   }
   
   DRETURN(0);
}

static int pygdi_qstat_error_jobs_finished(qstat_handler_t *handler, lList **alpp) {
   qstat_ctx_t *ctx = (qstat_ctx_t*)handler->ctx;
   
   DENTER(PYGDI_LAYER,"pygdi_qstat_error_jobs_finished");
   
   if (QueueInstanceSummaryResultImpl_addErrorJobs(ctx->jni_env, ctx->result_obj, ctx->job_ctx.list, alpp) != PYGDI_SUCCESS) {
      DRETURN(-1);
   }
   
   if (List_clear(ctx->jni_env, ctx->job_ctx.list, alpp) != PYGDI_SUCCESS) {
      DRETURN(-1);
   }
   
   DRETURN(0);
}

static int pygdi_qstat_zombie_jobs_finished(qstat_handler_t *handler, lList **alpp) {
   qstat_ctx_t *ctx = (qstat_ctx_t*)handler->ctx;
   
   DENTER(PYGDI_LAYER,"pygdi_qstat_zombie_jobs_finished");
   
   if (QueueInstanceSummaryResultImpl_addZombieJobs(ctx->jni_env, ctx->result_obj, ctx->job_ctx.list, alpp) != PYGDI_SUCCESS) {
      DRETURN(-1);
   }
   
   if (List_clear(ctx->jni_env, ctx->job_ctx.list, alpp) != PYGDI_SUCCESS) {
      DRETURN(-1);
   }
   
   DRETURN(0);
}


/*
 * Class:     com_sun_grid_pygdi_jni_PYGDIBaseImpl
 * Method:    nativeFillQueueInstanceSummary
 * Signature: (Lcom/sun/grid/pygdi/monitoring/QueueInstanceSummaryOptions;Ljava/util/List;)V
 */
JNIEXPORT void JNICALL Java_com_sun_grid_pygdi_jni_PYGDIBaseImpl_nativeFillQueueInstanceSummary
(JNIEnv *env, jobject pygdi, jobject options, jobject result) {

   pygdi_qstat_filter_t filter;
   sge_gdi_ctx_class_t * ctx = NULL;
   qstat_env_t qstat_env;
   lList *alp = NULL;
   qstat_handler_t qstat_handler;
   qstat_ctx_t qstat_ctx;
   pygdi_result_t ret = PYGDI_SUCCESS;
   rmon_ctx_t rmon_ctx;
   
   DENTER(PYGDI_LAYER, "Java_com_sun_grid_pygdi_jni_PYGDIBaseImpl_nativeFillQueueInstanceSummary");
   
   memset(&filter, 0, sizeof(pygdi_qstat_filter_t));
   memset(&qstat_env, 0, sizeof(qstat_env));
   
   pygdi_init_rmon_ctx(env, PYGDI_QSTAT_LOGGER, &rmon_ctx);
   rmon_set_thread_ctx(&rmon_ctx);
   
   ret = getGDIContext(env, pygdi, &ctx, &alp);
   if (ret != PYGDI_SUCCESS) {
      goto error;
   }
   sge_gdi_set_thread_local_ctx(ctx);
   
   if (options != NULL) {
      if((ret = BasicQueueOptions_getQueueFilter(env, options, &(filter.queue_filter), &alp)) != PYGDI_SUCCESS) {
         goto error;
      }
      if((ret = BasicQueueOptions_getResourceFilter(env, options, &(filter.resource_filter), &alp)) != PYGDI_SUCCESS) {
         goto error;
      }
      if((ret = BasicQueueOptions_getQueueStateFilter(env, options, &(filter.queue_state_filter), &alp)) != PYGDI_SUCCESS) {
         goto error;
      }
      if((ret = BasicQueueOptions_getQueueUserFilter(env, options, &(filter.queue_user_filter), &alp)) != PYGDI_SUCCESS) {
         goto error;
      }
      if((ret = QueueInstanceSummaryOptions_getResourceAttributeFilter(env, options, &(filter.resource_attribute_filter), &alp)) != PYGDI_SUCCESS) {
         goto error;
      }
      if((ret = QueueInstanceSummaryOptions_getPeFilter(env, options, &(filter.pe_filter), &alp)) != PYGDI_SUCCESS) {
         goto error;
      }
      if((ret = QueueInstanceSummaryOptions_getJobStateFilter(env, options, &(filter.job_state_filter), &alp)) != PYGDI_SUCCESS) {
         goto error;
      }
      if((ret = QueueInstanceSummaryOptions_getJobUserFilter(env, options, &(filter.job_user_filter), &alp)) != PYGDI_SUCCESS) {
         goto error;
      }
      
   }
   
   if ((ret = pygdi_qstat_env_init(env, ctx, &filter, &qstat_env, &alp)) != PYGDI_SUCCESS) {
      goto error;
   }
   
   if (options != NULL) {
      jboolean res = false;
      
      if ((ret=QueueInstanceSummaryOptions_showEmptyQueues(env, options, &res, &alp)) != PYGDI_SUCCESS) {
         goto error;
      }                  
      if (!res) {
         qstat_env.full_listing |= QSTAT_DISPLAY_NOEMPTYQ;
      }
      
      if ((ret = QueueInstanceSummaryOptions_showFullOutput(env, options, &res, &alp)) != PYGDI_SUCCESS) {
         goto error;
      }            
      if (res) {
#if 0
         qstat_env.full_listing |= QSTAT_DISPLAY_QRESOURCES|QSTAT_DISPLAY_FULL;
#else
         qstat_env.full_listing |= QSTAT_DISPLAY_FULL;
#endif         
      }
      
      if ((ret = QueueInstanceSummaryOptions_showRequestedResourcesForJobs(env, options, &res, &alp)) != PYGDI_SUCCESS) {
         goto error;
      }
      if (res) {
         qstat_filter_add_r_attributes(&qstat_env);
         qstat_env.full_listing |= QSTAT_DISPLAY_RESOURCES;
      }
      if ((ret = QueueInstanceSummaryOptions_showJobPriorities(env, options, &res, &alp)) != PYGDI_SUCCESS) {
         goto error;
      }
      if (res) {
         qstat_filter_add_pri_attributes(&qstat_env);
         qstat_env.full_listing |= QSTAT_DISPLAY_PRIORITY;
      }

      if ((ret = BasicQueueOptions_showAdditionalAttributes(env, options, &res, &alp)) != PYGDI_SUCCESS) {
         goto error;
      }
      if (res) {
         qstat_filter_add_ext_attributes(&qstat_env); 
         qstat_env.full_listing |= QSTAT_DISPLAY_EXTENDED;
      }
      
      if ((ret = QueueInstanceSummaryOptions_showJobUrgency(env, options, &res, &alp)) != PYGDI_SUCCESS) {
         goto error;
      }
      if (res) {
         qstat_filter_add_urg_attributes(&qstat_env); 
         qstat_env.full_listing |= QSTAT_DISPLAY_URGENCY;
      }
      if ((ret = QueueInstanceSummaryOptions_showExtendedSubTaskInfo(env, options, &res, &alp)) != PYGDI_SUCCESS) {
         goto error;
      }
      if (res) {
         qstat_filter_add_t_attributes(&qstat_env);
         qstat_env.full_listing |= QSTAT_DISPLAY_TASKS;
         qstat_env.group_opt |= GROUP_NO_PETASK_GROUPS;
      }

      if ((ret = QueueInstanceSummaryOptions_showArrayJobs(env, options, &res, &alp)) != PYGDI_SUCCESS) {
         goto error;
      }
      if (res) {
         qstat_env.group_opt |= GROUP_NO_TASK_GROUPS;
      }
      if ((ret = QueueInstanceSummaryOptions_showPEJobs(env, options, &res, &alp)) != PYGDI_SUCCESS) {
         goto error;
      }
      if (res) {
         qstat_env.group_opt |= GROUP_NO_PETASK_GROUPS;
      }
      
      {
         /* explain bits */
         jboolean is_explain_set = false;
         
         if ((ret = QueueInstanceSummaryOptions_isExplainSet(env, options, &is_explain_set, &alp)) != PYGDI_SUCCESS) {
            goto error;
         }
         if (is_explain_set) {
            jchar explain = 0;
            if ((ret = QueueInstanceSummaryOptions_getExplain(env, options, &explain, &alp)) != PYGDI_SUCCESS) {
               goto error;
            } else {
               u_long32 explain_bits = 0;
               switch(explain) {
                  case 0x61: /* UTF-8 'a' */
                     explain_bits |= QI_AMBIGUOUS;
                     break;
                  case 0x41: /* UTF-8 'A' */
                     explain_bits |= QI_ALARM;
                     break;
                  case 0x63: /* UTF-8 'c' */
                     explain_bits |= QI_SUSPEND_ALARM;
                     break;
                  case 0x45: /* UTF-8 'E' */
                     explain_bits|= QI_ERROR;
                     break;
                  default:
                     answer_list_add_sprintf(&alp, STATUS_ENOMGR, ANSWER_QUALITY_ERROR,
                                             "Unknown queue state (UTF-8 0x %x)", explain);
               }
               qstat_env.explain_bits = explain_bits;
               qstat_env.full_listing |= QSTAT_DISPLAY_FULL;
            }
         }
      }
   }   
   
   /* setup the internal context */
   memset(&qstat_ctx, 0, sizeof(qstat_ctx_t));
   
   qstat_ctx.jni_env = env;
   qstat_ctx.result_obj = result;
   
   /* setup the qstat_handler */
   memset(&qstat_handler, 0, sizeof(qstat_handler_t));
   
   qstat_handler.ctx = &qstat_ctx;
   qstat_handler.qstat_env = &qstat_env;
   qstat_handler.report_queue_started = pygdi_qstat_queue_started;
   qstat_handler.report_queue_summary = pygdi_qstat_queue_summary;
   qstat_handler.report_queue_load_alarm = pygdi_qstat_queue_load_alarm;
   qstat_handler.report_queue_suspend_alarm = pygdi_qstat_queue_suspend_alarm;
   qstat_handler.report_queue_message = pygdi_qstat_queue_message;
   qstat_handler.report_queue_resource = pygdi_qstat_queue_resource;
   qstat_handler.report_queue_jobs_finished = pygdi_qstat_queue_jobs_finished;
   qstat_handler.report_queue_finished = pygdi_qstat_queue_finished;
   qstat_handler.report_pending_jobs_finished = pygdi_qstat_pending_jobs_finished;
   qstat_handler.report_finished_jobs_finished = pygdi_qstat_finished_jobs_finished;
   qstat_handler.report_error_jobs_finished = pygdi_qstat_error_jobs_finished;
   qstat_handler.report_zombie_jobs_finished = pygdi_qstat_zombie_jobs_finished;
   
   if ((ret = pygdi_qstat_job_init(&(qstat_handler.job_handler), &(qstat_ctx.job_ctx), env, &alp)) != PYGDI_SUCCESS) {
      goto error;
   }
   
   qstat_no_group(&qstat_env, &qstat_handler, &alp);
   
   if (qstat_ctx.result != PYGDI_SUCCESS) {
      ret = qstat_ctx.result;
      goto error;
   }
   if (qstat_ctx.job_ctx.result != PYGDI_SUCCESS) {
      ret = qstat_ctx.job_ctx.result;
      goto error;
   }
      
error:
   qstat_env_destroy(&qstat_env);
   if (ret != PYGDI_SUCCESS) {
      throw_error_from_answer_list(env, ret, alp);
   }
   rmon_set_thread_ctx(NULL);
   pygdi_destroy_rmon_ctx(&rmon_ctx);
   DRETURN_VOID;
}


/*
 * Class:     com_sun_grid_pygdi_jni_PYGDIBaseImpl
 * Method:    nativeFillClusterQueueSummary
 * Signature: (Lcom/sun/grid/pygdi/monitoring/QueueFilter;Lcom/sun/grid/pygdi/monitoring/filter/ResourceFilter;Ljava/util/List;)V
 */
JNIEXPORT void JNICALL Java_com_sun_grid_pygdi_jni_PYGDIBaseImpl_nativeFillClusterQueueSummary
     (JNIEnv *env, jobject pygdi, jobject options, jobject result) {
 
   pygdi_qstat_filter_t filter;
   sge_gdi_ctx_class_t * ctx = NULL;
   qstat_env_t qstat_env;
   cqueue_summary_handler_t handler;
   lList *alp = NULL;
   pygdi_qstat_cqueue_summary_ctx_t cq_ctx;
   
   pygdi_result_t ret = PYGDI_SUCCESS;
   rmon_ctx_t rmon_ctx;
   
   DENTER(PYGDI_LAYER, "Java_com_sun_grid_pygdi_jni_PYGDIBaseImpl_nativeFillClusterQueueSummary");

   memset(&filter, 0, sizeof(pygdi_qstat_filter_t));
   memset(&qstat_env, 0, sizeof(qstat_env));

   pygdi_init_rmon_ctx(env, PYGDI_QSTAT_LOGGER, &rmon_ctx);
   rmon_set_thread_ctx(&rmon_ctx);
   
   if ((ret = getGDIContext(env, pygdi, &ctx, &alp)) != PYGDI_SUCCESS) {
      goto error;
   }
   sge_gdi_set_thread_local_ctx(ctx);
   
   
   if (options != NULL) {
      if((ret = BasicQueueOptions_getQueueFilter(env, options, &(filter.queue_filter), &alp)) != PYGDI_SUCCESS) {
         goto error;
      }
      if((ret = BasicQueueOptions_getResourceFilter(env, options, &(filter.resource_filter), &alp)) != PYGDI_SUCCESS) {
         goto error;
      }
      if((ret = BasicQueueOptions_getQueueStateFilter(env, options, &(filter.queue_state_filter), &alp)) != PYGDI_SUCCESS) {
         goto error;
      }
      if((ret = BasicQueueOptions_getQueueUserFilter(env, options, &(filter.queue_user_filter), &alp)) != PYGDI_SUCCESS) {
         goto error;
      }
   }
   
   if ((ret = pygdi_qstat_env_init(env, ctx, &filter, &qstat_env, &alp)) != PYGDI_SUCCESS) {
      goto error;
   }
   
   qstat_env.group_opt |= GROUP_CQ_SUMMARY;
   
   memset(&cq_ctx, 0, sizeof(pygdi_qstat_cqueue_summary_ctx_t));
   
   cq_ctx.jni_env = env;
   cq_ctx.list = result;
   
   memset(&handler, 0, sizeof(cqueue_summary_handler_t));
   handler.ctx = &cq_ctx;
   
   handler.report_cqueue = pygdi_qstat_cqueue_summary;
   
   qstat_cqueue_summary(&qstat_env, &handler, &alp);

   if (cq_ctx.result != PYGDI_SUCCESS) {
      ret = cq_ctx.result;
   }
   
error:
   qstat_env_destroy(&qstat_env);
   if (ret != PYGDI_SUCCESS) {
      throw_error_from_answer_list(env, ret, alp);
   }
   rmon_set_thread_ctx(NULL);
   pygdi_destroy_rmon_ctx(&rmon_ctx);
   DRETURN_VOID;
}



