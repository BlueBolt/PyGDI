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
// File:pygdi_event.c
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

#include <ctype.h>
#include <string.h>

#include "jni.h"

#include "rmon/sgermon.h"

#include "uti/sge_prog.h"
#include "uti/sge_bootstrap.h"
#include "uti/sge_log.h"
#include "uti/sge_error_class.h"
#include "uti/sge_time.h"

#include "cull/cull.h"

#include "commlib.h"
#include "cl_errors.h"

#include "lck/sge_mtutil.h"

#include "gdi/sge_gdi2.h"
#include "gdi/sge_gdi.h"
#include "gdi/sge_gdi_ctx.h"

#include "sgeobj/sge_all_listsL.h"
#include "sgeobj/sge_answer.h"
#include "sgeobj/sge_event.h"
#include "sgeobj/sge_event.h"

#include "evc/sge_event_client.h"

#include "evm/sge_event_master.h"

#include "mir/sge_mirror.h"

#include "pygdi_common.h"
#include "pygdi_event.h"
#include "pygdi_wrapper.h"
#include "pygdi_logging.h"
#include "pygdi.h"
#include "basis_types.h"

#define MAX_EVC_ARRAY_SIZE 1024
static pthread_mutex_t sge_evc_mutex = PTHREAD_MUTEX_INITIALIZER;
static bool is_evc_array_initialized = false;

typedef enum {
   RUNNING,
   INTERRUPTED,
   STOPPED
} elem_state_t;    

typedef struct {
   sge_evc_class_t *sge_evc;
   pthread_mutex_t elem_mutex;
   pthread_cond_t  cond_var;
   elem_state_t state;
} sge_evc_elem_t;

static sge_evc_elem_t sge_evc_array[MAX_EVC_ARRAY_SIZE];

static pygdi_result_t process_event(JNIEnv *env,  jobject eventList, lListElem *ev, lList** alpp);
                                             
static pygdi_result_t fill_job_usage_event(JNIEnv *env, jobject event_obj, lListElem *ev, lList **alpp);
static pygdi_result_t fill_job_event(JNIEnv *env, jobject event_obj, lListElem *ev, lList **alpp);

static pygdi_result_t fill_generic_event(JNIEnv *env, jobject event_obj, const char* beanClassName, 
                                        lDescr *descr, int event_action, lListElem *ev, lList **alpp);


static void initEVCArray(void) {
   int i = 0;
   DENTER(PYGDI_LAYER, "initEVCArray");

   if (!is_evc_array_initialized) {
      is_evc_array_initialized = true;
      for (i=0; i<MAX_EVC_ARRAY_SIZE; i++) {
         sge_evc_array[i].sge_evc = NULL;
         sge_evc_array[i].state = RUNNING;
         pthread_mutex_init(&(sge_evc_array[i].elem_mutex), NULL);
         pthread_cond_init(&(sge_evc_array[i].cond_var), NULL);
      }
   }
   DRETURN_VOID;
}

