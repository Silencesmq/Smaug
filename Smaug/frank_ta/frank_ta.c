#include <tee_internal_api.h>
#include <tee_internal_api_extensions.h>

#include <frank_ta.h>
#include <string.h>
#include <tee_api.h>
#include <tee_api_types.h>
#include <utee_syscalls.h>

#define AES128_KEY_BIT_SIZE		128
#define AES128_KEY_BYTE_SIZE		(AES128_KEY_BIT_SIZE / 8)
#define AES256_KEY_BIT_SIZE		256
#define AES256_KEY_BYTE_SIZE		(AES256_KEY_BIT_SIZE / 8)
#define AES_BLOCK_SIZE		16

#define DATA_SIZE 256

struct aes_cipher {
	uint32_t algo;			/* AES flavour */
	uint32_t mode;			/* Encode or decode */
	uint32_t key_size;		/* AES key size in byte */
	TEE_OperationHandle op_handle;	/* AES ciphering operation */
	TEE_ObjectHandle key_handle;	/* transient object to load the key */
};

char Aes128Key[] = {0x2FU, 0x58U, 0x7FU, 0xF0U, 0x43U, 0x83U, 0x95U, 0x3CU,
                      0x1DU, 0x44U, 0x05U, 0x2BU, 0x61U, 0x49U, 0x17U, 0xF8U};

char Aes128Iv[] = {0x1DU, 0x44U, 0x05U, 0x2BU, 0x61U, 0x49U, 0x17U, 0xF8U,
                     0x58U, 0xE0U, 0x90U, 0x43U, 0x84U, 0xA1U, 0xC1U, 0x75U};

//定义base64编码表  
unsigned char *base64_table="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

//根据base64表，以字符找到对应的十进制数据  
int table[]={0,0,0,0,0,0,0,0,0,0,0,0,
    		 0,0,0,0,0,0,0,0,0,0,0,0,
    		 0,0,0,0,0,0,0,0,0,0,0,0,
    		 0,0,0,0,0,0,0,62,0,0,0,
    		 63,52,53,54,55,56,57,58,
    		 59,60,61,0,0,0,0,0,0,0,0,
    		 1,2,3,4,5,6,7,8,9,10,11,12,
    		 13,14,15,16,17,18,19,20,21,
    		 22,23,24,25,0,0,0,0,0,0,26,
    		 27,28,29,30,31,32,33,34,35,
    		 36,37,38,39,40,41,42,43,44,
    		 45,46,47,48,49,50,51
    	     };

char sql_proxy[4096*DATA_SIZE];  // 1M storage for sql_proxy
char data[4096*DATA_SIZE];  // 1M storage for data
char base64[DATA_SIZE];  // 256B storage for base64
char name[50];
char value[50];
char secret[50];
char plain[50];

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
	struct aes_cipher *sess;

	/*
	 * Allocate and init ciphering materials for the session.
	 * The address of the structure is used as session ID for
	 * the client.
	 */
	sess = TEE_Malloc(sizeof(*sess), 0);
	if (!sess)
		return TEE_ERROR_OUT_OF_MEMORY;

	sess->key_handle = TEE_HANDLE_NULL;
	sess->op_handle = TEE_HANDLE_NULL;

	*session = (void *)sess;
	DMSG("Session %p: newly allocated", *session);

	return TEE_SUCCESS;
}

void TA_CloseSessionEntryPoint(void *session)
{
	struct aes_cipher *sess;

	/* Get ciphering context from session ID */
	DMSG("Session %p: release session", session);
	sess = (struct aes_cipher *)session;

	/* Release the session resources */
	if (sess->key_handle != TEE_HANDLE_NULL)
		TEE_FreeTransientObject(sess->key_handle);
	if (sess->op_handle != TEE_HANDLE_NULL)
		TEE_FreeOperation(sess->op_handle);
	TEE_Free(sess);
}

/*
 * base64 加密
 * 输入二进制数据，以及数据长度
 * 返回加密后字符串长度
 */
