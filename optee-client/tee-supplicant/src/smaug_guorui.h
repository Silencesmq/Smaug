#ifndef TA_MY_TEST_H
#define TA_MY_TEST_H

/*
 * This UUID is generated with uuidgen
 * the ITU-T UUID generator at http://www.itu.int/ITU-T/asn1/uuid.html
 */
#define TA_MY_TEST_UUID \
	{ 0x9aaaf200, 0x2450, 0x11e4, \
		{ 0xab, 0xe2, 0x00, 0x02, 0xa5, 0xd5, 0xc5, 0x1b} }

/* The function IDs implemented in this TA */
#define TA_MY_TEST_CMD_TO_VERIFY 		4
#define TA_MY_TEST_CMD_GET_HASH 		5
#define TA_MY_TEST_CMD_VERIFY_HASH		6

void updateHash(char* sql);
void selectHash(char* sql);
void initFile();

#endif /*TA_HELLO_WORLD_H*/
