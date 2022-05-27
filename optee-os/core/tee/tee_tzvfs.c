#include <assert.h>
#include <string.h>
#include <optee_rpc_cmd.h>
#include <kernel/thread.h>
#include <kernel/msg_param.h>
#include <tee/tee_svc.h>
#include <mm/tee_mm.h>
#include <mm/mobj.h>
#include <tee/tee_tzvfs.h>

// 打开文件. 调用成功时返回一个文件描述符fd, 调用失败时返回-1, 并修改errno
int syscall_tzvfs_open(int *tzvfs_errno, const char *filename, int flags, mode_t mode){
  int ret = -1;
  
  size_t size = TZVFS_FS_NAME_MAX;
  struct thread_param params[3];
  struct mobj *mobj = NULL;
  void *va;
  // 分配共享内存
  mobj = thread_rpc_alloc_payload(size);
  if (!mobj) {
    *tzvfs_errno = TEE_ERROR_OUT_OF_MEMORY;
    return -1;
  }
  if (mobj->size < size) {
    *tzvfs_errno = TEE_ERROR_SHORT_BUFFER;
    thread_rpc_free_payload(mobj);
    return -1;
  }
  // 获取分配的共享内存的虚拟地址
  va = mobj_get_va(mobj, 0);
  memcpy(va, filename, strlen(filename)+1);
  // 初始RPC参数
  params[0] = THREAD_PARAM_VALUE(IN, TZVFS_RPC_FS_OPEN, flags, mode);
  params[1] = THREAD_PARAM_MEMREF(IN, mobj, 0, size);
  params[2] = THREAD_PARAM_VALUE(OUT, 0, 0, 0);
  // 发起RPC调用
  if (TEE_SUCCESS == thread_rpc_cmd(OPTEE_MSG_RPC_CMD_TZVFS, 3, params)) {
    ret = params[2].u.value.a;
    if (ret == -1) *tzvfs_errno = params[2].u.value.b;
  }
  thread_rpc_free_payload(mobj);

  return ret;
}

// 若文件顺利关闭则返回0, 发生错误时返回-1
int syscall_tzvfs_close(int *tzvfs_errno, int fd){
  int ret = -1;

  struct thread_param params[2];
  params[0] = THREAD_PARAM_VALUE(IN, TZVFS_RPC_FS_CLOSE, fd, 0);
  params[1] = THREAD_PARAM_VALUE(OUT, 0, 0, 0);
  if (TEE_SUCCESS == thread_rpc_cmd(OPTEE_MSG_RPC_CMD_TZVFS, 2, params)) {
    ret = params[1].u.value.a;
    if (ret == -1) *tzvfs_errno = params[1].u.value.b;
  }

  return ret;
}

// 获取当前工作目录, 成功则返回当前工作目录; 如失败返回NULL, 错误代码存于errno
char *syscall_tzvfs_getcwd(int *tzvfs_errno, char *buf, size_t size){
  char* ret = NULL;

  struct thread_param params[3];
  struct mobj *mobj = NULL;
  void *va;
  // 分配共享内存
  mobj = thread_rpc_alloc_payload(size);
  if (!mobj) {
    *tzvfs_errno = TEE_ERROR_OUT_OF_MEMORY;
    return NULL;
  }
  if (mobj->size < size) {
    *tzvfs_errno = TEE_ERROR_SHORT_BUFFER;
    thread_rpc_free_payload(mobj);
    return NULL;
  }
  // 获取分配的共享内存的虚拟地址
  va = mobj_get_va(mobj, 0);
  // 初始RPC参数
  params[0] = THREAD_PARAM_VALUE(IN, TZVFS_RPC_FS_GETCWD, 0, 0);
  params[1] = THREAD_PARAM_MEMREF(OUT, mobj, 0, size);
  params[2] = THREAD_PARAM_VALUE(OUT, 0, 0, 0);
  // 发起RPC调用
  if (TEE_SUCCESS == thread_rpc_cmd(OPTEE_MSG_RPC_CMD_TZVFS, 3, params)) {
    memcpy(buf, va, size);
    ret = (char*)((int)params[2].u.value.a);
    if (ret == NULL) *tzvfs_errno = params[2].u.value.b;
  }
  thread_rpc_free_payload(mobj);

  return ret;
}

