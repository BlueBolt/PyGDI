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
// File:pygdi_logging.h
// 
// 
//******************************************************************************

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
