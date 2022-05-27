/*
 * Copyright (c) 2016, Linaro Limited
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <my_test_ta.h>
#include <tee_internal_api.h>
#include <tee_internal_api_extensions.h>
#include <tee_api_defines.h>
#include <trace.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define SHA256_DIGEST_LENGTH 32
#define rootFile "rootFile"
#define rootFileLen 8

/*
 * Called when the instance of the TA is created. This is the first call in
 * the TA.
 */
TEE_Result TA_CreateEntryPoint(void)
{
	DMSG("has been called");

	return TEE_SUCCESS;
}

/*
 * Called when the instance of the TA is destroyed if the TA has not
 * crashed or panicked. This is the last call in the TA.
 */
void TA_DestroyEntryPoint(void)
{
	DMSG("has been called");
}

/*
 * Called when a new session is opened to the TA. *sess_ctx can be updated
 * with a value to be able to identify this session in subsequent calls to the
 * TA. In this function you will normally do the global initialization for the
 * TA.
 */
TEE_Result TA_OpenSessionEntryPoint(uint32_t param_types,
		TEE_Param __maybe_unused params[4],
		void __maybe_unused **sess_ctx)
{
	uint32_t exp_param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_NONE,
						   TEE_PARAM_TYPE_NONE,
						   TEE_PARAM_TYPE_NONE,
						   TEE_PARAM_TYPE_NONE);

	DMSG("has been called");

	if (param_types != exp_param_types)
		return TEE_ERROR_BAD_PARAMETERS;

	/* Unused parameters */
	(void)&params;
	(void)&sess_ctx;

	/*
	 * The DMSG() macro is non-standard, TEE Internal API doesn't
	 * specify any means to logging from a TA.
	 */
	IMSG("Hello, TA Start!\n");

	/* If return value != TEE_SUCCESS the session will not be created. */
	return TEE_SUCCESS;
}

/*
 * Called when a session is closed, sess_ctx hold the value that was
 * assigned by TA_OpenSessionEntryPoint().
 */
void TA_CloseSessionEntryPoint(void __maybe_unused *sess_ctx)
{
	(void)&sess_ctx; /* Unused parameter */
	IMSG("Goodbye, TA END!\n");
}

//计算哈希值
static void g_CryptoTaHash_sha(unsigned char* input, uint32_t inLen, unsigned char* output, uint32_t* pOutLen)
{
    TEE_Result ret;
    TEE_OperationHandle l_OperationHandle;   
    //TEE_CRYPTO_ALGORITHM_ID l_AlgorithmId;

    //设置sha-256
    //l_AlgorithmId = TEE_ALG_SHA256;

    ret = TEE_AllocateOperation(&l_OperationHandle, TEE_ALG_SHA256, TEE_MODE_DIGEST, 0);
    if(ret != TEE_SUCCESS) 
    {
        printf("Allocate SHA operation handle fail\n");
        return;
    }

    TEE_DigestUpdate(l_OperationHandle, input, inLen);

    //计算哈希
    ret = TEE_DigestDoFinal(l_OperationHandle, NULL, 0, output, pOutLen);
    if(ret != TEE_SUCCESS)
    {
        printf("Do the final sha operation fail\n");
        return;
    }

    TEE_FreeOperation(l_OperationHandle);
}

//哈希值比较
static int compHash(const unsigned char* curData, const unsigned char* data){    
	for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {        
		if (*(curData + i) != *(data + i)){            
		return -1;        
		}    
	}    
	return 0;
}

//写入安全存储文件
static TEE_Result create_raw_object(unsigned char* data, unsigned int data_sz)
{

	TEE_ObjectHandle object;
	TEE_Result res;
	uint32_t obj_data_flag;

	/*
	 * Create object in secure storage and fill with data
	 */
	obj_data_flag = TEE_DATA_FLAG_ACCESS_READ |		/* we can later read the oject */
			TEE_DATA_FLAG_ACCESS_WRITE |		/* we can later write into the object */
			TEE_DATA_FLAG_ACCESS_WRITE_META |	/* we can later destroy or rename the object */
			TEE_DATA_FLAG_OVERWRITE;		/* destroy existing object of same ID */
					//obj_data_flag,

	res = TEE_CreatePersistentObject(TEE_STORAGE_PRIVATE,
					rootFile, rootFileLen,
					obj_data_flag,
					NULL,
					NULL, 0,		/* we may not fill it right now */
					&object);
	if (res != TEE_SUCCESS) {
		EMSG("TEE_CreatePersistentObject failed 0x%08x", res);
		return res;
	}

	//TEE_SeekObjectData()
	res = TEE_WriteObjectData(object, data, data_sz);
	if (res != TEE_SUCCESS) {
		EMSG("TEE_WriteObjectData failed 0x%08x", res);
		TEE_CloseAndDeletePersistentObject1(object);
	} else {
		TEE_CloseObject(object);
	}
	return res;
}


static TEE_Result get_hash(uint32_t param_types,
	TEE_Param params[4])
{
	uint32_t exp_param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INOUT,
						   TEE_PARAM_TYPE_MEMREF_INOUT,
						   TEE_PARAM_TYPE_NONE,
						   TEE_PARAM_TYPE_NONE);

	EMSG("get_hash has been called");

	if (param_types != exp_param_types)
		return TEE_ERROR_BAD_PARAMETERS;

	unsigned int data_sz = params[1].memref.size;
	unsigned char* data = TEE_Malloc(data_sz, 0);
	TEE_MemMove(data, params[1].memref.buffer, data_sz);
	printf("\ngethash:");
	for(int k=0; k<SHA256_DIGEST_LENGTH; k++) {
		printf("%02x", (int)data[k]);
	}
	printf("\n");

	//更新哈希值
	if(TEE_SUCCESS != create_raw_object(data, data_sz))
	{
		EMSG("update rootash  failed.");
	}
	//params[0].value.a = compHash(data, data);

	TEE_Free(data);
	return TEE_SUCCESS;
}