// 获取一些文件相关的信息, 成功执行时，返回0。失败返回-1，errno
// lstat函数是不穿透（不追踪）函数，对软链接文件进行操作时，操作的是软链接文件本身
int syscall_tzvfs_lstat(int *tzvfs_errno, const char* path, struct tzvfs_stat *buf) {
  int ret = -1;

  size_t size1 = TZVFS_FS_NAME_MAX;
  size_t size2 = sizeof(struct tzvfs_stat);
  struct thread_param params[4];
  struct mobj *mobj1 = NULL, *mobj2 = NULL;
  void *va1, *va2;
  // 分配共享内存
  mobj1 = thread_rpc_alloc_payload(size1);
  if (!mobj1) {
    *tzvfs_errno = TEE_ERROR_OUT_OF_MEMORY;
    return -1;
  }
  if (mobj1->size < size1) {
    *tzvfs_errno = TEE_ERROR_SHORT_BUFFER;
    thread_rpc_free_payload(mobj1);
    return -1;
  }
  mobj2 = thread_rpc_alloc_payload(size2);
  if (!mobj2) {
    *tzvfs_errno = TEE_ERROR_OUT_OF_MEMORY;
    return -1;
  }
  if (mobj2->size < size2) {
    *tzvfs_errno = TEE_ERROR_SHORT_BUFFER;
    thread_rpc_free_payload(mobj2);
    return -1;
  }
  // 获取分配的共享内存的虚拟地址
  va1 = mobj_get_va(mobj1, 0);
  va2 = mobj_get_va(mobj2, 0);
  memcpy(va1, path, strlen(path)+1);
  // 初始RPC参数
  params[0] = THREAD_PARAM_VALUE(IN, TZVFS_RPC_FS_LSTAT, 0, 0);
  params[1] = THREAD_PARAM_MEMREF(IN, mobj1, 0, size1);
  params[2] = THREAD_PARAM_MEMREF(OUT, mobj2, 0, size2);
  params[3] = THREAD_PARAM_VALUE(OUT, 0, 0, 0);
  // 发起RPC调用
  if (TEE_SUCCESS == thread_rpc_cmd(OPTEE_MSG_RPC_CMD_TZVFS, 4, params)) {
    memcpy(buf, va2, size2);
    ret = params[3].u.value.a;
    if (ret == -1) *tzvfs_errno = params[3].u.value.b;
  }
  thread_rpc_free_payload(mobj1);
  thread_rpc_free_payload(mobj2);
  
  return ret;
}

// 获取一些文件相关的信息, 成功执行时，返回0。失败返回-1，errno
// stat函数是穿透（追踪）函数，即对软链接文件进行操作时，操作的是链接到的那一个文件，不是软链接文件本身
int syscall_tzvfs_stat(int *tzvfs_errno, const char *path, struct tzvfs_stat *buf){
  int ret = -1;

  size_t size1 = TZVFS_FS_NAME_MAX;
  size_t size2 = sizeof(struct tzvfs_stat);
  struct thread_param params[4];
  struct mobj *mobj1 = NULL, *mobj2 = NULL;
  void *va1, *va2;
  // 分配共享内存
  mobj1 = thread_rpc_alloc_payload(size1);
  if (!mobj1) {
    *tzvfs_errno = TEE_ERROR_OUT_OF_MEMORY;
    return -1;
  }
  if (mobj1->size < size1) {
    *tzvfs_errno = TEE_ERROR_SHORT_BUFFER;
    thread_rpc_free_payload(mobj1);
    return -1;
  }
  mobj2 = thread_rpc_alloc_payload(size2);
  if (!mobj2) {
    *tzvfs_errno = TEE_ERROR_OUT_OF_MEMORY;
    return -1;
  }
  if (mobj2->size < size2) {
    *tzvfs_errno = TEE_ERROR_SHORT_BUFFER;
    thread_rpc_free_payload(mobj2);
    return -1;
  }
  // 获取分配的共享内存的虚拟地址
  va1 = mobj_get_va(mobj1, 0);
  va2 = mobj_get_va(mobj2, 0);
  memcpy(va1, path, strlen(path)+1);
  // 初始RPC参数
  params[0] = THREAD_PARAM_VALUE(IN, TZVFS_RPC_FS_STAT, 0, 0);
  params[1] = THREAD_PARAM_MEMREF(IN, mobj1, 0, size1);
  params[2] = THREAD_PARAM_MEMREF(OUT, mobj2, 0, size2);
  params[3] = THREAD_PARAM_VALUE(OUT, 0, 0, 0);
  // 发起RPC调用
  if (TEE_SUCCESS == thread_rpc_cmd(OPTEE_MSG_RPC_CMD_TZVFS, 4, params)) {
    memcpy(buf, va2, size2);
    ret = params[3].u.value.a;
    if (ret == -1) *tzvfs_errno = params[3].u.value.b;
  }  
  thread_rpc_free_payload(mobj1);
  thread_rpc_free_payload(mobj2);  
  
  return ret;
}

