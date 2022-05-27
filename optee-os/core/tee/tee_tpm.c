#include <assert.h>
#include <string.h>
#include <optee_rpc_cmd.h>
#include <kernel/thread.h>
#include <kernel/msg_param.h>
#include <tee/tee_svc.h>
#include <mm/tee_mm.h>
#include <mm/mobj.h>
#include <tee/tee_tpm.h>

TEE_Result syscall_tpm_get_version(void *buf)
{
    uint8_t *ree_shm = NULL;
    struct mobj *mobj = NULL;
    TEE_Result res;
    struct thread_param params[2];
    memset(params, 0, sizeof(params));
    
    params[0].attr = THREAD_PARAM_ATTR_VALUE_IN;
    params[0].u.value.a = OPTEE_TPM_VERSION;
    
    // 分配共享内存
    mobj = thread_rpc_alloc_payload(4096);
	if (!mobj)
		return TEE_ERROR_OUT_OF_MEMORY;
    
	if (mobj->size < 4096) {
		res = TEE_ERROR_SHORT_BUFFER;
		goto exit;
	}

    // 获取分配的共享内存的虚拟地址被保存在ree_shm中
    ree_shm = mobj_get_va(mobj, 0);
    // 检查虚拟地址是否有效
    assert(ree_shm);

	params[1].attr = THREAD_PARAM_ATTR_MEMREF_OUT;
	params[1].u.memref.size = 4096;
    params[1].u.memref.offs = 0;
	params[1].u.memref.mobj = mobj;

    res = thread_rpc_cmd(OPTEE_MSG_RPC_CMD_TPM, 2, params);
    if (res != TEE_SUCCESS)
		goto exit;
    
    //tee_shm = malloc(params[1].u.memref.size);
    //memcpy(tee_shm, ree_shm, params[1].u.memref.size);
    //tee_svc_copy_to_user(buf, tee_shm, params[1].u.memref.size);
    //free(tee_shm);

    memcpy(buf, ree_shm, params[1].u.memref.size);
exit:
	thread_rpc_free_payload(mobj);
	return res;
}
