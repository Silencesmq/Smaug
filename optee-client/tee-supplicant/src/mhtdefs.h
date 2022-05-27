#ifndef _MHTDEFS
#define _MHTDEFS

#include <defs.h>

typedef struct  _MHTNode
{
	uint32	m_pageNo;
	char	m_hash[32];
	/* a minus offset indicates the distance that the file pointer has to move back 
	from current node to the member.
	*/ 
	int 	m_lchildOffset;
	int  	m_rchildOffset;
	int 	m_parentOffset;
	int 	m_lchildPageNo;
	int 	m_rchildPageNo;
	int 	m_parentPageNo;
} MHTNode, *PMHTNode;

/*
Making an MHT node.
Parameters: 
	pageno: page number.
	d: data string.
Return: a pointer to an MHT node.
 */
PMHTNode makeMHTNode(int pageno, const char d[]);

/*
Making a MHT node with hashed zero.
Parameters: 
	pageno: page number.
Return: a pointer to an MHT node.
 */
PMHTNode makeZeroMHTNode(int pageno);

/*
Freeing a given MHT node.
Parameters:
	node_ptr: a 2-d pointer to the given node.
Return:
	NULL.
*/
void deleteMHTNode(PMHTNode *node_ptr);

/*
Generating a hash by page number with SHA256 algorithm
Parameter:
	page_no [IN]: page number.
	buf [OUT]: buffer holding output hash value.
	buf_len [IN]: the maximal size of given buffer (buf), which must be larger than 32 bytes.
Return:
	NULL.
*/
void generateHashByPageNo_SHA256(int page_no, char *buf, uint32 buf_len);

/*
Generating a hash by combining two given hashes with SHA256 algorithm
Parameter:
	str1 [IN]: the frist hash.
	str2 [IN]: the second hash.
	buf [OUT]: buffer holding output hash value.
	buf_len [IN]: the maximal size of given buffer (buf), which must be larger than 32 bytes.
Return:
	NULL.
*/
void generateCombinedHash_SHA256(uchar *hash1, uchar *hash2, uchar *buf, uint32 buf_len);

#endif