// fstat函数与stat函数的功能一样，只是第一个形参是文件描述符
int syscall_tzvfs_fstat(int *tzvfs_errno, int fd, struct tzvfs_stat *buf){
  int ret = -1;

  size_t size = sizeof(struct tzvfs_stat);
  struct thread_param params[3];
  struct mobj *mobj = NULL;
  void *va;
  // 分配共享内存
  mobj = thread_rpc_alloc_payload(size);
  if (!mobj) {
    *tzvfs_errno = TEE_ERROR_OUT_OF_MEMORY;
    return -1;
  }
  if (mobj->size < size) {
    *tzvfs_errno = TEE_ERROR_SHORT_BUFFER;
    thread_rpc_free_payload(mobj);
    return -1;
  }
  // 获取分配的共享内存的虚拟地址
  va = mobj_get_va(mobj, 0);
  // 初始RPC参数
  params[0] = THREAD_PARAM_VALUE(IN, TZVFS_RPC_FS_FSTAT, fd, 0);
  params[1] = THREAD_PARAM_MEMREF(OUT, mobj, 0, size);
  params[2] = THREAD_PARAM_VALUE(OUT, 0, 0, 0);
  // 发起RPC调用
  if (TEE_SUCCESS == thread_rpc_cmd(OPTEE_MSG_RPC_CMD_TZVFS, 3, params)) {
    memcpy(buf, va, size);
    ret = params[2].u.value.a;
    if (ret == -1) *tzvfs_errno = params[2].u.value.b;
  }
  thread_rpc_free_payload(mobj);

  return ret;
}

// 通过fcntl可以改变已打开的文件性质, F_SETLK 设置文件锁定的状态
// fcntl的返回值与命令有关。如果出错，所有命令都返回－1，如果成功则返回某个其他值。
int syscall_tzvfs_fcntl(int *tzvfs_errno, int fd, int cmd, struct tzvfs_flock *arg){
  int ret = -1;
  
  size_t size = sizeof(struct tzvfs_flock);
  struct thread_param params[3];
  struct mobj *mobj = NULL;
  void *va;
  // 分配共享内存
  mobj = thread_rpc_alloc_payload(size);
  if (!mobj) {
    *tzvfs_errno = TEE_ERROR_OUT_OF_MEMORY;
    return -1;
  }
  if (mobj->size < size) {
    *tzvfs_errno = TEE_ERROR_SHORT_BUFFER;
    thread_rpc_free_payload(mobj);
    return -1;
  }
  // 获取分配的共享内存的虚拟地址
  va = mobj_get_va(mobj, 0);
  memcpy(va, arg, size);
  // 初始RPC参数
  params[0] = THREAD_PARAM_VALUE(IN, TZVFS_RPC_FS_FCNTL, fd, cmd);
  params[1] = THREAD_PARAM_MEMREF(IN, mobj, 0, size);
  params[2] = THREAD_PARAM_VALUE(OUT, 0, 0, 0);
  // 发起RPC调用
  if (TEE_SUCCESS == thread_rpc_cmd(OPTEE_MSG_RPC_CMD_TZVFS, 3, params)) {
    ret = params[2].u.value.a;
    if (ret == -1) *tzvfs_errno = params[2].u.value.b;
  }
  thread_rpc_free_payload(mobj);

  return ret;
}

