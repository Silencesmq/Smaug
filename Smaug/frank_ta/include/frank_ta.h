#ifndef FRANK_TA_H
#define FRANK_TA_H

/*
 * This UUID is generated with uuidgen
 * the ITU-T UUID generator at http://www.itu.int/ITU-T/asn1/uuid.html
 */
#define FRANK_TA_UUID \
	{ 0x1111fadd, 0x99d5, 0x4afb, \
		{ 0xa1, 0xdc, 0xee, 0x3e, 0x9c, 0x61, 0xb0, 0x4d} }

/* The function IDs implemented in this TA */
#define TA_FRANK_CMD_INSERT     0
#define TA_FRANK_CMD_SELECT     1
#define TA_FRANK_CMD_WHERE      2
#define TA_FRANK_CMD_UPDATE		3
#define TA_FRANK_CMD_OPE        4

#endif /*FRANK_TA_H*/

