#include "defs.h"
#include "dbqueue.h"

const uchar g_zeroHash[HASH_LEN] = {0x5f, 0xec, 0xeb, 0x66, 0xff, 0xc8, 0x6f, 0x38,
									0xd9, 0x52, 0x78, 0x6c, 0x6d, 0x69, 0x6c, 0x79, 
									0xc2, 0xdb, 0xc2, 0x39, 0xdd, 0x4e, 0x91, 0xb4, 
									0x67, 0x29, 0xd7, 0x3a, 0x27, 0xfb, 0x57, 0xe9};

const int g_MhtAttribOffsetArray[MHT_BLOCK_ATRRIB_NUM] = {
	MHT_BLOCK_OFFSET_PAGENO,
	MHT_BLOCK_OFFSET_LEVEL,
	MHT_BLOCK_OFFSET_HASH,
	MHT_BLOCK_OFFSET_ISN,
	MHT_BLOCK_OFFSET_IZN,
	MHT_BLOCK_OFFSET_LCPN,
	MHT_BLOCK_OFFSET_LCOS,
	MHT_BLOCK_OFFSET_RCPN,
	MHT_BLOCK_OFFSET_RCOS,
	MHT_BLOCK_OFFSET_PPN,
	MHT_BLOCK_OFFSET_POS,
	MHT_BLOCK_OFFSET_RSVD
};

/****** global variable definitions ******/
PQNode g_pQHeader = NULL;	// pointer to queue's header
PQNode g_pQ = NULL;			// pointer to queue's tail (current element)
int g_mhtFileFD = MHT_INVALID_FILE_DSCPT;		// file descriptor for the MHT file
int g_mhtFileFdRd = MHT_INVALID_FILE_DSCPT;		// file descriptor only for reading the MHT file
uint32 g_mhtFileRootNodeOffset = UNASSIGNED_OFFSET;	// root node offset (in byte)
uint32 g_mhtFirstSplymtLeafOffset = UNASSIGNED_OFFSET;	// offset of the first supplementary leaf node (in byte)
uint32 g_mhtFileFSLO = UNASSIGNED_OFFSET;	// first supplementary leaf offset (in byte)

void nop() {
	return;
}

void check_pointer(void* ptr, const char *ptr_name) {
	if(!ptr){
		printf("Pointer %s is NULL.\n", ptr_name);
	}

	return;
}

void debug_print(const char *from, const char *dbg_msg) {
	if(!from || !dbg_msg) {
		printf("DBGMSG ERROR: null message from a null source.\n");
		return;
	}

	printf("DBGMSG: from %s; %s.\n", from, dbg_msg);

	return;
}

void print_buffer_in_byte_hex( uchar *buf, uint32 buf_len){
	int i = 0;

	if(!buf || buf_len <= 0){
		printf("Parameter \"buf\" cannot be NULL, or buf_len cannot be equal to/less than 0.\n");
		return;
	}
	for(i = 0; i < buf_len; i++) {
		printf("%#04x  ", buf[i]);
	}
	printf("\n");
}