// read会把参数fd所指的文件传送count个字节到buf指针所指的内存中
// 返回值为实际读取到的字节数, 如果返回0, 表示已到达文件尾或是无可读取的数据
// 当有错误发生时则返回-1, 错误代码存入errno 中
ssize_t syscall_tzvfs_read(int *tzvfs_errno, int fd, void *buf, size_t count){
  ssize_t ret = -1;

  size_t size = count;
  struct thread_param params[3];
  struct mobj *mobj = NULL;
  void *va;
  // 分配共享内存
  mobj = thread_rpc_alloc_payload(size);
  if (!mobj) {
    *tzvfs_errno = TEE_ERROR_OUT_OF_MEMORY;
    return -1;
  }
  if (mobj->size < size) {
    *tzvfs_errno = TEE_ERROR_SHORT_BUFFER;
    thread_rpc_free_payload(mobj);
    return -1;
  }
  // 获取分配的共享内存的虚拟地址
  va = mobj_get_va(mobj, 0);
  // 初始RPC参数
  params[0] = THREAD_PARAM_VALUE(IN, TZVFS_RPC_FS_READ, fd, 0);
  params[1] = THREAD_PARAM_MEMREF(OUT, mobj, 0, size);
  params[2] = THREAD_PARAM_VALUE(OUT, 0, 0, 0);
  // 发起RPC调用
  if (TEE_SUCCESS == thread_rpc_cmd(OPTEE_MSG_RPC_CMD_TZVFS, 3, params)) {
    memcpy(buf, va, size);
    ret = params[2].u.value.a;
    if (ret == -1) *tzvfs_errno = params[2].u.value.b;
  }
  thread_rpc_free_payload(mobj);
  
  return ret;
}

// write函数把buf中nbyte写入文件描述符handle所指的文档
// 成功时返回写的字节数，错误时返回-1
ssize_t syscall_tzvfs_write(int *tzvfs_errno, int fd, const void *buf, size_t count){
  ssize_t ret = -1;

  size_t size = count;
  struct thread_param params[3];
  struct mobj *mobj = NULL;
  void *va;
  // 分配共享内存
  mobj = thread_rpc_alloc_payload(size);
  if (!mobj) {
    *tzvfs_errno = TEE_ERROR_OUT_OF_MEMORY;
    return -1;
  }
  if (mobj->size < size) {
    *tzvfs_errno = TEE_ERROR_SHORT_BUFFER;
    thread_rpc_free_payload(mobj);
    return -1;
  }
  // 获取分配的共享内存的虚拟地址
  va = mobj_get_va(mobj, 0);
  memcpy(va, buf, size);
  // 初始RPC参数
  params[0] = THREAD_PARAM_VALUE(IN, TZVFS_RPC_FS_WRITE, fd, 0);
  params[1] = THREAD_PARAM_MEMREF(IN, mobj, 0, size);
  params[2] = THREAD_PARAM_VALUE(OUT, 0, 0, 0);
  // 发起RPC调用
  if (TEE_SUCCESS == thread_rpc_cmd(OPTEE_MSG_RPC_CMD_TZVFS, 3, params)) {
    ret = params[2].u.value.a;
    if (ret == -1) *tzvfs_errno = params[2].u.value.b;
  }
  thread_rpc_free_payload(mobj);
  
  return ret;
}

// geteuid()用来取得执行目前进程有效的用户识别码
// 返回有效的用户识别码
uid_t syscall_tzvfs_geteuid(int *tzvfs_errno){
  uid_t ret = -1;

  struct thread_param params[2];
  params[0] = THREAD_PARAM_VALUE(IN, TZVFS_RPC_FS_GETEUID, 0, 0);
  params[1] = THREAD_PARAM_VALUE(OUT, 0, 0, 0);
  // 发起RPC调用
  if (TEE_SUCCESS == thread_rpc_cmd(OPTEE_MSG_RPC_CMD_TZVFS, 2, params)) {
    ret = params[1].u.value.a;
    if (ret == (unsigned int)-1) *tzvfs_errno = params[1].u.value.b;
  }
  
  return ret;
}

int syscall_tzvfs_unlink(int *tzvfs_errno, const char *pathname){
  int ret = -1;
  
  size_t size = TZVFS_FS_NAME_MAX;
  struct thread_param params[3];
  struct mobj *mobj = NULL;
  void *va;
  // 分配共享内存
  mobj = thread_rpc_alloc_payload(size);
  if (!mobj) {
    *tzvfs_errno = TEE_ERROR_OUT_OF_MEMORY;
    return -1;
  }
  if (mobj->size < size) {
    *tzvfs_errno = TEE_ERROR_SHORT_BUFFER;
    thread_rpc_free_payload(mobj);
    return -1;
  }
  // 获取分配的共享内存的虚拟地址
  va = mobj_get_va(mobj, 0);
  memcpy(va, pathname, strlen(pathname)+1);
  // 初始RPC参数
  params[0] = THREAD_PARAM_VALUE(IN, TZVFS_RPC_FS_UNLINK, 0, 0);
  params[1] = THREAD_PARAM_MEMREF(IN, mobj, 0, size);
  params[2] = THREAD_PARAM_VALUE(OUT, 0, 0, 0);
  // 发起RPC调用
  if (TEE_SUCCESS == thread_rpc_cmd(OPTEE_MSG_RPC_CMD_TZVFS, 3, params)) {
    ret = params[2].u.value.a;
    if (ret == -1) *tzvfs_errno = params[2].u.value.b;
  }
  thread_rpc_free_payload(mobj);

  return ret;
}

