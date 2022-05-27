#ifndef TEE_TPM_H
#define TEE_TPM_H

#include <tee_api_types.h>

TEE_Result syscall_tpm_get_version(void *buf);

#endif /* TEE_TPM_H */
