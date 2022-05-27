#include <mhtfile.h>
#define LENGTH 100
extern PQNode g_pQHeader;
extern PQNode g_pQ;
extern int g_mhtFileFD;
extern int getCount(sqlite3* db, const char* dbName);
extern int getInfo1(sqlite3* db, const char* dbName, unsigned int* pgnoNums, unsigned char* dataHash,const unsigned start, const unsigned end, const unsigned int l);


PMHT_FILE_HEADER makeMHTFileHeader(){
    PMHT_FILE_HEADER pmht_file_header = NULL;

    pmht_file_header = (PMHT_FILE_HEADER) malloc(sizeof(MHT_FILE_HEADER));

    if(!pmht_file_header){
        check_pointer(pmht_file_header, "pmht_file_header");
        debug_print("makeMHTFileHeader", "Creating MHT file header failed");
        return NULL;
    }

    memset(pmht_file_header->m_magicStr, 0, MHT_FILE_MAGIC_STRING_LEN);
    memcpy(pmht_file_header->m_magicStr, MHT_FILE_MAGIC_STRING, sizeof(MHT_FILE_MAGIC_STRING));
    pmht_file_header->m_rootNodeOffset = UNASSIGNED_OFFSET;
    pmht_file_header->m_firstSupplementaryLeafOffset = UNASSIGNED_OFFSET;
    memset(pmht_file_header->m_Reserved, RESERVED_CHAR, MHT_HEADER_RSVD_SIZE);

    return pmht_file_header;
}

void freeMHTFileHeader(PMHT_FILE_HEADER *pmht_file_header){
	(*pmht_file_header) != NULL ? free(*pmht_file_header) : nop();
	*pmht_file_header = NULL;
	return;
}

void initMHTBlock(PMHT_BLOCK *pmht_block){
	if(!*pmht_block){
		check_pointer(*pmht_block, "*pmht_block");
		debug_print("initMHTBlock", "*pmht_block cannot be NULL");
		return;
	}

	(*pmht_block)->m_pageNo = UNASSIGNED_PAGENO;
	(*pmht_block)->m_nodeLevel = 0;
	memset((*pmht_block)->m_hash, 0, HASH_LEN);
	(*pmht_block)->m_isSupplementaryNode = (uchar)FALSE;
	(*pmht_block)->m_isZeroNode = (uchar)FALSE;
	(*pmht_block)->m_lChildPageNo = UNASSIGNED_PAGENO;
	(*pmht_block)->m_lChildOffset = UNASSIGNED_OFFSET;
	(*pmht_block)->m_rChildPageNo = UNASSIGNED_PAGENO;
	(*pmht_block)->m_rChildOffset = UNASSIGNED_OFFSET;
	(*pmht_block)->m_parentPageNo = UNASSIGNED_PAGENO;
	(*pmht_block)->m_parentOffset = UNASSIGNED_OFFSET;
	memset((*pmht_block)->m_Reserved, 'R', MHT_BLOCK_RSVD_SIZE);

	return;
}

PMHT_BLOCK makeMHTBlock(){
	PMHT_BLOCK pmht_block = NULL;

	pmht_block = (PMHT_BLOCK) malloc(sizeof(MHT_BLOCK));
	if(!pmht_block)	// Failed to allocate memory
		return NULL;
	initMHTBlock(&pmht_block);

	return pmht_block;
}

void freeMHTBlock(PMHT_BLOCK *pmht_block){
	(*pmht_block) != NULL ? free(*pmht_block) : nop();
	*pmht_block = NULL;
	return;
}

int initOpenMHTFileWR(uchar *pathname){
	int ret = -1;

	if(!pathname){
		debug_print("initOpenMHTFile", "Null pathname");
		return ret;
	}

	// file descriptor 0, 1 and 2 refer to STDIN, STDOUT and STDERR
	if(g_mhtFileFdRd > 2) {
		fo_close_mhtfile(g_mhtFileFdRd);
	}

	// open MHT file in "READ & WRITE" mode
	g_mhtFileFdRd = fo_open_mhtfile(pathname);
	ret = g_mhtFileFdRd;

	return ret;
}

PMHT_FILE_HEADER readMHTFileHeader(int fd) {
	PMHT_FILE_HEADER mht_file_header_ptr = NULL;
	uchar *mht_file_header_buffer = NULL;
	uchar *tmp_ptr = NULL;

	if(fd < 3){
		debug_print("readMHTFileHeader", "Invalid file descriptor for reading");
		return NULL;
	}

	if(!(mht_file_header_buffer = (uchar*) malloc(MHT_HEADER_LEN))) {
		debug_print("readMHTFileHeader", "Failed to allocate memory for mht_file_header_buffer");
		return NULL;
	}
	memset(mht_file_header_buffer, 0, MHT_HEADER_LEN);

	fo_read_mht_file_header(fd, mht_file_header_buffer, MHT_HEADER_LEN);
	// Preparing to parse MHT file header buffer into MHT file header structure
	if(!(mht_file_header_ptr = (PMHT_FILE_HEADER) malloc(sizeof(MHT_FILE_HEADER)))) {
		debug_print("readMHTFileHeader", "Failed to allocate memory for mht_file_header_ptr");
		return NULL;
	}
	tmp_ptr = mht_file_header_buffer;
	memcpy(mht_file_header_ptr->m_magicStr, tmp_ptr, MHT_FILE_MAGIC_STRING_LEN);
	tmp_ptr += MHT_FILE_MAGIC_STRING_LEN;
	memcpy(&(mht_file_header_ptr->m_rootNodeOffset), tmp_ptr, sizeof(uint32));
	tmp_ptr += sizeof(uint32);
	memcpy(&(mht_file_header_ptr->m_firstSupplementaryLeafOffset), tmp_ptr, sizeof(uint32));
	tmp_ptr += sizeof(uint32);
	memcpy(mht_file_header_ptr->m_Reserved, tmp_ptr, MHT_HEADER_RSVD_SIZE);

	return mht_file_header_ptr;
}

int locateMHTBlockOffsetByPageNo(int fd, int page_no) {
	PMHT_BLOCK mhtblk_ptr = NULL;	// MHT block preserving the found page
	uchar *rootnode_buf = NULL;		// root node block buffer
	uchar *tmpblk_buf = NULL;	// temporarily storing MHT block buffer
	uchar *childblk_buf = NULL;
	PMHT_FILE_HEADER mhtfilehdr_ptr = NULL;
	bool	bPageBlockFound = FALSE;
	uint32	node_level = NODELEVEL_LEAF;
	uint32	node_page_no = UNASSIGNED_PAGENO;		// temporarily preserving MHT block's page number
	int ret_offset = -1;

	if(fd < 3){
		debug_print("locateMHTBlockOffsetByPageNo", "Invalid file descriptor");
		return -1;
	}

	if(page_no < 0) {
		debug_print("locateMHTBlockOffsetByPageNo", "Invalid page number");
		return -1;
	}

	if(!(mhtfilehdr_ptr = readMHTFileHeader(fd))){
		debug_print("locateMHTBlockOffsetByPageNo", "Failed to read MHT file header");
		return -1;
	}

	rootnode_buf = (uchar*) malloc(MHT_BLOCK_SIZE);
	childblk_buf = (uchar*) malloc(MHT_BLOCK_SIZE);
	memset(rootnode_buf, 0, MHT_BLOCK_SIZE);
	memset(childblk_buf, 0, MHT_BLOCK_SIZE);
	// after reading root node block, the file pointer is at the end of the file.
	fo_read_mht_block2(fd, rootnode_buf, MHT_BLOCK_SIZE, mhtfilehdr_ptr->m_rootNodeOffset, SEEK_SET);
	// reset the file pointer to the beginning of the root node block
	fo_locate_mht_pos(fd, -MHT_BLOCK_SIZE, SEEK_CUR);
	tmpblk_buf = rootnode_buf;
	// binary search algorithm
	//printf("serachby:\n");
	while((node_level = *((int*)(tmpblk_buf + MHT_BLOCK_OFFSET_LEVEL))) > NODELEVEL_LEAF) {
		node_page_no = *((int*)(tmpblk_buf + MHT_BLOCK_OFFSET_PAGENO));
		/*printf("search pageNo: %d\n", node_page_no);
	    for(int k=0; k<SHA256_DIGEST_LENGTH; k++) {
	    	printf("%02x", (int)(tmpblk_buf + MHT_BLOCK_OFFSET_HASH)[k]);
	    }*/

		if(page_no <= node_page_no){	// go to left child block
			fo_read_mht_block(fd, childblk_buf, MHT_BLOCK_SIZE, 
								*((int*)(tmpblk_buf + MHT_BLOCK_OFFSET_LCOS)), SEEK_CUR);
		}
		else{	// go to right child block
			fo_read_mht_block(fd, childblk_buf, MHT_BLOCK_SIZE, 
								*((int*)(tmpblk_buf + MHT_BLOCK_OFFSET_RCOS)), SEEK_CUR);
		}
		// reset the file pointer to the beginning of the current left child node block
		ret_offset = fo_locate_mht_pos(fd, -MHT_BLOCK_SIZE, SEEK_CUR);
		tmpblk_buf = childblk_buf;
		/*for(int k=0; k<SHA256_DIGEST_LENGTH; k++) {
			printf("%02x", (int)(tmpblk_buf + MHT_BLOCK_OFFSET_HASH)[k]);
		}
		printf("\n");*/
	} // while

	// Now, tmpblk_buf stores the final leaf node block
	if(page_no != *((int*)(tmpblk_buf + MHT_BLOCK_OFFSET_PAGENO))) {
		//printf("offset:%d\n", ret_offset);
		debug_print("locateMHTBlockOffsetByPageNo", "No page found");
		return -1;	// search failed
	}
	/*printf("search:\n");
	for(int k=0; k<SHA256_DIGEST_LENGTH; k++) {
		printf("%02x", (int)(tmpblk_buf + MHT_BLOCK_OFFSET_HASH)[k]);
	}printf("\n");*/
	// free memory
	free(rootnode_buf);
	free(childblk_buf);
	free(mhtfilehdr_ptr);
	//printf("offset:%d\n", ret_offset);
	return ret_offset;
}

PMHT_BLOCK searchPageByNo(int fd, int page_no) {
	PMHT_BLOCK mhtblk_ptr = NULL;
	uchar *block_buf = NULL;

	if(locateMHTBlockOffsetByPageNo(fd, page_no) <= 0){
		debug_print("searchPageByNo", "No page found");
		return NULL;
	}

	// Page block found and the file pointer is at the beginning of the block
	// read block buffer
	block_buf = (uchar*) malloc (MHT_BLOCK_SIZE);
	memset(block_buf, 0, MHT_BLOCK_SIZE);
	fo_read_mht_block2(g_mhtFileFdRd, block_buf, MHT_BLOCK_SIZE, 0, SEEK_CUR);

	// build a MHT block structure to store the page info.
	mhtblk_ptr = makeMHTBlock();
	unserialize_mht_block(block_buf, MHT_BLOCK_SIZE, &mhtblk_ptr);

	return mhtblk_ptr;
	
	/*PMHT_BLOCK mhtblk_ptr = NULL;	// MHT block preserving the found page
	uchar *rootnode_buf = NULL;		// root node block buffer
	uchar *tmpblk_buf = NULL;	// temporarily storing MHT block buffer
	uchar *childblk_buf = NULL;
	PMHT_FILE_HEADER mhtfilehdr_ptr = NULL;
	bool	bPageBlockFound = FALSE;
	uint32	node_level = NODELEVEL_LEAF;
	uint32	node_page_no = UNASSIGNED_PAGENO;		// temporarily preserving MHT block's page number

	if(g_mhtFileFdRd < 3){
		debug_print("searchPageByNo", "Invalid file descriptor");
		return NULL;
	}

	if(page_no < 0) {
		debug_print("searchPageByNo", "Invalid page number");
		return NULL;
	}

	if(!(mhtfilehdr_ptr = readMHTFileHeader())){
		debug_print("searchPageByNo", "Failed to read MHT file header");
		return NULL;
	}

	rootnode_buf = (uchar*) malloc(MHT_BLOCK_SIZE);
	childblk_buf = (uchar*) malloc(MHT_BLOCK_SIZE);
	memset(rootnode_buf, 0, MHT_BLOCK_SIZE);
	memset(childblk_buf, 0, MHT_BLOCK_SIZE);
	// after reading root node block, the file pointer is at the end of the file.
	fo_read_mht_block2(g_mhtFileFdRd, rootnode_buf, MHT_BLOCK_SIZE, mhtfilehdr_ptr->m_rootNodeOffset, SEEK_SET);
	// reset the file pointer to the beginning of the root node block
	fo_locate_mht_pos(g_mhtFileFdRd, -MHT_BLOCK_SIZE, SEEK_CUR);
	tmpblk_buf = rootnode_buf;
	// binary search algorithm
	while((node_level = *((int*)(tmpblk_buf + MHT_BLOCK_OFFSET_LEVEL))) > NODELEVEL_LEAF) {
		node_page_no = *((int*)(tmpblk_buf + MHT_BLOCK_OFFSET_PAGENO));
		//printf("pageNo: %d\n", node_page_no);
		if(page_no <= node_page_no){	// go to left child block
			fo_read_mht_block(g_mhtFileFdRd, childblk_buf, MHT_BLOCK_SIZE, 
								*((int*)(tmpblk_buf + MHT_BLOCK_OFFSET_LCOS)), SEEK_CUR);
			// reset the file pointer to the beginning of the current left child node block
			fo_locate_mht_pos(g_mhtFileFdRd, -MHT_BLOCK_SIZE, SEEK_CUR);
			tmpblk_buf = childblk_buf;
		}
		else{	// go to right child block
			fo_read_mht_block(g_mhtFileFdRd, childblk_buf, MHT_BLOCK_SIZE, 
								*((int*)(tmpblk_buf + MHT_BLOCK_OFFSET_RCOS)), SEEK_CUR);
			// reset the file pointer to the beginning of the current left child node block
			fo_locate_mht_pos(g_mhtFileFdRd, -MHT_BLOCK_SIZE, SEEK_CUR);
			tmpblk_buf = childblk_buf;
		}
	} // while
	// final leaf node has been reached when loop ends
	if(page_no != *((int*)(tmpblk_buf + MHT_BLOCK_OFFSET_PAGENO))) {
		debug_print("searchPageByNo", "No page found");
		return NULL;
	}

	// build a MHT block structure to store the page info.
	mhtblk_ptr = makeMHTBlock();
	unserialize_mht_block(tmpblk_buf, MHT_BLOCK_SIZE, &mhtblk_ptr);
	// printf("Found page: %d\n", *((int*)(tmpblk_buf + MHT_BLOCK_OFFSET_PAGENO)));

	// free memory
	free(rootnode_buf);
	free(childblk_buf);
	free(mhtfilehdr_ptr);

	return mhtblk_ptr;
	*/
}

