#ifndef TEE_TPM_H
#define TEE_TPM_H

#define TPM_RESP_MAX_SIZE		4096	///< This is the maximum possible TPM response size in bytes.

// TPM_PT constants
#define PT_FIXED_SELECTOR		1		///< Fixed GetCapability Flags
#define PT_VAR_SELECTOR			2		///< Variable GetCapability Flags

//-------------"Macros"-------------
#define MEMSET_FREE(x, y) if (NULL != x) { memset(x, 0, y); free(x); x = NULL; } ///< Sets memory to 0, frees memory and sets pointer to NULL.

typedef unsigned char   uint8_t;

static const uint8_t tpm2_getcapability_fixed[] ={
	0x80, 0x01,			// TPM_ST_NO_SESSIONS
	0x00, 0x00, 0x00, 0x16,		// commandSize
	0x00, 0x00, 0x01, 0x7A,		// TPM_CC_GetCapability
	0x00, 0x00, 0x00, 0x06,		// TPM_CAP_TPM_PROPERTIES (Property Type: TPM_PT)
	0x00, 0x00, 0x01, 0x00,		// Property: TPM_PT_FAMILY_INDICATOR: PT_GROUP * 1 + 0
	0x00, 0x00, 0x00, 0x66		// PropertyCount 102 (from 100 - 201)
};

int invoke_tpm(int cmd, int *size, char *outbuf);

#endif /* TEE_TPM_H */