int syscall_tzvfs_access(int *tzvfs_errno, const char *pathname, int mode){
  int ret = -1;

  size_t size = TZVFS_FS_NAME_MAX;
  struct thread_param params[3];
  struct mobj *mobj = NULL;
  void *va;
  // 分配共享内存
  mobj = thread_rpc_alloc_payload(size);
  if (!mobj) {
    *tzvfs_errno = TEE_ERROR_OUT_OF_MEMORY;
    return -1;
  }
  if (mobj->size < size) {
    *tzvfs_errno = TEE_ERROR_SHORT_BUFFER;
    thread_rpc_free_payload(mobj);
    return -1;
  }
  // 获取分配的共享内存的虚拟地址
  va = mobj_get_va(mobj, 0);
  memcpy(va, pathname, strlen(pathname)+1);
  // 初始RPC参数
  params[0] = THREAD_PARAM_VALUE(IN, TZVFS_RPC_FS_ACCESS, mode, 0);
  params[1] = THREAD_PARAM_MEMREF(IN, mobj, 0, size);
  params[2] = THREAD_PARAM_VALUE(OUT, 0, 0, 0);
  // 发起RPC调用
  if (TEE_SUCCESS == thread_rpc_cmd(OPTEE_MSG_RPC_CMD_TZVFS, 3, params)) {
    ret = params[2].u.value.a;
    if (ret == -1) *tzvfs_errno = params[2].u.value.b;
  }
  thread_rpc_free_payload(mobj);

  return ret;
}

void *syscall_tzvfs_mmap(int *tzvfs_errno, void *addr, size_t len, int prot, int flags, int fildes, off_t off){
  void* ret = (void *)-1;

  struct thread_param params[4];
  params[0] = THREAD_PARAM_VALUE(IN, TZVFS_RPC_FS_MMAP, 0, 0);
  params[1] = THREAD_PARAM_VALUE(IN, (int)addr, len, prot);
  params[2] = THREAD_PARAM_VALUE(IN, flags, fildes, off);
  params[3] = THREAD_PARAM_VALUE(OUT, 0, 0, 0);
  // 发起RPC调用
  if (TEE_SUCCESS == thread_rpc_cmd(OPTEE_MSG_RPC_CMD_TZVFS, 4, params)) {
    ret = (void*)((int)params[3].u.value.a);
    if (ret == (void *)-1) *tzvfs_errno = params[3].u.value.b;
  }  
  
  return ret;
}

void *syscall_tzvfs_mremap(int *tzvfs_errno, void *old_address, size_t old_size, size_t new_size, int flags){
  void* ret = (void *)-1;

  struct thread_param params[3];
  params[0] = THREAD_PARAM_VALUE(IN, TZVFS_RPC_FS_MREMAP, (int)old_address, old_size);
  params[1] = THREAD_PARAM_VALUE(IN, new_size, flags, 0);
  params[2] = THREAD_PARAM_VALUE(OUT, 0, 0, 0);
  // 发起RPC调用
  if (TEE_SUCCESS == thread_rpc_cmd(OPTEE_MSG_RPC_CMD_TZVFS, 3, params)) {
    ret = (void*)((int)params[2].u.value.a);
    if (ret == (void *)-1) *tzvfs_errno = params[2].u.value.b;
  }  

  return ret;
}

int syscall_tzvfs_munmap(int *tzvfs_errno, void *addr, size_t length){
  int ret = -1;

  struct thread_param params[2];
  params[0] = THREAD_PARAM_VALUE(IN, TZVFS_RPC_FS_MUNMAP, (int)addr, length);
  params[1] = THREAD_PARAM_VALUE(OUT, 0, 0, 0);
  // 发起RPC调用
  if (TEE_SUCCESS == thread_rpc_cmd(OPTEE_MSG_RPC_CMD_TZVFS, 2, params)) {
    ret = params[1].u.value.a;
    if (ret == -1) *tzvfs_errno = params[1].u.value.b;
  }  

  return ret;
}