/*int updateMHTBlockHashByPageNo(int page_no, uchar *hash_val, uint32 hash_val_len) {
	uchar *block_buf = NULL;
	//需要更新的MHT_block的偏移量
	int update_blobk_offset = 0;

	int offset = -1;

	if(page_no < 0) {
		debug_print("updateMHTBlockHashByPageNo", "Invalid page number");
		return -1;
	}

	if(!hash_val || hash_val_len != HASH_LEN){
		debug_print("updateMHTBlockHashByPageNo", "Invalid hash_val or hash_val_len");
		return -1;
	}

	if((update_blobk_offset = locateMHTBlockOffsetByPageNo(page_no)) <= 0){
		debug_print("updateMHTBlockHashByPageNo", "No page found");
		return -2;
	}

	offset = update_blobk_offset;
	//更新指定页码的MHT_block块
	//读取MHT_block内容（使用绝对偏移量）
	block_buf = (uchar*) malloc(MHT_BLOCK_SIZE);
	memset(block_buf, 0, MHT_BLOCK_SIZE);
	fo_read_mht_block2(g_mhtFileFdRd, block_buf, MHT_BLOCK_SIZE, update_blobk_offset, SEEK_SET);

	//将更新后的哈希值写入文件中
	//1.替换旧哈希值
	memcpy(block_buf + MHT_BLOCK_OFFSET_HASH, hash_val, HASH_LEN);
	//2.将文件读写光标重新定位到更新块的开头
	fo_locate_mht_pos(g_mhtFileFdRd, update_blobk_offset, SEEK_SET);
	//3.写入文件
	fo_update_mht_block(g_mhtFileFdRd, block_buf, MHT_BLOCK_SIZE, 0, SEEK_CUR);

	//更新整条验证路径
	//记录节点对应偏移量
	int parent_offset = 0;
	//临时存放MHT节点信息
	uchar *temp_block_buf = NULL;
	//存放计算后的新哈希值
	uchar *new_hash = NULL;


	int temp_blobk_offset = 0;
	temp_block_buf = (uchar*) malloc(MHT_BLOCK_SIZE);
	new_hash = (uchar*) malloc(HASH_LEN);
	memset(new_hash, 0, HASH_LEN);

	printf("update_offset:%d\n", update_blobk_offset);
	while((block_buf+MHT_BLOCK_OFFSET_RSVD)[0] != 0x01 && (parent_offset = *((int*)(block_buf+MHT_BLOCK_OFFSET_POS))) !=0)
	{
		//1.读取父节点信息
		temp_blobk_offset = update_blobk_offset + parent_offset * MHT_BLOCK_SIZE;
		printf("temp_blobk_offset:%d\n", temp_blobk_offset);
		memset(temp_block_buf, 0, MHT_BLOCK_SIZE);
		fo_read_mht_block2(g_mhtFileFdRd, temp_block_buf, MHT_BLOCK_SIZE, temp_blobk_offset, SEEK_SET);

		//2.更新其哈希值并写入文件
		cal_parent_nodes_sha256(temp_block_buf, temp_blobk_offset);
		fo_update_mht_block2(g_mhtFileFdRd, temp_block_buf, MHT_BLOCK_SIZE, temp_blobk_offset, SEEK_SET);

		//3.更新MHT块相关信息
		memset(block_buf, 0, MHT_BLOCK_SIZE);
		memcpy(block_buf, temp_block_buf, MHT_BLOCK_SIZE);
		update_blobk_offset = temp_blobk_offset;
		//fo_read_mht_block2(g_mhtFileFdRd, block_buf, MHT_BLOCK_SIZE, update_blobk_offset, SEEK_SET);
		//printf("update_offset:%d\n", update_blobk_offset);
	}

	//将文件读写光标重新定位到更新页码块的开头
	offset = fo_locate_mht_pos(g_mhtFileFdRd, offset, SEEK_SET);
	free(block_buf);
	free(temp_block_buf);
	free(new_hash);
	return offset;
}*/
int updateMHTBlockHashByPageNo(int page_no, uchar *hash_val, uint32 hash_val_len, int fd) {
	uchar *block_buf = NULL;
	//需要更新的MHT_block的偏移量
	//The offset of the MHT_block that needs to be updated
	int update_blobk_offset = 0;

	int update_res = -1;

	if(page_no < 0) {
		debug_print("updateMHTBlockHashByPageNo", "Invalid page number");
		return -1;
	}

	if(!hash_val || hash_val_len != HASH_LEN){
		debug_print("updateMHTBlockHashByPageNo", "Invalid hash_val or hash_val_len");
		return -1;
	}

	if((update_blobk_offset = locateMHTBlockOffsetByPageNo(fd, page_no)) <= 0){
		debug_print("updateMHTBlockHashByPageNo", "No page found");
		return -2;
	}

	//更新指定页码的MHT_block块
	//Update the MHT_block block of the specified page number
	//读取MHT_block内容（使用绝对偏移量）
	//Read MHT_block content (using absolute offset)
	block_buf = (uchar*) malloc(MHT_BLOCK_SIZE);
	memset(block_buf, 0, MHT_BLOCK_SIZE);
	fo_read_mht_block2(fd, block_buf, MHT_BLOCK_SIZE, update_blobk_offset, SEEK_SET);

	//将更新后的哈希值写入文件中
	//Write the updated hash value to the file
	//1.替换旧哈希值
	//1. Replace the old hash value
	memcpy(block_buf + MHT_BLOCK_OFFSET_HASH, hash_val, HASH_LEN);
	//2.将文件读写光标重新定位到更新块的开头
	//2. Reposition the file read and write cursor to the beginning of the update block
	fo_locate_mht_pos(fd, update_blobk_offset, SEEK_SET);
	//3.写入文件
	//3. Write to file
	fo_update_mht_block(fd, block_buf, MHT_BLOCK_SIZE, 0, SEEK_CUR);
	//4.更新验证路径
	//4. Update verification path
	update_res = updatePathToRoot(block_buf, update_blobk_offset, fd);
	//5.将文件读写光标重新定位到更新块的开头
	//5. Reposition the file read and write cursor to the beginning of the update block
	update_blobk_offset = fo_locate_mht_pos(fd, update_blobk_offset, SEEK_SET);

	free(block_buf);
	return update_res == 0 ? update_blobk_offset : 0 ;
}

int updatePathToRoot(uchar *update_block_buf, int update_blobk_offset, int fd){
	//更新整条验证路径
	//Update the entire verification path
	//记录节点对应偏移量
	//Record the corresponding offset of the node
	int parent_offset = 0;
	//临时存放MHT节点信息
	// Temporarily store MHT node information
	uchar *temp_block_buf = NULL;
	//存放计算后的新哈希值
	//Store the new hash value after calculation
	uchar *new_hash = NULL;

	if(!update_block_buf || update_blobk_offset <= 0){
		debug_print("updatePathToRoot", "Invalid update_block_buf or update_blobk_offset");
		return -1;
	}

	int temp_blobk_offset = 0;
	temp_block_buf = (uchar*) malloc(MHT_BLOCK_SIZE);
	new_hash = (uchar*) malloc(HASH_LEN);
	memset(new_hash, 0, HASH_LEN);

	temp_blobk_offset = update_blobk_offset;
	memcpy(temp_block_buf, update_block_buf, MHT_BLOCK_SIZE);
	//printf("updatePathToRoot：update_blobk_offset:%d\n", update_blobk_offset);
	while((temp_block_buf+MHT_BLOCK_OFFSET_RSVD)[0] != 0x01 && (parent_offset = *((int*)(temp_block_buf+MHT_BLOCK_OFFSET_POS))) !=0)
	{
		//1.读取父节点信息
		//1. Read parent node information
		temp_blobk_offset = temp_blobk_offset + parent_offset * MHT_BLOCK_SIZE;
		//printf("updatePathToRoot：temp_blobk_offset:%d\n", temp_blobk_offset);
		memset(temp_block_buf, 0, MHT_BLOCK_SIZE);
		fo_read_mht_block2(fd, temp_block_buf, MHT_BLOCK_SIZE, temp_blobk_offset, SEEK_SET);

		//2.更新其哈希值并写入文件
		//2. Update its hash value and write to the file
		cal_parent_nodes_sha256(fd, temp_block_buf, temp_blobk_offset);
		fo_update_mht_block2(fd, temp_block_buf, MHT_BLOCK_SIZE, temp_blobk_offset, SEEK_SET);
		fsync(fd);
	}
	free(temp_block_buf);
	free(new_hash);
	return 0;
}

/*----------  Helper Functions  ---------------*/

