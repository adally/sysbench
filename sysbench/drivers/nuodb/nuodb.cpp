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

/* 
# This file contains the glue functions between the sysbench C API and the NuoDB C++ api
# The NuoDB C++ calls had to be abstracted in a separate file as placining them directly in
# drv_nuodb.cc causes gcc to complain about sysbench headers not being syntactically valid
*/

#include <cstddef>
#include <Connection.h>
#include <SQLException.h>
#include "nuodb.h"
#include <stdio.h>

#include <string.h>

void * nuodb_create_connection(const char * chorus, const char * user, const char * password, const char * schema)
{
  try
  {
    NuoDB::Connection *conn = createConnection();

    NuoDB::Properties *properties = conn->allocProperties();
    properties->putValue("user", user);
    properties->putValue("password", password);
    properties->putValue("schema", schema);

    conn->openDatabase(chorus, properties);
    
    return (void *) conn;
  }
  catch (NuoDB::SQLException& xcp)
  {
    return NULL;
  }
}

void nuodb_close_connection(void * conn)
{
  NuoDB::Connection *nuodb_conn = (NuoDB::Connection *) conn;
  nuodb_conn->close();
}

/*
# We have separate nuodb_execute() and nuodb_execute_query()
# as NuoDB's executeQuery() method only allows SELECTs
*/

int nuodb_execute(void * conn, const char * query)
{
  try
  {
    NuoDB::Connection *nuodb_conn = (NuoDB::Connection *) conn;
    NuoDB::Statement *nuodb_stmt = nuodb_conn->createStatement();
    nuodb_stmt->execute(query);
    return SB_NUODB_OK;
  }
  catch (NuoDB::SQLException& xcp)
  {
    if (strstr(xcp.getText(), "conflict") || strstr(xcp.getText(), "deadlock") || strstr(xcp.getText(), "timeout") || strstr(xcp.getText(), "duplicate") || strstr(xcp.getText(), "pending")) {
      return SB_NUODB_DEADLOCK;
    } else {
      printf("Exception in nuodb_execute(): %s\n", xcp.getText());
      printf("Query: %s\n", query);
      return SB_NUODB_ERROR;
    }
  }
}

int nuodb_execute_query(void * conn, const char * query, void * rs)
{
  NuoDB::Statement * nuodb_stmt;
  try
  {
    NuoDB::Connection *nuodb_conn = (NuoDB::Connection *) conn;
    nuodb_stmt = nuodb_conn->createStatement();
    NuoDB::ResultSet *nuodb_rs = nuodb_stmt->executeQuery(query);
    nuodb_stmt->close();
    rs = nuodb_rs;
    return SB_NUODB_OK;
  }
  catch (NuoDB::SQLException& xcp)
  {
    if (nuodb_stmt != NULL) {
      nuodb_stmt->close();
    }

    if (strstr(xcp.getText(), "conflict") || strstr(xcp.getText(), "deadlock") || strstr(xcp.getText(), "timeout") || strstr(xcp.getText(), "duplicate")) {
      return SB_NUODB_DEADLOCK;
    } else {
      printf("Exception in nuodb_execute_query(): %s\n", xcp.getText());
      printf("Query: %s\n", query);
      return SB_NUODB_ERROR;
    }
  }
}

void * nuodb_prepare_statement(void * conn, const char * query)
{
  try
  {
    NuoDB::Connection *nuodb_conn = (NuoDB::Connection *) conn;
    NuoDB::PreparedStatement *nuodb_stmt = nuodb_conn->prepareStatement(query);
    return nuodb_stmt;
  }
  catch (NuoDB::SQLException& xcp)
  {
    printf("Exception in nuodb_prepare_statement(): %s\n", xcp.getText());
    printf("Query: %s\n", query);
    return NULL;
  }
}

void nuodb_bind_param_int(void * stmt, unsigned int pos, int value)
{
  NuoDB::PreparedStatement *nuodb_stmt = (NuoDB::PreparedStatement *) stmt;
  nuodb_stmt->setInt(pos, (int) value);
}

void nuodb_bind_param_string(void * stmt, unsigned int pos, const char * value)
{
  NuoDB::PreparedStatement *nuodb_stmt = (NuoDB::PreparedStatement *) stmt;
  nuodb_stmt->setString(pos, value);
}

void nuodb_bind_param_null(void * stmt, unsigned int pos)
{
  NuoDB::PreparedStatement *nuodb_stmt = (NuoDB::PreparedStatement *) stmt;
  nuodb_stmt->setNull(pos, 0);
}

int nuodb_execute_prepared_statement(void * stmt, void * rs)
{
  try
  {
    NuoDB::PreparedStatement *nuodb_stmt = (NuoDB::PreparedStatement *) stmt;
    NuoDB::ResultSet *nuodb_rs = nuodb_stmt->executeQuery();
    rs = nuodb_rs;
    return SB_NUODB_OK;
  }
  catch (NuoDB::SQLException& xcp)
  {
    if (strstr(xcp.getText(), "conflict") || strstr(xcp.getText(), "deadlock") || strstr(xcp.getText(), "timeout") || strstr(xcp.getText(), "duplicate")) {
      return SB_NUODB_DEADLOCK;
    } else {
      printf("Exception: %s\n", xcp.getText());
      return SB_NUODB_ERROR;
    }
  }
}

/*
# As with the other Sysbench drivers, the nuodb_fetch_result() does not return
# anything meaningful. Instead, it just iterates over the result set in order
# to make sure that we have read all the data from the server.
*/

unsigned long long nuodb_fetch_result(void * rs)
{
  NuoDB::ResultSet *nuodb_rs = (NuoDB::ResultSet *) rs;

  int ncolumns = nuodb_rs->getMetaData()->getColumnCount();
  int rows = 0;
  int i;
  
  while (nuodb_rs->next()) {
    rows++;
    for (i = 1; i <= ncolumns; i++) {
      nuodb_rs->getString(i);
    }
  }

  return rows;
}

void nuodb_close_result(void * rs)
{
  NuoDB::ResultSet *nuodb_rs = (NuoDB::ResultSet *) rs;
  nuodb_rs->close();
}

void nuodb_close_statement(void * stmt)
{
  NuoDB::Statement *nuodb_stmt = (NuoDB::Statement *) stmt;
  nuodb_stmt->close();
}
