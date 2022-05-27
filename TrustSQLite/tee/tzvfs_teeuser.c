#include <tee_internal_api.h>
#include "tzvfs_teeuser.h"

int tzvfs_errno;

static TEE_Time start, end;

// 打开文件. 调用成功时返回一个文件描述符fd, 调用失败时返回-1, 并修改errno
int tzvfs_open(const char *filename, int flags, mode_t mode){
  int ret = -1;
  
  TEE_GetREETime(&start);
  ret = utee_tzvfs_open(&tzvfs_errno, filename, flags, mode);
  TEE_GetREETime(&end);
  // DMSG("has been called, filename=%s, flags=%d, mode=%d, ret=%d, tzvfs_errno=%d", filename, flags, mode, ret, tzvfs_errno);
  DMSG("time: %d ms", 1000*(end.seconds-start.seconds) + (end.millis-start.millis));
  return ret;
}

// 若文件顺利关闭则返回0, 发生错误时返回-1
int tzvfs_close(int fd){
  int ret = -1;
  
  TEE_GetREETime(&start);
  ret = utee_tzvfs_close(&tzvfs_errno, fd);
  TEE_GetREETime(&end);
  // DMSG("has been called, fd=%d, ret=%d, tzvfs_errno=%d", fd, ret, tzvfs_errno);
  DMSG("time: %d ms", 1000*(end.seconds-start.seconds) + (end.millis-start.millis));
  return ret;
}

// 获取当前工作目录, 成功则返回当前工作目录; 如失败返回NULL, 错误代码存于errno
char *tzvfs_getcwd(char *buf, size_t size){
  char* ret = NULL;
  
  TEE_GetREETime(&start);
  ret = utee_tzvfs_getcwd(&tzvfs_errno, buf, size);
  TEE_GetREETime(&end);
  // DMSG("has been called, buf=%x, size=%d, ret=%d, tzvfs_errno=%d", buf, size, ret, tzvfs_errno);
  DMSG("time: %d ms", 1000*(end.seconds-start.seconds) + (end.millis-start.millis));
  return ret;
}

// 获取一些文件相关的信息, 成功执行时，返回0。失败返回-1，errno
// lstat函数是不穿透（不追踪）函数，对软链接文件进行操作时，操作的是软链接文件本身
int tzvfs_lstat( const char* path, struct tzvfs_stat *buf ) {
  int ret = -1;

  TEE_GetREETime(&start);
  ret = utee_tzvfs_lstat(&tzvfs_errno, path, buf);
  TEE_GetREETime(&end);
  // DMSG("has been called, path=%s, buf=%x, ret=%d, sizeof(struct tzvfs_flock)=%d, tzvfs_errno=%d", path, buf, ret, sizeof(struct tzvfs_flock), tzvfs_errno);
  DMSG("time: %d ms", 1000*(end.seconds-start.seconds) + (end.millis-start.millis));
  return ret;
}

// 获取一些文件相关的信息, 成功执行时，返回0。失败返回-1，errno
// stat函数是穿透（追踪）函数，即对软链接文件进行操作时，操作的是链接到的那一个文件，不是软链接文件本身
int tzvfs_stat(const char *path, struct tzvfs_stat *buf){
  int ret = -1;

  TEE_GetREETime(&start);
  ret = utee_tzvfs_stat(&tzvfs_errno, path, buf);
  TEE_GetREETime(&end);
  // DMSG("has been called, path=%s, buf=%x, ret=%d, tzvfs_errno=%d", path, buf, ret, tzvfs_errno);
  DMSG("time: %d ms", 1000*(end.seconds-start.seconds) + (end.millis-start.millis));
  return ret;
}

// fstat函数与stat函数的功能一样，只是第一个形参是文件描述符
int tzvfs_fstat(int fd, struct tzvfs_stat *buf){
  int ret = -1;

  TEE_GetREETime(&start);
  ret = utee_tzvfs_fstat(&tzvfs_errno, fd, buf);
  TEE_GetREETime(&end);
  // DMSG("has been called, fd=%d, buf=%x, ret=%d, tzvfs_errno=%d", fd, buf, ret, tzvfs_errno);
  DMSG("time: %d ms", 1000*(end.seconds-start.seconds) + (end.millis-start.millis));
  return ret;
}