void deal_with_remaining_nodes_in_queue(PQNode *pQHeader, PQNode *pQ, int fd){
	PQNode qnode_ptr = NULL;
	PQNode current_qnode_ptr = NULL;
	PQNode bkwd_ptr = NULL;
	PQNode cbd_qnode_ptr = NULL;
	PQNode peeked_qnode_ptr = NULL;
	PQNode popped_qnode_ptr = NULL;
	PQNode lchild_ptr = NULL;
	PQNode rchild_ptr = NULL;
	PMHTNode mhtnode_ptr = NULL;
	uint32 qHeaderLevel = 0;
	bool bCombined = FALSE;
	bool bDequeueExec = FALSE;	// whether dequeue is executed (for printf control)
	bool bEnctrFirstSplymtLeaf = FALSE;		// whether firstly encountering the first supplementary leaf
	char tmp_hash_buffer[SHA256_DIGEST_LENGTH] = {0};
	uchar *mhtblk_buffer = NULL;

	// Both of these two pointer cannot be NULL.
	if(!*pQHeader || !*pQ){
		check_pointer(*pQHeader, "*pQHeader");
		check_pointer(*pQ, "*pQ");
		return;
	}

	// Queue cannot be empty.
	if(*pQHeader == *pQ){
		return debug_print("deal_with_remaining_nodes_in_queue", "queue cannot be empty");
	}

	/* When pQ->level > pHeader->next->m_level, loop ends.
	Note that the loop ending condition reveals that the final root node
	has been created. */

	// temporarily storing header level
	qHeaderLevel = (*pQHeader)->next->m_level;
	while((*pQ)->m_level <= qHeaderLevel){
		mhtnode_ptr = makeZeroMHTNode(UNASSIGNED_PAGENO);
		check_pointer(mhtnode_ptr, "mhtnode_ptr");
		qnode_ptr = makeQNode2(mhtnode_ptr, 
							   NODELEVEL_LEAF, 
							   (uchar) TRUE,
							   (uchar) TRUE, 
							   UNASSIGNED_PAGENO);
		check_pointer(qnode_ptr, "qnode_ptr");
		enqueue(pQHeader, pQ, qnode_ptr);

		current_qnode_ptr = *pQ;
		while((bkwd_ptr = lookBackward(current_qnode_ptr)) 
			&& bkwd_ptr != *pQHeader) {
			check_pointer(bkwd_ptr, "bkwd_ptr");
			if(bkwd_ptr->m_level > (*pQ)->m_level)
				break;
			if(bkwd_ptr->m_level == (*pQ)->m_level) {
				lchild_ptr = bkwd_ptr;
				rchild_ptr = *pQ;
				cbd_qnode_ptr = makeCombinedQNode(lchild_ptr, rchild_ptr);
				bCombined = TRUE;
				check_pointer(cbd_qnode_ptr, "cbd_qnode_ptr");
				enqueue(pQHeader, pQ, cbd_qnode_ptr);
				deal_with_nodes_offset(cbd_qnode_ptr, lchild_ptr, rchild_ptr);
				deal_with_interior_nodes_pageno(cbd_qnode_ptr, lchild_ptr, rchild_ptr);
			} //if
			current_qnode_ptr = current_qnode_ptr->prev;
			check_pointer(current_qnode_ptr, "current_qnode_ptr");
		} // while
		if(bCombined) {
			while((peeked_qnode_ptr = peekQueue(*pQHeader)) && 
				   peeked_qnode_ptr->m_level < cbd_qnode_ptr->m_level) {
				popped_qnode_ptr = dequeue(pQHeader, pQ);
				check_pointer(popped_qnode_ptr, "popped_qnode_ptr");
				// Building MHT blocks based on dequeued nodes, then writing to MHT file.
				mhtblk_buffer = (uchar*) malloc(MHT_BLOCK_SIZE);
				memset(mhtblk_buffer, 0, MHT_BLOCK_SIZE);
				qnode_to_mht_buffer(popped_qnode_ptr, &mhtblk_buffer, MHT_BLOCK_SIZE);
				if(fd > 0) {
					// record the first supplementary leaf node offset to g_mhtFirstSplymtLeafOffset
					if(!bEnctrFirstSplymtLeaf && 
						popped_qnode_ptr->m_MHTNode_ptr->m_pageNo == UNASSIGNED_PAGENO && 
						popped_qnode_ptr->m_level == 0) {
						g_mhtFirstSplymtLeafOffset = fo_locate_mht_pos(fd, 0, SEEK_CUR);
						bEnctrFirstSplymtLeaf = TRUE;
					}
					fo_update_mht_block(fd, mhtblk_buffer, MHT_BLOCK_SIZE, 0, SEEK_CUR);
				}
				free(mhtblk_buffer); mhtblk_buffer = NULL;

				//print_qnode_info(popped_qnode_ptr);
				deleteQNode(&popped_qnode_ptr);
				bDequeueExec = TRUE;
			} // while
			bDequeueExec ? printf("\n\n") : nop();
			bDequeueExec = FALSE;
			bCombined = FALSE;
		}// if
	} // while
}

uint32 compute_relative_distance_between_2_nodes(PQNode qnode1_ptr, 
												 PQNode qnode2_ptr,
												 uchar	pov) {
	uint32 ret_val = -1;
	uint32 counter = 0;
	PQNode tmp_node_ptr = NULL;

	if(!qnode1_ptr || !qnode2_ptr) {
		check_pointer(qnode1_ptr, "qnode1_ptr");
		check_pointer(qnode2_ptr, "qnode2_ptr");
		return ret_val;
	}

	tmp_node_ptr = qnode1_ptr;
	while(tmp_node_ptr != qnode2_ptr && tmp_node_ptr) {
		counter ++;
		tmp_node_ptr = tmp_node_ptr->next;
	}

	ret_val = pov == 0x01 ? counter : -counter;

	return ret_val;
}


void deal_with_nodes_offset(PQNode parent_ptr, PQNode lchild_ptr, PQNode rchild_ptr){
	int d1 = -1, d2 = -1;

	if(!parent_ptr || !lchild_ptr || !rchild_ptr){
		check_pointer(parent_ptr, "parent_ptr");
		check_pointer(lchild_ptr, "lchild_ptr");
		check_pointer(rchild_ptr, "rchild_ptr");
		return;
	}

	d1 = compute_relative_distance_between_2_nodes(lchild_ptr, parent_ptr, 0x02);
	d2 = compute_relative_distance_between_2_nodes(rchild_ptr, parent_ptr, 0x02);

	parent_ptr->m_MHTNode_ptr->m_lchildOffset = d1;
	parent_ptr->m_MHTNode_ptr->m_rchildOffset = d2;

	lchild_ptr->m_MHTNode_ptr->m_parentOffset = -d1;
	rchild_ptr->m_MHTNode_ptr->m_parentOffset = -d2;

	return;
}

void deal_with_interior_nodes_pageno(PQNode parent_ptr, PQNode lchild_ptr, PQNode rchild_ptr) {
	if(!parent_ptr || !lchild_ptr || !rchild_ptr){
		check_pointer(parent_ptr, "parent_ptr");
		check_pointer(lchild_ptr, "lchild_ptr");
		check_pointer(rchild_ptr, "rchild_ptr");
		return;
	}

	/* lchild_ptr and rchild_ptr are leaf nodes. */
	if(lchild_ptr->m_level <= 0 || rchild_ptr->m_level <= 0){
		parent_ptr->m_MHTNode_ptr->m_pageNo = lchild_ptr->m_MHTNode_ptr->m_pageNo;
		parent_ptr->m_RMSTL_page_no = rchild_ptr->m_MHTNode_ptr->m_pageNo;
	}
	else{	/* lchild_ptr and rchild_ptr are interior nodes. */
		parent_ptr->m_MHTNode_ptr->m_pageNo = lchild_ptr->m_RMSTL_page_no;
		parent_ptr->m_RMSTL_page_no = rchild_ptr->m_RMSTL_page_no;
	}
	parent_ptr->m_MHTNode_ptr->m_lchildPageNo = lchild_ptr->m_MHTNode_ptr->m_pageNo;
	parent_ptr->m_MHTNode_ptr->m_rchildPageNo = rchild_ptr->m_MHTNode_ptr->m_pageNo;
	lchild_ptr->m_MHTNode_ptr->m_parentPageNo = parent_ptr->m_MHTNode_ptr->m_pageNo;
	rchild_ptr->m_MHTNode_ptr->m_parentPageNo = parent_ptr->m_MHTNode_ptr->m_pageNo;

	return;
}

int serialize_mht_block(PMHT_BLOCK pmht_block, 
						uchar **block_buf, 
						uint32 block_buf_len) {
	int ret = 0;	// 0 refers to none data has been processed.
	char *p_buf = NULL;

	if(!pmht_block || !*block_buf || !block_buf || block_buf_len != MHT_BLOCK_SIZE) {
		check_pointer(pmht_block, "pmht_block");
		check_pointer(*block_buf, "*block_buf");
		check_pointer(block_buf, "block_buf");
		debug_print("serialize_mht_block", "error parameters");
		return ret;
	}
	
	p_buf = *block_buf;
	memset(p_buf, 0, MHT_BLOCK_SIZE);
	memcpy(p_buf, &(pmht_block->m_pageNo), sizeof(int));
	p_buf += sizeof(int);
	ret += sizeof(int);
	memcpy(p_buf, &(pmht_block->m_nodeLevel), sizeof(int));
	p_buf += sizeof(int);
	ret += sizeof(int);
	memcpy(p_buf, pmht_block->m_hash, HASH_LEN);
	p_buf += HASH_LEN;
	ret += HASH_LEN;
	*p_buf = pmht_block->m_isSupplementaryNode;
	p_buf += sizeof(char);
	ret += sizeof(char);
	*p_buf = pmht_block->m_isZeroNode;
	p_buf += sizeof(char);
	ret += sizeof(char);
	memcpy(p_buf, &(pmht_block->m_lChildPageNo), sizeof(int));
	p_buf += sizeof(int);
	ret += sizeof(int);
	memcpy(p_buf, &(pmht_block->m_lChildOffset), sizeof(int));
	p_buf += sizeof(int);
	ret += sizeof(int);
	memcpy(p_buf, &(pmht_block->m_rChildPageNo), sizeof(int));
	p_buf += sizeof(int);
	ret += sizeof(int);
	memcpy(p_buf, &(pmht_block->m_rChildOffset), sizeof(int));
	p_buf += sizeof(int);
	ret += sizeof(int);
	memcpy(p_buf, &(pmht_block->m_parentPageNo), sizeof(int));
	p_buf += sizeof(int);
	ret += sizeof(int);
	memcpy(p_buf, &(pmht_block->m_parentOffset), sizeof(int));
	p_buf += sizeof(int);
	ret += sizeof(int);
	memcpy(p_buf, pmht_block->m_Reserved, MHT_BLOCK_RSVD_SIZE);
	ret += MHT_BLOCK_RSVD_SIZE;

	return ret;
}

int serialize_mht_file_header(PMHT_FILE_HEADER pmht_file_header, 
                            uchar **header_buf,
                            uint32 header_buf_len){
	int ret = 0;	// 0 refers to none data has been processed.
	char *p_buf = NULL;

	if(!pmht_file_header || !*header_buf || !header_buf || header_buf_len != MHT_HEADER_LEN) {
		check_pointer(pmht_file_header, "pmht_file_header");
		check_pointer(*header_buf, "*header_buf");
		check_pointer(header_buf, "header_buf");
		debug_print("serialize_mht_file_header", "error parameters");
		return ret;
	}

	p_buf = *header_buf;
	memset(p_buf, 0, MHT_HEADER_LEN);
	memcpy(p_buf, pmht_file_header->m_magicStr, MHT_FILE_MAGIC_STRING_LEN);
	p_buf += MHT_FILE_MAGIC_STRING_LEN;
	ret += MHT_FILE_MAGIC_STRING_LEN;
	memcpy(p_buf, &(pmht_file_header->m_rootNodeOffset), sizeof(uint32));
	p_buf += sizeof(uint32);
	ret += sizeof(uint32);
	memcpy(p_buf, &(pmht_file_header->m_firstSupplementaryLeafOffset), sizeof(uint32));
	p_buf += sizeof(uint32);
	ret += sizeof(uint32);
	memcpy(p_buf, pmht_file_header->m_Reserved, MHT_HEADER_RSVD_SIZE);
	ret += MHT_HEADER_RSVD_SIZE;

	return ret;
}

int unserialize_mht_block(char *block_buf, 
						  uint32 block_buf_len, 
						  PMHT_BLOCK *pmht_block) {
	int ret = 0;	// 0 refers to none data has been processed.
	PMHT_BLOCK tmpblk_ptr = NULL;

	if(!pmht_block || !block_buf || block_buf_len != MHT_BLOCK_SIZE) {
		check_pointer(pmht_block, "pmht_block");
		check_pointer(block_buf, "block_buf");
		debug_print("unserialize_mht_block", "error parameters");
		return ret;
	}

	tmpblk_ptr = *pmht_block;
	tmpblk_ptr->m_pageNo = *((int*)(block_buf + MHT_BLOCK_OFFSET_PAGENO)); ret += sizeof(int);
	tmpblk_ptr->m_nodeLevel = *((int*)(block_buf + MHT_BLOCK_OFFSET_LEVEL)); ret += sizeof(int);
	memcpy(tmpblk_ptr->m_hash, block_buf + MHT_BLOCK_OFFSET_HASH, HASH_LEN); ret += HASH_LEN;
	tmpblk_ptr->m_isSupplementaryNode = *((char*)(block_buf + MHT_BLOCK_OFFSET_ISN)); ret += sizeof(char);
	tmpblk_ptr->m_isZeroNode = *((char*)(block_buf + MHT_BLOCK_OFFSET_IZN)); ret += sizeof(char);
	tmpblk_ptr->m_lChildPageNo = *((int*)(block_buf + MHT_BLOCK_OFFSET_LCPN)); ret += sizeof(int);
	tmpblk_ptr->m_lChildOffset = *((int*)(block_buf + MHT_BLOCK_OFFSET_LCOS)); ret += sizeof(int);
	tmpblk_ptr->m_rChildPageNo = *((int*)(block_buf + MHT_BLOCK_OFFSET_RCPN)); ret += sizeof(int);
	tmpblk_ptr->m_rChildOffset = *((int*)(block_buf + MHT_BLOCK_OFFSET_RCOS)); ret += sizeof(int);
	tmpblk_ptr->m_parentPageNo = *((int*)(block_buf + MHT_BLOCK_OFFSET_PPN)); ret += sizeof(int);
	tmpblk_ptr->m_parentOffset = *((int*)(block_buf + MHT_BLOCK_OFFSET_POS)); ret += sizeof(int);
	memcpy(tmpblk_ptr->m_Reserved, block_buf + MHT_BLOCK_OFFSET_RSVD, MHT_BLOCK_RSVD_SIZE); ret += MHT_BLOCK_RSVD_SIZE;

	return ret;

	/*
	pmht_block->m_pageNo = *((int*)p_buf);
	p_buf += sizeof(int);
	pmht_block->m_nodeLevel = *((int*)p_buf);
	p_buf += sizeof(int);
	memcpy(pmht_block->m_hash, p_buf, HASH_LEN);
	p_buf += HASH_LEN;
	*/
}

