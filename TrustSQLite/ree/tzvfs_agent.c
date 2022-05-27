#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/time.h>  // utimes() and gettimeofday()
#include <stdarg.h>  // for variable arguments functions
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#define __USE_GNU 1 // remove warning of mremap
#include <sys/mman.h>
#include <time.h>  // time() and localtime()
#include "tzvfs.h"

int tzvfs_errno;

// 打开文件. 调用成功时返回一个文件描述符fd, 调用失败时返回-1, 并修改errno
int tzvfs_open(const char *filename, int flags, mode_t mode){
  int ret = -1;
  ret = open(filename, flags, mode);
  printf("DMSG: call %s, res = %d, flags=%x, mode=%x, %s.\n", __func__, ret, flags, mode, strerror(errno));
  if (ret == -1) tzvfs_errno = errno;
  return ret;
}

// 若文件顺利关闭则返回0, 发生错误时返回-1
int tzvfs_close(int fd){
  int ret = -1;
  ret = close(fd);
  printf("DMSG: call %s, res = %d, %s.\n", __func__, ret, strerror(errno));
  if (ret == -1) tzvfs_errno = errno;
  return ret;
}

// 获取当前工作目录, 成功则返回当前工作目录; 如失败返回NULL, 错误代码存于errno
char *tzvfs_getcwd(char *buf, size_t size){
  char* ret = NULL;
  ret = getcwd(buf, size);
  printf("DMSG: call %s, res = %s, %s.\n", __func__, ret, strerror(errno));
  if (ret == NULL) tzvfs_errno = errno;
  return ret;
}

// 获取一些文件相关的信息, 成功执行时，返回0。失败返回-1，errno
// lstat函数是不穿透（不追踪）函数，对软链接文件进行操作时，操作的是软链接文件本身
int tzvfs_lstat( const char* path, struct tzvfs_stat *buf ) {
  int ret = -1;
  ret = lstat(path, (struct stat*)buf);
  printf("DMSG: call %s, res = %d, path = %s, %s.\n", __func__, ret, path, strerror(errno));
  if (ret == -1) { tzvfs_errno = errno; errno = 0; }
  return ret;
}

// 获取一些文件相关的信息, 成功执行时，返回0。失败返回-1，errno
// stat函数是穿透（追踪）函数，即对软链接文件进行操作时，操作的是链接到的那一个文件，不是软链接文件本身
int tzvfs_stat(const char *path, struct tzvfs_stat *buf){
  int ret = -1;
  ret = stat(path, (struct stat*)buf);
  printf("DMSG: call %s, res = %d, path = %s, %s.\n", __func__, ret, path, strerror(errno));
  if (ret == -1) { tzvfs_errno = errno; errno = 0; }
  return ret;
}

// fstat函数与stat函数的功能一样，只是第一个形参是文件描述符
int tzvfs_fstat(int fd, struct tzvfs_stat *buf){
  int ret = -1;
  ret = fstat(fd, (struct stat*)buf);
  printf("DMSG: call %s, res = %d, fd = %d, %s.\n", __func__, ret, fd, strerror(errno));
  if (ret == -1) { tzvfs_errno = errno; errno = 0; }
  return ret;
}

// 通过fcntl可以改变已打开的文件性质, F_SETLK 设置文件锁定的状态
// fcntl的返回值与命令有关。如果出错，所有命令都返回－1，如果成功则返回某个其他值。
int tzvfs_fcntl(int fd, int cmd, ... /* arg */ ){
  int ret = -1;
  
  // Read one argument
  va_list valist;
  va_start(valist, cmd);
  struct flock* arg = va_arg(valist, struct flock*);
  va_end(valist);
  
  ret = fcntl(fd, cmd, arg);
  printf("DMSG: call %s, res = %d, fd = %d, cmd = %d, arg = %x, %s.\n", __func__, ret, fd, cmd, arg, strerror(errno));
  if (ret == -1) tzvfs_errno = errno;
  return ret;
}