static pygdi_result_t lockEVC(const char *func, int index, sge_evc_elem_t **outelem, lList **alpp) {

   pygdi_result_t ret = PYGDI_SUCCESS;

   DENTER(PYGDI_LAYER, "lockEVC");
   if (index >= 0 && index < MAX_EVC_ARRAY_SIZE) {
      sge_evc_elem_t *elem = &sge_evc_array[index];
/*       DPRINTF(("%s lockEVC: event client at evc_index %d\n", func, index)); */
      pthread_mutex_lock(&(elem->elem_mutex));
      if (elem->sge_evc == NULL) {
         *outelem = NULL;
/*          DPRINTF(("%s unlockEVC: event client at evc_index %d\n", func, index)); */
         pthread_mutex_unlock(&(elem->elem_mutex));
         answer_list_add_sprintf(alpp, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR, 
                                "event client sge_evc_index[%d] is already closed", index);
         ret = PYGDI_ILLEGAL_STATE;
      } else {
         *outelem = elem;
      }   
   } else {
       answer_list_add(alpp, "event has not a valid evc index", STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
       ret = PYGDI_ILLEGAL_STATE;
   }
   DRETURN(ret);
      
}

static void unlockEVC(const char *func, int index) {
   DENTER(PYGDI_LAYER, "unlockEVC");
   if (index >= 0 && index < MAX_EVC_ARRAY_SIZE) {
/*       DPRINTF(("%s unlockEVC: event client at evc_index %d\n", func, index)); */
      pthread_mutex_unlock(&(sge_evc_array[index].elem_mutex));
   }   
   DRETURN_VOID;
}

static pygdi_result_t waitEVC(const char *func, int index, lList **elist, lList **alpp) 
{
   pygdi_result_t ret = PYGDI_SUCCESS;
   sge_evc_elem_t *elem = NULL;
   
   DENTER(PYGDI_LAYER, "waitEVC");
   if ((ret = lockEVC(func, index, &elem, alpp)) == PYGDI_SUCCESS) {
      sge_gdi_ctx_class_t *gdi_ctx = elem->sge_evc->get_gdi_ctx(elem->sge_evc);
      if (gdi_ctx && gdi_ctx->is_qmaster_internal_client(gdi_ctx)) {
         while (true) {
            if (elem->state == STOPPED) {
               break;
            }
            if (elem->state == INTERRUPTED) {
               elem->state = RUNNING;
               break;
            }
            if (elem->sge_evc->ec_evco_exit(elem->sge_evc)) {
               break;
            }
            if (elem->sge_evc->ec_evco_triggered(elem->sge_evc)) {
               elem->sge_evc->ec_get(elem->sge_evc, elist, false);
               break;
            }
            pthread_cond_wait(&(elem->cond_var), &(elem->elem_mutex));
         }
      } else {
         elem->sge_evc->ec_get(elem->sge_evc, elist, false);
      }
      unlockEVC(func, index);
   }
   DRETURN(ret);
}



static int pygdi_get_evc_by_evid_and_lock(u_long32 evid, sge_evc_elem_t **elem, lList **alpp) {

   int i = 0;
   int evc_index = -1;

   DENTER(PYGDI_LAYER, "pygdi_get_evc_by_evid_and_lock");
   
   *elem = NULL;

   for (i = 0; i < MAX_EVC_ARRAY_SIZE; i++) {
      sge_evc_elem_t *tmpelem = NULL;
      if (lockEVC("pygdi_get_evc_by_evid_and_lock", i, &tmpelem, alpp) == PYGDI_SUCCESS) {
         if (tmpelem->sge_evc->ec_get_id(tmpelem->sge_evc) == evid) {
            *elem = tmpelem;
            evc_index = i;
            break;
         }
         unlockEVC("pygdi_get_evc_by_evid_and_lock", i);
      }
   }

   DRETURN(evc_index);
}   

static void pygdi_event_update_func(u_long32 evid, lList **alpp, lList *event_list) 
{
   sge_evc_elem_t *elem = NULL;
   int evc_index = -1;

   DENTER(TOP_LAYER, "pygdi_event_update_func");

   evc_index = pygdi_get_evc_by_evid_and_lock(evid, &elem, alpp);

   if (elem) {
      int num_events = elem->sge_evc->ec_signal(elem->sge_evc, alpp, event_list);
      if (num_events > 0) {
         pthread_cond_broadcast(&(elem->cond_var));
      } else {
         elem->sge_evc->ec_ack(elem->sge_evc);
         elem->sge_evc->ec_commit(elem->sge_evc, NULL);
      }
      unlockEVC("pygdi_event_update_func", evc_index);
   } else {
      DPRINTF(("elem is NULL"));
   }

   DRETURN_VOID;
}

/*
 * Class:     com_sun_grid_pygdi_jni_EventClientImpl
 * Method:    interruptNative
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_com_sun_grid_pygdi_jni_EventClientImpl_interruptNative(JNIEnv *env, jobject evcobj, jint evc_index)
{
   sge_evc_elem_t *elem = NULL;
   rmon_ctx_t rmon_ctx;
   lList *alp = NULL;
   pygdi_result_t ret = PYGDI_ERROR;

   DENTER(TOP_LAYER, "Java_com_sun_grid_pygdi_jni_EventClientImpl_interruptNative");

   pygdi_init_rmon_ctx(env, PYGDI_EVENT_LOGGER, &rmon_ctx);
   rmon_set_thread_ctx(&rmon_ctx);
   
   if ((ret = lockEVC("Java_com_sun_grid_pygdi_jni_EventClientImpl_interruptNative", evc_index, &elem, &alp)) == PYGDI_SUCCESS) {
      elem->state = INTERRUPTED;
      pthread_cond_broadcast(&(elem->cond_var));
      pygdi_log_printf(env, PYGDI_EVENT_LOGGER, FINER,
                      "interrupting sge_evc_array[%d] event client", evc_index);
                      
      unlockEVC("Java_com_sun_grid_pygdi_jni_EventClientImpl_closeNative", evc_index);
   } else {
      throw_error_from_answer_list(env, ret, alp);
   }

   lFreeList(&alp);
   rmon_set_thread_ctx(NULL);
   pygdi_destroy_rmon_ctx(&rmon_ctx);
   
   DRETURN_VOID;
}


/*
 * Class:     com_sun_grid_pygdi_jni_EventClientImpl
 * Method:    closeNative
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_com_sun_grid_pygdi_jni_EventClientImpl_closeNative(JNIEnv *env, jobject evcobj, jint evc_index)
{
   sge_evc_elem_t *elem = NULL;
   rmon_ctx_t rmon_ctx;
   lList *alp = NULL;
   pygdi_result_t ret = PYGDI_ERROR;

   DENTER(TOP_LAYER, "Java_com_sun_grid_pygdi_jni_EventClientImpl_closeNative");

   pygdi_init_rmon_ctx(env, PYGDI_EVENT_LOGGER, &rmon_ctx);
   rmon_set_thread_ctx(&rmon_ctx);
   
   pthread_mutex_lock(&sge_evc_mutex);

   if ((ret = lockEVC("Java_com_sun_grid_pygdi_jni_EventClientImpl_closeNative", evc_index, &elem, &alp)) == PYGDI_SUCCESS) {
      u_long32 reg_id = elem->sge_evc->ec_get_id(elem->sge_evc);

      elem->state = STOPPED;

      pygdi_log_printf(env, PYGDI_EVENT_LOGGER, FINER,
                      "closing sge_evc_array[%d] event client %d", evc_index, (int)reg_id);
                      
      /* shutdown the event client */
      if (elem->sge_evc->ec_deregister(elem->sge_evc)) {
         jobject logger = pygdi_get_logger(env, PYGDI_EVENT_LOGGER);
         if (pygdi_is_loggable(env, logger, FINER)) {
            pygdi_log_printf(env, PYGDI_EVENT_LOGGER, FINER,
                            "deregistered sge_evc_array[%d] event client %d", evc_index, (int)reg_id);
         }
      } else {
         THROW_ERROR((env, PYGDI_ILLEGAL_STATE, "sge_evc->ec_deregister failed"));
      }
      /* destroy the event client */
      sge_evc_class_destroy(&(elem->sge_evc));
      sge_evc_array[evc_index].sge_evc = NULL;
      pthread_cond_broadcast(&(elem->cond_var));
      unlockEVC("Java_com_sun_grid_pygdi_jni_EventClientImpl_closeNative", evc_index);
   } else {
      throw_error_from_answer_list(env, ret, alp);
   }
   pthread_mutex_unlock(&sge_evc_mutex);

   lFreeList(&alp);
   rmon_set_thread_ctx(NULL);
   pygdi_destroy_rmon_ctx(&rmon_ctx);
   
   DRETURN_VOID;
}