int base64_encode(char *str, int str_len)  
{ 
    int len;  // 编码后字符串长度
    int i,j;

//计算经过base64编码后的字符串长度
    if(str_len % 3 == 0)  
        len=str_len/3*4;  
    else  
        len=(str_len/3+1)*4;  
    
    base64[len]='\0';
  
//以3个8位字符为一组进行编码  
    for(i=0,j=0;i<len-2;j+=3,i+=4)  
    {  
        base64[i] = base64_table[str[j]>>2]; //取出第一个字符的前6位并找出对应的结果字符  
        base64[i+1] = base64_table[(str[j]&0x3)<<4 | (str[j+1]>>4)]; //将第一个字符的后位与第二个字符的前4位进行组合并找到对应的结果字符  
        base64[i+2] = base64_table[(str[j+1]&0xf)<<2 | (str[j+2]>>6)]; //将第二个字符的后4位与第三个字符的前2位组合并找出对应的结果字符  
        base64[i+3] = base64_table[str[j+2]&0x3f]; //取出第三个字符的后6位并找出结果字符  
    }  
  
    switch(str_len % 3)  
    {  
        case 1:  
            base64[i-2]='=';  
            base64[i-1]='=';  
            break;  
        case 2:  
            base64[i-1]='=';  
            break;  
    }  
    return len;  
}  

/*
 * base64 解密
 * 输入base64字符串，以及字符串长度不包括'\0'
 * 返回解密后二进制数据长度
 */
uint32_t base64_decode(char *code, int len)  
{   
    int str_len;  
    int i,j;
   
//判断编码后的字符串后是否有=  
    if(strstr(code,"=="))  
        str_len = len/4*3-2;  
    else if(strstr(code,"="))  
        str_len=len/4*3-1;  
    else  
        str_len=len/4*3;  
   
    base64[str_len]='\0';  
  
//以4个字符为一位进行解码  
    for(i=0,j=0;i < len-2;j+=3,i+=4)  
    {  
        base64[j]=((unsigned char)table[code[i]])<<2 | (((unsigned char)table[code[i+1]])>>4); //取出第一个字符对应base64表的十进制数的前6位与第二个字符对应base64表的十进制数的后2位进行组合  
        base64[j+1]=(((unsigned char)table[code[i+1]])<<4) | (((unsigned char)table[code[i+2]])>>2); //取出第二个字符对应base64表的十进制数的后4位与第三个字符对应bas464表的十进制数的后4位进行组合  
        base64[j+2]=(((unsigned char)table[code[i+2]])<<6) | ((unsigned char)table[code[i+3]]); //取出第三个字符对应base64表的十进制数的后2位与第4个字符进行组合  
    }  
  
    return str_len;  
  
}  

#if 0
static TEE_Result aes(void *session, uint32_t mode, char *in, char *out, uint32_t sz)
{
	struct aes_cipher *sess;
	TEE_Attribute attr;
	TEE_Result res;

	/* Get ciphering context from session ID */
	DMSG("Session %p: get ciphering resources", session);
	sess = (struct aes_cipher *)session;

	sess->algo = TEE_ALG_AES_CBC_NOPAD; 
	sess->mode = mode;
	sess->key_size = AES128_KEY_BYTE_SIZE;  // AES256_KEY_BYTE_SIZE

	/* Free potential previous operation */
	if (sess->op_handle != TEE_HANDLE_NULL)
		TEE_FreeOperation(sess->op_handle);

	// 1. 分配AES操作需要的句柄 sess->op_handle
	/* Allocate operation: AES/CTR, mode and size from params */
	res = TEE_AllocateOperation(&sess->op_handle, sess->algo, sess->mode, sess->key_size * 8);
	if (res != TEE_SUCCESS) {
		EMSG("Failed to allocate operation");
		sess->op_handle = TEE_HANDLE_NULL;
		goto err;
	}

	/* Free potential previous transient object */
	if (sess->key_handle != TEE_HANDLE_NULL)
		TEE_FreeTransientObject(sess->key_handle);

	// 2. 分配一个未初始化的临时object空间用来存放密码对象 sess->key_handle
	/* Allocate transient object according to target key size */
	res = TEE_AllocateTransientObject(TEE_TYPE_AES, sess->key_size * 8, &sess->key_handle);
	if (res != TEE_SUCCESS) {
		EMSG("Failed to allocate transient object");
		sess->key_handle = TEE_HANDLE_NULL;
		goto err;
	}

	// 3. 使用 key 中的数据初始化属性 ID 为 TEE_ATTR_SECRET_VALUE 属性变量
	TEE_InitRefAttribute(&attr, TEE_ATTR_SECRET_VALUE, Aes128Key, sess->key_size);

	// 4. 将属性变量赋值到 object 中 sess->key_handle
	res = TEE_PopulateTransientObject(sess->key_handle, &attr, 1);
	if (res != TEE_SUCCESS) {
		EMSG("TEE_PopulateTransientObject failed, %x", res);
		goto err;
	}
    
	// 5. 将存放密钥的 object 中的相关内容 sess->key_handle 保存到操作句柄 sess->op_handle
	res = TEE_SetOperationKey(sess->op_handle, sess->key_handle);
	if (res != TEE_SUCCESS) {
		EMSG("TEE_SetOperationKey failed %x", res);
		goto err;
	}

	// 6. 使用初始化向量初始化对称加密操作
	TEE_CipherInit(sess->op_handle, Aes128Iv, AES_BLOCK_SIZE);

	// 7. 开始使用 AES 算法解密或者解密数据
	res = TEE_CipherDoFinal(sess->op_handle, in, sz, out, &sz);
	return res;

err:
	if (sess->op_handle != TEE_HANDLE_NULL)
		TEE_FreeOperation(sess->op_handle);
	sess->op_handle = TEE_HANDLE_NULL;

	if (sess->key_handle != TEE_HANDLE_NULL)
		TEE_FreeTransientObject(sess->key_handle);
	sess->key_handle = TEE_HANDLE_NULL;

	return res;
}
#endif