// read会把参数fd所指的文件传送count个字节到buf指针所指的内存中
// 返回值为实际读取到的字节数, 如果返回0, 表示已到达文件尾或是无可读取的数据
// 当有错误发生时则返回-1, 错误代码存入errno 中
ssize_t tzvfs_read(int fd, void *buf, size_t count){
  ssize_t ret = -1;
  ret = read(fd, buf, count);
  printf("DMSG: call %s, res = %d, count = %d, %s.\n", __func__, ret, count, strerror(errno));
  if (ret == -1) tzvfs_errno = errno;
  return ret;
}

// write函数把buf中nbyte写入文件描述符handle所指的文档
// 成功时返回写的字节数，错误时返回-1
ssize_t tzvfs_write(int fd, const void *buf, size_t count){
  ssize_t ret = -1;
  ret = write(fd, buf, count);
  printf("DMSG: call %s, res = %d, %s.\n", __func__, ret, strerror(errno));
  if (ret == -1) tzvfs_errno = errno;
  return ret;
}

// geteuid()用来取得执行目前进程有效的用户识别码
// 返回有效的用户识别码
uid_t tzvfs_geteuid(void){
  uid_t ret = -1;
  ret = geteuid();
  printf("DMSG: call %s, res = %d, %s.\n", __func__, ret, strerror(errno));
  if (ret == -1) tzvfs_errno = errno;
  return ret;
}

int tzvfs_unlink(const char *pathname){
  int ret = -1;
  ret = unlink(pathname);
  printf("DMSG: %s, res = %d, %s.\n", __func__, ret, strerror(errno));
  if (ret == -1) tzvfs_errno = errno;
  return ret;
}

int tzvfs_access(const char *pathname, int mode){
  int ret = -1;
  ret = access(pathname, mode);
  //printf("DMSG: %s, res = %d, %s.\n", __func__, ret, strerror(errno));
  if (ret == -1) tzvfs_errno = errno;
  return ret;
}

void *tzvfs_mmap(void *addr, size_t len, int prot, int flags, int fildes, off_t off){
  void* ret = (void *)-1;
  ret = mmap(addr, len, prot, flags, fildes, off);
  printf("DMSG: %s, res = %x, %s.\n", __func__, ret, strerror(errno));
  if (ret == (void *)-1) tzvfs_errno = errno;
  return ret;
}

void *tzvfs_mremap(void *old_address, size_t old_size, size_t new_size, int flags, ... /* void *new_address */){
  void* ret = (void *)-1;
  // Read one argument
  va_list valist;
  va_start(valist, flags);
  void* new_address = va_arg(valist, void*);
  va_end(valist);
  ret = mremap(old_address, old_size, new_size, flags, new_address);
  printf("DMSG: %s, res = %x, %s.\n", __func__, ret, strerror(errno));
  if (ret == (void *)-1) tzvfs_errno = errno;
  return ret;
}

int tzvfs_munmap(void *addr, size_t length){
  int ret = -1;
  ret = munmap(addr, length);
  printf("DMSG: %s, res = %d, %s.\n", __func__, ret, strerror(errno));
  if (ret == -1) tzvfs_errno = errno;
  return ret;
}

size_t tzvfs_strcspn(const char *str1, const char *str2){
  size_t ret = 0;
  ret = strcspn(str1, str2);
  printf("DMSG: %s, res = %d.\n", __func__, ret);
  return ret;
}

int tzvfs_utimes(const char *filename, const struct tzvfs_timeval times[2]){
  int ret = -1;
  ret = utimes(filename, (struct timeval*)times);
  printf("DMSG: %s, res = %d, %s.\n", __func__, ret, strerror(errno));
  if (ret == -1) tzvfs_errno = errno;
  return ret;
}

off_t tzvfs_lseek(int fd, off_t offset, int whence){
  off_t ret = -1;
  ret = lseek(fd, offset, whence);
  printf("DMSG: %s, fd = %d, offset = %d, res = %d, %s.\n", __func__, fd, offset, ret, strerror(errno));
  if (ret == -1) tzvfs_errno = errno;
  return ret;
}

