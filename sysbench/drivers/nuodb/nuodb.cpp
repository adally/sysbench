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
    Connection *conn;
    conn = createConnection();

    Properties *properties = conn->allocProperties();
    properties->putValue("user", user);
    properties->putValue("password", password);
    properties->putValue("schema", schema);

    conn->openDatabase(chorus, properties);
    
    return (void *) conn;
  }
  catch (SQLException& xcp)
  {
    return NULL;
  }
}

void nuodb_close_connection(void * conn)
{
  Connection *nuodb_conn = (Connection *) conn;
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
    Connection *nuodb_conn = (Connection *) conn;
    Statement *nuodb_stmt = nuodb_conn->createStatement();
    nuodb_stmt->execute(query);
    return SB_NUODB_OK;
  }
  catch (SQLException& xcp)
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
  Statement * nuodb_stmt;
  try
  {
    Connection *nuodb_conn = (Connection *) conn;
    nuodb_stmt = nuodb_conn->createStatement();
    ResultSet *nuodb_rs = nuodb_stmt->executeQuery(query);
    nuodb_stmt->close();
    rs = nuodb_rs;
    return SB_NUODB_OK;
  }
  catch (SQLException& xcp)
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
    Connection *nuodb_conn = (Connection *) conn;
    PreparedStatement *nuodb_stmt = nuodb_conn->prepareStatement(query);
    return nuodb_stmt;
  }
  catch (SQLException& xcp)
  {
    printf("Exception in nuodb_prepare_statement(): %s\n", xcp.getText());
    printf("Query: %s\n", query);
    return NULL;
  }
}

void nuodb_bind_param_int(void * stmt, unsigned int pos, int value)
{
  PreparedStatement *nuodb_stmt = (PreparedStatement *) stmt;
  nuodb_stmt->setInt(pos, (int) value);
}

void nuodb_bind_param_string(void * stmt, unsigned int pos, const char * value)
{
  PreparedStatement *nuodb_stmt = (PreparedStatement *) stmt;
  nuodb_stmt->setString(pos, value);
}

void nuodb_bind_param_null(void * stmt, unsigned int pos)
{
  PreparedStatement *nuodb_stmt = (PreparedStatement *) stmt;
  nuodb_stmt->setNull(pos, 0);
}

int nuodb_execute_prepared_statement(void * stmt, void * rs)
{
  try
  {
    PreparedStatement *nuodb_stmt = (PreparedStatement *) stmt;
    ResultSet *nuodb_rs = nuodb_stmt->executeQuery();
    rs = nuodb_rs;
    return SB_NUODB_OK;
  }
  catch (SQLException& xcp)
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
  ResultSet *nuodb_rs = (ResultSet *) rs;

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
  ResultSet *nuodb_rs = (ResultSet *) rs;
  nuodb_rs->close();
}

void nuodb_close_statement(void * stmt)
{
  Statement *nuodb_stmt = (Statement *) stmt;
  nuodb_stmt->close();
}