static TEE_Result verify_hash(uint32_t param_types,
	TEE_Param params[4])
{
	uint32_t exp_param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INOUT,
						   TEE_PARAM_TYPE_VALUE_INOUT,TEE_PARAM_TYPE_MEMREF_INOUT,TEE_PARAM_TYPE_NONE);

	DMSG("verify_hash has been called");
	unsigned int pOutLen = 32;

	if (param_types != exp_param_types)
		return TEE_ERROR_BAD_PARAMETERS;

	//读取保存的根哈希值
	TEE_ObjectHandle object;
	TEE_ObjectInfo object_info;
	TEE_Result res;
	uint32_t read_bytes;

	unsigned char* rootHash = TEE_Malloc(SHA256_DIGEST_LENGTH, 0);
	res = TEE_OpenPersistentObject(TEE_STORAGE_PRIVATE,
					rootFile, rootFileLen,
					TEE_DATA_FLAG_ACCESS_READ |
					TEE_DATA_FLAG_SHARE_READ,
					&object);
	if (res != TEE_SUCCESS) {
		EMSG("Failed to open persistent object, res=0x%08x", res);
		return res;
	}

	res = TEE_GetObjectInfo1(object, &object_info);
	if (res != TEE_SUCCESS) {
		EMSG("Failed to create persistent object, res=0x%08x", res);
		return res;
	}

	if (object_info.dataSize > SHA256_DIGEST_LENGTH) {
		res = TEE_ERROR_SHORT_BUFFER;
		return res;
	}

	res = TEE_ReadObjectData(object, rootHash, object_info.dataSize,
				 &read_bytes);
	if (res != TEE_SUCCESS || read_bytes != object_info.dataSize) {
		EMSG("TEE_ReadObjectData failed");
		return res;
	}
	/*printf("\nhash:");
	for(int k=0; k<SHA256_DIGEST_LENGTH; k++) {
		printf("%02x", (int)rootHash[k]);
	}*/

			//获取ca传递的数据
			unsigned char* rHash;
			int temp = 0;
			int* tempFlag;
			unsigned char dataHash[SHA256_DIGEST_LENGTH];
			unsigned char data[2*SHA256_DIGEST_LENGTH];

			rHash = TEE_Malloc(params[0].memref.size, 0);
			TEE_MemMove(rHash, params[0].memref.buffer, params[0].memref.size);
			temp = params[1].value.a;
			tempFlag = TEE_Malloc(params[2].memref.size, 0);
			TEE_MemMove(tempFlag, params[2].memref.buffer, params[2].memref.size);

			memset(dataHash, 0, sizeof(unsigned char) * SHA256_DIGEST_LENGTH);
			memcpy(dataHash, rHash+temp*SHA256_DIGEST_LENGTH, SHA256_DIGEST_LENGTH); 

			//验证验证路径
			for(int j=temp-1; j>0; j--)
			{
				memset(data, 0, 2*SHA256_DIGEST_LENGTH);

				if(tempFlag[j] == 1)
				{
					memcpy(data, rHash+(j*SHA256_DIGEST_LENGTH), SHA256_DIGEST_LENGTH);
					memcpy(data+SHA256_DIGEST_LENGTH, dataHash, SHA256_DIGEST_LENGTH);
				}
				else
				{
					memcpy(data, dataHash, SHA256_DIGEST_LENGTH);
					memcpy(data+SHA256_DIGEST_LENGTH, rHash+j*SHA256_DIGEST_LENGTH, SHA256_DIGEST_LENGTH);
				}

				memset(dataHash, 0, SHA256_DIGEST_LENGTH);
				g_CryptoTaHash_sha(data, 2*SHA256_DIGEST_LENGTH, dataHash, &pOutLen);
			}

	//比较重新计算得到的哈希值与原哈希值
	printf("\nhash:");
	for(int k=0; k<SHA256_DIGEST_LENGTH; k++) {
		printf("%02x", (int)rootHash[k]);
	}
	printf("\n calcu hash:");
	for(int k=0; k<SHA256_DIGEST_LENGTH; k++) {
		printf("%02x", (int)dataHash[k]);
	}
	params[1].value.a = compHash(dataHash, rootHash);

	DMSG("verify_hash has been finished");
	TEE_Free(rHash);
	TEE_Free(tempFlag);
	TEE_CloseObject(object);
	return TEE_SUCCESS;
}
/*
 * Called when a TA is invoked. sess_ctx hold that value that was
 * assigned by TA_OpenSessionEntryPoint(). The rest of the paramters
 * comes from normal world.
 */
TEE_Result TA_InvokeCommandEntryPoint(void __maybe_unused *sess_ctx,
			uint32_t cmd_id,
			uint32_t param_types, TEE_Param params[4])
{
	(void)&sess_ctx; /* Unused parameter */

	switch (cmd_id) {
	case TA_MY_TEST_CMD_GET_HASH:
		return get_hash(param_types, params);
	case TA_MY_TEST_CMD_VERIFY_HASH:
		return verify_hash(param_types, params);
	default:
		return TEE_ERROR_BAD_PARAMETERS;
	}
}
