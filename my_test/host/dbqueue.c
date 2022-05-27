#include "defs.h"
#include "mhtdefs.h"
//#include "sha256.h"
#include "dbqueue.h"

PQNode makeQHeader() {
	PQNode node_ptr = NULL;
	node_ptr = (PQNode) malloc(sizeof(QNode));
	if(node_ptr == NULL)
		return NULL;
	node_ptr->m_length = 0;
	node_ptr->m_MHTNode_ptr = NULL;
	node_ptr->m_is_supplementary_node = (uchar) FALSE;
	node_ptr->m_is_zero_node = (uchar) FALSE;
	node_ptr->m_RMSTL_page_no = UNASSIGNED_PAGENO;
	node_ptr->prev = NULL;
	node_ptr->next = NULL;

	return node_ptr;
}

PQNode makeQNode(PMHTNode pmhtnode, uint16 level){
	PQNode node_ptr = NULL;
	if(pmhtnode == NULL || level < 0)
		return NULL;
	node_ptr = (PQNode) malloc(sizeof(QNode));
	if(node_ptr == NULL)
		return NULL;
	node_ptr->m_level = level;
	node_ptr->m_MHTNode_ptr = pmhtnode;
	node_ptr->m_is_supplementary_node = (uchar) FALSE;
	node_ptr->m_is_zero_node = (uchar) FALSE;
	node_ptr->m_RMSTL_page_no = UNASSIGNED_PAGENO;
	node_ptr->prev = NULL;
	node_ptr->next = NULL;

	return node_ptr;
}

PQNode makeQNode2(PMHTNode pmhtnode, 
				  uint16 level,
				  uchar ISN,
				  uchar IZN,
				  int RMSTLPN) {
	PQNode node_ptr = NULL;
	if(pmhtnode == NULL || level < 0)
		return NULL;
	node_ptr = (PQNode) malloc(sizeof(QNode));
	if(node_ptr == NULL)
		return NULL;
	node_ptr->m_level = level;
	node_ptr->m_MHTNode_ptr = pmhtnode;
	node_ptr->m_is_supplementary_node = (uchar) ISN;
	node_ptr->m_is_zero_node = (uchar) IZN;
	node_ptr->m_RMSTL_page_no = RMSTLPN;
	node_ptr->prev = NULL;
	node_ptr->next = NULL;

	return node_ptr;
}

PQNode makeCombinedQNodeFromSingleNode(PQNode node_ptr) {
	return makeCombinedQNode(node_ptr, 
		makeQNode(makeMHTNode(SINGLENODECMB_PAGENO, ZERO_STR), node_ptr->m_level + 1));
}

PQNode makeCombinedQNode(PQNode node1_ptr, PQNode node2_ptr) {
	PQNode new_qnode_ptr = NULL;
	PMHTNode new_mhtnode_ptr = NULL;
	char tmp_buf[HASH_LEN] = {0};
	uint32 tmp_level = 0;

	if(!node1_ptr || !node2_ptr){
		check_pointer(node1_ptr, "node1_ptr");
		check_pointer(node2_ptr, "node2_ptr");
		return NULL;
	}

	tmp_level = node1_ptr->m_level + 1;
	generateCombinedHash_SHA256(node1_ptr->m_MHTNode_ptr->m_hash,
		node2_ptr->m_MHTNode_ptr->m_hash,
		tmp_buf, 
		HASH_LEN);
	new_mhtnode_ptr = makeMHTNode(UNASSIGNED_PAGENO, tmp_buf);
	check_pointer(new_mhtnode_ptr, "new_mhtnode_ptr");
	new_qnode_ptr = makeQNode(new_mhtnode_ptr, tmp_level);
	check_pointer(new_qnode_ptr, "new_qnode_ptr");

	return new_qnode_ptr;
}

void deleteQNode(PQNode *node_ptr){
	if(*node_ptr){
		(*node_ptr)->m_MHTNode_ptr ? deleteMHTNode(&((*node_ptr)->m_MHTNode_ptr)) : nop();
		free(*node_ptr);
		*node_ptr = NULL;
	}

	return;
}

