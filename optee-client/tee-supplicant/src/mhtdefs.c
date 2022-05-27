//#include "sha256.h"
#include <mhtdefs.h>

PMHTNode makeMHTNode(int pageno, const char d[]){
	PMHTNode node_ptr = NULL;
	if(d == NULL)
		return NULL;
	node_ptr = (PMHTNode) malloc(sizeof(MHTNode));
	if(node_ptr == NULL)
		return NULL;
	node_ptr->m_pageNo = pageno;
	memcpy(node_ptr->m_hash, d, HASH_LEN);	// HASH_LEN == SHA256_DIGEST_LENGTH == 32

	node_ptr->m_lchildPageNo = node_ptr->m_rchildPageNo = node_ptr->m_parentPageNo = UNASSIGNED_PAGENO;
	node_ptr->m_lchildOffset = node_ptr->m_rchildOffset = node_ptr->m_parentOffset = UNASSIGNED_OFFSET;
	//node_ptr->m_lchildOffset = node_ptr->m_lchildPageNo = UNASSIGNED_PAGENO;
	//node_ptr->m_rchildOffset = node_ptr->m_rchildPageNo = UNASSIGNED_PAGENO;
	//node_ptr->m_parentOffset = node_ptr->m_parentPageNo = UNASSIGNED_PAGENO;

	return node_ptr;
}

PMHTNode makeZeroMHTNode(int pageno){
	return makeMHTNode(pageno, g_zeroHash);
}

void deleteMHTNode(PMHTNode *node_ptr){
	if(*node_ptr){
		free(*node_ptr);
		*node_ptr = NULL;
	}

	return;
}

void generateHashByPageNo_SHA256(int page_no, char *buf, uint32 buf_len){
	char tmp_buf[32]={0};

	if(page_no < 0){
		printf("Page number must larger than 0.\n");
		return;
	}

	if(!buf || buf_len < SHA256_DIGEST_LENGTH){
		printf("buf cannot be NULL and buf_len must larger than 32 bytes.\n");
		return;
	}

	sprintf(tmp_buf, "%d", page_no);
	SHA256_CTX ctx;
	SHA256_Init(&ctx);
	SHA256_Update(&ctx, tmp_buf, strlen(tmp_buf));
	SHA256_Final(buf, &ctx);

	return;
}

void generateCombinedHash_SHA256(uchar *hash1, uchar *hash2, uchar *buf, uint32 buf_len){
	uchar tmp_buf[SHA256_DIGEST_LENGTH * 2] = {0};

	if(!hash1 || !hash2){
		printf("Parameters \"hash1\" and \"hash2\" cannot be NULL.\n");
		return;
	}

	if(!buf || buf_len < SHA256_DIGEST_LENGTH){
		printf("buf cannot be NULL and buf_len must larger than 32 bytes.\n");
		return;
	}

	memcpy(tmp_buf, hash1, SHA256_DIGEST_LENGTH);
	memcpy(tmp_buf + SHA256_DIGEST_LENGTH, hash2, SHA256_DIGEST_LENGTH);
	SHA256_CTX ctx;
	SHA256_Init(&ctx);
	SHA256_Update(&ctx, tmp_buf, SHA256_DIGEST_LENGTH*2);
	SHA256_Final(buf, &ctx);

	return;
}