static TEE_Result aes_prepare(void *session, uint32_t algo, uint32_t mode, uint32_t key_size)
{
	struct aes_cipher *sess;
	TEE_Attribute attr;
	TEE_Result res;

	/* Get ciphering context from session ID */
	//DMSG("Session %p: get ciphering resources", session);
	sess = (struct aes_cipher *)session;

	sess->algo = algo;  // TEE_ALG_AES_CBC_NOPAD
	sess->mode = mode;
	sess->key_size = key_size;  // AES256_KEY_BYTE_SIZE

	/* Free potential previous operation */
	if (sess->op_handle != TEE_HANDLE_NULL)
		TEE_FreeOperation(sess->op_handle);

	// 1. 分配AES操作需要的句柄 sess->op_handle
	/* Allocate operation: AES/CTR, mode and size from params */
	res = TEE_AllocateOperation(&sess->op_handle, sess->algo, sess->mode, sess->key_size * 8);
	if (res != TEE_SUCCESS) {
		EMSG("Failed to allocate operation");
		sess->op_handle = TEE_HANDLE_NULL;
		goto err;
	}

	/* Free potential previous transient object */
	if (sess->key_handle != TEE_HANDLE_NULL)
		TEE_FreeTransientObject(sess->key_handle);

	// 2. 分配一个未初始化的临时object空间用来存放密码对象 sess->key_handle
	/* Allocate transient object according to target key size */
	res = TEE_AllocateTransientObject(TEE_TYPE_AES, sess->key_size * 8, &sess->key_handle);
	if (res != TEE_SUCCESS) {
		EMSG("Failed to allocate transient object");
		sess->key_handle = TEE_HANDLE_NULL;
		goto err;
	}

	// 3. 使用 key 中的数据初始化属性 ID 为 TEE_ATTR_SECRET_VALUE 属性变量
	TEE_InitRefAttribute(&attr, TEE_ATTR_SECRET_VALUE, Aes128Key, sess->key_size);

	// 4. 将属性变量赋值到 object 中 sess->key_handle
	res = TEE_PopulateTransientObject(sess->key_handle, &attr, 1);
	if (res != TEE_SUCCESS) {
		EMSG("TEE_PopulateTransientObject failed, %x", res);
		goto err;
	}
    
	// 5. 将存放密钥的 object 中的相关内容 sess->key_handle 保存到操作句柄 sess->op_handle
	res = TEE_SetOperationKey(sess->op_handle, sess->key_handle);
	if (res != TEE_SUCCESS) {
		EMSG("TEE_SetOperationKey failed %x", res);
		goto err;
	}

	return TEE_SUCCESS;

err:
	if (sess->op_handle != TEE_HANDLE_NULL)
		TEE_FreeOperation(sess->op_handle);
	sess->op_handle = TEE_HANDLE_NULL;

	if (sess->key_handle != TEE_HANDLE_NULL)
		TEE_FreeTransientObject(sess->key_handle);
	sess->key_handle = TEE_HANDLE_NULL;

	return res;
}