int tzvfs_fsync(int fd){
  int ret = -1;
  ret = fsync(fd);
  printf("DMSG: %s, res = %d, %s.\n", __func__, ret, strerror(errno));
  if (ret == -1) tzvfs_errno = errno;
  return ret;
}

long int tzvfs_sysconf(int name){
  long int ret = -1;
  ret = sysconf(name);
  printf("DMSG: %s, res = %d, %s.\n", __func__, ret, strerror(errno));
  if (ret == -1) tzvfs_errno = errno;
  return ret;
}

char* tzvfs_getenv(const char *name){
  char* ret = NULL;
  ret = getenv(name);
  printf("DMSG: %s, res = %s, %s.\n", __func__, ret, strerror(errno));
  return ret;
}

pid_t tzvfs_getpid(void){
  pid_t ret = -1;
  ret = getpid();
  printf("DMSG: %s, res = %d, %s.\n", __func__, ret, strerror(errno));
  if (ret == -1) tzvfs_errno = errno;
  return ret;
}

time_t tzvfs_time(time_t *t){
  time_t ret = -1;
  ret = time(t);
  printf("DMSG: %s, res = %d, %s.\n", __func__, ret, strerror(errno));
  if (ret == -1) tzvfs_errno = errno;
  return ret;
}

unsigned int tzvfs_sleep(unsigned int seconds){
  unsigned int ret;
  ret = sleep(seconds);
  printf("DMSG: %s, res = %d, %s.\n", __func__, ret, strerror(errno));
  return ret;
}

int tzvfs_gettimeofday(struct tzvfs_timeval *tv, struct tzvfs_timezone *tz){
  int ret = -1;
  ret = gettimeofday((struct timeval*)tv, (struct timezone*)tz);
  printf("DMSG: %s, res = %d, %s.\n", __func__, ret, strerror(errno));
  if (ret == -1) tzvfs_errno = errno;
  return ret;
}

struct tzvfs_tm *tzvfs_localtime(const time_t *timep){
  struct tzvfs_tm * ret = NULL;
  ret = (struct tzvfs_tm*)localtime(timep);
  printf("DMSG: %s, res = %x, %s.\n", __func__, ret, strerror(errno));
  if (ret == NULL) tzvfs_errno = errno;
  return ret;
}

int tzvfs_fchown(int fd, uid_t owner, gid_t group){
  int ret = -1;
  ret = fchown(fd, owner, group);
  printf("DMSG: %s, res = %d, %s.\n", __func__, ret, strerror(errno));
  if (ret == -1) tzvfs_errno = errno;
  return 0;
}

int tzvfs_ftruncate(int fd, off_t length){
  int ret = 0;

  printf("%s\n", __func__);
  tzvfs_errno = errno;
  return ret;
}

int tzvfs_fchmod(int fd, mode_t mode){
  printf("%s\n", __func__);
  tzvfs_errno = errno;
  return 0;
}

void *tzvfs_dlopen(const char *filename, int flag){
  printf("%s\n", __func__);
  tzvfs_errno = errno;
  return NULL;
}

char *tzvfs_dlerror(void){
  printf("%s\n", __func__);
  tzvfs_errno = errno;
  return NULL;
}

void *tzvfs_dlsym(void *handle, const char *symbol){
  printf("%s\n", __func__);
  tzvfs_errno = errno;
}

int tzvfs_dlclose(void *handle){
  printf("%s\n", __func__);
  tzvfs_errno = errno;
  return 0;
}

int tzvfs_mkdir(const char *pathname, mode_t mode) {
  printf("%s\n", __func__);
  tzvfs_errno = errno;
  return 0;
}

int tzvfs_rmdir(const char *pathname){
  printf("%s\n", __func__);
  tzvfs_errno = errno;
  return 0;
}

ssize_t tzvfs_readlink(const char *path, char *buf, size_t bufsiz){
  printf("%s\n", __func__);
  tzvfs_errno = errno;
  return 0;
}