size_t syscall_tzvfs_strcspn(int *tzvfs_errno, const char *str1, const char *str2){
  size_t ret = 0;
  tzvfs_errno = tzvfs_errno;

  size_t size1 = TZVFS_FS_NAME_MAX;
  size_t size2 = TZVFS_FS_NAME_MAX;
  struct thread_param params[4];
  struct mobj *mobj1 = NULL, *mobj2 = NULL;
  void *va1, *va2;
  // 分配共享内存
  mobj1 = thread_rpc_alloc_payload(size1);
  if (!mobj1) {
    *tzvfs_errno = TEE_ERROR_OUT_OF_MEMORY;
    return -1;
  }
  if (mobj1->size < size1) {
    *tzvfs_errno = TEE_ERROR_SHORT_BUFFER;
    thread_rpc_free_payload(mobj1);
    return -1;
  }
  mobj2 = thread_rpc_alloc_payload(size2);
  if (!mobj2) {
    *tzvfs_errno = TEE_ERROR_OUT_OF_MEMORY;
    return -1;
  }
  if (mobj2->size < size2) {
    *tzvfs_errno = TEE_ERROR_SHORT_BUFFER;
    thread_rpc_free_payload(mobj2);
    return -1;
  }
  // 获取分配的共享内存的虚拟地址
  va1 = mobj_get_va(mobj1, 0);
  va2 = mobj_get_va(mobj2, 0);
  memcpy(va1, str1, strlen(str1)+1);
  memcpy(va2, str2, strlen(str2)+1);
  // 初始RPC参数
  params[0] = THREAD_PARAM_VALUE(IN, TZVFS_RPC_FS_STRCSPN, 0, 0);
  params[1] = THREAD_PARAM_MEMREF(IN, mobj1, 0, size1);
  params[2] = THREAD_PARAM_MEMREF(IN, mobj2, 0, size2);
  params[3] = THREAD_PARAM_VALUE(OUT, 0, 0, 0);
  // 发起RPC调用
  if (TEE_SUCCESS == thread_rpc_cmd(OPTEE_MSG_RPC_CMD_TZVFS, 4, params)) {
    ret = params[3].u.value.a;
  }  
  thread_rpc_free_payload(mobj1);
  thread_rpc_free_payload(mobj2);
  
  return ret;
}

int syscall_tzvfs_utimes(int *tzvfs_errno, const char *filename, const struct tzvfs_timeval times[2]){
  int ret = -1;
  times = times;

  size_t size = TZVFS_FS_NAME_MAX;
  struct thread_param params[3];
  struct mobj *mobj = NULL;
  void *va;
  // 分配共享内存
  mobj = thread_rpc_alloc_payload(size);
  if (!mobj) {
    *tzvfs_errno = TEE_ERROR_OUT_OF_MEMORY;
    return -1;
  }
  if (mobj->size < size) {
    *tzvfs_errno = TEE_ERROR_SHORT_BUFFER;
    thread_rpc_free_payload(mobj);
    return -1;
  }
  // 获取分配的共享内存的虚拟地址
  va = mobj_get_va(mobj, 0);
  memcpy(va, filename, strlen(filename)+1);
  // 初始RPC参数
  params[0] = THREAD_PARAM_VALUE(IN, TZVFS_RPC_FS_UTIMES, 0, 0);
  params[1] = THREAD_PARAM_MEMREF(IN, mobj, 0, size);
  params[2] = THREAD_PARAM_VALUE(OUT, 0, 0, 0);
  // 发起RPC调用
  if (TEE_SUCCESS == thread_rpc_cmd(OPTEE_MSG_RPC_CMD_TZVFS, 3, params)) {
    ret = params[2].u.value.a;
    if (ret == -1) *tzvfs_errno = params[2].u.value.b;
  }
  thread_rpc_free_payload(mobj);
  
  return ret;
}

off_t syscall_tzvfs_lseek(int *tzvfs_errno, int fd, off_t offset, int whence){
  off_t ret = -1;

  struct thread_param params[3];
  params[0] = THREAD_PARAM_VALUE(IN, TZVFS_RPC_FS_LSEEK, 0, 0);
  params[1] = THREAD_PARAM_VALUE(IN, fd, offset, whence);
  params[2] = THREAD_PARAM_VALUE(OUT, 0, 0, 0);
  // 发起RPC调用
  if (TEE_SUCCESS == thread_rpc_cmd(OPTEE_MSG_RPC_CMD_TZVFS, 3, params)) {
    ret = params[2].u.value.a;
    if (ret == -1) *tzvfs_errno = params[2].u.value.b;
  } 

  return ret;
}

