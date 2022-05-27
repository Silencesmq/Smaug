#ifndef _DBQUEUE
#define _DBQUEUE

#include "mhtdefs.h"

typedef struct _QNode
{
	union{
		uint32	m_level;
		uint32	m_length;
	};
	uchar m_is_supplementary_node;	/* temporarily marking whether the node is a supplementary node to build a complete MHT */
	uchar m_is_zero_node;	/* temporarily marking whether node's hash is hashed zero */
	uint32 m_RMSTL_page_no;	/* temporarily storing the page number of the right-most sub-tree leaf, used to craete binary search structure */
	PMHTNode m_MHTNode_ptr;
	struct _QNode *prev;
	struct _QNode *next;
} QNode, *PQNode;

/*
Making a queue header, which only points to the first element of the queue,
and holds the queue length.
Parameters: NULL
Return: a pointer to queue header node
 */
PQNode makeQHeader();

/*
Making a queue node from an MHT node.
Parameters: 
	pmhtnode: a pointer to an MHT node.
	level: the level of the MHT node.
Return: a pointer to a new created queue node.
 */
PQNode makeQNode(PMHTNode pmhtnode, uint16 level);

/*
Making a queue node from an MHT node with more parameters.
Parameters: 
	pmhtnode: a pointer to an MHT node.
	level: the level of the MHT node.
	ISN: whether the node is a supplementary node.
	IZN: whether the node contains a hashed zero.
	RMSTLPN: page no. of the leaf node of the right-most subtree.
Return: a pointer to a new created queue node.
 */
PQNode makeQNode2(PMHTNode pmhtnode, 
				  uint16 level,
				  uchar ISN,
				  uchar IZN,
				  int RMSTLPN);

/*
Making a combined queue node from a given node.
Parameters:
	node_ptr: the given node pointer.
Return:
	The new created node pointer.
 */
PQNode makeCombinedQNodeFromSingleNode(PQNode node_ptr);

/*
Making a combined queue node from two given nodes.
Parameters:
	node1_ptr: the first given node pointer.
	node2_ptr: the second given node pointer.
Return:
	The new created node pointer.
 */
PQNode makeCombinedQNode(PQNode node1_ptr, PQNode node2_ptr);

/*
Freeing a given queue node.
Parameters:
	node_ptr: a 2-d pointer to the given node.
Return:
	NULL.
*/
void deleteQNode(PQNode *node_ptr);


/*
Initializing a queue.
Parameters: 
	pQHeader: a 2-d pointer to queue header (out).
	pQ: a 2-d pointer to queue tail (out).
Return: the tail pointer of the queue.
 */
void initQueue(PQNode *pQHeader, PQNode *pQ);

/*
Parameters:
	pQHeader: a pointer to the queue header
	pQ: the tail pointer of the queue.
	pNode: the node being inserted.
Return: the new tail pointer of the queue
 */
PQNode enqueue(PQNode *pQHeader, PQNode *pQ, PQNode pNode);

/*
Parameters:
	pQHeader: a pointer to the queue header
	pQ: the tail pointer of the queue.
Return: the dequeued node pointer.
 */
PQNode dequeue(PQNode *pQHeader, PQNode *pQ);

/*
Returning the first node of the queue without dequeuing it.
Parameters:
	pQHeader: a pointer to queue header.
Return:
	The peeked node pointer.
 */
PQNode peekQueue(PQNode pQHeader);

/*
Freeing a queue.
Parameters:
	pQHeader: a pointer to the queue header.
	pQ: a pointer to the queue tail.
Return: NULL.
 */
void freeQueue(PQNode *pQHeader, PQNode *pQ);

/*
Freeing a queue.
Parameters:
	pQHeader: a pointer to the queue header.
Return: NULL.
 */
void freeQueue2(PQNode *pQHeader);

/*
Freeing a queue.
Parameters:
	pQHeader: a pointer to the queue tail.
Return: NULL.
 */
void freeQueue3(PQNode *pQ);

/*
Continously fetching the previous node of the current node (pNode) in the queue till the first node is encountered.
The first node is the next of the header node. 
Parameters: 
	pNode: the current node pointer.
Return: the previous node pointer, and null if the first node has been reached to.
 */
PQNode lookBackward(PQNode pNode);


void printQueue(PQNode pQHeader);


#endif