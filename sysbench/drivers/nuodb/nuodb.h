#ifdef __cplusplus
extern "C" {
#endif

#define SB_NUODB_OK 0
#define SB_NUODB_ERROR 1
#define SB_NUODB_DEADLOCK 2

void * nuodb_create_connection(const char * host, const char * user, const char * password, const char * database, const char * schema);

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
