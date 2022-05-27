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

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "sqlite3.h"
#include "mhtfile.h"
#include "defs.h"
#include <openssl/sha.h>
#include <unistd.h>
#include <time.h>
/* OP-TEE TEE client API (built by optee_client) */
#include <tee_client_api.h>

/* To the the UUID (found the the TA's h-file(s)) */
#include <my_test_ta.h>
FILE* fpM = NULL;
//char* dbName = "/data/aes_cbc_128.db";
char* dbName = "/mnt/host/aes_cbc_128_palin.db";

extern int getUpInPgno(unsigned int** pgnos);
extern int getSelectPgno(unsigned int** pgnos);
extern int getDiffSelectPgno(unsigned int** pgnos);
extern int getDiffUpdatePgno(unsigned int** pgnos);
extern int getInfo1(sqlite3* db, const char* dbName, unsigned int* pgnoNums, unsigned char* dataHash,const unsigned start, const unsigned end, const unsigned int l);

void updateHash(char* sql)
{
	//计算时间
	struct timeval startTime, endTime;
	double diff_t;
	gettimeofday(&startTime,NULL);

	//执行sqlite代码
	printf("SQL：%s\n", sql);
    char *errmsg;
    sqlite3 *db;

    if(SQLITE_OK != sqlite3_open(dbName,&db) )
    { 
        printf("sqlite3_open error\n");
        exit(1);
    }
	char **result;
	int nrow, ncolumn;

	if (sqlite3_get_table(db, sql, &result, &nrow, &ncolumn, &errmsg) != 0)
	{
	  	printf("error : %s\n", errmsg);
	  	sqlite3_free(errmsg);
	}
	//获取页码信息
	//去重页码
	unsigned int *updateDiff;
	int updateDiffNum = getDiffUpdatePgno(&updateDiff);
	printf("updateDiffNum: %d\n", updateDiffNum);

	//ta-ca交互
	TEEC_Result res;
	TEEC_Context ctx;
	TEEC_Session sess;
	TEEC_Operation op;
	TEEC_UUID uuid = TA_MY_TEST_UUID;
	uint32_t origin;

	res = TEEC_InitializeContext(NULL, &ctx);
	if (res != TEEC_SUCCESS)
		errx(1, "TEEC_InitializeContext failed with code 0x%x", res);

	res = TEEC_OpenSession(&ctx, &sess, &uuid,
			       TEEC_LOGIN_PUBLIC, NULL, NULL, &origin);
	if (res != TEEC_SUCCESS)
		errx(1, "TEEC_Opensession failed with code 0x%x origin 0x%x",
			res, origin);

		memset(&op, 0, sizeof(op));
		//传输数据类型 2.哈希值 
		op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INOUT,
						   			TEEC_MEMREF_TEMP_INOUT,
						   			TEEC_NONE, TEEC_NONE);
	//ta-ca传输变量
	//初始化
	unsigned int  pgnoNums;
	unsigned char dataHash[SHA256_DIGEST_LENGTH];
	unsigned char rootHash[SHA256_DIGEST_LENGTH];
	int block_offset = -1;
	for(int i=0; i<updateDiffNum; i++)
	{
		int tempPgno = updateDiff[i];
		if(getInfo1(db, dbName, &pgnoNums, dataHash,tempPgno, tempPgno, 1))
		{
			//printf("tempPgno:%d\n", tempPgno);
			block_offset = -1;
			block_offset = updateByPgno(tempPgno, dataHash, HASH_LEN, rootHash);
			//如果页面不存在，则执行插入操作
			if(block_offset == -2) {
				insertNewPage(tempPgno, dataHash, HASH_LEN, rootHash);
			}
			else if(block_offset <= 0){
				printf("Update failed.\n");
				exit(0);
			}
			/*printf("the return value:%d\n", block_offset);
			printf("\n");*/

			//传入TA
			memset(&op, 0, sizeof(op));
			//传输数据类型 2.哈希值 
			op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INOUT,
							   			TEEC_MEMREF_TEMP_INOUT,
							   			TEEC_NONE, TEEC_NONE);
			op.params[1].tmpref.buffer = rootHash;
			op.params[1].tmpref.size = sizeof(unsigned char) * SHA256_DIGEST_LENGTH;
			res = TEEC_InvokeCommand(&sess, TA_MY_TEST_CMD_GET_HASH, &op,&origin);
			if (res != TEEC_SUCCESS)
			{
				errx(1, "TEEC_InvokeCommand failed with code 0x%x origin 0x%x",res, origin);
			}		
		}
	}

	TEEC_CloseSession(&sess);
	TEEC_FinalizeContext(&ctx);

	gettimeofday(&endTime,NULL);
	diff_t = 1000000*(endTime.tv_sec-startTime.tv_sec)+endTime.tv_usec-startTime.tv_usec;
	printf("\ntime: %lf", diff_t/1000000);
	sqlite3_free_table(result);
	sqlite3_close(db);
}