int syscall_tzvfs_fsync(int *tzvfs_errno, int fd){
  int ret = -1;

  struct thread_param params[2];
  params[0] = THREAD_PARAM_VALUE(IN, TZVFS_RPC_FS_FSYNC, fd, 0);
  params[1] = THREAD_PARAM_VALUE(OUT, 0, 0, 0);
  // 发起RPC调用
  if (TEE_SUCCESS == thread_rpc_cmd(OPTEE_MSG_RPC_CMD_TZVFS, 2, params)) {
    ret = params[1].u.value.a;
    if (ret == -1) *tzvfs_errno = params[1].u.value.b;
  } 

  return ret;
}

char* syscall_tzvfs_getenv(int *tzvfs_errno, const char *name){
  char* ret = NULL;
  tzvfs_errno = tzvfs_errno;

  unsigned int size = TZVFS_FS_NAME_MAX;
  struct thread_param params[3];
  struct mobj *mobj = NULL;
  void *va;
  // 分配共享内存
  mobj = thread_rpc_alloc_payload(size);
  if (!mobj) {
    *tzvfs_errno = TEE_ERROR_OUT_OF_MEMORY;
    return NULL;
  }
  if (mobj->size < size) {
    *tzvfs_errno = TEE_ERROR_SHORT_BUFFER;
    thread_rpc_free_payload(mobj);
    return NULL;
  }
  // 获取分配的共享内存的虚拟地址
  va = mobj_get_va(mobj, 0);
  memcpy(va, name, strlen(name)+1);
  // 初始RPC参数
  params[0] = THREAD_PARAM_VALUE(IN, TZVFS_RPC_FS_GETENV, 0, 0);
  params[1] = THREAD_PARAM_MEMREF(IN, mobj, 0, size);
  params[2] = THREAD_PARAM_VALUE(OUT, 0, 0, 0);
  // 发起RPC调用
  if (TEE_SUCCESS == thread_rpc_cmd(OPTEE_MSG_RPC_CMD_TZVFS, 3, params)) {
    ret = (char*)((int)params[2].u.value.a);
  }
  thread_rpc_free_payload(mobj);
  
  return ret;
}

pid_t syscall_tzvfs_getpid(int *tzvfs_errno){
  pid_t ret = -1;

  struct thread_param params[2];
  params[0] = THREAD_PARAM_VALUE(IN, TZVFS_RPC_FS_GETPID, 0, 0);
  params[1] = THREAD_PARAM_VALUE(OUT, 0, 0, 0);
  // 发起RPC调用
  if (TEE_SUCCESS == thread_rpc_cmd(OPTEE_MSG_RPC_CMD_TZVFS, 2, params)) {
    ret = params[1].u.value.a;
    if (ret == -1) *tzvfs_errno = params[1].u.value.b;
  } 

  return ret;
}

time_t syscall_tzvfs_time(int *tzvfs_errno, time_t *t){
  time_t ret = -1;

  struct thread_param params[2];
  params[0] = THREAD_PARAM_VALUE(IN, TZVFS_RPC_FS_TIME, 0, 0);
  params[1] = THREAD_PARAM_VALUE(OUT, 0, 0, 0);
  // 发起RPC调用
  if (TEE_SUCCESS == thread_rpc_cmd(OPTEE_MSG_RPC_CMD_TZVFS, 2, params)) {
    *t = params[1].u.value.a;
    ret = params[1].u.value.a;
    if (ret == -1) *tzvfs_errno = params[1].u.value.b;
  } 

  return ret;
}

unsigned int syscall_tzvfs_sleep(int *tzvfs_errno, unsigned int seconds){
  unsigned int ret = 0;
  tzvfs_errno = tzvfs_errno;

  struct thread_param params[2];
  params[0] = THREAD_PARAM_VALUE(IN, TZVFS_RPC_FS_SLEEP, seconds, 0);
  params[1] = THREAD_PARAM_VALUE(OUT, 0, 0, 0);
  // 发起RPC调用
  if (TEE_SUCCESS == thread_rpc_cmd(OPTEE_MSG_RPC_CMD_TZVFS, 2, params)) {
    ret = params[1].u.value.a;
  } 

  return ret;
}