static TEE_Result aes(void *session, uint32_t algo, uint32_t mode, uint32_t key_size, char *in, char *out, uint32_t sz)
{
	struct aes_cipher *sess;	
	TEE_Result res;
	sess = (struct aes_cipher *)session;
	if (sess->op_handle == TEE_HANDLE_NULL) {
		res = aes_prepare(session, algo, mode, key_size);
		if (res != TEE_SUCCESS) {
			EMSG("aes_prepare failed %x", res);
			return res;
		}
	}

	// 6. 使用初始化向量初始化对称加密操作
	TEE_CipherInit(sess->op_handle, Aes128Iv, AES_BLOCK_SIZE);

	// 7. 开始使用 AES 算法解密或者解密数据
	// res = TEE_CipherUpdate(sess->op_handle, in, sz, out, &sz);
	res = TEE_CipherDoFinal(sess->op_handle, in, sz, out, &sz);
	if (res != TEE_SUCCESS) {
		EMSG("TEE_CipherDoFinal failed %x", res);
	}
	return res;
}

/*
 * sql 加密
 * 输入需要加密处理的 sql 语句
 * 跳过 Integer 列，对单引号的 TEXT 列进行加密
 */
static TEE_Result sql_aes(void *session, char *str)
{
	/* version 1
	char *p = str, *ch;
    char value[50];
    ch = strstr(p, "values");
    p = ch + strlen("values("); 
    while (*p) {
        ch = strchr(p, ',');
		if (ch == 0) ch = strchr(p, ')');
        memset(value, 0, 50);
        if (*p == '\'')  {
			strncpy(value, p+1, ch-p-2);
			aes(session, TEE_MODE_ENCRYPT, value, p+1, strlen(value));
		}
        p = ch + 2;
    }
	*/
	char *p = str, *q = sql_proxy, *ch;
	uint32_t len;
	
    ch = strstr(p, "values");
    memset(sql_proxy, 0, 4096*DATA_SIZE);
    strncpy(q, p, ch - p + strlen("values("));
    q = q + (ch - p + strlen("values("));
    p = ch + strlen("values("); 
    while (*p) {
        ch = strchr(p, ',');
        if (ch == 0) ch = strchr(p, ')');
        memset(value, 0, 50);
        if (*p == '\'') {
            strncpy(value, p+1, ch-p-2);
            if (strlen(value) % 16 != 0) {
                memset(value+strlen(value), '\6', 16 - (strlen(value) % 16));
                *q++ = '\'';
				aes(session, TEE_ALG_AES_CBC_NOPAD, TEE_MODE_ENCRYPT, AES128_KEY_BYTE_SIZE, value, secret, strlen(value));
				len = base64_encode(secret, strlen(value));
				strncpy(q, base64, len);
                q = q + len;
                *q++ = '\'';
                strncpy(q, ch, 2);
                q = q + 2;
            }
        }
        else {
            strncpy(q, p, ch + 2 - p);
            q = q + (ch + 2 - p);
        }
        p = ch + 2;
    }

	return TEE_SUCCESS;
}

/*
 * sql 返回结果解密
 * 输入需要解密处理的 sql 返回结果
 * 对需要解密的列执行解码解密操作
 */
