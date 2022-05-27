#include <tee_internal_api.h>
#include <tee_internal_api_extensions.h>

#include <frank_ta.h>
#include <string.h>
#include <utee_syscalls.h>
#include <sqlite3.h>

static TEE_Time start, end;

TEE_Result TA_CreateEntryPoint(void)
{
	/* Nothing to do */
	return TEE_SUCCESS;
}

void TA_DestroyEntryPoint(void)
{
	/* Nothing to do */
}

TEE_Result TA_OpenSessionEntryPoint(uint32_t __unused param_types,
				    TEE_Param __unused params[4],
				    void __unused **session)
{
	/* Nothing to do */
	return TEE_SUCCESS;
}

void TA_CloseSessionEntryPoint(void __unused *session)
{
	/* Nothing to do */
}

static cmd_test(uint32_t param_types,
	TEE_Param params[4])
{
	const uint32_t exp_param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT,
						   TEE_PARAM_TYPE_NONE,
						   TEE_PARAM_TYPE_NONE,
						   TEE_PARAM_TYPE_NONE);
    	char *data;
	uint32_t data_len;

	DMSG("has been called");

	if (param_types != exp_param_types)
		return TEE_ERROR_BAD_PARAMETERS;
    
	data = params[0].memref.buffer;
	data_len = params[0].memref.size;
	IMSG("data: %s\ndata_len: %u from NW", data, data_len);
	return TEE_SUCCESS;
}

static int callback(void *NotUsed, int argc, char **argv, char **azColName)
{
	int i;
	for (int i=0; i < argc; i++) {
		IMSG("%s = %s", azColName[i], argv[i] ? argv[i] : "NULL");	
	}
	return 0;
}

static TEE_Result cmd_sqlite(uint32_t param_types,
	TEE_Param params[4])
{
	const uint32_t exp_param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT,
						   TEE_PARAM_TYPE_NONE,
						   TEE_PARAM_TYPE_NONE,
						   TEE_PARAM_TYPE_NONE);
       	char *data;
	uint32_t data_len;
	sqlite3 *db;
	int rc;
	char *zErrMsg = 0;

	DMSG("has been called");

	if (param_types != exp_param_types)
		return TEE_ERROR_BAD_PARAMETERS;
    
	data = params[0].memref.buffer;
	data_len = params[0].memref.size;
	
	TEE_GetREETime(&start);
	rc = sqlite3_open("/home/root/trustsqlitefile.db", &db);
	if(rc){
		IMSG("Can't open database: %s\n", sqlite3_errmsg(db));
		sqlite3_close(db);
		return TEE_SUCCESS;
	}
    TEE_GetREETime(&end);
	DMSG("sqlite3_open time: %d ms", 1000*(end.seconds-start.seconds) + (end.millis-start.millis));

	TEE_GetREETime(&start);
	rc = sqlite3_exec(db, data, callback, 0, &zErrMsg);
	if(rc != SQLITE_OK){
		IMSG("SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}
	TEE_GetREETime(&end);
	DMSG("sqlite3_exec time: %d ms", 1000*(end.seconds-start.seconds) + (end.millis-start.millis));
	
	TEE_GetREETime(&start);
	sqlite3_close(db);
	TEE_GetREETime(&end);
	DMSG("sqlite3_close time: %d ms", 1000*(end.seconds-start.seconds) + (end.millis-start.millis));
	return TEE_SUCCESS;
}

TEE_Result TA_InvokeCommandEntryPoint(void __unused *session,
				      uint32_t command,
				      uint32_t param_types,
				      TEE_Param params[4])
{
	switch (command) {
	case TA_FRANK_CMD_TEST:
		return cmd_test(param_types, params);
	case TA_FRANK_CMD_SQLite:
	    	return cmd_sqlite(param_types, params);
	default:
		EMSG("Command ID 0x%x is not supported", command);
		return TEE_ERROR_NOT_SUPPORTED;
	}
}