int qnode_to_mht_buffer(PQNode qnode_ptr, uchar **mht_block_buf, uint32 mht_block_buf_len) {
	int ret = 0;
	uchar *p_buf = NULL;

	if(!qnode_ptr || !(qnode_ptr->m_MHTNode_ptr) || !*mht_block_buf || !mht_block_buf || mht_block_buf_len != MHT_BLOCK_SIZE) {
		check_pointer(qnode_ptr, "qnode_ptr");
		check_pointer(*mht_block_buf, "*mht_block_buf");
		check_pointer(mht_block_buf, "mht_block_buf");
		debug_print("qnode_to_mht_buffer", "Error parameters");
		return ret;
	}

	memset(*mht_block_buf, 0, MHT_BLOCK_SIZE);
	p_buf = *mht_block_buf;
	memcpy(p_buf, &(qnode_ptr->m_MHTNode_ptr->m_pageNo), sizeof(int));
	p_buf += sizeof(int);
	ret += sizeof(int);
	memcpy(p_buf, &(qnode_ptr->m_level), sizeof(int));
	p_buf += sizeof(int);
	ret += sizeof(int);
	memcpy(p_buf, qnode_ptr->m_MHTNode_ptr->m_hash, HASH_LEN);
	p_buf += HASH_LEN;
	ret += HASH_LEN;
	*p_buf = qnode_ptr->m_is_supplementary_node;
	p_buf += sizeof(char);
	ret += sizeof(char);
	*p_buf = qnode_ptr->m_is_zero_node;
	p_buf += sizeof(char);
	ret += sizeof(char);
	memcpy(p_buf, &(qnode_ptr->m_MHTNode_ptr->m_lchildPageNo), sizeof(int));
	p_buf += sizeof(int);
	ret += sizeof(int);
	memcpy(p_buf, &(qnode_ptr->m_MHTNode_ptr->m_lchildOffset), sizeof(int));
	p_buf += sizeof(int);
	ret += sizeof(int);
	memcpy(p_buf, &(qnode_ptr->m_MHTNode_ptr->m_rchildPageNo), sizeof(int));
	p_buf += sizeof(int);
	ret += sizeof(int);
	memcpy(p_buf, &(qnode_ptr->m_MHTNode_ptr->m_rchildOffset), sizeof(int));
	p_buf += sizeof(int);
	ret += sizeof(int);
	memcpy(p_buf, &(qnode_ptr->m_MHTNode_ptr->m_parentPageNo), sizeof(int));
	p_buf += sizeof(int);
	ret += sizeof(int);
	memcpy(p_buf, &(qnode_ptr->m_MHTNode_ptr->m_parentOffset), sizeof(int));
	p_buf += sizeof(int);
	ret += sizeof(int);
	memset(p_buf, 'R', MHT_BLOCK_RSVD_SIZE);
	ret += MHT_BLOCK_RSVD_SIZE;

	return ret;
}

int convert_qnode_to_mht_block(PQNode qnode_ptr, PMHT_BLOCK *mhtblk_ptr) {
	if(!qnode_ptr || !(*mhtblk_ptr)){
		check_pointer(qnode_ptr, "qnode_ptr");
		check_pointer(*mhtblk_ptr, "*mhtblk_ptr");
		debug_print("convert_qnode_to_mht_block", "Null parameters");
		return -1;
	}

	(*mhtblk_ptr)->m_pageNo = qnode_ptr->m_MHTNode_ptr->m_pageNo;
	(*mhtblk_ptr)->m_nodeLevel = qnode_ptr->m_level;
	memcpy((*mhtblk_ptr)->m_hash, qnode_ptr->m_MHTNode_ptr->m_hash, HASH_LEN);
	(*mhtblk_ptr)->m_isSupplementaryNode = qnode_ptr->m_is_supplementary_node;
	(*mhtblk_ptr)->m_isZeroNode = qnode_ptr->m_is_zero_node;
	(*mhtblk_ptr)->m_lChildPageNo = qnode_ptr->m_MHTNode_ptr->m_lchildPageNo;
	(*mhtblk_ptr)->m_lChildOffset = qnode_ptr->m_MHTNode_ptr->m_lchildOffset;
	(*mhtblk_ptr)->m_rChildPageNo = qnode_ptr->m_MHTNode_ptr->m_rchildPageNo;
	(*mhtblk_ptr)->m_rChildOffset = qnode_ptr->m_MHTNode_ptr->m_rchildOffset;
	(*mhtblk_ptr)->m_parentPageNo = qnode_ptr->m_MHTNode_ptr->m_parentPageNo;
	(*mhtblk_ptr)->m_parentOffset = qnode_ptr->m_MHTNode_ptr->m_parentOffset;
	memset((*mhtblk_ptr)->m_Reserved, 'R', MHT_BLOCK_RSVD_SIZE);

	return 0;
}

void *get_section_addr_in_mht_block_buffer(uchar *mht_blk_buffer, uint32 mht_blk_buffer_len, uint32 offset) {
	if(!mht_blk_buffer || mht_blk_buffer_len != MHT_BLOCK_SIZE || !is_valid_offset_in_mht_block_buffer(offset)) {
		check_pointer(mht_blk_buffer, "mht_blk_buffer");
		debug_print("get_section_addr_in_mht_block_buffer", "Invalid parameters");
		return NULL;
	}
	
	return (void*)(mht_blk_buffer + offset);
}

bool is_valid_offset_in_mht_block_buffer(uint32 offset){
	int i = 0;
	for(i = 0; i < MHT_BLOCK_ATRRIB_NUM; i++){
		if(g_MhtAttribOffsetArray[i] == offset)
			return TRUE;
	}

	return FALSE;
}

int find_the_first_leaf_splymt_block_by_offset(int fd, int offset) {
	uchar node_type = 'R';
	int reserved_val = 'RRRR';
	uchar reserved_buffer[MHT_BLOCK_RSVD_SIZE] = {0};
	uchar *mht_buf_ptr = NULL;

	if(fd < 3) {
		debug_print("find_the_first_leaf_splymt_block_by_offset", "Invalid file descriptor fd");
		return -1;
	}

	if(offset < 0) {
		debug_print("find_the_first_leaf_splymt_block_by_offset", "offset must >= 0");
		return -1;
	}

	// force the file pointer to offset
	fo_locate_mht_pos(fd, offset, SEEK_SET);
	// offset must be the beginning of an MHT block
	fo_read_mht_file(fd, reserved_buffer, MHT_BLOCK_RSVD_SIZE, -MHT_BLOCK_RSVD_SIZE, SEEK_CUR);
	if(reserved_val != *((int*)reserved_buffer)) {
		debug_print("find_the_first_leaf_splymt_block_by_offset", "offset must be the beginning of an MHT block");
		return -1;
	}
	// now, the file pointer is still at the given offset
	// read MHT block
	mht_buf_ptr = (uchar*) malloc(MHT_BLOCK_SIZE);
	memset(mht_buf_ptr, 0, MHT_BLOCK_SIZE);
	fo_read_mht_block(fd, mht_buf_ptr, MHT_BLOCK_SIZE, 0, SEEK_CUR);
	// check whether reaching at the root block
	while((mht_buf_ptr + MHT_BLOCK_OFFSET_RSVD)[0] != 0x01) {
		// proper block is found
		if(*((int*)(mht_buf_ptr + MHT_BLOCK_OFFSET_LEVEL)) == NODELEVEL_LEAF && 
			*(mht_buf_ptr + MHT_BLOCK_OFFSET_ISN) == TRUE) {
				//printf("FFL:%hhu\n", *(mht_buf_ptr + MHT_BLOCK_OFFSET_ISN));
			return fo_locate_mht_pos(fd, 0, SEEK_CUR) - MHT_BLOCK_SIZE;
		}
		memset(mht_buf_ptr, 0, MHT_BLOCK_SIZE);
		fo_read_mht_block(fd, mht_buf_ptr, MHT_BLOCK_SIZE, 0, SEEK_CUR);
	}

	free(mht_buf_ptr);
	return -1;	// no proper block is found
}

void print_qnode_info(PQNode qnode_ptr){
	if(!qnode_ptr){
		check_pointer(qnode_ptr, "qnode_ptr");
		debug_print("//print_qnode_info", "Null parameters");
		return;
	}

	printf("PageNo|Level|LCPN|LCOS|RCPN|RCOS|PPN|POS: %d|%d|%d|%d|%d|%d|%d|%d\t", 
			qnode_ptr->m_MHTNode_ptr->m_pageNo, 
			qnode_ptr->m_level,
			qnode_ptr->m_MHTNode_ptr->m_lchildPageNo,
			qnode_ptr->m_MHTNode_ptr->m_lchildOffset,
			qnode_ptr->m_MHTNode_ptr->m_rchildPageNo,
			qnode_ptr->m_MHTNode_ptr->m_rchildOffset,
			qnode_ptr->m_MHTNode_ptr->m_parentPageNo,
			qnode_ptr->m_MHTNode_ptr->m_parentOffset);

	return;
}

void cal_parent_nodes_sha256(int fd, uchar *parent_block_buf, int offset)
{
	//存放左右孩子对应信息
	//Store left and right child node information
	int lchild_offset = 0;
	int rchild_offset = 0;
	uchar lhash[HASH_LEN] = {0};
	uchar rhash[HASH_LEN] = {0};
	//存放计算得到的新哈希值
	//Store the calculated new hash value
	uchar new_hash[HASH_LEN] = {0};

	//获取对应信息
	//Get corresponding information
	check_pointer((void*)parent_block_buf, "update_parent_block_buf");
	lchild_offset = *((int*)(parent_block_buf + MHT_BLOCK_OFFSET_LCOS));
	rchild_offset = *((int*)(parent_block_buf + MHT_BLOCK_OFFSET_RCOS));

	memset(lhash, 0, HASH_LEN);
	memset(rhash, 0, HASH_LEN);
	fo_read_mht_file(fd, lhash, HASH_LEN, lchild_offset*MHT_BLOCK_SIZE+MHT_BLOCK_OFFSET_HASH+offset, SEEK_SET);
	fo_read_mht_file(fd, rhash, HASH_LEN, rchild_offset*MHT_BLOCK_SIZE+MHT_BLOCK_OFFSET_HASH+offset, SEEK_SET);

	//计算新的哈希值并替换
	//Calculate the new hash value and replace
	memset(new_hash, 0, HASH_LEN);
	generateCombinedHash_SHA256(lhash, rhash, new_hash, HASH_LEN);
	memcpy(parent_block_buf + MHT_BLOCK_OFFSET_HASH, new_hash, HASH_LEN);
	//测试输出新得到的哈希值
	//Test and output the newly obtained hash value
	/*printf("parent:%d\t",offset);
	for(int k=0; k<SHA256_DIGEST_LENGTH; k++) {
		printf("%02x", (int)(parent_block_buf + MHT_BLOCK_OFFSET_HASH)[k]);
	}
	printf("\n");*/
	/*printf("parent:%d\t",offset);
	for(int k=0; k<SHA256_DIGEST_LENGTH; k++) {
		printf("%02x", (int)new_hash[k]);
	}
	printf("\n");*/
}

