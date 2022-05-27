delete all include file which optee dosen't support
<time.h>                21134 #include <time.h> -> // #include <time.h>
<math.h>                30423 #include <math.h> -> // #include <math.h>
<sys/types.h>           32588 #include <sys/types.h> -> // #include <sys/types.h>
<sys/stat.h>            32589 #include <sys/stat.h> -> // #include <sys/stat.h>
<fcntl.h>               32590 #include <fcntl.h> -> // #include <fcntl.h>
<sys/ioctl.h>           32591 #include <sys/ioctl.h> -> // #include <sys/ioctl.h>
<unistd.h>              32592 #include <unistd.h> -> // #include <unistd.h> 
<sys/time.h>            32594 #include <sys/time.h> -> // #include <sys/time.h>
<sys/mman.h>            32597 # include <sys/mman.h> -> // # include <sys/mman.h>

114691 zString += strcspn((const char*)zString, zStop); -> zString += tzvfs_strcspn((const char*)zString, zStop);
34989 utimes(zLockFile, NULL); -> tzvfs_utimes(zLockFile, NULL);
35998 newOffset = lseek(id->h, offset, SEEK_SET); -> newOffset = tzvfs_lseek(id->h, offset, SEEK_SET);
36110 i64 iSeek = lseek(fd, iOff, SEEK_SET); -> i64 iSeek = tzvfs_lseek(fd, iOff, SEEK_SET);
33961 zErr = strerror(iErrno); -> zErr = "frank surprise error";
36240 # define fdatasync fsync -> # define fdatasync tzvfs_fsync
36905 return (int)sysconf(_SC_PAGESIZE); -> return 4096;
38385 if( !azDirs[0] ) azDirs[0] = getenv("SQLITE_TMPDIR"); -> if( !azDirs[0] ) azDirs[0] = tzvfs_getenv("SQLITE_TMPDIR");
38386 if( !azDirs[1] ) azDirs[1] = getenv("TMPDIR"); -> if( !azDirs[1] ) azDirs[1] = tzvfs_getenv("TMPDIR"); 
32685 #define osGetpid(X) (pid_t)getpid() -> #define osGetpid(X) (pid_t)tzvfs_getpid()
36177 time(&t); -> tzvfs_time(&t);
39246 time(&t); -> tzvfs_time(&t);
39216 sleep(seconds); -> tzvfs_sleep(seconds);
39254 (void)gettimeofday(&sNow, 0); -> (void)tzvfs_gettimeofday(&sNow, 0);
21606 pX = localtime(t); -> pX = tzvfs_localtime(t);

stat -> tzvfs_stat
33097 #define osStat      ((int(*)(const char*,struct stat*))aSyscall[4].pCurrent) -> #define osStat      ((int(*)(const char*,struct tzvfs_stat*))aSyscall[4].pCurrent)
33110 #define osFstat     ((int(*)(int,struct stat*))aSyscall[5].pCurrent) -> #define osFstat     ((int(*)(int,struct tzvfs_stat*))aSyscall[5].pCurrent)
33230 #define osLstat      ((int(*)(const char*,struct stat*))aSyscall[27].pCurrent) -> #define osLstat      ((int(*)(const char*,struct tzvfs_stat*))aSyscall[27].pCurrent)
33391 struct stat statbuf; -> struct tzvfs_stat statbuf;
34062 struct stat statbuf; -> struct tzvfs_stat statbuf;
34151 struct stat buf; -> struct tzvfs_stat buf;
34169 struct stat buf; -> struct tzvfs_stat buf;
36509 struct stat buf; -> struct tzvfs_stat buf;
36548 struct stat buf; -> struct tzvfs_stat buf;
37799 struct stat statbuf; -> struct tzvfs_stat statbuf;
38377 struct stat buf; -> struct tzvfs_stat buf;
38460 struct stat sStat; -> struct tzvfs_stat sStat;
38507 struct stat sStat; -> struct tzvfs_stat sStat;
38954 struct stat buf; -> struct tzvfs_stat buf;
39028 struct stat buf; -> struct tzvfs_stat buf;

flock -> tzvfs_flock
34221 struct flock lock; -> struct tzvfs_flock lock;
34298 static int unixFileLock(unixFile *pFile, struct flock *pLock){ -> static int unixFileLock(unixFile *pFile, struct tzvfs_flock *pLock){
34305 struct flock lock; -> struct tzvfs_flock lock;
34391 struct flock lock; -> struct tzvfs_flock lock;
34601 struct flock lock; -> struct tzvfs_flock lock;

timeval -> tzvfs_timeval
39248 struct timeval sNow; -> struct tzvfs_timeval sNow;

tm -> tzvfs_tm
21598 static int osLocaltime(time_t *t, struct tm *pTm){ -> static int osLocaltime(time_t *t, struct tzvfs_tm *pTm){
21601 struct tm *pX; -> struct tzvfs_tm *pX;
21644 struct tm sLocal; -> struct tzvfs_tm sLocal;