void selectHash(char* sql){
	printf("SQL：%s\n", sql);
	//计算时间
	struct timeval startTime, endTime;
	double diff_t;
	gettimeofday(&startTime,NULL);

	//执行sqlite代码
    char *errmsg = NULL;
    sqlite3 *db;
    if(SQLITE_OK != sqlite3_open(dbName,&db) )
    { 
        printf("sqlite3_open error\n");
        exit(1);
    }

	char **result = NULL;
	int nrow, ncolumn, i, j;

	if (sqlite3_get_table(db, sql, &result, &nrow, &ncolumn, &errmsg) != 0)
	{
	  	printf("error : %s\n", errmsg);
	  	sqlite3_free(errmsg);
	}

	//获取页码信息
	//全部页码
	unsigned int *selectAll;
	int selectAllNum = getSelectPgno(&selectAll);
	printf("selectAllPgno: %d\n", selectAllNum);

	//去重页码
	unsigned int *selectDiff;
	int selectDiffNum = getDiffSelectPgno(&selectDiff); 
	printf("selectDiffPgno: %d\n", selectDiffNum);


	//ta-ca交互
	TEEC_Result res;
	TEEC_Context ctx;
	TEEC_Session sess;
	TEEC_Operation op;
	TEEC_UUID uuid = TA_MY_TEST_UUID;
	uint32_t origin;

	res = TEEC_InitializeContext(NULL, &ctx);
	if (res != TEEC_SUCCESS)
		errx(1, "TEEC_InitializeContext failed with code 0x%x", res);

	res = TEEC_OpenSession(&ctx, &sess, &uuid,
			       TEEC_LOGIN_PUBLIC, NULL, NULL, &origin);
	if (res != TEEC_SUCCESS)
		errx(1, "TEEC_Opensession failed with code 0x%x origin 0x%x",
			res, origin);
	
	memset(&op, 0, sizeof(op));
	//传输数据类型 1.哈希值 2.节点个数 3.节点类型
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INOUT,
						   			TEEC_VALUE_INOUT, TEEC_MEMREF_TEMP_INOUT, TEEC_NONE);
	//ta-ca传输变量
	//存放验证路径
	unsigned char* rHash;
	//存放左右孩子标记 1为左，2为右
	int* tempFlag;

	//对数据的完整性进行判断
	//存储损坏页码，以便后续进行对比
	unsigned int *destoryPgno;
	destoryPgno = (unsigned int *)malloc(sizeof(unsigned int) * selectDiffNum);
	memset(destoryPgno, 0, sizeof(unsigned int) * selectDiffNum);

	//初始化
	int index1 = 0;
	unsigned int  pgnoNums;
	unsigned char dataHash[SHA256_DIGEST_LENGTH];

	for(int i=0; i<selectDiffNum; i++)
	{
		int tempPgno = selectDiff[i];
		memset(dataHash, 0, sizeof(unsigned char) * SHA256_DIGEST_LENGTH);

		memset(&op, 0, sizeof(op));
		op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INOUT,
						   			TEEC_VALUE_INOUT, TEEC_MEMREF_TEMP_INOUT, TEEC_NONE);
		//获取当前页码哈希值
		if(getInfo1(db, dbName, &pgnoNums, dataHash,tempPgno, tempPgno, 1))
		{
		
			//获取验证路径		
			int index = getVerifyPathByPageNo(tempPgno, &rHash, &tempFlag);
			memcpy(rHash+index*SHA256_DIGEST_LENGTH, dataHash, SHA256_DIGEST_LENGTH);

			/*printf("\nccc:");
			for(int k=0; k<SHA256_DIGEST_LENGTH; k++) {
			 printf("%02x", (int)dataHash[k]);
			}*/

			//将数据传入TA
			//验证路径
			op.params[0].tmpref.buffer = rHash;
			op.params[0].tmpref.size = sizeof(unsigned char) * (index+1)*SHA256_DIGEST_LENGTH;
			//用计算的哈希值个数，与树高相同
			op.params[1].value.a = index;
			//节点类型标记
			op.params[2].tmpref.buffer = tempFlag;
			op.params[2].tmpref.size = sizeof(int) * index+1;
			res = TEEC_InvokeCommand(&sess, TA_MY_TEST_CMD_VERIFY_HASH, &op,
				 &origin);
			if (res != TEEC_SUCCESS)
				errx(1, "TEEC_InvokeCommand failed with code 0x%x origin 0x%x",
					res, origin);
			if(op.params[1].value.a == -1){
				destoryPgno[index1++] = tempPgno;
				printf("data has destoryed:%d\n",tempPgno);
			}
		}
	}
	
	TEEC_CloseSession(&sess);
	TEEC_FinalizeContext(&ctx);

	gettimeofday(&endTime,NULL);
	diff_t = 1000000*(endTime.tv_sec-startTime.tv_sec)+endTime.tv_usec-startTime.tv_usec;
	printf("\ntime: %lf", diff_t/1000000);
	
	//

	//输出结果

	sqlite3_free_table(result);
	sqlite3_close(db);
	free(errmsg);
}