/*
 * Class:     com_sun_grid_pygdi_jni_EventClientImpl
 * Method:    initNative
 * Signature: (Lcom/sun/grid/pygdi/jni/PYGDI;I)I
 */
JNIEXPORT jint JNICALL Java_com_sun_grid_pygdi_jni_EventClientImpl_initNative(JNIEnv *env, jobject evcobj, jobject pygdi, jint reg_id)
{
   char* argv[] = { "" };
   int argc = 1;

   sge_evc_class_t *evc = NULL;
   int evc_index = -1;
   int i;
   sge_gdi_ctx_class_t *sge_gdi_ctx = NULL;
   lList *alp = NULL;
   
   pygdi_result_t ret = PYGDI_SUCCESS;
   rmon_ctx_t rmon_ctx;
   
   DENTER_MAIN(TOP_LAYER, "Java_com_sun_grid_pygdi_jni_EventClientImpl_initNative");

   pygdi_init_rmon_ctx(env, PYGDI_EVENT_LOGGER, &rmon_ctx);
   rmon_set_thread_ctx(&rmon_ctx);
   
   if ((ret = getGDIContext(env, pygdi, &sge_gdi_ctx, &alp)) != PYGDI_SUCCESS) {
      ret = PYGDI_ILLEGAL_STATE;
      goto error;
   }
   evc = sge_evc_class_create(sge_gdi_ctx, (ev_registration_id)reg_id, &alp, sge_gdi_ctx->get_component_name(sge_gdi_ctx)); 
   if (!evc) {
      throw_error_from_answer_list(env, PYGDI_ERROR, alp);
      goto error;
   }
   if (sge_gdi_ctx->is_qmaster_internal_client(sge_gdi_ctx)) {
      lInit(nmv);
      /* local event client */
      evc->ec_local.update_func = pygdi_event_update_func;
      evc->ec_local.mod_func = sge_mod_event_client;
      evc->ec_local.add_func = sge_add_event_client;
      evc->ec_local.remove_func = sge_remove_event_client;
      evc->ec_local.ack_func = sge_handle_event_ack;
      evc->ec_local.init = true;
   }
   /*
   ** set the timeout to 1 sec
   */
   evc->ec_set_edtime(evc, 1);

   initEVCArray();
   pthread_mutex_lock(&sge_evc_mutex);
   for (i=0; i<MAX_EVC_ARRAY_SIZE; i++) {
      if (sge_evc_array[i].sge_evc == NULL) {
         pthread_mutex_lock(&(sge_evc_array[i].elem_mutex));
         sge_evc_array[i].sge_evc = evc;
         sge_evc_array[i].state = RUNNING;
         evc_index = i;
         pthread_mutex_unlock(&(sge_evc_array[i].elem_mutex));
         break;
      }   
   }
   pthread_mutex_unlock(&sge_evc_mutex);

   if (evc_index < 0) {
       sge_evc_class_destroy(&evc);
       ret = PYGDI_ERROR;
       answer_list_add(&alp, "Too many pygdi connections", STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
   }

error:

   if (ret != PYGDI_SUCCESS) {
      throw_error_from_answer_list(env, ret, alp);
   }
   lFreeList(&alp);
   rmon_set_thread_ctx(NULL);
   pygdi_destroy_rmon_ctx(&rmon_ctx);
   
   DRETURN(evc_index);
}


/*
 * Class:     com_sun_grid_pygdi_jni_EventClientImpl
 * Method:    register
 * Signature: ()V
 */
JNIEXPORT jint JNICALL Java_com_sun_grid_pygdi_jni_EventClientImpl_registerNative(JNIEnv *env, jobject evcobj, jint evc_index)
{
   lList *alp = NULL;
   sge_evc_elem_t *elem = NULL;
   pygdi_result_t ret = PYGDI_SUCCESS;
   ev_registration_id id = EV_ID_ANY;
   rmon_ctx_t rmon_ctx;
   
   DENTER(PYGDI_LAYER, "Java_com_sun_grid_pygdi_jni_EventClientImpl_registerNative");

   pygdi_init_rmon_ctx(env, PYGDI_EVENT_LOGGER, &rmon_ctx);
   rmon_set_thread_ctx(&rmon_ctx);
   
   if ((ret = lockEVC("Java_com_sun_grid_pygdi_jni_EventClientImpl_registerNative", evc_index, &elem, &alp)) == PYGDI_SUCCESS) {
      if (!elem->sge_evc->ec_register(elem->sge_evc, false, &alp, NULL)) {
         if (answer_list_has_error(&alp)) {
            throw_error_from_answer_list(env, PYGDI_ERROR, alp);
         } else {
            throw_error(env, PYGDI_ERROR, "ec_register returned false");
         }
      } else {
         id = elem->sge_evc->ec_get_id(elem->sge_evc);
         DPRINTF(("event client with id %d successfully registered\n", id));
      }
      unlockEVC("Java_com_sun_grid_pygdi_jni_EventClientImpl_registerNative", evc_index);
   } else {
      throw_error_from_answer_list(env, ret, alp);
   }
   
   lFreeList(&alp);
   rmon_set_thread_ctx(NULL);
   pygdi_destroy_rmon_ctx(&rmon_ctx);
   
   DRETURN((jint)id);
}

/*
 * Class:     com_sun_grid_pygdi_jni_EventClientImpl
 * Method:    setFlushNative
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_com_sun_grid_pygdi_jni_EventClientImpl_setFlushNative(JNIEnv *env, jobject evcobj, jint evcIndex, jint eventId, jboolean flush, jint time)
{
   lList *alp = NULL;
   sge_evc_elem_t *elem = NULL;
   pygdi_result_t ret = PYGDI_SUCCESS;
   rmon_ctx_t rmon_ctx;
   ev_event myevent = (ev_event)eventId;

   DENTER(PYGDI_LAYER, "Java_com_sun_grid_pygdi_jni_EventClientImpl_setFlushNative");

   pygdi_init_rmon_ctx(env, PYGDI_EVENT_LOGGER, &rmon_ctx);
   rmon_set_thread_ctx(&rmon_ctx);
   
   if ((ret = lockEVC("Java_com_sun_grid_pygdi_jni_EventClientImpl_setFlushNative", evcIndex, &elem, &alp)) == PYGDI_SUCCESS) {
      if (!elem->sge_evc->ec_set_flush(elem->sge_evc, myevent, (bool)flush, time)) {
         THROW_ERROR((env, PYGDI_ERROR, "ec_set_flush failed"));
      }
      unlockEVC("Java_com_sun_grid_pygdi_jni_EventClientImpl_setFlushNative", evcIndex);
   } else {
      throw_error_from_answer_list(env, ret, alp);
   }

   lFreeList(&alp);
   rmon_set_thread_ctx(NULL);
   pygdi_destroy_rmon_ctx(&rmon_ctx);
   
   DRETURN_VOID;
}

/*
 * Class:     com_sun_grid_pygdi_jni_EventClientImpl
 * Method:    getFlushNative
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_com_sun_grid_pygdi_jni_EventClientImpl_getFlushNative(JNIEnv *env, jobject evcobj, jint evc_index, jint eventId)
{
   lList *alp = NULL;
   sge_evc_elem_t *elem = NULL;
   pygdi_result_t ret = PYGDI_SUCCESS;
   rmon_ctx_t rmon_ctx;
   ev_event myevent = (ev_event)eventId;
   jint time = 0;
   DENTER(PYGDI_LAYER, "Java_com_sun_grid_pygdi_jni_EventClientImpl_getFlushNative");

   pygdi_init_rmon_ctx(env, PYGDI_EVENT_LOGGER, &rmon_ctx);
   rmon_set_thread_ctx(&rmon_ctx);
   
   if ((ret = lockEVC("Java_com_sun_grid_pygdi_jni_EventClientImpl_getFlushNative", evc_index, &elem, &alp)) == PYGDI_SUCCESS) {
      time = elem->sge_evc->ec_get_flush(elem->sge_evc, myevent);
      unlockEVC("Java_com_sun_grid_pygdi_jni_EventClientImpl_getFlushNative", evc_index);
   } else {
      throw_error_from_answer_list(env, ret, alp);
   }

   lFreeList(&alp);
   rmon_set_thread_ctx(NULL);
   pygdi_destroy_rmon_ctx(&rmon_ctx);
   
   DRETURN(time);
}

/*
 * Class:     com_sun_grid_pygdi_jni_EventClientImpl
 * Method:    subscribeNative
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_sun_grid_pygdi_jni_EventClientImpl_subscribeNative(JNIEnv *env, jobject evcobj, jint evc_index, jint eventId, jboolean subscribe)
{
   lList *alp = NULL;
   sge_evc_elem_t *elem = NULL;
   pygdi_result_t ret = PYGDI_SUCCESS;
   rmon_ctx_t rmon_ctx;
   ev_event myevent = (ev_event)eventId;

   DENTER(PYGDI_LAYER, "Java_com_sun_grid_pygdi_jni_EventClientImpl_subscribeNative");

   pygdi_init_rmon_ctx(env, PYGDI_EVENT_LOGGER, &rmon_ctx);
   rmon_set_thread_ctx(&rmon_ctx);
   
   if ((ret = lockEVC("Java_com_sun_grid_pygdi_jni_EventClientImpl_subscribeNative", evc_index, &elem, &alp)) == PYGDI_SUCCESS) {
      if (subscribe) {
         if (!elem->sge_evc->ec_subscribe(elem->sge_evc, myevent)) {
            THROW_ERROR((env, PYGDI_ERROR, "ec_subscribe failed"));
         }
      } else {
         if (!elem->sge_evc->ec_unsubscribe(elem->sge_evc, myevent)) {
            THROW_ERROR((env, PYGDI_ERROR, "ec_unsubscribe failed"));
         }
      }
      unlockEVC("Java_com_sun_grid_pygdi_jni_EventClientImpl_subscribeNative", evc_index);
   } else {
      throw_error_from_answer_list(env, ret, alp);
   }
   
   lFreeList(&alp);
   rmon_set_thread_ctx(NULL);
   pygdi_destroy_rmon_ctx(&rmon_ctx);
   
   DRETURN_VOID;
}

JNIEXPORT void JNICALL Java_com_sun_grid_pygdi_jni_EventClientImpl_commitNative(JNIEnv *env, jobject evcobj, jint evc_index)
{
   lList *alp = NULL;
   sge_evc_elem_t *elem = NULL;
   pygdi_result_t ret = PYGDI_SUCCESS;
   rmon_ctx_t rmon_ctx;

   DENTER(PYGDI_LAYER, "Java_com_sun_grid_pygdi_jni_EventClientImpl_commitNative");

   pygdi_init_rmon_ctx(env, PYGDI_EVENT_LOGGER, &rmon_ctx);
   rmon_set_thread_ctx(&rmon_ctx);
   
   if ((ret = lockEVC("Java_com_sun_grid_pygdi_jni_EventClientImpl_commitNative", evc_index, &elem, &alp)) == PYGDI_SUCCESS) {
      if (!elem->sge_evc->ec_commit(elem->sge_evc, &alp)) {
         throw_error_from_answer_list(env, PYGDI_ERROR, alp);
      }
      unlockEVC("Java_com_sun_grid_pygdi_jni_EventClientImpl_commitNative", evc_index);
   } else {
      throw_error_from_answer_list(env, ret, alp);
   }
   
   lFreeList(&alp);
   rmon_set_thread_ctx(NULL);
   pygdi_destroy_rmon_ctx(&rmon_ctx);
   
   DRETURN_VOID;
}


/*
 * Class:     com_sun_grid_pygdi_jni_EventClientImpl
 * Method:    fillEvents
 * Signature: (Ljava/util/List;)V
 */
JNIEXPORT void JNICALL Java_com_sun_grid_pygdi_jni_EventClientImpl_fillEvents(JNIEnv *env, jobject evcobj, jint evc_index, jobject eventList)
{
   lList *elist = NULL;
   lList *alp = NULL;
   lListElem *ev = NULL;
   sge_evc_elem_t *elem = NULL;
   pygdi_result_t ret = PYGDI_SUCCESS;
   jobject logger = NULL;
   rmon_ctx_t rmon_ctx;
   
   DENTER(PYGDI_LAYER, "Java_com_sun_grid_pygdi_jni_EventClientImpl_fillEvents");

   pygdi_init_rmon_ctx(env, PYGDI_EVENT_LOGGER, &rmon_ctx);
   rmon_set_thread_ctx(&rmon_ctx);
   
   logger = pygdi_get_logger(env, PYGDI_EVENT_LOGGER);
   if (pygdi_is_loggable(env, logger, FINER)) {
      pygdi_log(env, logger, FINER, "before ec_get");
   }
   ret = waitEVC("Java_com_sun_grid_pygdi_jni_EventClientImpl_fillEvents wait", evc_index, &elist, &alp);
   if (ret != PYGDI_SUCCESS) {
      throw_error_from_answer_list(env, ret, alp);
      goto error;
   }
   if (pygdi_is_loggable(env, logger, FINER)) {
      pygdi_log(env, logger, FINER, "after ec_get");
   }

   for_each(ev, elist) {
      
      if (pygdi_is_loggable(env, logger, FINER)) {
         pygdi_log(env, logger, FINER, "before process_event");
      }

      ret = process_event(env, eventList, ev, &alp);
      
      if (pygdi_is_loggable(env, logger, FINER)) {
         pygdi_log(env, logger, FINER, "after process_event");
      }
      
      if (ret != PYGDI_SUCCESS) {
         
         if (pygdi_is_loggable(env, logger, WARNING)) {
            
            dstring ds = DSTRING_INIT;
            
            answer_list_to_dstring(alp, &ds);
            lFreeList(&alp);
            pygdi_log(env, logger, WARNING, sge_dstring_get_string(&ds));
            sge_dstring_free(&ds);
         }
      }
   }
   lFreeList(&elist);

   if (pygdi_is_loggable(env, logger, FINER)) {
      pygdi_log(env, logger, FINER, "before ec_wait");
   }

   if ((ret = lockEVC("Java_com_sun_grid_pygdi_jni_EventClientImpl_fillEvents 2", evc_index, &elem, &alp)) == PYGDI_SUCCESS) {
      if (elem->state == RUNNING) {
         elem->sge_evc->ec_wait(elem->sge_evc);
      }   
      unlockEVC("Java_com_sun_grid_pygdi_jni_EventClientImpl_fillEvents 2", evc_index);
   } else {
      throw_error_from_answer_list(env, ret, alp);
      goto error;
   }
   if (pygdi_is_loggable(env, logger, FINER)) {
      pygdi_log(env, logger, FINER, "after ec_wait");
   }

   
   {
      jint size = 0;
      if ((ret = List_size(env, eventList, &size, &alp)) != PYGDI_SUCCESS) {
         throw_error_from_answer_list(env, PYGDI_ILLEGAL_STATE, alp);
      }
      DPRINTF(("Received %d events\n", size));
   }

error:

   lFreeList(&alp);
   rmon_set_thread_ctx(NULL);
   pygdi_destroy_rmon_ctx(&rmon_ctx);
      
   DRETURN_VOID;
}

static pygdi_result_t process_event(JNIEnv *env, jobject eventList, lListElem *ev, lList** alpp) {
   
   pygdi_result_t ret = PYGDI_SUCCESS;
   u_long32 event_type = lGetUlong(ev, ET_type);
   jobject event = NULL;
   jlong timestamp = 0;
   jint  evtId = 0;
   DENTER(PYGDI_LAYER, "process_event");
   
   {
      dstring evc_buffer = DSTRING_INIT;
      const char* evc_text = event_text(ev, &evc_buffer);
      
      pygdi_log_printf(env, PYGDI_EVENT_LOGGER, FINE,
                      "BEGIN: event %s --------------", evc_text);
      
      pygdi_log_listelem(env, PYGDI_EVENT_LOGGER, FINE, ev);
      
      pygdi_log_printf(env, PYGDI_EVENT_LOGGER, FINE,
                      "END event %s --------------", evc_text);
                      
      sge_dstring_free(&evc_buffer);
   }
   
   evtId = lGetUlong(ev, ET_number);
   timestamp = lGetUlong(ev, ET_timestamp);

   switch(event_type) {

      case sgeE_JOB_MOD_SCHED_PRIORITY:     /*35 + event job modify priority */
         ret = EventFactoryBase_static_createJobPriorityModEvent(env, timestamp, evtId, &event, alpp);
         
         if (ret == PYGDI_SUCCESS) {
              ret = fill_generic_event(env, event, "com/sun/grid/pygdi/configuration/Job", JB_Type, SGE_EMA_MOD, ev, alpp);
         }
         break;
      case sgeE_QINSTANCE_SOS:              /*68 + event queue instance sos */
         ret = EventFactoryBase_static_createQueueInstanceSuspendEvent(env, timestamp, evtId, &event, alpp);
         
         if (ret == PYGDI_SUCCESS) {
              ret = fill_generic_event(env, event, "com/sun/grid/pygdi/configuration/QueueInstance", QU_Type, SGE_EMA_MOD, ev, alpp);
         }
         break;
      case sgeE_QINSTANCE_USOS:             /*69 + event queue instance usos */
         ret = EventFactoryBase_static_createQueueInstanceUnsuspendEvent(env, timestamp, evtId, &event, alpp);
         
         if (ret == PYGDI_SUCCESS) {
              ret = fill_generic_event(env, event, "com/sun/grid/pygdi/configuration/QueueInstance", QU_Type, SGE_EMA_MOD, ev, alpp);
         }
         break;
      case sgeE_JOB_FINAL_USAGE:            /*37 + event job final usage report after job end */
           ret = EventFactoryBase_static_createJobFinalUsageEvent(env, timestamp, evtId, &event, alpp);
           if (ret == PYGDI_SUCCESS) {
              ret = fill_job_usage_event(env, event, ev, alpp);
           }
           break;
      case sgeE_JOB_USAGE:                  /*36 + event job online usage */
           ret = EventFactoryBase_static_createJobUsageEvent(env, timestamp, evtId, &event, alpp);
           if (ret == PYGDI_SUCCESS) {
              ret = fill_job_usage_event(env, event, ev, alpp);
           }
           break;
      case sgeE_JOB_FINISH:                 /*38 + job finally finished or aborted (user view) */
           ret = EventFactoryBase_static_createJobFinishEvent(env, timestamp, evtId, &event, alpp);
           if (ret == PYGDI_SUCCESS) {
              ret = fill_job_event(env, event, ev, alpp);
           }
           break;
      case sgeE_QMASTER_GOES_DOWN:
           ret = EventFactoryBase_static_createQmasterGoesDownEvent(env, timestamp, evtId, &event, alpp);
           break;
      case sgeE_SCHEDDMONITOR:
           ret = EventFactoryBase_static_createSchedulerRunEvent(env, timestamp, evtId, &event, alpp);
           break;
      case sgeE_SHUTDOWN:        
           ret = EventFactoryBase_static_createShutdownEvent(env, timestamp, evtId, &event, alpp);
           break;
      default:
         ret = process_generic_event(env, &event, ev, alpp);

   }
   
   if (ret == PYGDI_SUCCESS) {
      jboolean addResult = false;
      ret = List_add(env, eventList, event, &addResult, alpp);
   }
   
   DRETURN(ret);
   
}


static pygdi_result_t fill_job_usage_event(JNIEnv *env, jobject event_obj, 
                                            lListElem *ev, lList **alpp) {
     pygdi_result_t ret = PYGDI_SUCCESS;
     lListElem *ep = NULL;
     
     DENTER(PYGDI_LAYER, "fill_job_event");
     ret = fill_job_event(env, event_obj, ev, alpp);
     
     /* Job usage events has a list of UA_Type in the ET_new_version */
     for_each(ep, lGetList(ev, ET_new_version)) {
        const char* name = lGetString(ep, UA_name);
        jdouble value = (jdouble)lGetDouble(ep, UA_value);
        
        ret = JobUsageEvent_addUsage(env, event_obj, name, value, alpp);
        if (ret != PYGDI_SUCCESS) {
           break;
        }
     }
     DRETURN(ret);
}

static pygdi_result_t fill_job_event(JNIEnv *env, jobject event_obj, lListElem *ev, lList **alpp) {                                               
     pygdi_result_t ret = PYGDI_SUCCESS;
     u_long32 job_id = lGetUlong(ev, ET_intkey);
     u_long32 ja_task_id = lGetUlong(ev, ET_intkey2);
     const char* pe_task_id = lGetString(ev, ET_strkey);
     
     DENTER(PYGDI_LAYER, "fill_job_event");
     
     ret = JobEvent_setJobId(env, event_obj, (jint)job_id, alpp);
     if (ret != PYGDI_SUCCESS) {
        DRETURN(ret);
     }
     ret = JobEvent_setTaskId(env, event_obj, (jint)ja_task_id, alpp);
     if (ret != PYGDI_SUCCESS) {
        DRETURN(ret);
     }
     ret = JobEvent_setPeTaskId(env, event_obj, pe_task_id, alpp);
     DRETURN(ret);
}
   
pygdi_result_t create_generic_event(JNIEnv *env, jobject *event_obj, const char* beanClassName, 
                                 const char* cullTypeName, lDescr *descr, int event_action, lListElem *ev, lList **alpp)
{
   jobject event = NULL;
   jlong timestamp = 0;
   jint  evtId = 0;
   pygdi_result_t ret = PYGDI_SUCCESS;
   
   DENTER(PYGDI_LAYER, "handleEvent");

   evtId = lGetUlong(ev, ET_number);
   timestamp = lGetUlong(ev, ET_timestamp);

   if (!descr) {
      answer_list_add(alpp, "descr is NULL", STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR); 
      DRETURN(PYGDI_ILLEGAL_ARGUMENT);
   }   

   switch (event_action) {
      case SGE_EMA_LIST:
         DPRINTF(("Handle list event\n"));
         ret = EventFactory_static_createListEvent(env, cullTypeName, timestamp, evtId, &event, alpp);
         break;
      case SGE_EMA_ADD:
         DPRINTF(("Handle add event\n"));
         ret = EventFactory_static_createAddEvent(env, cullTypeName, timestamp, evtId, &event, alpp);
         break;
      case SGE_EMA_MOD:
         DPRINTF(("Handle mod event\n"));
         ret = EventFactory_static_createModEvent(env, cullTypeName, timestamp, evtId, &event, alpp);
         break;
      case SGE_EMA_DEL:
         DPRINTF(("Handle del event\n"));
         ret = EventFactory_static_createDelEvent(env, cullTypeName, timestamp, evtId, &event, alpp);
         break;
      default:
         answer_list_add_sprintf(alpp, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR, 
                                 "Event action not yet handled %d", event_action);
                                 
         DRETURN(PYGDI_ILLEGAL_STATE);
   }

   if (ret != PYGDI_SUCCESS) {
      DRETURN(ret);
   }
   
   ret = fill_generic_event(env, event, beanClassName, descr, event_action, ev, alpp);
   if (ret != PYGDI_SUCCESS) {
      DRETURN(ret);
   }
   *event_obj = event;
   DRETURN(ret);
}

static pygdi_result_t fill_generic_event(JNIEnv *env, jobject event_obj, const char* beanClassName, 
                                        lDescr *descr, int event_action, lListElem *ev, lList **alpp) {

   lListElem *ep = NULL;                                           
   jclass beanClass = NULL;
   pygdi_result_t ret = PYGDI_SUCCESS;
   
   DENTER(PYGDI_LAYER, "fill_generic_event");
                                           
   if (beanClassName == NULL) {
      answer_list_add(alpp, "beanClassName is NULL", STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR); 
      DRETURN(PYGDI_ILLEGAL_ARGUMENT);
   }

   beanClass = (*env)->FindClass(env, beanClassName);
   if (test_jni_error(env, "handleEvent: FindClass failed", alpp)) {
      DRETURN(PYGDI_ILLEGAL_ARGUMENT);
   }
   
   if (event_action == SGE_EMA_LIST) {
      jobject  obj = NULL;
      
      for_each(ep, lGetList(ev, ET_new_version)) {
         if ((ret = listelem_to_obj(env, ep, &obj, descr, beanClass, alpp)) != PYGDI_SUCCESS) {
            DRETURN(ret);
         }
         ret = ListEvent_add(env, event_obj, obj, alpp);
         if (ret != PYGDI_SUCCESS) {
            DRETURN(ret);
         }
      }
   } else {
      jobject  obj = NULL;

      ret = ChangedObjectEvent_setPKInfo(env, event_obj,
                                        lGetUlong(ev,ET_intkey),
                                        lGetUlong(ev,ET_intkey2),
                                        lGetString(ev,ET_strkey),
                                        lGetString(ev,ET_strkey2),
                                        alpp);
                                        
      if (ret != PYGDI_SUCCESS) {
         DRETURN(ret);
      }
      
      ep = lFirst(lGetList(ev, ET_new_version));
      
      /* DEL events do not have a new version */
      if (ep != NULL) {
         if ((ret = listelem_to_obj(env, ep, &obj, descr, beanClass, alpp)) != PYGDI_SUCCESS) {
            DRETURN(ret);
         }
         ret = ChangedObjectEvent_setChangedObject(env, event_obj, obj, alpp);
         if (ret != PYGDI_SUCCESS) {
            DRETURN(ret);
         }
      } else if (event_action != SGE_EMA_DEL) {
         jclass evt_class = NULL;
         jstring evt_classname_obj = NULL;
         const char* evt_classname = NULL;
         
         ret = Object_getClass(env, event_obj, &evt_class, alpp);
         if (ret != PYGDI_SUCCESS) {
            DRETURN(ret);
         }
         
         ret = Class_getName(env, evt_class, &evt_classname_obj, alpp);
         if (ret != PYGDI_SUCCESS) {
            DRETURN(ret);
         }
         evt_classname = (*env)->GetStringUTFChars(env, evt_classname_obj, 0);
         if (evt_classname == NULL) {
            answer_list_add(alpp, "fill_generic_event: GetStringUTFChars failed. Out of memory.", STATUS_EMALLOC, ANSWER_QUALITY_ERROR);
            DRETURN(PYGDI_ERROR);
         }
         pygdi_log_printf(env, PYGDI_EVENT_LOGGER, WARNING,
                         "generic event did not contain a new version (%s)",
                         evt_classname);
         (*env)->ReleaseStringUTFChars(env, evt_classname_obj, evt_classname);
      }
   }
   
   DRETURN(PYGDI_SUCCESS);
}