static TEE_Result sql_decrypt(void *session, char *str)
{
	char *p = str, *q = sql_proxy, *ch, tmp;
	uint32_t str_len;

	memset(sql_proxy, 0, 4096*DATA_SIZE);
    while (*p) {
        ch = strchr(p, ':');
		memset(name, 0, 50);
		strncpy(name, p, ch-p-1);
		p = ch + 2;
		ch = strchr(p, '\n');
		memset(value, 0, 50);
		strncpy(value, p, ch-p);
		// "Time", "Longitude", "Latitude", "Altitude", "Direction", "Speed", "Hdop"
        if (strcmp(name, "I") == 0 || strcmp(name, "Status") == 0 || strcmp(name, "GMS") == 0 || strcmp(name, "GWk") == 0 ||
			strcmp(name, "NSats") == 0 || strcmp(name, "HDop") == 0 || strcmp(name, "Lat") == 0 || strcmp(name, "Lng") == 0 ||
			strcmp(name, "Alt") == 0 || strcmp(name, "Spd") == 0 || strcmp(name, "GCrs") == 0 || strcmp(name, "VZ") == 0 ||
			strcmp(name, "Yaw") == 0 || strcmp(name, "U") == 0) {
			str_len = base64_decode(value, strlen(value));
			memset(plain, 0, 50);
            aes(session, TEE_ALG_AES_CBC_NOPAD, TEE_MODE_DECRYPT, AES128_KEY_BYTE_SIZE, base64, plain, str_len);
			strncpy(q, name, strlen(name));
			q = q + strlen(name);
			*q++ = '=';
			strncpy(q, plain, strlen(plain));
			q = q + strlen(plain);
			*q++ = '\n';
        } else {
			strncpy(q, name, strlen(name));
			q = q + strlen(name);
			*q++ = '=';
			strncpy(q, value, strlen(value));
			q = q + strlen(value);
			*q++ = '\n';
		}
        p = ch + 1;
    }
	return TEE_SUCCESS;
}

/*
 * sql 加密
 * 输入需要加密处理的 sql where语句
 * 对 name=value 中的 value 进行加密
 */
static TEE_Result sql_where(void *session, char *str)
{
	char *p = str, *q = sql_proxy, *ch;
	uint32_t len;
	
	// SELECT * FROM gps WHERE Lat=x AND Lng=y AND Alt=z AND Spd=w AND VZ=v;
	memset(sql_proxy, 0, 4096*DATA_SIZE);
	p = strstr(p, "WHERE ");
	p = p + strlen("WHERE ");
	strncpy(q, str, p-str);
	q = q + (p-str);
    while (*p) {
        ch = strchr(p, '=');
		memset(name, 0, 50);
		strncpy(name, p, ch-p);
		strncpy(q, name, strlen(name));
		q = q + strlen(name);
		*q++ = '=';
		p = ch + 1;
		ch = strchr(p, ' ');
		if (ch == 0) {
			ch = strchr(p, ';');
			memset(value, 0, 50);
			strncpy(value, p, ch-p);
			if (strlen(value) % 16 != 0) 
            	memset(value+strlen(value), '\6', 16 - (strlen(value) % 16));
			aes(session, TEE_ALG_AES_CBC_NOPAD, TEE_MODE_ENCRYPT, AES128_KEY_BYTE_SIZE, value, secret, strlen(value));
			len = base64_encode(secret, strlen(value));
			*q++ = '\'';
			strncpy(q, base64, len);
            q = q + len;
			*q++ = '\'';
            *q++ = ';';
			p = ch + 1;
			continue;
		}
		memset(value, 0, 50);
		strncpy(value, p, ch-p);
		if (strlen(value) % 16 != 0) 
            memset(value+strlen(value), '\6', 16 - (strlen(value) % 16));
		aes(session, TEE_ALG_AES_CBC_NOPAD, TEE_MODE_ENCRYPT, AES128_KEY_BYTE_SIZE, value, secret, strlen(value));
		len = base64_encode(secret, strlen(value));
		*q++ = '\'';
		strncpy(q, base64, len);
        q = q + len;
		*q++ = '\'';
		strncpy(q, " AND ", strlen(" AND "));
        q = q + strlen(" AND ");
		p = ch + strlen("AND ");
	}
	return TEE_SUCCESS;
}

/*
 * sql 加密
 * 输入需要加密处理的 sql update语句
 * 对 name=value 中的 value 进行加密
 */