void initFile()
{
	TEEC_Result res;
	TEEC_Context ctx;
	TEEC_Session sess;
	TEEC_Operation op;
	TEEC_UUID uuid = TA_MY_TEST_UUID;
	uint32_t origin;

	int fd = 0;
    PMHT_FILE_HEADER mhtfilehdr_ptr = NULL;
	unsigned char tmp_hash_buffer[SHA256_DIGEST_LENGTH] = {0};

	//创建MHTFILE
	buildMHTFileByDatabase(dbName);
	//读取根结点
	if( (fd = initOpenMHTFileWR(MHT_DEFAULT_FILE_NAME))  < 2){
		printf("Failed to open file %s\n", MHT_DEFAULT_FILE_NAME);
		exit(0);
	}
	if(!(mhtfilehdr_ptr = readMHTFileHeader(fd)))
    {
		debug_print("insertpageinmht", "Failed to read MHT file header");
		return -1;
	}
	fo_read_mht_file(fd, tmp_hash_buffer, SHA256_DIGEST_LENGTH, mhtfilehdr_ptr->m_rootNodeOffset+MHT_BLOCK_OFFSET_HASH, SEEK_SET);

	res = TEEC_InitializeContext(NULL, &ctx);
	if (res != TEEC_SUCCESS)
		errx(1, "TEEC_InitializeContext failed with code 0x%x", res);

	res = TEEC_OpenSession(&ctx, &sess, &uuid,
			       TEEC_LOGIN_PUBLIC, NULL, NULL, &origin);
	if (res != TEEC_SUCCESS)
		errx(1, "TEEC_Opensession failed with code 0x%x origin 0x%x",
			res, origin);

	memset(&op, 0, sizeof(op));
	//传输数据类型 2.哈希值 
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INOUT,
						   			TEEC_MEMREF_TEMP_INOUT,
						   			TEEC_NONE, TEEC_NONE);
	op.params[1].tmpref.buffer = tmp_hash_buffer;
	op.params[1].tmpref.size = sizeof(unsigned char) * SHA256_DIGEST_LENGTH;
	res = TEEC_InvokeCommand(&sess, TA_MY_TEST_CMD_GET_HASH, &op,&origin);
	if (res != TEEC_SUCCESS)
	{
		errx(1, "TEEC_InvokeCommand failed with code 0x%x origin 0x%x",res, origin);
	}	
	/*for(int k=0; k<SHA256_DIGEST_LENGTH; k++) {
		printf("%02x", (int)tmp_hash_buffer[k]);
	}
	printf("\n");*/
	TEEC_CloseSession(&sess);
	TEEC_FinalizeContext(&ctx);
	fo_close_mhtfile(fd);
}

