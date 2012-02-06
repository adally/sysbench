/* Copyright (C) 2005 MySQL AB

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef HAVE_STRING_H
# include <string.h>
#endif

#include "sb_options.h"
#include "db_driver.h"

#include "nuodb.h"

/* NuoDB driver arguments */

static sb_arg_t nuodb_drv_args[] =
{
  {"nuodb-host", "NuoDB server host", SB_ARG_TYPE_STRING, "localhost"},
  {"nuodb-user", "NuoDB user", SB_ARG_TYPE_STRING, "cloud"},
  {"nuodb-password", "NuoDB password", SB_ARG_TYPE_STRING, "user"},
  {"nuodb-database", "NuoDB database name", SB_ARG_TYPE_STRING, "test"},
  {"nuodb-schema", "NuoDB schema name", SB_ARG_TYPE_STRING, "test"},
  
  {NULL, NULL, SB_ARG_TYPE_NULL, NULL}
};

typedef struct
{
  char               *host;
  char               *user;
  char               *password;
  char               *db;
} nuodb_drv_args_t;

/* NuoSQL driver capabilities */

static drv_caps_t nuodb_drv_caps =
{
  1,    /* multi_rows_insert */
  1,    /* prepared_statements */
  0,    /* auto_increment */
  1,    /* needs_commit */
  0,    /* serial */
  0,    /* unsigned int */
  
  NULL  /* table_options_str */
};

static nuodb_drv_args_t args;          /* driver args */

static char use_ps; /* whether server-side prepared statemens should be used */

static int nuodb_drv_init(void);
static int nuodb_drv_describe(drv_caps_t *);
static int nuodb_drv_connect(db_conn_t *);
static int nuodb_drv_disconnect(db_conn_t *);
static int nuodb_drv_prepare(db_stmt_t *, const char *);
static int nuodb_drv_bind_param(db_stmt_t *, db_bind_t *, unsigned int);
static int nuodb_drv_bind_result(db_stmt_t *, db_bind_t *, unsigned int);
static int nuodb_drv_execute(db_stmt_t *, db_result_set_t *);
static int nuodb_drv_fetch(db_result_set_t *);
static int nuodb_drv_fetch_row(db_result_set_t *, db_row_t *);
static unsigned long long nuodb_drv_num_rows(db_result_set_t *);
static int nuodb_drv_query(db_conn_t *, const char *, db_result_set_t *);
static int nuodb_drv_free_results(db_result_set_t *);
static int nuodb_drv_close(db_stmt_t *);
static int nuodb_drv_store_results(db_result_set_t *);
static int nuodb_drv_done(void);

/* PgSQL driver definition */

static db_driver_t nuodb_driver =
{
  .sname = "nuodb",
  .lname = "NuoDB driver",
  .args = nuodb_drv_args,
  .ops =
  {
    nuodb_drv_init,
    nuodb_drv_describe,
    nuodb_drv_connect,
    nuodb_drv_disconnect,
    nuodb_drv_prepare,
    nuodb_drv_bind_param,
    nuodb_drv_bind_result,
    nuodb_drv_execute,
    nuodb_drv_fetch,
    nuodb_drv_fetch_row,
    nuodb_drv_num_rows,
    nuodb_drv_free_results,
    nuodb_drv_close,
    nuodb_drv_query,
    nuodb_drv_store_results,
    nuodb_drv_done
  },
  .listitem = {NULL, NULL}
};


/* Register NuoDB driver */

int register_driver_nuodb(sb_list_t *drivers)
{
  SB_LIST_ADD_TAIL(&nuodb_driver.listitem, drivers);

  return SB_DB_ERROR_NONE;
}


/* NuoDB driver initialization */


int nuodb_drv_init(void)
{
  return SB_DB_ERROR_NONE;
}


/* Describe database capabilities */


int nuodb_drv_describe(drv_caps_t *caps)
{
  *caps = nuodb_drv_caps;

  return SB_DB_ERROR_NONE;
}

/* Connect to database */

int nuodb_drv_connect(db_conn_t *sb_conn)
{
  sb_conn->ptr = nuodb_create_connection(
    sb_get_value_string("nuodb-host"),
    sb_get_value_string("nuodb-user"),
    sb_get_value_string("nuodb-password"),
    sb_get_value_string("nuodb-database"),
    sb_get_value_string("nuodb-schema")
  );

  if (sb_conn->ptr == NULL) {
    return 1;
  } else {
    return 0;
  }
}

/* Disconnect from database */

int nuodb_drv_disconnect(db_conn_t *sb_conn)
{
  nuodb_close_connection(sb_conn->ptr);

  return SB_DB_ERROR_NONE;
}