static TEE_Result sql_update(void *session, char *str)
{
	char *p = str, *q = sql_proxy, *ch;
	uint32_t len;
	
	// UPDATE gps_table SET U=true WHERE Lat=x AND Lng=y AND Alt=z AND Spd=w AND VZ=v;
	memset(sql_proxy, 0, 4096*DATA_SIZE);
	p = strstr(p, "SET U=");
	p = p + strlen("SET U=");
	strncpy(q, str, p-str);
	q = q + (p-str);
	ch = strchr(p, ' ');
	memset(value, 0, 50);
	strncpy(value, p, ch-p);
	if (strlen(value) % 16 != 0) 
        memset(value+strlen(value), '\6', 16 - (strlen(value) % 16));
	aes(session, TEE_ALG_AES_CBC_NOPAD, TEE_MODE_ENCRYPT, AES128_KEY_BYTE_SIZE, value, secret, strlen(value));
	len = base64_encode(secret, strlen(value));
	*q++ = '\'';
	strncpy(q, base64, len);
    q = q + len;
	*q++ = '\'';
	p = strstr(p, "WHERE ");
	p = p + strlen("WHERE ");
	strncpy(q, ch, p-ch);
	q = q + (p-ch);
    while (*p) {
        ch = strchr(p, '=');
		memset(name, 0, 50);
		strncpy(name, p, ch-p);
		strncpy(q, name, strlen(name));
		q = q + strlen(name);
		*q++ = '=';
		p = ch + 1;
		ch = strchr(p, ' ');
		if (ch == 0) {
			ch = strchr(p, ';');
			memset(value, 0, 50);
			strncpy(value, p, ch-p);
			if (strlen(value) % 16 != 0) 
            	memset(value+strlen(value), '\6', 16 - (strlen(value) % 16));
			aes(session, TEE_ALG_AES_CBC_NOPAD, TEE_MODE_ENCRYPT, AES128_KEY_BYTE_SIZE, value, secret, strlen(value));
			len = base64_encode(secret, strlen(value));
			*q++ = '\'';
			strncpy(q, base64, len);
            q = q + len;
			*q++ = '\'';
            *q++ = ';';
			p = ch + 1;
			continue;
		}
		memset(value, 0, 50);
		strncpy(value, p, ch-p);
		if (strlen(value) % 16 != 0) 
            memset(value+strlen(value), '\6', 16 - (strlen(value) % 16));
		aes(session, TEE_ALG_AES_CBC_NOPAD, TEE_MODE_ENCRYPT, AES128_KEY_BYTE_SIZE, value, secret, strlen(value));
		len = base64_encode(secret, strlen(value));
		*q++ = '\'';
		strncpy(q, base64, len);
        q = q + len;
		*q++ = '\'';
		strncpy(q, " AND ", strlen(" AND "));
        q = q + strlen(" AND ");
		p = ch + strlen("AND ");
	}
	return TEE_SUCCESS;
}

static TEE_Result sqlite_cmd_insert_handle(void *session, uint32_t param_types,
	TEE_Param params[4])
{
	const uint32_t exp_param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_OUTPUT,
						   TEE_PARAM_TYPE_NONE,
						   TEE_PARAM_TYPE_NONE,
						   TEE_PARAM_TYPE_NONE);
	TEE_Time startEncTime, startSqlTime, endSqlTime;
	char sql[] = "insert into gps values(106300, 49258051, '0', '6', '281830200', '2153', '10', '1.21', '-35.3632609', '149.1652298', '582.87', '0.024', '308.4817', '-1.629', '0', '1');";

	DMSG("has been called");

	if (param_types != exp_param_types)
		return TEE_ERROR_BAD_PARAMETERS;
    
	/* Get the request length & point of responding buffer */
	// data = params[0].memref.buffer;
	// data_len = params[0].memref.size;
	
	TEE_GetREETime(&startEncTime);
	sql_aes(session, sql);
	IMSG("SQL Reconstruction: %s", sql_proxy);
	TEE_GetREETime(&startSqlTime);
	IMSG("timeEnc: %d %d\n", 1000 * (startSqlTime.seconds-startEncTime.seconds), startSqlTime.millis-startEncTime.millis);

	IMSG("time utee_sqlite_execstart: %d %d\n", startSqlTime.seconds, startSqlTime.millis);
	memset(data, 0, DATA_SIZE);
	utee_sqlite_exec(sql_proxy, strlen(sql_proxy)+1, data, 4096*DATA_SIZE);
	TEE_GetREETime(&endSqlTime);
	IMSG("time utee_sqlite_execend: %d %d\n", endSqlTime.seconds, endSqlTime.millis);

	//IMSG("timeSqlProxy: %d %d\n", 1000 * (endSqlTime.seconds-startSqlTime.seconds), endSqlTime.millis-startSqlTime.millis);

	return TEE_SUCCESS;
}

