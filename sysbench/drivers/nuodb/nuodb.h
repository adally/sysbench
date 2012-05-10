/****************************************************************************
 * Copyright (c) 2012, NuoDB, Inc.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of NuoDB, Inc. nor the names of its contributors may
 *       be used to endorse or promote products derived from this software
 *       without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NUODB, INC. BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 ****************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

#define SB_NUODB_OK 0
#define SB_NUODB_ERROR 1
#define SB_NUODB_DEADLOCK 2

void * nuodb_create_connection(const char * chorus, const char * user, const char * password, const char * schema);

void nuodb_close_connection(void * conn);

int nuodb_execute_query(void * conn, const char * query, void * rs);

int nuodb_execute(void * conn, const char * query);

int nuodb_execute_statement(void * stmt);

unsigned long long nuodb_fetch_result(void * stmt);

void nuodb_close_result(void * rs);

void nuodb_close_statement(void * stmt);

void * nuodb_prepare_statement(void * conn, const char * query);

int nuodb_execute_prepared_statement(void * stmt, void * rs);

void nuodb_bind_param_int(void * stmt, unsigned int pos, int value);

void nuodb_bind_param_string(void * stmt, unsigned int pos, const char * value);

void nuodb_bind_param_null(void * stmt, unsigned int pos);

#ifdef __cplusplus
}
#endif
