#include <err.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>

/* OP-TEE TEE client API (built by optee_client) */
#include <tee_client_api.h>

/* TA API: UUID and command IDs */
#include "frank_ca.h"

/* TEE resources */
struct tee_ctx {
	TEEC_Context ctx;
	TEEC_Session sess;
};

char data[1048576] = {0};

void prepare_tee_session(struct tee_ctx *ctx)
{
	TEEC_UUID uuid = FRANK_TA_UUID;
	uint32_t err_origin;
	TEEC_Result res;

	/* Initialize a context connecting us to the TEE */
	res = TEEC_InitializeContext(NULL, &ctx->ctx);
	if (res != TEEC_SUCCESS)
		errx(1, "TEEC_InitializeContext failed with code 0x%x", res);
	
	/* open a session with the TA */
	res = TEEC_OpenSession(&ctx->ctx, &ctx->sess, &uuid,
			       TEEC_LOGIN_PUBLIC, NULL, NULL, &err_origin);
	if (res != TEEC_SUCCESS)
		errx(1, "TEEC_Opensession failed with code 0x%x origin 0x%x",
			res, err_origin);
}

void terminate_tee_seesion(struct tee_ctx *ctx)
{
	TEEC_CloseSession(&ctx->sess);
	TEEC_FinalizeContext(&ctx->ctx);
}

TEEC_Result sqlite_cmd_insert(struct tee_ctx *ctx, char *data, size_t data_len)
{
	TEEC_Operation op;
	uint32_t err_origin;
	TEEC_Result res;

	/* Clear the TEEC_Operation struct */
	memset(&op, 0, sizeof(op));
	/* Prepare the argument */
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_OUTPUT, TEEC_NONE,
					TEEC_NONE, TEEC_NONE);
	op.params[0].tmpref.size = data_len;
	op.params[0].tmpref.buffer = data;

	res = TEEC_InvokeCommand(&ctx->sess,
				 TA_FRANK_CMD_INSERT,
				 &op, &err_origin);
	if (res != TEEC_SUCCESS)
		errx(1, "sqlite_cmd_insert failed with code 0x%x origin 0x%x",
			res, err_origin);

	return res;
}

TEEC_Result sqlite_cmd_select(struct tee_ctx *ctx, char *data, size_t data_len)
{
	TEEC_Operation op;
	uint32_t err_origin;
	TEEC_Result res;

	/* Clear the TEEC_Operation struct */
	memset(&op, 0, sizeof(op));
	/* Prepare the argument */
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_OUTPUT, TEEC_NONE,
					TEEC_NONE, TEEC_NONE);
	op.params[0].tmpref.size = data_len;
	op.params[0].tmpref.buffer = data;

	res = TEEC_InvokeCommand(&ctx->sess,
				 TA_FRANK_CMD_SELECT,
				 &op, &err_origin);
	if (res != TEEC_SUCCESS)
		errx(1, "sqlite_cmd_select failed with code 0x%x origin 0x%x",
			res, err_origin);

	return res;
}

TEEC_Result sqlite_cmd_where(struct tee_ctx *ctx, char *data, size_t data_len)
{
	TEEC_Operation op;
	uint32_t err_origin;
	TEEC_Result res;

	/* Clear the TEEC_Operation struct */
	memset(&op, 0, sizeof(op));
	/* Prepare the argument */
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_OUTPUT, TEEC_NONE,
					TEEC_NONE, TEEC_NONE);
	op.params[0].tmpref.size = data_len;
	op.params[0].tmpref.buffer = data;

	res = TEEC_InvokeCommand(&ctx->sess,
				 TA_FRANK_CMD_WHERE,
				 &op, &err_origin);
	if (res != TEEC_SUCCESS)
		errx(1, "sqlite_cmd_where failed with code 0x%x origin 0x%x",
			res, err_origin);

	return res;
}

TEEC_Result sqlite_cmd_update(struct tee_ctx *ctx, char *data, size_t data_len)
{
	TEEC_Operation op;
	uint32_t err_origin;
	TEEC_Result res;

	/* Clear the TEEC_Operation struct */
	memset(&op, 0, sizeof(op));
	/* Prepare the argument */
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_OUTPUT, TEEC_NONE,
					TEEC_NONE, TEEC_NONE);
	op.params[0].tmpref.size = data_len;
	op.params[0].tmpref.buffer = data;

	res = TEEC_InvokeCommand(&ctx->sess,
				 TA_FRANK_CMD_UPDATE,
				 &op, &err_origin);
	if (res != TEEC_SUCCESS)
		errx(1, "sqlite_cmd_update failed with code 0x%x origin 0x%x",
			res, err_origin);

	return res;
}

TEEC_Result sqlite_cmd_ope(struct tee_ctx *ctx, char *data, size_t data_len)
{
	TEEC_Operation op;
	uint32_t err_origin;
	TEEC_Result res;
	double random = 0.0;

	/* Clear the TEEC_Operation struct */
	memset(&op, 0, sizeof(op));
	/* Prepare the argument */
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_OUTPUT, TEEC_NONE,
					TEEC_NONE, TEEC_NONE);
	op.params[0].tmpref.size = sizeof(random);
	op.params[0].tmpref.buffer = &random;

	res = TEEC_InvokeCommand(&ctx->sess,
				 TA_FRANK_CMD_OPE,
				 &op, &err_origin);
	if (res != TEEC_SUCCESS)
		errx(1, "TEEC_InvokeCommand failed with code 0x%x origin 0x%x",
			res, err_origin);

	printf ("Generating random = %f", random);
	return res;
}

int main(int argc, char **argv)
{
	struct timeval startTime, endTime, start, end;
	struct tee_ctx ctx;
	if (argc != 2) {
		fprintf(stderr, "Usage: %s command\n", argv[0]);
		return(1);
	}

	gettimeofday(&startTime,NULL);

	gettimeofday(&start,NULL);
	prepare_tee_session(&ctx);
	gettimeofday(&end,NULL);
	printf("time prepare_tee_session: %f\n", 1000*(end.tv_sec-start.tv_sec)+(end.tv_usec-start.tv_usec)/1000.0);

	if (strcmp(argv[1], "insert") == 0) {
		sqlite_cmd_insert(&ctx, data, 4096U);
	}
	else if (strcmp(argv[1], "select") == 0) {
		sqlite_cmd_select(&ctx, data, 1048576U);
		printf("%s", data);
	}
	else if (strcmp(argv[1], "where") == 0) {
		sqlite_cmd_where(&ctx, data, 1048576U);
		printf("%s", data);
	}
	else if (strcmp(argv[1], "update") == 0) {
		sqlite_cmd_update(&ctx, data, 4096U);
	}
	else if (strcmp(argv[1], "ope") == 0) {
		sqlite_cmd_ope(&ctx, data, 4096U);
	}

	gettimeofday(&start,NULL);
	terminate_tee_seesion(&ctx);
	gettimeofday(&end,NULL);
	printf("time terminate_tee_session: %f\n", 1000*(end.tv_sec-start.tv_sec)+(end.tv_usec-start.tv_usec)/1000.0);

	gettimeofday(&endTime,NULL);
	printf("time all: %f\n", 1000*(endTime.tv_sec-startTime.tv_sec)+(endTime.tv_usec-startTime.tv_usec)/1000.0);
	
	return 0;
}