static TEE_Result sqlite_cmd_select_handle(void *session, uint32_t param_types,
	TEE_Param params[4])
{
	const uint32_t exp_param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_OUTPUT,
						   TEE_PARAM_TYPE_NONE,
						   TEE_PARAM_TYPE_NONE,
						   TEE_PARAM_TYPE_NONE);
	TEE_Time endDecTime, startSqlTime, endSqlTime;
	char *ree_buf;
	char sql[] = "select * FROM gps limit 5;";

	DMSG("has been called");

	if (param_types != exp_param_types)
		return TEE_ERROR_BAD_PARAMETERS;
    
	/* Get the request length & point of responding buffer */
	ree_buf = params[0].memref.buffer;
	// ree_buf_sz = params[0].memref.size;
	
	TEE_GetREETime(&startSqlTime);
	IMSG("time utee_sqlite_execstart: %d %d\n", startSqlTime.seconds, startSqlTime.millis);
	memset(data, 0, DATA_SIZE);
	utee_sqlite_exec(sql, strlen(sql)+1, data, 4096*DATA_SIZE);
	//IMSG("data:\n%s", data);
	TEE_GetREETime(&endSqlTime);
	IMSG("time utee_sqlite_execend: %d %d\n", endSqlTime.seconds, endSqlTime.millis);

	sql_decrypt(session, data);
	TEE_GetREETime(&endDecTime);
	IMSG("timeDec: %d %d\n", 1000 * (endDecTime.seconds-endSqlTime.seconds), endDecTime.millis-endSqlTime.millis);



	//IMSG("select_res:\n%s", sql_proxy);
	memcpy(ree_buf, sql_proxy, strlen(sql_proxy));
	//memcpy(ree_buf, data, strlen(data));

	return TEE_SUCCESS;
}

static TEE_Result sqlite_cmd_where_handle(void *session, uint32_t param_types,
	TEE_Param params[4])
{
	const uint32_t exp_param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_OUTPUT,
						   TEE_PARAM_TYPE_NONE,
						   TEE_PARAM_TYPE_NONE,
						   TEE_PARAM_TYPE_NONE);
	TEE_Time startEncTime, endDecTime, startSqlTime, endSqlTime;
	char *ree_buf;
	char sql[] = "select * FROM gps WHERE Lat=-35.3632683 AND Lng=149.1652492 AND Alt=624.64 AND Spd=2.545 AND VZ=0.166;";

	DMSG("has been called");

	if (param_types != exp_param_types)
		return TEE_ERROR_BAD_PARAMETERS;
    
	/* Get the request length & point of responding buffer */
	ree_buf = params[0].memref.buffer;
	// ree_buf_sz = params[0].memref.size;
	
	TEE_GetREETime(&startEncTime);
	sql_where(session, sql);
	IMSG("SQL Reconstruction: %s", sql_proxy);
	TEE_GetREETime(&startSqlTime);
	IMSG("timeEnc: %d %d\n", 1000 * (startSqlTime.seconds-startEncTime.seconds), startSqlTime.millis-startEncTime.millis);

	IMSG("time utee_sqlite_execstart: %d %d\n", startSqlTime.seconds, startSqlTime.millis);
	memset(data, 0, DATA_SIZE);
	utee_sqlite_exec(sql_proxy, strlen(sql_proxy)+1, data, 4096*DATA_SIZE);
	TEE_GetREETime(&endSqlTime);
	IMSG("time utee_sqlite_execend: %d %d\n", endSqlTime.seconds, endSqlTime.millis);
	aes_prepare(session, TEE_ALG_AES_CBC_NOPAD, TEE_MODE_DECRYPT, AES128_KEY_BYTE_SIZE);
	//IMSG("SECRET Data :\n %s", data);
	sql_decrypt(session, data);
	//IMSG("After Decrypt :\n %s", sql_proxy);
	TEE_GetREETime(&endDecTime);
	IMSG("timeDec: %d %d\n", 1000 * (endDecTime.seconds-endSqlTime.seconds), endDecTime.millis-endSqlTime.millis);


	//IMSG("select_res:\n%s", sql_proxy);
	memcpy(ree_buf, sql_proxy, strlen(sql_proxy));
	//memcpy(ree_buf, data, strlen(data));

	return TEE_SUCCESS;
}