PQNode makeQNodebyMHTBlock(PMHT_BLOCK mhtblk_ptr, int RMSTLPN)
{
	//printf("test makeQNodebyMHTBlock begin\n");
	PQNode node_ptr = NULL;
	PMHTNode mhtnode_ptr = NULL;

	if(!mhtblk_ptr)
		return NULL;

	node_ptr = (PQNode) malloc(sizeof(QNode));
	if(node_ptr == NULL)
		return NULL;

	mhtnode_ptr = (PMHTNode)malloc(sizeof(MHTNode));
	if(mhtnode_ptr == NULL)
		return NULL;
	mhtnode_ptr->m_pageNo = mhtblk_ptr->m_pageNo;
	memcpy(mhtnode_ptr->m_hash, mhtblk_ptr->m_hash, HASH_LEN);
	mhtnode_ptr->m_lchildOffset = mhtblk_ptr->m_lChildOffset;
	mhtnode_ptr->m_rchildOffset = mhtblk_ptr->m_rChildOffset;
	mhtnode_ptr->m_parentOffset = mhtblk_ptr->m_parentOffset;
	mhtnode_ptr->m_lchildPageNo = mhtblk_ptr->m_lChildPageNo;
	mhtnode_ptr->m_rchildPageNo = mhtblk_ptr->m_rChildPageNo;
	mhtnode_ptr->m_parentPageNo = mhtblk_ptr->m_parentPageNo;


	node_ptr->m_level = mhtblk_ptr->m_nodeLevel;
	node_ptr->m_MHTNode_ptr = mhtnode_ptr;
	node_ptr->m_is_supplementary_node = mhtblk_ptr->m_isSupplementaryNode;
	node_ptr->m_is_zero_node = mhtblk_ptr->m_isZeroNode;
	node_ptr->m_RMSTL_page_no = RMSTLPN;
	node_ptr->prev = NULL;
	node_ptr->next = NULL;

	return node_ptr;
}

PQNode mht_buffer_to_qnode(uchar *mht_block_buf, int offset)
{
	//printf("test mht_buffer_to_qnode begin\n");
	PQNode qnode_ptr = NULL;
	int block_offset = 0;
	int rchild_offset = 0;
	//临时存放MHT节点信息
	// Temporarily store MHT node information
	uchar *temp_block_buf = NULL;
	uint32 most_right_pgno = UNASSIGNED_PAGENO;
	PMHT_BLOCK mhtblk_ptr = NULL;

	if(!mht_block_buf)
	{
		debug_print("mht_buffer_to_qnode", "Invalid mht_block_buf");
		return NULL;
	}
	//memcpy(mht_block_buf+MHT_BLOCK_OFFSET_RSVD,)
	memset(mht_block_buf+MHT_BLOCK_OFFSET_RSVD, 'R', MHT_BLOCK_RSVD_SIZE);
	//查找其最右子树的页码
	//Find the page number of its rightmost subtree
	block_offset = offset;
	temp_block_buf = (uchar*) malloc(MHT_BLOCK_SIZE);
	memset(temp_block_buf, 0, MHT_BLOCK_SIZE);
	memcpy(temp_block_buf, mht_block_buf, MHT_BLOCK_SIZE);
	while((rchild_offset = *((int*)(temp_block_buf + MHT_BLOCK_OFFSET_RCOS))) !=0)
	{
		//读取右孩子节点信息
		//Read the right child node information
		block_offset = block_offset + rchild_offset * MHT_BLOCK_SIZE;
		//printf("block_offset:%d\n", block_offset);
		memset(temp_block_buf, 0, MHT_BLOCK_SIZE);
		fo_read_mht_block2(g_mhtFileFdRd, temp_block_buf, MHT_BLOCK_SIZE, block_offset, SEEK_SET);
	}
	most_right_pgno = *((int*)(temp_block_buf + MHT_BLOCK_OFFSET_PAGENO));
	//printf("most_right_pgno:%d\n", most_right_pgno);
	
	//构造队列节点
	//Construct the queue node
	//1.恢复MHT节点
	//1. Restore the MHT node
	mhtblk_ptr = makeMHTBlock();
	unserialize_mht_block(mht_block_buf, MHT_BLOCK_SIZE, &mhtblk_ptr);
	//2.生成队列节点
	//2. Generate queue node
	qnode_ptr = makeQNodebyMHTBlock(mhtblk_ptr, most_right_pgno);
	if(!qnode_ptr)
	{
		return NULL;
	}
	return qnode_ptr;
}

void update_interior_nodes_pageno(uchar *mht_block_buf, int offset, int fd)
{
	uchar *pmht_buf_ptr = NULL;
	uchar *lmht_buf_ptr = NULL;
	uchar *rmht_buf_ptr = NULL;
	int parent_offset = UNASSIGNED_OFFSET;
	int lchild_offset = UNASSIGNED_OFFSET;
	int rchild_offset = UNASSIGNED_OFFSET;
	uint32 rmsl_pgno = UNASSIGNED_PAGENO;
	uint32 parent_pgno = UNASSIGNED_PAGENO;
	uint32 lchild_pgno = UNASSIGNED_PAGENO;
	uint32 rchild_pgno = UNASSIGNED_PAGENO;

	int temp_offset = UNASSIGNED_OFFSET;

	if(!mht_block_buf){
		debug_print("update_interior_nodes_pageno", "Invalid mht_block_buf");
		return ;
	}

	if(fd < 3)
    {
		debug_print("update_interior_nodes_pageno", "Invalid file descriptor");
		return ;
	}

	//孩子节点为叶节点
	//When the child node is a leaf node
	pmht_buf_ptr = (uchar*) malloc(MHT_BLOCK_SIZE);
	memset(pmht_buf_ptr, 0, MHT_BLOCK_SIZE);
	lmht_buf_ptr = (uchar*) malloc(MHT_BLOCK_SIZE);
	memset(lmht_buf_ptr, 0, MHT_BLOCK_SIZE);
	rmht_buf_ptr = (uchar*) malloc(MHT_BLOCK_SIZE);
	memset(rmht_buf_ptr, 0, MHT_BLOCK_SIZE);

	//读取对应页码信息进行修改
	//Read the corresponding page number information to modify
	parent_offset = (*((int*)(mht_block_buf + MHT_BLOCK_OFFSET_POS))) *  MHT_BLOCK_SIZE + offset;
	//printf("offset : %d\t", offset);
	//printf("parent_offset : %d\t", parent_offset);
	fo_read_mht_block2(fd, pmht_buf_ptr, MHT_BLOCK_SIZE, parent_offset, SEEK_SET);

	lchild_offset = (*((int*)(pmht_buf_ptr + MHT_BLOCK_OFFSET_LCOS))) * MHT_BLOCK_SIZE + parent_offset;
	//printf("lchild_offset: %d\t", lchild_offset);
	fo_read_mht_block2(fd, lmht_buf_ptr, MHT_BLOCK_SIZE, lchild_offset, SEEK_SET);
	lchild_pgno = *((int*)(lmht_buf_ptr + MHT_BLOCK_OFFSET_PAGENO)) ;
	//printf("lchild_pgno : %d\t", lchild_pgno);

	rchild_offset = (*((int*)(pmht_buf_ptr + MHT_BLOCK_OFFSET_RCOS))) * MHT_BLOCK_SIZE + parent_offset;
	//printf("rchild_offset: %d\t", rchild_offset);
	fo_read_mht_block2(fd, rmht_buf_ptr, MHT_BLOCK_SIZE, rchild_offset , SEEK_SET);
	rchild_pgno = *((int*)(rmht_buf_ptr + MHT_BLOCK_OFFSET_PAGENO));
	//printf("rchild_pgno : %d\t", rchild_pgno);

	parent_pgno = lchild_pgno;
	//printf("parent_pgno : %d\t", parent_pgno);
	rmsl_pgno = rchild_pgno;
	//printf("rmsl_pgno : %d\n", rmsl_pgno);

	memcpy(pmht_buf_ptr + MHT_BLOCK_OFFSET_PAGENO, &parent_pgno, sizeof(int));
	memcpy(pmht_buf_ptr + MHT_BLOCK_OFFSET_LCPN, &lchild_pgno, sizeof(int));
	memcpy(pmht_buf_ptr + MHT_BLOCK_OFFSET_RCPN, &rchild_pgno, sizeof(int));
	memcpy(lmht_buf_ptr + MHT_BLOCK_OFFSET_PPN, &parent_pgno, sizeof(int));
	memcpy(rmht_buf_ptr + MHT_BLOCK_OFFSET_PPN, &parent_pgno, sizeof(int));

	fo_update_mht_block2(fd, pmht_buf_ptr, MHT_BLOCK_SIZE, parent_offset , SEEK_SET);
	fo_update_mht_block2(fd, lmht_buf_ptr, MHT_BLOCK_SIZE, lchild_offset, SEEK_SET);
	fo_update_mht_block2(fd, rmht_buf_ptr, MHT_BLOCK_SIZE, rchild_offset, SEEK_SET);
	//沿路径更新其父节点的页码信息
	//Update the page number information of its parent node along the path
	while((pmht_buf_ptr +MHT_BLOCK_OFFSET_RSVD)[0] != 0x01 && (temp_offset= *((int*)(pmht_buf_ptr +MHT_BLOCK_OFFSET_POS))) !=0)
	{
		memset(pmht_buf_ptr, 0, MHT_BLOCK_SIZE);
		memset(lmht_buf_ptr, 0, MHT_BLOCK_SIZE);
		memset(rmht_buf_ptr, 0, MHT_BLOCK_SIZE);

		parent_offset = temp_offset * MHT_BLOCK_SIZE + parent_offset;
		fo_read_mht_block2(fd, pmht_buf_ptr, MHT_BLOCK_SIZE, parent_offset, SEEK_SET);

		lchild_offset = (*((int*)(pmht_buf_ptr + MHT_BLOCK_OFFSET_LCOS))) * MHT_BLOCK_SIZE + parent_offset;
		fo_read_mht_block2(fd, lmht_buf_ptr, MHT_BLOCK_SIZE, lchild_offset, SEEK_SET);
		lchild_pgno = *((int*)(lmht_buf_ptr + MHT_BLOCK_OFFSET_PAGENO));
		//查找对应RMSL即查找其最右子树的页码
		//Find the corresponding RMSL that is to find the page number of its rightmost subtree
		int block_offset = lchild_offset ;
		int temp_roffset = UNASSIGNED_OFFSET;
		uchar *temp_block_buf  = NULL;
		temp_block_buf = (uchar*) malloc(MHT_BLOCK_SIZE);
		memset(temp_block_buf, 0, MHT_BLOCK_SIZE);
		memcpy(temp_block_buf, lmht_buf_ptr, MHT_BLOCK_SIZE);
		while((temp_roffset = *((int*)(temp_block_buf + MHT_BLOCK_OFFSET_RCOS))) !=0)
		{
			//读取右孩子节点信息
			//Read the right child node information
			block_offset = block_offset + temp_roffset * MHT_BLOCK_SIZE;
			//printf("block_offset:%d\n", block_offset);
			memset(temp_block_buf, 0, MHT_BLOCK_SIZE);
			fo_read_mht_block2(fd, temp_block_buf, MHT_BLOCK_SIZE, block_offset, SEEK_SET);
		}
		rmsl_pgno = *((int*)(temp_block_buf + MHT_BLOCK_OFFSET_PAGENO));
		free(temp_block_buf );

		rchild_offset = (*((int*)(pmht_buf_ptr + MHT_BLOCK_OFFSET_RCOS))) * MHT_BLOCK_SIZE + parent_offset;
		fo_read_mht_block2(fd, rmht_buf_ptr, MHT_BLOCK_SIZE, rchild_offset , SEEK_SET);
		rchild_pgno = *((int*)(rmht_buf_ptr + MHT_BLOCK_OFFSET_PAGENO));
		//printf("rchild_pgno : %d\t", rchild_pgno);

		parent_pgno = rmsl_pgno;
		//printf("parent_pgno : %d\t", parent_pgno);
		//printf("rmsl_pgno : %d\n", rmsl_pgno);

		memcpy(pmht_buf_ptr + MHT_BLOCK_OFFSET_PAGENO, &parent_pgno, sizeof(int));
		memcpy(pmht_buf_ptr + MHT_BLOCK_OFFSET_LCPN, &lchild_pgno, sizeof(int));
		memcpy(pmht_buf_ptr + MHT_BLOCK_OFFSET_RCPN, &rchild_pgno, sizeof(int));
		memcpy(lmht_buf_ptr + MHT_BLOCK_OFFSET_PPN, &parent_pgno, sizeof(int));
		memcpy(rmht_buf_ptr + MHT_BLOCK_OFFSET_PPN, &parent_pgno, sizeof(int));

		fo_update_mht_block2(fd, pmht_buf_ptr, MHT_BLOCK_SIZE, parent_offset , SEEK_SET);
		fo_update_mht_block2(fd, lmht_buf_ptr, MHT_BLOCK_SIZE, lchild_offset, SEEK_SET);
		fo_update_mht_block2(fd, rmht_buf_ptr, MHT_BLOCK_SIZE, rchild_offset, SEEK_SET);
	}

	free(pmht_buf_ptr);
	free(lmht_buf_ptr);
	free(rmht_buf_ptr);

	return;
}