int syscall_tzvfs_gettimeofday(int *tzvfs_errno, struct tzvfs_timeval *tv, struct tzvfs_timezone *tz){
  int ret = -1;
  tz = tz;

  size_t size = sizeof(struct tzvfs_timeval);
  struct thread_param params[3];
  struct mobj *mobj = NULL;
  void *va;
  // 分配共享内存
  mobj = thread_rpc_alloc_payload(size);
  if (!mobj) {
    *tzvfs_errno = TEE_ERROR_OUT_OF_MEMORY;
    return -1;
  }
  if (mobj->size < size) {
    *tzvfs_errno = TEE_ERROR_SHORT_BUFFER;
    thread_rpc_free_payload(mobj);
    return -1;
  }
  // 获取分配的共享内存的虚拟地址
  va = mobj_get_va(mobj, 0);
  // 初始RPC参数
  params[0] = THREAD_PARAM_VALUE(IN, TZVFS_RPC_FS_GETTIMEOFDAY, 0, 0);
  params[1] = THREAD_PARAM_MEMREF(OUT, mobj, 0, size);
  params[2] = THREAD_PARAM_VALUE(OUT, 0, 0, 0);
  // 发起RPC调用
  if (TEE_SUCCESS == thread_rpc_cmd(OPTEE_MSG_RPC_CMD_TZVFS, 3, params)) {
    memcpy(tv, va, size);
    ret = params[2].u.value.a;
    if (ret == -1) *tzvfs_errno = params[2].u.value.b;
  }
  thread_rpc_free_payload(mobj);

  return ret;
}

int syscall_tzvfs_fchown(int *tzvfs_errno, int fd, uid_t owner, gid_t group){
  int ret = -1;

  struct thread_param params[3];
  params[0] = THREAD_PARAM_VALUE(IN, TZVFS_RPC_FS_FCHOWN, 0, 0);
  params[1] = THREAD_PARAM_VALUE(IN, fd, owner, group);
  params[2] = THREAD_PARAM_VALUE(OUT, 0, 0, 0);
  // 发起RPC调用
  if (TEE_SUCCESS == thread_rpc_cmd(OPTEE_MSG_RPC_CMD_TZVFS, 3, params)) {
    ret = params[2].u.value.a;
    if (ret == -1) *tzvfs_errno = params[2].u.value.b;
  } 
  
  return ret;
}

/*
int tzvfs_ftruncate(int *tzvfs_errno, int fd, off_t length){
  DMSG("%s: haven't been realized!\n", __func__);
  return 0;
}

int tzvfs_fchmod(int *tzvfs_errno, int fd, mode_t mode){
  DMSG("%s: haven't been realized!\n", __func__);
  return 0;
}

void *tzvfs_dlopen(int *tzvfs_errno, const char *filename, int flag){
  DMSG("%s: haven't been realized!\n", __func__);
  return NULL;
}

char *tzvfs_dlerror(int *tzvfs_errno){
  DMSG("%s: haven't been realized!\n", __func__);
  return NULL;
}

void *tzvfs_dlsym(int *tzvfs_errno, void *handle, const char *symbol){
  DMSG("%s: haven't been realized!\n", __func__);
  return NULL;
}

int tzvfs_dlclose(int *tzvfs_errno, void *handle){
  DMSG("%s: haven't been realized!\n", __func__);
  return 0;
}

int tzvfs_mkdir(int *tzvfs_errno, const char *pathname, mode_t mode) {
  DMSG("%s: haven't been realized!\n", __func__);
  return 0;
}

int tzvfs_rmdir(int *tzvfs_errno, const char *pathname){
  DMSG("%s: haven't been realized!\n", __func__);
  return 0;
}

ssize_t tzvfs_readlink(int *tzvfs_errno, const char *path, char *buf, size_t bufsiz){
  DMSG("%s: haven't been realized!\n", __func__);
  return 0;
}

long int tzvfs_sysconf(int *tzvfs_errno, int name){
  long int ret = -1;
  DMSG("%s: haven't been realized!\n", __func__);
  return ret;
}

struct tzvfs_tm *tzvfs_localtime(int *tzvfs_errno, const time_t *timep){
  struct tzvfs_tm * ret = NULL;
  DMSG("%s: haven't been realized!\n", __func__);
  return ret;
}
*/