static TEE_Result sqlite_cmd_update_handle(void *session, uint32_t param_types,
	TEE_Param params[4])
{
	const uint32_t exp_param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_OUTPUT,
						   TEE_PARAM_TYPE_NONE,
						   TEE_PARAM_TYPE_NONE,
						   TEE_PARAM_TYPE_NONE);
	TEE_Time startEncTime, startSqlTime, endSqlTime;
	char sql[] = "update gps SET U=0 WHERE Lat=-35.3632683 AND Lng=149.1652492 AND Alt=624.64 AND Spd=2.545 AND VZ=0.166;";

	DMSG("has been called");

	if (param_types != exp_param_types)
		return TEE_ERROR_BAD_PARAMETERS;
    
	/* Get the request length & point of responding buffer */
	//data = params[0].memref.buffer;
	//data_len = params[0].memref.size;
	
	TEE_GetREETime(&startEncTime);
	sql_update(session, sql);
	//IMSG("sql: %s", sql_proxy);
	TEE_GetREETime(&startSqlTime);
	IMSG("timeEnc: %d %d\n", 1000 * (startSqlTime.seconds-startEncTime.seconds), startSqlTime.millis-startEncTime.millis);

	IMSG("time utee_sqlite_execstart: %d %d\n", startSqlTime.seconds, startSqlTime.millis);
	memset(data, 0, DATA_SIZE);
	utee_sqlite_exec(sql_proxy, strlen(sql_proxy)+1, data, 4096*DATA_SIZE);
	TEE_GetREETime(&endSqlTime);
	IMSG("time utee_sqlite_execend: %d %d\n", endSqlTime.seconds, endSqlTime.millis);

	//IMSG("timeSqlProxy: %d %d\n", 1000 * (endSqlTime.seconds-startSqlTime.seconds), endSqlTime.millis-startSqlTime.millis);

	return TEE_SUCCESS;
}

static TEE_Result sqlite_cmd_ope_handle(void *session, uint32_t param_types,
	TEE_Param params[4])
{
	const uint32_t exp_param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_OUTPUT,
						   TEE_PARAM_TYPE_NONE,
						   TEE_PARAM_TYPE_NONE,
						   TEE_PARAM_TYPE_NONE);
	unsigned int rand = 0;
	double *ree_buf;
	
	DMSG("has been called");
	if (param_types != exp_param_types)
		return TEE_ERROR_BAD_PARAMETERS;

	/* Get the request length & point of responding buffer */
	ree_buf = params[0].memref.buffer;
	// ree_buf_sz = params[0].memref.size;

	/*
	 * The TEE_GenerateRandom function is a part of TEE Internal Core API,
	 * which generates random data
	 *
	 * Parameters:
	 * @ randomBuffer : Reference to generated random data
	 * @ randomBufferLen : Byte length of requested random data
	 */
	TEE_GenerateRandom(&rand, sizeof(unsigned int));
	*ree_buf = ((double)rand) / (1LL << 32);  // generate random in [0, 1)
	
	return TEE_SUCCESS;	
}

TEE_Result TA_InvokeCommandEntryPoint(void *session,
				      uint32_t command,
				      uint32_t param_types,
				      TEE_Param params[4])
{
	switch (command) {
	case TA_FRANK_CMD_INSERT:
	    return sqlite_cmd_insert_handle(session, param_types, params);
	case TA_FRANK_CMD_SELECT:
	    return sqlite_cmd_select_handle(session, param_types, params);
	case TA_FRANK_CMD_WHERE:
	    return sqlite_cmd_where_handle(session, param_types, params);
	case TA_FRANK_CMD_UPDATE:
	    return sqlite_cmd_update_handle(session, param_types, params);
	case TA_FRANK_CMD_OPE:
		return sqlite_cmd_ope_handle(session, param_types, params);
	default:
		EMSG("Command ID 0x%x is not supported", command);
		return TEE_ERROR_NOT_SUPPORTED;
	}
}