int extentTheMHT(int fd)
{
    //填充节点的偏移量
    //Fill the offset of the node
    int supplementaryNode_offset = 0;
    uchar *mhtblk_buffer = NULL;
    //文件头信息
    //File header information
    PMHT_FILE_HEADER mhtfilehdr_ptr = NULL;
    //存放根结点信息
    //Store root node information
    uchar *rootnode_buf = NULL;
    PQNode qnode_ptr = NULL;

    //查找当前MHT是否有可以填充的节点
    //Find out if there are nodes that can be filled
    //1通过MHT文件头信息获取填充节点的offset
    //1 Obtain the offset of the filling node through the MHT file header information
    if(fd < 3)
    {
        debug_print("extentTheMHT", "Invalid file descriptor");
        return -1;
    }

    if(!(mhtfilehdr_ptr = readMHTFileHeader(fd)))
    {
        debug_print("extentTheMHT", "Failed to read MHT file header");
        return -1;
    }
    supplementaryNode_offset = mhtfilehdr_ptr->m_firstSupplementaryLeafOffset;
    printf("supplementaryNode_offset :%d\n",supplementaryNode_offset );

    g_mhtFileRootNodeOffset = mhtfilehdr_ptr->m_rootNodeOffset;
    if(supplementaryNode_offset == 0)
    {
        PQNode popped_qnode_ptr = NULL;
        //构造补充节点写入文件
        //Construct supplementary node to write file
        //a.读取当前根结点信息
        //a. Read the current root node information
        rootnode_buf = (uchar*) malloc(MHT_BLOCK_SIZE);
        memset(rootnode_buf, 0, MHT_BLOCK_SIZE);
        printf("rootNodeOffset: %d\n", mhtfilehdr_ptr->m_rootNodeOffset);
        fo_read_mht_block2(fd, rootnode_buf, MHT_BLOCK_SIZE, mhtfilehdr_ptr->m_rootNodeOffset, SEEK_SET);

        //b.将根结点入队，进行二叉树补全
        //b. Put the root node into the team and perform binary tree completion
        initQueue(&g_pQHeader, &g_pQ);
        qnode_ptr = mht_buffer_to_qnode(rootnode_buf, mhtfilehdr_ptr->m_rootNodeOffset);
        if(!qnode_ptr)
        {
            return -1;
        }
        enqueue(&g_pQHeader, &g_pQ, qnode_ptr);
        //将光标定位到原来根结点的位置进行写入
        //Position the cursor to the position of the original root node for writing
        fo_locate_mht_pos(fd, mhtfilehdr_ptr->m_rootNodeOffset, SEEK_SET);
        deal_with_remaining_nodes_in_queue(&g_pQHeader, &g_pQ, fd);
        if(g_pQHeader->m_length > 1)
        {
            deal_with_remaining_nodes_in_queue(&g_pQHeader, &g_pQ, fd);
        }

        //将新生成的根结点写入文件
        //Write the newly generated root node into the file
        popped_qnode_ptr = dequeue(&g_pQHeader, &g_pQ);
        check_pointer(popped_qnode_ptr, "popped_qnode_ptr");
        //由队列节点创建根结点
        //Create the root node by the queue node
        mhtblk_buffer = (uchar*) malloc(MHT_BLOCK_SIZE);
        memset(mhtblk_buffer, 0, MHT_BLOCK_SIZE);
        qnode_to_mht_buffer(popped_qnode_ptr, &mhtblk_buffer, MHT_BLOCK_SIZE);
        //设置根结点的保留区域，可与其他节点区别
        //Set the reserved area of the root node, which can be distinguished from other nodes
        (mhtblk_buffer + MHT_BLOCK_OFFSET_RSVD)[0] = 0x01;
        g_mhtFileRootNodeOffset = fo_locate_mht_pos(fd, 0, SEEK_CUR);
        //printf("g_mhtFileRootNodeOffset:%d\n",g_mhtFileRootNodeOffset);
        fo_update_mht_block(fd, mhtblk_buffer, MHT_BLOCK_SIZE, 0, SEEK_CUR);
        free(mhtblk_buffer);
        mhtblk_buffer = NULL;
        //print_qnode_info(popped_qnode_ptr);
        deleteQNode(&popped_qnode_ptr);

        //更新补充节点偏移量
        //Update the supplementary node offset
        //supplementaryNode_offset = g_mhtFirstSplymtLeafOffset;
    }
    //更新补充节点偏移量
    //Update the supplementary node offset
    supplementaryNode_offset = find_the_first_leaf_splymt_block_by_offset(fd, mhtfilehdr_ptr->m_rootNodeOffset);

    //更新MHT头文件信息
    //update the MHT file header
    supplementaryNode_offset = find_the_first_leaf_splymt_block_by_offset(fd, supplementaryNode_offset);
    //printf("update  supplementaryNode_offset: %d\n", supplementaryNode_offset);
    uchar *mhthdr_buffer = NULL;
    mhthdr_buffer = (uchar*) malloc(MHT_HEADER_LEN);
    mhtfilehdr_ptr->m_rootNodeOffset = g_mhtFileRootNodeOffset;
    //printf("mhtfilehdr_ptr->m_rootNodeOffset: %d\n", mhtfilehdr_ptr->m_rootNodeOffset);
    mhtfilehdr_ptr->m_firstSupplementaryLeafOffset = supplementaryNode_offset == -1? 0 : supplementaryNode_offset;
    serialize_mht_file_header(mhtfilehdr_ptr, &mhthdr_buffer, MHT_HEADER_LEN);
    fo_update_mht_file_header(fd, mhthdr_buffer, MHT_HEADER_LEN);

    free(mhthdr_buffer);
    free(rootnode_buf);
    free(mhtblk_buffer);
    freeMHTFileHeader(&mhtfilehdr_ptr);
    return supplementaryNode_offset;
}
/*----------  File Operation Functions  ------------*/

int fo_create_mhtfile(const char *pathname){
	int file_descriptor = -1;
	int open_flags;
	mode_t file_perms;

	if(!pathname){
		check_pointer((char*)pathname, "pathname");
		debug_print("fo_create_mhtfile", "Null pathname");
		return file_descriptor;
	}

	open_flags = O_CREAT | O_WRONLY | O_TRUNC;
	file_perms = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP;

	file_descriptor = open(pathname, open_flags, file_perms);

	return file_descriptor;
}

int fo_open_mhtfile(const char *pathname){
	int file_descriptor = -1;
	int open_flags;
	mode_t file_perms;

	if(!pathname){
		check_pointer((char*)pathname, "pathname");
		debug_print("fo_create_mhtfile", "Null pathname");
		return file_descriptor;
	}

	open_flags = O_RDWR;
	file_perms = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP;

	file_descriptor = open(pathname, open_flags, file_perms);

	return file_descriptor;
}

ssize_t fo_read_mht_file_header(int fd, uchar *buffer, uint32 buffer_len){
	ssize_t bytes_read = -1;

	if(fd < 0){
		debug_print("fo_read_mht_file_header", "Invalid fd");
		return bytes_read;
	}

	if(!buffer || buffer_len != MHT_HEADER_LEN){
		check_pointer(buffer, "buffer");
		debug_print("fo_read_mht_file_header", "Error parameters");
		return bytes_read;
	}

	/* Moving file pointer to the beginning of the file */
	lseek(fd, 0, SEEK_SET);
	bytes_read = read(fd, buffer, buffer_len);

	return bytes_read;
}

ssize_t fo_update_mht_file_header(int fd, uchar *buffer, uint32 buffer_len){
	ssize_t bytes_write = -1;

	if(fd < 0){
		debug_print("fo_update_mht_file_header", "Invalid fd");
		return bytes_write;
	}

	if(!buffer || buffer_len != MHT_HEADER_LEN){
		check_pointer(buffer, "buffer");
		debug_print("fo_update_mht_file_header", "Error parameters");
		return bytes_write;
	}

	/* Moving file pointer to the beginning of the file */
	lseek(fd, 0, SEEK_SET);
	bytes_write = write(fd, buffer, buffer_len);

	return bytes_write;
}

ssize_t fo_read_mht_block(int fd, 
						  uchar *buffer, 
						  uint32 buffer_len, 
						  int rel_distance, 
						  int whence){
	ssize_t bytes_read = -1;

	if(fd < 0){
		debug_print("fo_read_mht_block", "Invalid fd");
		return bytes_read;
	}

	if(!buffer || buffer_len != MHT_BLOCK_SIZE){
		check_pointer(buffer, "buffer");
		debug_print("fo_read_mht_block", "Error parameters");
		return bytes_read;
	}

	/* Moving file pointer to the position whence + rel_distance * MHT_BLOCK_SIZE */
	fo_locate_mht_pos(fd, rel_distance * MHT_BLOCK_SIZE, whence);
	bytes_read = read(fd, buffer, buffer_len);

	return bytes_read;
}

ssize_t fo_read_mht_block2(int fd, 
                            uchar *buffer, 
                            uint32 buffer_len, 
                            int offset,         // number of bytes from whence
                            int whence){
	ssize_t bytes_read = -1;

	if(fd < 0){
		debug_print("fo_read_mht_block2", "Invalid fd");
		return bytes_read;
	}

	if(!buffer || buffer_len != MHT_BLOCK_SIZE){
		check_pointer(buffer, "buffer");
		debug_print("fo_read_mht_block2", "Error parameters");
		return bytes_read;
	}

	/* Moving file pointer to the position: whence + offset */
	fo_locate_mht_pos(fd, offset, whence);
	bytes_read = read(fd, buffer, buffer_len);

	return bytes_read;
}

ssize_t fo_read_mht_file(int fd, 
                            uchar *buffer, 
                            uint32 buffer_len, 
                            int offset,         // number of bytes from whence
                            int whence){
	ssize_t bytes_read = -1;

	if(fd < 0){
		debug_print("fo_read_mht_file", "Invalid fd");
		return bytes_read;
	}

	if(!buffer || buffer_len <= 0){
		check_pointer(buffer, "buffer");
		debug_print("fo_read_mht_file", "Error parameters");
		return bytes_read;
	}

	/* Moving file pointer to the position: whence + offset */
	fo_locate_mht_pos(fd, offset, whence);
	bytes_read = read(fd, buffer, buffer_len);

	return bytes_read;
}

ssize_t fo_update_mht_block(int fd, 
							uchar *buffer, 
							uint32 buffer_len, 
							int rel_distance, 
							int whence){
	ssize_t bytes_write = -1;

	if(fd < 0){
		debug_print("fo_update_mht_block", "Invalid fd");
		return bytes_write;
	}

	if(!buffer || buffer_len != MHT_BLOCK_SIZE){
		check_pointer(buffer, "buffer");
		debug_print("fo_update_mht_block", "Error parameters");
		return bytes_write;
	}

	fo_locate_mht_pos(fd, rel_distance * MHT_BLOCK_SIZE, whence);
	bytes_write = write(fd, buffer, buffer_len);

	return bytes_write;
}

ssize_t fo_update_mht_block2(int fd, 
                             uchar *buffer, 
                             uint32 buffer_len, 
                             int offset,         
                             int whence){
	ssize_t bytes_write = -1;

	if(fd < 0){
		debug_print("fo_update_mht_block", "Invalid fd");
		return bytes_write;
	}

	if(!buffer || buffer_len != MHT_BLOCK_SIZE){
		check_pointer(buffer, "buffer");
		debug_print("fo_update_mht_block2", "Error parameters");
		return bytes_write;
	}

	fo_locate_mht_pos(fd, offset, whence);
	bytes_write = write(fd, buffer, buffer_len);

	return bytes_write;
}



off_t fo_locate_mht_pos(int fd, off_t offset, int whence){
	if(fd < 0){
		debug_print("fo_update_mht_header_block", "Invalid fd");
		return -1;
	}

	return lseek(fd, offset, whence);
}

int fo_close_mhtfile(int fd){
	return close(fd);
}

int fo_copy_file(char* srcPath,char *destPath)
{
	int fd_src, fd_dest, ret;
	fd_src = open(srcPath, O_RDONLY);
		
	if(fd_src < 3)
	{
		perror("srcPath");
		return -1;
	}
	fd_dest = open(destPath, O_WRONLY|O_CREAT, 0755);
	if(fd_dest < 3)
	{
		close(fd_src);
		perror("destPath");
		return -1;
	}
	char buf[1024] = "";
	do
	{
    	memset(buf,0,sizeof(buf));
		ret = read(fd_src, buf, sizeof(buf));
		if(ret>0)
			write(fd_dest, buf, ret);
	}while(ret >0);
	close(fd_src);
	close(fd_dest);
	return 0;
}