// 通过fcntl可以改变已打开的文件性质, F_SETLK 设置文件锁定的状态
// fcntl的返回值与命令有关。如果出错，所有命令都返回-1，如果成功则返回某个其他值。
int tzvfs_fcntl(int fd, int cmd, ... /* arg */ ){
  int ret = -1;
  
  // Read one argument
  va_list valist;
  va_start(valist, cmd);
  struct tzvfs_flock* arg = va_arg(valist, struct tzvfs_flock*);
  va_end(valist);
  
  TEE_GetREETime(&start);
  ret = utee_tzvfs_fcntl(&tzvfs_errno, fd, cmd, arg);
  TEE_GetREETime(&end);
  // DMSG("has been called, fd=%d, cmd=%d, arg=%x, ret=%d, tzvfs_errno=%d", fd, cmd, arg, ret, sizeof(struct tzvfs_flock), tzvfs_errno);
  DMSG("time: %d ms", 1000*(end.seconds-start.seconds) + (end.millis-start.millis));
  return ret;
}

// read会把参数fd所指的文件传送count个字节到buf指针所指的内存中
// 返回值为实际读取到的字节数, 如果返回0, 表示已到达文件尾或是无可读取的数据
// 当有错误发生时则返回-1, 错误代码存入errno 中
ssize_t tzvfs_read(int fd, void *buf, size_t count){
  ssize_t ret = -1;

  TEE_GetREETime(&start);
  ret = utee_tzvfs_read(&tzvfs_errno, fd, buf, count);
  TEE_GetREETime(&end);
  // DMSG("has been called, fd=%d, buf=%x, count=%d, ret=%d, tzvfs_errno=%d", fd, buf, count, ret, tzvfs_errno);
  DMSG("time: %d ms", 1000*(end.seconds-start.seconds) + (end.millis-start.millis));
  return ret;
}

pid_t tzvfs_getpid(void){
  pid_t ret = -1;

  TEE_GetREETime(&start);
  ret = utee_tzvfs_getpid(&tzvfs_errno);
  TEE_GetREETime(&end);
  // DMSG("has been called, ret=%d, tzvfs_errno=%d", ret, tzvfs_errno);
  DMSG("time: %d ms", 1000*(end.seconds-start.seconds) + (end.millis-start.millis));
  return ret;
}

// geteuid()用来取得执行目前进程有效的用户识别码
// 返回有效的用户识别码
uid_t tzvfs_geteuid(void){
  uid_t ret = -1;

  TEE_GetREETime(&start);
  ret = utee_tzvfs_geteuid(&tzvfs_errno);
  TEE_GetREETime(&end);
  // DMSG("has been called, ret=%d, tzvfs_errno=%d", ret, tzvfs_errno);
  DMSG("time: %d ms", 1000*(end.seconds-start.seconds) + (end.millis-start.millis));
  return ret;
}

off_t tzvfs_lseek(int fd, off_t offset, int whence){
  off_t ret = -1;

  TEE_GetREETime(&start);
  ret = utee_tzvfs_lseek(&tzvfs_errno, fd, offset, whence);
  TEE_GetREETime(&end);
  // DMSG("has been called, fd=%d, offset=%d, whence=%d, ret=%d, tzvfs_errno=%d", fd, offset, whence, ret, tzvfs_errno);
  DMSG("time: %d ms", 1000*(end.seconds-start.seconds) + (end.millis-start.millis));
  return ret;
}

// write函数把buf中nbyte写入文件描述符handle所指的文档
// 成功时返回写的字节数，错误时返回-1
ssize_t tzvfs_write(int fd, const void *buf, size_t count){
  ssize_t ret = -1;

  // DMSG("has been called");
  ret = utee_tzvfs_write(&tzvfs_errno, fd, buf, count);
  
  return ret;
}

int tzvfs_unlink(const char *pathname){
  int ret = -1;
  
  // DMSG("has been called");
  ret = utee_tzvfs_unlink(&tzvfs_errno, pathname);

  return ret;
}

int tzvfs_access(const char *pathname, int mode){
  int ret = -1;

  // DMSG("has been called");
  ret = utee_tzvfs_access(&tzvfs_errno, pathname, mode);

  return ret;
}

