#ifndef PYGDI_LOGGING_H
#define PYGDI_LOGGING_H

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


typedef enum {
   SEVERE = 0,
   WARNING,
   INFO,
   CONFIG,
   FINE,
   FINER,
   FINEST,
   LOG_LEVEL_COUNT
} log_level_t;

#define PYGDI_LOGGER        "com.sun.grid.pygdi.PYGDI"
#define PYGDI_QSTAT_LOGGER  "com.sun.grid.pygdi.monitoring.qstat"
#define PYGDI_QHOST_LOGGER  "com.sun.grid.pygdi.monitoring.qhost"
#define PYGDI_EVENT_LOGGER  "com.sun.grid.pygdi.event"

jobject pygdi_get_logger(JNIEnv *env, const char* logger);
jboolean pygdi_is_loggable(JNIEnv *env, jobject logger, log_level_t level);
void pygdi_log_printf(JNIEnv *env, const char* logger, log_level_t level, const char* fmt, ...);
void pygdi_log(JNIEnv *env, jobject logger, log_level_t level, const char* msg);
void pygdi_log_list(JNIEnv *env, const char* logger, log_level_t level, lList* list);
void pygdi_log_listelem(JNIEnv *env, const char* logger, log_level_t level, lListElem *elem);
void pygdi_log_answer_list(JNIEnv *env, const char* logger, lList *alp);

int pygdi_init_rmon_ctx(JNIEnv *env, const char* logger, rmon_ctx_t *ctx);
void pygdi_destroy_rmon_ctx(rmon_ctx_t *rmon_ctx);

#endif