void fo_printMHTFile(int fd)
{
	uchar *mhtblk_buffer = NULL;
    PMHT_FILE_HEADER mhtfilehdr_ptr = NULL;
    PMHT_BLOCK tmpblk_ptr = NULL;

	if(fd < 0){
		debug_print("fo_printMHTFile", "Invalid fd");
		return;
	}

    tmpblk_ptr = makeMHTBlock();
    if(!(mhtfilehdr_ptr = readMHTFileHeader(fd)))
    {
        debug_print("insertpageinmht", "Failed to read MHT file header");
        return;
    }

    printf("FSLLOS: %d,   RNO:%d\n",mhtfilehdr_ptr->m_firstSupplementaryLeafOffset, mhtfilehdr_ptr->m_rootNodeOffset);
    mhtblk_buffer = (uchar*) malloc(MHT_BLOCK_SIZE);
    while(fo_locate_mht_pos(fd, 0, SEEK_CUR) != mhtfilehdr_ptr->m_rootNodeOffset)
    {
        memset(mhtblk_buffer, 0, MHT_BLOCK_SIZE);
        fo_read_mht_block2(fd, mhtblk_buffer, MHT_BLOCK_SIZE, 0, SEEK_CUR);
        unserialize_mht_block(mhtblk_buffer,  MHT_BLOCK_SIZE, &tmpblk_ptr);
        printf("pgno:%d\t level: %d\t ppgno:%d\t  lpgno:%d\t rpgno:%d\n",tmpblk_ptr->m_pageNo, tmpblk_ptr->m_nodeLevel, tmpblk_ptr->m_parentPageNo, tmpblk_ptr->m_lChildPageNo, tmpblk_ptr->m_rChildPageNo);
		//printf("pgno:%d\t ISN: %hhu\n",tmpblk_ptr->m_pageNo, tmpblk_ptr->m_isSupplementaryNode);
    }
}

void buildMHTFileByDatabase(const char* dbName){
	PQNode popped_qnode_ptr = NULL;
	PMHT_FILE_HEADER mht_file_header_ptr = NULL;
	uchar *mhtblk_buffer = NULL;
	uchar *mhthdr_buffer = NULL;

	// Preparing MHT file
	// Creating a new MHT file. Note that if the file exists, it will be truncated!
	if((g_mhtFileFD = fo_create_mhtfile(MHT_DEFAULT_FILE_NAME)) < 0) {
		debug_print("buildMHTFileByDatabase", "Creating MHT file failed!");
		return;
	}

	// Moving file pointer to 128th bytes to 
	// reserve space for header block (root node)
	if(fo_locate_mht_pos(g_mhtFileFD, MHT_HEADER_LEN, SEEK_CUR) < 0) {
		debug_print("buildMHTFileByDatabase", "Reserving space for root node failed!");
		return;
	}

	// Initializing MHT file header
	// Header will be updated at the end of building MHT file
	mht_file_header_ptr = makeMHTFileHeader();
	
	process_database_info(&g_pQHeader, &g_pQ, dbName);
	/* g_pQHeader->m_length > 1 indicates that there are at least 1 floating leaf node in the queue, 
	   thus, supplementary nodes must be added to construct a complete binary tree. If g_pQHeader->m_length = 1,
	   that means only the top root node remains in the queue. In other words, the number of leaf nodes (N_l) 
	   satifies that log_2(N_l) is an integer.
	*/
	if(g_pQHeader->m_length > 1)
		deal_with_remaining_nodes_in_queue(&g_pQHeader, &g_pQ, g_mhtFileFD);

	//dequeue remaining nodes (actually, only root node remains)
	while(popped_qnode_ptr = dequeue(&g_pQHeader, &g_pQ)){
		check_pointer(popped_qnode_ptr, "popped_qnode_ptr");
		// Building MHT blocks based on dequeued nodes, then writing to MHT file.
		mhtblk_buffer = (uchar*) malloc(MHT_BLOCK_SIZE);
		memset(mhtblk_buffer, 0, MHT_BLOCK_SIZE);
		qnode_to_mht_buffer(popped_qnode_ptr, &mhtblk_buffer, MHT_BLOCK_SIZE);
		// set the first byte of the root buffer to 0x01, which means this buffer stores root node
		(mhtblk_buffer + MHT_BLOCK_OFFSET_RSVD)[0] = 0x01;
		if(g_mhtFileFD > 0) {
			g_mhtFileRootNodeOffset = fo_locate_mht_pos(g_mhtFileFD, 0, SEEK_CUR);	//temporarily storing root node offset in MHT file
			fo_update_mht_block(g_mhtFileFD, mhtblk_buffer, MHT_BLOCK_SIZE, 0, SEEK_CUR);	// write root block to MHT file
		}
		free(mhtblk_buffer); mhtblk_buffer = NULL;

		//print_qnode_info(popped_qnode_ptr);
		deleteQNode(&popped_qnode_ptr);
	} //while

	/***** Updating MHT file header *****/
	mhthdr_buffer = (uchar*) malloc(MHT_HEADER_LEN);
	if(mht_file_header_ptr && mhthdr_buffer){
		mht_file_header_ptr->m_rootNodeOffset = g_mhtFileRootNodeOffset;
		mht_file_header_ptr->m_firstSupplementaryLeafOffset = g_mhtFirstSplymtLeafOffset;
		serialize_mht_file_header(mht_file_header_ptr, &mhthdr_buffer, MHT_HEADER_LEN);
		fo_update_mht_file_header(g_mhtFileFD, mhthdr_buffer, MHT_HEADER_LEN);
		free(mhthdr_buffer);
		freeMHTFileHeader(&mht_file_header_ptr);
	}

	//printQueue(g_pQHeader);

	freeQueue(&g_pQHeader, &g_pQ);
	fo_close_mhtfile(g_mhtFileFD);

	return;
}
void process_database_info(PQNode *pQHeader, PQNode *pQ, const char* dbName) {
	char tmp_hash_buffer[SHA256_DIGEST_LENGTH] = {0};
	int i = 0;
	int diff = 0;
	PQNode qnode_ptr = NULL;
	PQNode bkwd_ptr = NULL;
	PQNode current_qnode_ptr = NULL;
	PQNode cbd_qnode_ptr = NULL;
	PQNode popped_qnode_ptr = NULL;
	PQNode peeked_qnode_ptr = NULL;
	PQNode lchild_ptr = NULL;
	PQNode rchild_ptr = NULL;
	PMHTNode mhtnode_ptr = NULL;
	bool bCombined = FALSE;
	bool bDequeueExec = FALSE;	// whether dequeue is executed (for printf control)
	PMHT_BLOCK mht_blk_ptr = NULL;
	uchar *mhtblk_buffer = NULL;

	if(*pQHeader != NULL && *pQ != NULL){
		freeQueue(pQHeader, pQ);
		*pQ = NULL;
	}
	else if(*pQHeader){
		freeQueue2(pQHeader);
	}
	else if(*pQ){
		freeQueue3(pQ);
	}
	else{	// both of g_pQHeader and g_pQ are NULL
		;	// do nothing
	}
	initQueue(pQHeader, pQ);
	check_pointer((void*)*pQHeader, "pQHeader");
	check_pointer((void*)*pQ, "pQ");


	char *errmsg;
    sqlite3 *db;

    if(SQLITE_OK != sqlite3_open(dbName,&db) )
    { 
        printf("sqlite3_open error\n");
        exit(1);
    }
    char* sql = "SELECT name FROM sqlite_master WHERE type='table'";
    if(SQLITE_OK != sqlite3_exec(db,sql,NULL,NULL,&errmsg) )
    {
        printf("process_database_info:sqlite3_exec() falied！\n");
        return;
    }
    //pgno获取数据页面页码，dataHash存放数据页面哈希值
    unsigned int* pgnoNums;
    pgnoNums = (unsigned int*)malloc(sizeof(unsigned int) * LENGTH);
    unsigned char* dataHash;
    dataHash = (unsigned char *)malloc(sizeof(unsigned char)*LENGTH*SHA256_DIGEST_LENGTH);
    int count = getCount(db, dbName);
    //printf("%d\n", count);

    int n = 0;
    int start = 1;
    int end = 0x7fffffff;
	memset(pgnoNums, 0, sizeof(unsigned int) * LENGTH);
	memset(dataHash, 0, sizeof(unsigned char) *LENGTH*SHA256_DIGEST_LENGTH);

	n = getInfo1(db, dbName, pgnoNums, dataHash, start, end, LENGTH);
	for(i = 0; i<count; i+=LENGTH){
		//读取信息构建树
		for(int j=0; j<n; j++){
			memset(tmp_hash_buffer, 0, SHA256_DIGEST_LENGTH);
			memcpy(tmp_hash_buffer, dataHash+j*SHA256_DIGEST_LENGTH, SHA256_DIGEST_LENGTH);
			mhtnode_ptr = makeMHTNode(pgnoNums[j], tmp_hash_buffer); 
			check_pointer((void*)mhtnode_ptr, "mhtnode_ptr");
			qnode_ptr = makeQNode(mhtnode_ptr, NODELEVEL_LEAF); 
			check_pointer((void*)qnode_ptr, "qnode_ptr");
			enqueue(pQHeader, pQ, qnode_ptr);

			current_qnode_ptr = g_pQ;
			while ((bkwd_ptr = lookBackward(current_qnode_ptr)) && bkwd_ptr != g_pQHeader){
				check_pointer(bkwd_ptr, "bkwd_ptr");
				if(bkwd_ptr->m_level > (*pQ)->m_level)
					break;
				if(bkwd_ptr->m_level == (*pQ)->m_level) {
					lchild_ptr = bkwd_ptr;
					rchild_ptr = *pQ;
					cbd_qnode_ptr = makeCombinedQNode(lchild_ptr, rchild_ptr);
					bCombined = TRUE;
					check_pointer(cbd_qnode_ptr, "cbd_qnode_ptr");
					enqueue(pQHeader, pQ, cbd_qnode_ptr);
					deal_with_nodes_offset(cbd_qnode_ptr, lchild_ptr, rchild_ptr);
					deal_with_interior_nodes_pageno(cbd_qnode_ptr, lchild_ptr, rchild_ptr);
				}
				current_qnode_ptr = current_qnode_ptr->prev;
				check_pointer(current_qnode_ptr, "current_qnode_ptr");
			}

			//dequeue till encountering the new created combined node
			if(bCombined) {
				while ((peeked_qnode_ptr = peekQueue(*pQHeader)) && peeked_qnode_ptr->m_level < cbd_qnode_ptr->m_level) {
					popped_qnode_ptr = dequeue(pQHeader, pQ);
					check_pointer(popped_qnode_ptr, "popped_qnode_ptr");

					// Building MHT blocks based on dequeued nodes, then writing to MHT file.
					mhtblk_buffer = (uchar*) malloc(MHT_BLOCK_SIZE);
					memset(mhtblk_buffer, 0, MHT_BLOCK_SIZE);
					qnode_to_mht_buffer(popped_qnode_ptr, &mhtblk_buffer, MHT_BLOCK_SIZE);
					if(g_mhtFileFD > 0) {
						fo_update_mht_block(g_mhtFileFD, mhtblk_buffer, MHT_BLOCK_SIZE, 0, SEEK_CUR);
					}
					free(mhtblk_buffer); mhtblk_buffer = NULL;
					//print_qnode_info(popped_qnode_ptr);
					deleteQNode(&popped_qnode_ptr);
					bDequeueExec = TRUE;
				}
				//bDequeueExec ? printf("\n\n") : nop();
				bDequeueExec = FALSE;
				bCombined = FALSE;
			}
        }
		start = pgnoNums[n-1] + 1;
		memset(pgnoNums, 0, sizeof(unsigned int) * LENGTH);
		memset(dataHash, 0, sizeof(unsigned char) *LENGTH*SHA256_DIGEST_LENGTH);
        n = getInfo1(db, dbName, pgnoNums, dataHash, start,end, LENGTH);
	} 
	free(pgnoNums);
	free(dataHash);
	sqlite3_close(db);
}