/* Prepare statement */

int nuodb_drv_prepare(db_stmt_t *stmt, const char *query)
{
  stmt->ptr = nuodb_prepare_statement(stmt->connection->ptr, query);

  if (stmt->ptr != NULL) {
    return 0;
  } else {
    return 1;
  }
}

/* Bind parameters for prepared statement */

int nuodb_drv_bind_param(db_stmt_t *stmt, db_bind_t *params, unsigned int len)
{
  unsigned int i;

  if (stmt->ptr == NULL)
    return 1;

  for (i = 0; i < len; i++) {
    db_bind_t* param = &params[i];
    if (params[i].is_null) {
      nuodb_bind_param_null(stmt->ptr, i + 1);

    } else {
      switch (params[i].type) {
        case DB_TYPE_INT:
          nuodb_bind_param_int(stmt->ptr, i + 1, *((int *)param->buffer));
          break;

        case DB_TYPE_CHAR:
        case DB_TYPE_VARCHAR:
          {
            char * copy_str = strcpy(calloc(params[i].data_len + 1, sizeof(char)), params[i].buffer);
            nuodb_bind_param_string(stmt->ptr, i + 1, copy_str);
            free(copy_str);
            break;
          }

        default:
          log_text(LOG_FATAL, "nuodb_drv_bind_param() does not support param type %i", params[i].type);
          return 1;
      }
    }
  }

  return 0;
}

/* Bind results for prepared statement */

int nuodb_drv_bind_result(db_stmt_t *stmt, db_bind_t *params, unsigned int len)
{
  (void) stmt;
  (void) params;
  (void) len;

  log_text(LOG_FATAL, "nuodb_drv_bind_result() is not supported by NuoDB sysbench driver.");

  return 1;
}

/* Execute prepared statement */

int nuodb_drv_execute(db_stmt_t *stmt, db_result_set_t *rs)
{
  int nuodb_code = nuodb_execute_prepared_statement(stmt->ptr, rs->ptr);

  if (nuodb_code == SB_NUODB_DEADLOCK) {
    return SB_DB_ERROR_DEADLOCK;
  } else if (nuodb_code == SB_NUODB_ERROR) {
    return SB_DB_ERROR_FAILED;
  } else {
    return SB_DB_ERROR_NONE;
  }
}

/* Execute SQL query */

int nuodb_drv_query(db_conn_t *sb_conn, const char *query, db_result_set_t *rs)
{
  int nuodb_code;
  log_text(LOG_DEBUG, "nuodb_execute_query(%s)", query);

  if (strncmp(query, "SELECT", 6) == 0 || strncmp(query, "select", 6) == 0) {
    nuodb_code = nuodb_execute_query(sb_conn->ptr, query, rs->ptr);
  } else {
    nuodb_code = nuodb_execute(sb_conn->ptr, query);
  }

  if (nuodb_code == SB_NUODB_DEADLOCK) {
    return SB_DB_ERROR_DEADLOCK;
  } else if (nuodb_code == SB_NUODB_ERROR) {
    return SB_DB_ERROR_FAILED;
  } else {
    return SB_DB_ERROR_NONE;
  }
}

/* Fetch row from result set of a prepared statement */

int nuodb_drv_fetch(db_result_set_t *rs)
{
  (void) rs;

  log_text(LOG_FATAL, "nuodb_drv_fetch() is not supported by NuoDB sysbench driver.");

  return 1;
}

/* Fetch row from result set of a query */

int nuodb_drv_fetch_row(db_result_set_t *rs, db_row_t *row)
{
  (void) rs;
  (void) row;

  log_text(LOG_FATAL, "nuodb_drv_fetch_row() is not supported by NuoDB sysbench driver.");

  return 1;
}

/* Return the number of rows in a result set */

unsigned long long nuodb_drv_num_rows(db_result_set_t *rs)
{
  return rs->nrows;
}

/* Store results from the last query */

int nuodb_drv_store_results(db_result_set_t *rs)
{
  if (rs->ptr != NULL) {
    rs->nrows = nuodb_fetch_result(rs->ptr);
    return 0;
  } else {
    return 1;
  }
}

/* Free result set */

int nuodb_drv_free_results(db_result_set_t *rs)
{
  if (rs->ptr != NULL) {
    nuodb_close_result(rs->ptr);
    return 0;
  } else {
    return 1;
  }
}

/* Close prepared statement */

int nuodb_drv_close(db_stmt_t *stmt)
{
  nuodb_close_statement(stmt->ptr);

  return 0;
}

/* Uninitialize driver */
int nuodb_drv_done(void)
{
  return SB_DB_ERROR_NONE;
}