void *tzvfs_mmap(void *addr, size_t len, int prot, int flags, int fildes, off_t off){
  void* ret = (void *)-1;

  // DMSG("has been called");
  ret =  utee_tzvfs_mmap(&tzvfs_errno, addr, len, prot, flags, fildes, off);
  
  return ret;
}

void *tzvfs_mremap(void *old_address, size_t old_size, size_t new_size, int flags, ... /* void *new_address */){
  void* ret = (void *)-1;

  // DMSG("has been called");
  ret =  utee_tzvfs_mremap(&tzvfs_errno, old_address, old_size, new_size, flags);

  return ret;
}

int tzvfs_munmap(void *addr, size_t length){
  int ret = -1;

  // DMSG("has been called");
  ret = utee_tzvfs_munmap(&tzvfs_errno, addr, length);

  return ret;
}

size_t tzvfs_strcspn(const char *str1, const char *str2){
  size_t ret = 0;

  // DMSG("has been called");
  ret = utee_tzvfs_strcspn(&tzvfs_errno, str1, str2);
  
  return ret;
}

int tzvfs_utimes(const char *filename, const struct tzvfs_timeval times[2]){
  int ret = -1;

  // DMSG("has been called");
  ret = utee_tzvfs_utimes(&tzvfs_errno, filename, times);
  
  return ret;
}

int tzvfs_fsync(int fd){
  int ret = -1;

  // DMSG("has been called");
  ret = utee_tzvfs_fsync(&tzvfs_errno, fd);

  return ret;
}

char* tzvfs_getenv(const char *name){
  char* ret = NULL;

  // DMSG("has been called");
  ret = utee_tzvfs_getenv(&tzvfs_errno, name);
  
  return ret;
}

time_t tzvfs_time(time_t *t){
  time_t ret = -1;

  // DMSG("has been called");
  ret =  utee_tzvfs_time(&tzvfs_errno, t);

  return ret;
}

unsigned int tzvfs_sleep(unsigned int seconds){
  unsigned int ret;

  // DMSG("has been called");
  ret = utee_tzvfs_sleep(&tzvfs_errno, seconds);

  return ret;
}

int tzvfs_gettimeofday(struct tzvfs_timeval *tv, struct tzvfs_timezone *tz){
  int ret = -1;

  // DMSG("has been called");
  ret = utee_tzvfs_gettimeofday(&tzvfs_errno, tv, tz);

  return ret;
}

int tzvfs_fchown(int fd, uid_t owner, gid_t group){
  int ret = -1;

  // DMSG("has been called");
  ret = utee_tzvfs_fchown(&tzvfs_errno, fd, owner, group);
  
  return ret;
}

int tzvfs_ftruncate(int fd, off_t length){
  // DMSG("%s: haven't been realized!\n", __func__);
  return 0;
}

int tzvfs_fchmod(int fd, mode_t mode){
  // DMSG("%s: haven't been realized!\n", __func__);
  return 0;
}

void *tzvfs_dlopen(const char *filename, int flag){
  // DMSG("%s: haven't been realized!\n", __func__);
  return NULL;
}

char *tzvfs_dlerror(void){
  // DMSG("%s: haven't been realized!\n", __func__);
  return NULL;
}

void *tzvfs_dlsym(void *handle, const char *symbol){
  // DMSG("%s: haven't been realized!\n", __func__);
  return NULL;
}

int tzvfs_dlclose(void *handle){
  // DMSG("%s: haven't been realized!\n", __func__);
  return 0;
}

int tzvfs_mkdir(const char *pathname, mode_t mode) {
  // DMSG("%s: haven't been realized!\n", __func__);
  return 0;
}

int tzvfs_rmdir(const char *pathname){
  // DMSG("%s: haven't been realized!\n", __func__);
  return 0;
}

ssize_t tzvfs_readlink(const char *path, char *buf, size_t bufsiz){
  // DMSG("%s: haven't been realized!\n", __func__);
  return 0;
}

long int tzvfs_sysconf(int name){
  long int ret = -1;
  // DMSG("%s: haven't been realized!\n", __func__);
  return ret;
}

struct tzvfs_tm *tzvfs_localtime(const time_t *timep){
  struct tzvfs_tm * ret = NULL;
  // DMSG("%s: haven't been realized!\n", __func__);
  return ret;
}