int main(int argc, char* argv[])
{
	//如果第一次运行
	initFile();

	char *sql;
	//比较判断
    /*if(0 == strncmp(sql, "select"))
	{
		selectHash(sql);
	}
	else if(0 == strncmp(sql, "insert") || 0 == strncmp(sql, "update"))
	{
		updateHash(sql);
	}*/

	switch(atoi(argv[1]))
    {
            case 1:
				initFile();
				sql = "UPDATE gps SET Lng=150 WHERE Lng=149.1648080"; //2
				updateHash(sql);

        	break;
			case 2:
				sql = "UPDATE gps SET Lng=149.1648080 WHERE Lng=150"; //4
				updateHash(sql);
        	break;
            case 3:             
				sql = "UPDATE gps SET Lng=150 WHERE Lng=149.1652298";
				updateHash(sql);
                break;
			case 4:             
				sql = "UPDATE gps SET Lng=149.1652298 WHERE Lng=150";
				updateHash(sql);
                break;
			case 5:             
				sql = "UPDATE gps SET Lng=150 WHERE Lng=149.1654848";
				updateHash(sql);
                break;
			case 6:             
				sql = "UPDATE gps SET Lng=149.1654848 WHERE Lng=150";
				updateHash(sql);
                break;
			case 7:             
				sql = "UPDATE gps SET U=0 WHERE Lat=-35.3632683 AND Lng=149.1652492 AND Alt=624.64 AND Spd=2.545";
				updateHash(sql);
                break;
			case 8:             
				sql = "UPDATE gps SET U=1 WHERE Lat=-35.3632683 AND Lng=149.1652492 AND Alt=624.64 AND Spd=2.545";
				updateHash(sql);
                break;
			case 9:             
				sql = "UPDATE gps SET U=0 WHERE Lat=-35.3632683 AND Lng=149.1652492 AND Alt=624.64 AND Spd=2.545 AND VZ=0.166";
				updateHash(sql);
                break;
			case 10:             
				sql = "UPDATE gps SET U=1 WHERE Lat=-35.3632683 AND Lng=149.1652492 AND Alt=624.64 AND Spd=2.545 AND VZ=0.166";
				updateHash(sql);
                break;
			case 11:
				sql = "SELECT * FROM gps WHERE Lng=149.1648080"; //2 2
				selectHash(sql);
        		break;
			case 12:
				sql = "SELECT * FROM gps WHERE Lng=149.1654848";//318 257
				selectHash(sql);
        		break;
            case 13:
                sql = "SELECT * FROM gps WHERE Lng=149.1652298";//719 66
				selectHash(sql);
                break;
            case 14:
                sql = "SELECT * FROM gps WHERE 149.1652301";//106284 2518
				selectHash(sql);
                break;
            case 15:
				sql = "SELECT * FROM gps WHERE Lng=149.1652301";//186 27
				selectHash(sql);
                break;
			case 16:
				//将根结点信息写入安全存储文件(只有第一次时需要)
				initFile();
		break;
        }

	return 0;
}
