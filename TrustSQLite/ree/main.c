#include <err.h>
#include <stdio.h>
#include <string.h>

/* OP-TEE TEE client API (built by optee_client) */
#include <tee_client_api.h>

/* TA API: UUID and command IDs */
#include "frank_ca.h"

/* TEE resources */
struct tee_ctx {
	TEEC_Context ctx;
	TEEC_Session sess;
};

char data[4096] = "select * from stu";

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

TEEC_Result cmd_test(struct tee_ctx *ctx, char *data, size_t data_len)
{
	TEEC_Operation op;
	uint32_t err_origin;
	TEEC_Result res;

	/* Clear the TEEC_Operation struct */
	memset(&op, 0, sizeof(op));
	/* Prepare the argument */
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT, TEEC_NONE,
					TEEC_NONE, TEEC_NONE);
	op.params[0].tmpref.size = data_len;
	op.params[0].tmpref.buffer = data;

	res = TEEC_InvokeCommand(&ctx->sess,
				 TA_FRANK_CMD_TEST,
				 &op, &err_origin);
	if (res != TEEC_SUCCESS)
		errx(1, "cmd_test failed with code 0x%x origin 0x%x",
			res, err_origin);

	return res;
}

TEEC_Result cmd_sqlite(struct tee_ctx *ctx, char *data, size_t data_len)
{
	TEEC_Operation op;
	uint32_t err_origin;
	TEEC_Result res;

	/* Clear the TEEC_Operation struct */
	memset(&op, 0, sizeof(op));
	/* Prepare the argument */
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT, TEEC_NONE,
					TEEC_NONE, TEEC_NONE);
	op.params[0].tmpref.size = data_len;
	op.params[0].tmpref.buffer = data;

	res = TEEC_InvokeCommand(&ctx->sess,
				 TA_FRANK_CMD_SQLite,
				 &op, &err_origin);
	if (res != TEEC_SUCCESS)
		errx(1, "cmd_sqlite failed with code 0x%x origin 0x%x",
			res, err_origin);

	return res;
}

int main(int argc, char **argv)
{
	struct tee_ctx ctx;

	if (argc != 2) {
		fprintf(stderr, "Usage: %s command\n", argv[0]);
		return(1);
	}

	prepare_tee_session(&ctx);

	if (memcmp(argv[1], "test", 4) == 0) {
		cmd_test(&ctx, data, strlen(data)+1);
	}
	else if (memcmp(argv[1], "sqlite", 6) == 0) {
		cmd_sqlite(&ctx, data, strlen(data)+1);
	}
	else {
		printf("nothing command!\n");
	}

	terminate_tee_seesion(&ctx);

	return 0;
}