int getVerifyPathByPageNo(int page_no, unsigned char** rHash, int** tempFlag) {
	//PMHT_BLOCK mhtblk_ptr = NULL;	// MHT block preserving the found page
	uchar *rootnode_buf = NULL;		// root node block buffer
	uchar *tmpblk_buf = NULL;	    // temporarily storing MHT block buffer
	uchar *childblk_buf = NULL;
	PMHT_FILE_HEADER mhtfilehdr_ptr = NULL;
	//bool	bPageBlockFound = FALSE;
	uint32	node_level = NODELEVEL_LEAF;
	uint32	node_page_no = UNASSIGNED_PAGENO;		// temporarily preserving MHT block's page number
	int ret_offset = -1;

	int fd = -1;

	if((fd = initOpenMHTFileWR(MHT_DEFAULT_FILE_NAME)) < 2){
		printf("Failed to open file %s\n", MHT_DEFAULT_FILE_NAME);
		exit(0);
	}


	if(fd < 3){
		debug_print("getVerifyPathByPageNo", "Invalid file descriptor");
		return -1;
	}

	if(page_no < 0) {
		debug_print("getVerifyPathByPageNo", "Invalid page number");
		return -1;
	}

	if(!(mhtfilehdr_ptr = readMHTFileHeader(fd))){
		debug_print("getVerifyPathByPageNo", "Failed to read MHT file header");
		return -1;
	}

	rootnode_buf = (uchar*) malloc(MHT_BLOCK_SIZE);
	childblk_buf = (uchar*) malloc(MHT_BLOCK_SIZE);
	memset(rootnode_buf, 0, MHT_BLOCK_SIZE);
	memset(childblk_buf, 0, MHT_BLOCK_SIZE);
	fo_read_mht_block2(fd, rootnode_buf, MHT_BLOCK_SIZE, mhtfilehdr_ptr->m_rootNodeOffset, SEEK_SET);
	fo_locate_mht_pos(fd, -MHT_BLOCK_SIZE, SEEK_CUR);
	tmpblk_buf = rootnode_buf;

	int offset = mhtfilehdr_ptr->m_rootNodeOffset;
	int t = *((int*)(tmpblk_buf + MHT_BLOCK_OFFSET_LEVEL));
	//printf("\nlevle:%d\n", t);

	*rHash = (unsigned char*)malloc(sizeof(unsigned char)*(t+2)*HASH_LEN);
	*tempFlag = (int*)malloc(sizeof(int) * (t+1));
	memset(*rHash, 0, sizeof(unsigned char)*(t+2)*HASH_LEN);
	memset(*tempFlag, 0, sizeof(int) * (t+1));
	int child_offset = 0;

	//记录根结点
	memcpy(*rHash, rootnode_buf+MHT_BLOCK_OFFSET_HASH, HASH_LEN); 
	int index = 1;

	// binary search algorithm
	while((node_level = *((int*)(tmpblk_buf + MHT_BLOCK_OFFSET_LEVEL))) > NODELEVEL_LEAF) {
		node_page_no = *((int*)(tmpblk_buf + MHT_BLOCK_OFFSET_PAGENO));
	    offset =  lseek (fd, 0, SEEK_CUR);
	    /*printf("pageNo: %d  node_level: %d\n", node_page_no, node_level);
	    for(int k=0; k<SHA256_DIGEST_LENGTH; k++) {
	    	printf("%02x", (int)(tmpblk_buf + MHT_BLOCK_OFFSET_HASH)[k]);
	    }
	    printf("\n");*/

		if(page_no <= node_page_no){ // go to left child block
	    	//记录右孩子hash
	   		child_offset = *((int*)(tmpblk_buf + MHT_BLOCK_OFFSET_RCOS));
	   		fo_read_mht_file(fd, (*rHash)+index*HASH_LEN, HASH_LEN,child_offset*MHT_BLOCK_SIZE+MHT_BLOCK_OFFSET_HASH+offset, SEEK_SET);
	   		(*tempFlag)[index] = 2;
			/*printf("\n");
			for(int k=0; k<SHA256_DIGEST_LENGTH; k++) {
				printf("%02x", (int)((*rHash)+index*HASH_LEN)[k]);
			}
			printf("\n");*/

	   		//读取左孩子
	   		fo_locate_mht_pos(fd, offset, SEEK_SET);
			fo_read_mht_block(fd, childblk_buf, MHT_BLOCK_SIZE, 
								*((int*)(tmpblk_buf + MHT_BLOCK_OFFSET_LCOS)), SEEK_CUR);
		}
		else{ // go to right child block
		 	child_offset = *((int*)(tmpblk_buf + MHT_BLOCK_OFFSET_LCOS));
		   	fo_read_mht_file(fd, (*rHash)+index*HASH_LEN, HASH_LEN,child_offset*MHT_BLOCK_SIZE+MHT_BLOCK_OFFSET_HASH+offset, SEEK_SET);
		   	(*tempFlag)[index] = 1;
			/*printf("\n");
			for(int k=0; k<SHA256_DIGEST_LENGTH; k++) {
				printf("%02x", (int)((*rHash)+index*HASH_LEN)[k]);
			}
	  		printf("\n");*/
		   	fo_locate_mht_pos(fd, offset, SEEK_SET);
			fo_read_mht_block(fd, childblk_buf, MHT_BLOCK_SIZE, 
								*((int*)(tmpblk_buf + MHT_BLOCK_OFFSET_RCOS)), SEEK_CUR);
		}
		index++;
		// reset the file pointer to the beginning of the current left child node block
		ret_offset = fo_locate_mht_pos(fd, -MHT_BLOCK_SIZE, SEEK_CUR);
		tmpblk_buf = childblk_buf;

	} // while

	// Now, tmpblk_buf stores the final leaf node block
	if(page_no != *((int*)(tmpblk_buf + MHT_BLOCK_OFFSET_PAGENO))) {
		//printf("offset:%d\n", ret_offset);
		debug_print("getVerifyPathByPageNo", "No page found");
		return -1;	// search failed
	}

	//locateMHTBlockOffsetByPageNo(fd, 164);
	// free memory
	free(rootnode_buf);
	free(childblk_buf);
	free(mhtfilehdr_ptr);
	fo_close_mhtfile(fd);
	return index;
}
int updateByPgno(int page_no, uchar *hash_val, uint32 hash_val_len, uchar *rootHash){
	uchar *block_buf = NULL;
	int fd = 0;
	if((fd = initOpenMHTFileWR(MHT_DEFAULT_FILE_NAME)) < 2){
		printf("Failed to open file %s\n", MHT_DEFAULT_FILE_NAME);
		exit(0);
	}
	//需要更新的MHT_block的偏移量
	//The offset of the MHT_block that needs to be updated
	int update_blobk_offset = 0;

	int update_res = -1;

	if(page_no < 0) {
		debug_print("updateByPgno", "Invalid page number");
		return -1;
	}

	if(!hash_val || hash_val_len != HASH_LEN){
		debug_print("updateByPgno", "Invalid hash_val or hash_val_len");
		return -1;
	}

	if((update_blobk_offset = locateMHTBlockOffsetByPageNo(fd, page_no)) <= 0){
		debug_print("updateByPgno", "No page found");
		return -2;
	}
	//更新指定页码的MHT_block块
	//Update the MHT_block block of the specified page number
	//读取MHT_block内容（使用绝对偏移量）
	//Read MHT_block content (using absolute offset)
	block_buf = (uchar*) malloc(MHT_BLOCK_SIZE);
	memset(block_buf, 0, MHT_BLOCK_SIZE);
	fo_read_mht_block2(fd, block_buf, MHT_BLOCK_SIZE, update_blobk_offset, SEEK_SET);

	//将更新后的哈希值写入文件中
	//Write the updated hash value to the file
	//1.替换旧哈希值
	//1. Replace the old hash value
	memcpy(block_buf + MHT_BLOCK_OFFSET_HASH, hash_val, HASH_LEN);
	//2.将文件读写光标重新定位到更新块的开头
	//2. Reposition the file read and write cursor to the beginning of the update block
	fo_locate_mht_pos(fd, update_blobk_offset, SEEK_SET);
	//3.写入文件
	//3. Write to file
	fo_update_mht_block(fd, block_buf, MHT_BLOCK_SIZE, 0, SEEK_CUR);
	//4.更新验证路径
	//4. Update verification path
	update_res = updatePathToRoot(block_buf, update_blobk_offset, fd);
	//5.将文件读写光标重新定位到更新块的开头
	//5. Reposition the file read and write cursor to the beginning of the update block
	update_blobk_offset = fo_locate_mht_pos(fd, update_blobk_offset, SEEK_SET);
	//locateMHTBlockOffsetByPageNo(fd, page_no);
	//6.读取根结点
	PMHT_FILE_HEADER mhtfilehdr_ptr = NULL;
	if(!(mhtfilehdr_ptr = readMHTFileHeader(fd)))
    {
		debug_print("updateByPgno", "Failed to read MHT file header");
		return -1;
	}
	fo_read_mht_file(fd, rootHash, HASH_LEN, mhtfilehdr_ptr->m_rootNodeOffset + MHT_BLOCK_OFFSET_HASH, SEEK_SET);
	//printf("after update:");
	//locateMHTBlockOffsetByPageNo(fd, page_no);
	free(block_buf);
	fo_close_mhtfile(fd);
	return update_res == 0 ? update_blobk_offset : 0 ;
}

int insertNewPage(int page_no, uchar *hash_val, uint32 hash_val_len, uchar *rootHash)
{

	int fd = MHT_INVALID_FILE_DSCPT;
    int supplementaryNode_offset = 0;
	uchar *mhtblk_buffer = NULL;
    //文件头信息
    PMHT_FILE_HEADER mhtfilehdr_ptr = NULL;

    if(page_no < 0) {
        debug_print("insertNewPage", "Invalid page number");
        return -1;
    }

    if(!hash_val || hash_val_len != HASH_LEN){
        debug_print("insertNewPage", "Invalid hash_val or hash_val_len");
        return -1;
    }

	if( (fd = initOpenMHTFileWR(MHT_DEFAULT_FILE_NAME))  < 3){
		printf("Failed to open file %s\n", MHT_TMP_FILE_NAME);
		exit(0);
	}

	//判断是否有可填充节点
    if(!(mhtfilehdr_ptr = readMHTFileHeader(fd)))
    {
        debug_print("insertNewPage", "Failed to read MHT file header");
        return -1;
    }
    supplementaryNode_offset = mhtfilehdr_ptr->m_firstSupplementaryLeafOffset;
	g_mhtFileRootNodeOffset = mhtfilehdr_ptr->m_rootNodeOffset;
	printf("supplementaryNode_offset :%d\n",supplementaryNode_offset );
    //没有可以进行填充的节点，需要添加新的补全节点;否则进行下一步更新操作
    if(supplementaryNode_offset == 0)
    {
		supplementaryNode_offset = extentTheMHT(fd);
    }

	//修改节点信息并写入文件
	bool insert_isn = FALSE;
	bool insert_izn = FALSE;
	mhtblk_buffer = (uchar*) malloc(MHT_BLOCK_SIZE);
	memset(mhtblk_buffer, 0, MHT_BLOCK_SIZE);

	fo_read_mht_block2(fd, mhtblk_buffer, MHT_BLOCK_SIZE, supplementaryNode_offset, SEEK_SET);
	memcpy(mhtblk_buffer+MHT_BLOCK_OFFSET_PAGENO, &page_no, sizeof(int));
	printf("mhtblk_buffer + MHT_BLOCK_OFFSET_PAGENO: %d\t", *((int*)(mhtblk_buffer + MHT_BLOCK_OFFSET_PAGENO)));
	memcpy(mhtblk_buffer+MHT_BLOCK_OFFSET_HASH, hash_val, HASH_LEN);
	memcpy(mhtblk_buffer+MHT_BLOCK_OFFSET_ISN, &insert_isn, sizeof(char));
	memcpy(mhtblk_buffer+MHT_BLOCK_OFFSET_IZN, &insert_izn, sizeof(char));
    //将更新信息块写入文件
    if( fo_update_mht_block2(fd, mhtblk_buffer, MHT_BLOCK_SIZE, supplementaryNode_offset, SEEK_SET) <= 0)
    {
        return -1;
    }
 	//更新验证路径
    updatePathToRoot(mhtblk_buffer, supplementaryNode_offset, fd);
    //修改插入更新后的页码
    update_interior_nodes_pageno(mhtblk_buffer, supplementaryNode_offset, fd);

	//找到下一个填充节点并更新MHT头文件信息
	supplementaryNode_offset = find_the_first_leaf_splymt_block_by_offset(fd, supplementaryNode_offset);
	printf("update supplementaryNode_offset: %d\n", supplementaryNode_offset);
	uchar *mhthdr_buffer = NULL;
	mhthdr_buffer = (uchar*) malloc(MHT_HEADER_LEN);
	mhtfilehdr_ptr->m_rootNodeOffset = g_mhtFileRootNodeOffset;
	mhtfilehdr_ptr->m_firstSupplementaryLeafOffset = supplementaryNode_offset == -1? 0 : supplementaryNode_offset;
	serialize_mht_file_header(mhtfilehdr_ptr, &mhthdr_buffer, MHT_HEADER_LEN);
	fo_update_mht_file_header(fd, mhthdr_buffer, MHT_HEADER_LEN);

	//读取更新后的根节点
	fo_read_mht_file(fd, rootHash, HASH_LEN, mhtfilehdr_ptr->m_rootNodeOffset + MHT_BLOCK_OFFSET_HASH, SEEK_SET);

	fo_close_mhtfile(fd);
    free(mhthdr_buffer);
	free(mhtblk_buffer);
	freeMHTFileHeader(&mhtfilehdr_ptr);

    return 0;
}