PQNode lookBackward(PQNode pNode){
	if(!pNode)
		return NULL;

	if(!pNode->prev) {	//header node
		return NULL;
	}

	return pNode->prev;
}

void initQueue(PQNode *pQHeader, PQNode *pQ){
	if(*pQHeader != NULL){
		free(*pQHeader);
		*pQHeader = NULL;
	}

	(*pQHeader) = makeQHeader();
	*pQ = *pQHeader;
	return;
}

PQNode enqueue(PQNode *pQHeader, PQNode *pQ, PQNode pNode){
	if(*pQHeader == NULL && *pQ == NULL && pNode == NULL)
		return NULL;
	(*pQ)->next = pNode;
	pNode->prev = *pQ;
	pNode->next = NULL;
	*pQ = pNode;
	(*pQHeader)->m_length++;

	return pNode;
}

PQNode dequeue(PQNode *pQHeader, PQNode *pQ){
	PQNode tmp_ptr = NULL;
	if(*pQ == *pQHeader){	// empty queue
		//printf("Empty queue.\n");
		return NULL;
	}
	tmp_ptr = (*pQHeader)->next;
	(*pQHeader)->next = tmp_ptr->next;
	if(tmp_ptr->next)	//otherwise, tmp_ptr == pQ.
		tmp_ptr->next->prev = *pQHeader;
	else
		*pQ = *pQHeader;

	(*pQHeader)->m_length > 0 ? (*pQHeader)->m_length-- : nop();

	return tmp_ptr;
}

PQNode peekQueue(PQNode pQHeader){
	if(pQHeader && pQHeader->next)
		return (PQNode)(pQHeader->next);
	return NULL;
}

void freeQueue(PQNode *pQHeader, PQNode *pQ) {
	PQNode tmp_ptr = NULL;
	if(!(*pQHeader))
		return;
	tmp_ptr = (*pQHeader)->next;
	if(!tmp_ptr){
		free(*pQHeader);
		*pQHeader = NULL;
		return;
	}
	while(tmp_ptr = dequeue(pQHeader, pQ)){
		tmp_ptr->m_MHTNode_ptr != NULL ? free(tmp_ptr->m_MHTNode_ptr) : nop();
		free(tmp_ptr);
		tmp_ptr = NULL;
	}
	free(*pQHeader);
	*pQHeader = NULL;
	*pQ = NULL;
	return;
}

void freeQueue2(PQNode *pQHeader){
	PQNode tmp_ptr = NULL;
	if(!(*pQHeader))
		return;
	tmp_ptr = (*pQHeader)->next;
	if(!tmp_ptr){
		free(*pQHeader);
		*pQHeader = NULL;
		return;
	}
	while(tmp_ptr = ((*pQHeader)->next)){
		(*pQHeader)->next = tmp_ptr->next;
		if(tmp_ptr->next) {
			tmp_ptr->next->prev = *pQHeader;
			tmp_ptr->m_MHTNode_ptr != NULL ? free(tmp_ptr->m_MHTNode_ptr) : nop();
			free(tmp_ptr);
		}
	}
	free(*pQHeader);
	*pQHeader = NULL;
	return;
}

void freeQueue3(PQNode *pQ) {
	PQNode tmp_ptr = NULL;
	if(!(*pQ))
		return;
	tmp_ptr = *pQ;
	while(tmp_ptr->prev){
		tmp_ptr = tmp_ptr->prev;
		if(tmp_ptr->prev == NULL)	//tmp_ptr == pQHeader
			break;
	}
	if(tmp_ptr == (*pQ))
	{	
		*pQ = NULL;
		return;
	}
	return freeQueue2(&tmp_ptr);
}

/********* Test & Debug ********/

void printQueue(PQNode pQHeader) {
	PQNode tmp_ptr = NULL;
	uint32 i = 1;

	if(!pQHeader){
		check_pointer(pQHeader, "printQueue: pQHeader");
		return;
	}

	tmp_ptr = pQHeader->next;
	while(tmp_ptr){
		printf("%d: PageNo-Level: %d-%d\n", i, tmp_ptr->m_MHTNode_ptr->m_pageNo, tmp_ptr->m_level);
		tmp_ptr = tmp_ptr->next;
		i++;
	}

	return;
}
