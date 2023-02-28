#define _GNU_SOURCE
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <sys/ioctl.h>
#include <sys/file.h>
#ifdef HAVE_XATTR
#include <sys/xattr.h>
#endif /* HAVE_XATTR */

#ifdef linux
/* For pread()/pwrite()/utimensat() */
#define _XOPEN_SOURCE 700
#endif

#define ERRNO_NOOP -999

#include "unreliablefs_ops.h"

const char *fuse_op_name[] = {
    "getattr",
    "readlink",
    "mknod",
    "mkdir",
    "unlink",
    "rmdir",
    "symlink",
    "rename",
    "link",
    "chmod",
    "chown",
    "truncate",
    "open",
    "read",
    "write",
    "statfs",
    "flush",
    "release",
    "fsync",
#ifdef HAVE_XATTR
    "setxattr",
    "getxattr",
    "listxattr",
    "removexattr",
#endif /* HAVE_XATTR */
    "opendir",
    "readdir",
    "releasedir",
    "fsyncdir",
    "access",
    "creat",
    "ftruncate",
    "fgetattr",
    "lock",
#if !defined(__OpenBSD__)
    "ioctl",
#endif /* __OpenBSD__ */
#ifdef HAVE_FLOCK
    "flock",
#endif /* HAVE_FLOCK */
#ifdef HAVE_FALLOCATE
    "fallocate",
#endif /* HAVE_FALLOCATE */
#ifdef HAVE_UTIMENSAT
    "utimens",
#endif /* HAVE_UTIMENSAT */
    "lstat"
};

extern int error_inject(const char* path, fuse_op operation);

void pushQueue(struct queueNode **q,  int op, const char* buf, size_t size, off_t offset, int fd){
    struct queueNode *newNode = (struct queueNode*)malloc(sizeof(struct queueNode));
    newNode->op = op;
    newNode->buf = buf;
    newNode->size = size;
    newNode->offset = offset;
    newNode->fd = fd;
    newNode->next = NULL;
    struct queueNode *ptr = *q; 
    if(*q == NULL)	*q = newNode;
    else
    {
        while(ptr->next != NULL) ptr = ptr->next;
        ptr->next = newNode;
    }
}

int findInQueue(struct queueNode **q,  int op){
	struct queueNode *ptr = *q;
	int index = 0;
	if(*q == NULL) {
        return -1;
    }
	while(ptr != NULL)
	{
		if(ptr->op == op) 
			return index;
		ptr = ptr->next;
		index++;
	}
	return -1;
}

void delFromQueue(struct queueNode **q, int index)
{
	struct queueNode *ptr = *q;
	struct queueNode *prev = NULL;
	int pos = 0;
	if(index == 0)
	{
		*q = (*q)->next;
		//free(ptr);
		return;
	}
	while(ptr!= NULL)
	{
		if(pos == index)
		{
			prev->next = ptr->next;
			//free(ptr);
			return;
		}
		prev = ptr;
		ptr = ptr->next;
		pos++;
	}
	return;
}

int sizeQueue(struct queueNode **q)
{
    struct queueNode *ptr = *q;
    int count = 0;
    if(ptr == NULL)
        return 0;
    while(ptr != NULL)
    {
        ptr = ptr->next;
        count++;
    }
    return count;
}

void clearQueue(struct queueNode **q)
{
    struct queueNode *ptr = *q;
    if(ptr == NULL)
        return;
    while (*q != NULL)
    {
        *q = (*q)->next;
        //free(ptr);
        ptr = *q;
    }

}
void printQueue(struct queueNode *q)
{
    struct queueNode *ptr = q;
    while(ptr!=NULL) {
        printf("%d,", ptr->op);
        ptr = ptr->next;
    }
    printf("\n");
}
struct queueNode* getValueQueue(struct queueNode **q, int index)
{
    struct queueNode *ptr = *q;
    if (ptr == NULL)
        return NULL;
    for(int i = 0; i < index; i++)
    {
        ptr = ptr->next;
    }
    return (ptr);
}

/*int getMinValueIndexQueue(struct queueNode **q)
{
    struct queueNode *ptr = *q;
    if (ptr == NULL)
        return -1;
    int min = getValueQueue(q, 0);
    int minIdx = 0;
    for(int i=0 ; ptr != NULL; ptr=ptr->next, i++)
        if(ptr->op < min)    
        {
            min = ptr->op;
            minIdx = i;
        }
    return minIdx;
}*/


int unreliable_lstat(const char *path, struct stat *buf)
{
    int ret = error_inject(path, OP_LSTAT);
    if (ret == -ERRNO_NOOP) {
        return 0;
    } else if (ret) {
        return ret;
    }

    memset(buf, 0, sizeof(struct stat));
    if (lstat(path, buf) == -1) {
        return -errno;
    }

    return 0;
}

int unreliable_getattr(const char *path, struct stat *buf)
{
    int ret = error_inject(path, OP_GETATTR);
    if (ret == -ERRNO_NOOP) {
        return 0;
    } else if (ret) {
        return ret;
    }

    memset(buf, 0, sizeof(struct stat));
    if (lstat(path, buf) == -1) {
        return -errno;
    }

    return 0;
}

int unreliable_readlink(const char *path, char *buf, size_t bufsiz)
{
    int ret = error_inject(path, OP_READLINK);
    if (ret == -ERRNO_NOOP) {
        return 0;
    } else if (ret) {
        return ret;
    }

    ret = readlink(path, buf, bufsiz);
    if (ret == -1) {
        return -errno;
    }
    buf[ret] = 0;

    return 0;
}

int unreliable_mknod(const char *path, mode_t mode, dev_t dev)
{
    int ret = error_inject(path, OP_MKNOD);
    if (ret == -ERRNO_NOOP) {
        return 0;
    } else if (ret) {
        return ret;
    }

    ret = mknod(path, mode, dev);    
    if (ret == -1) {
        return -errno;
    }

    return 0;
}

int unreliable_mkdir(const char *path, mode_t mode)
{
    int ret = error_inject(path, OP_MKDIR);
    if (ret == -ERRNO_NOOP) {
        return 0;
    } else if (ret) {
        return ret;
    }

    ret = mkdir(path, mode);
    if (ret == -1) {
        return -errno;
    }

    return 0;
}

int unreliable_unlink(const char *path)
{
    int ret = error_inject(path, OP_UNLINK);
    if (ret == -ERRNO_NOOP) {
        return 0;
    } else if (ret) {
        return ret;
    }

    ret = unlink(path); 
    if (ret == -1) {
        return -errno;
    }

    return 0;
}

int unreliable_rmdir(const char *path)
{
    int ret = error_inject(path, OP_RMDIR);
    if (ret == -ERRNO_NOOP) {
        return 0;
    } else if (ret) {
        return ret;
    }

    ret = rmdir(path); 
    if (ret == -1) {
        return -errno;
    }

    return 0;
}

int unreliable_symlink(const char *target, const char *linkpath)
{
    int ret = error_inject(target, OP_SYMLINK);
    if (ret == -ERRNO_NOOP) {
        return 0;
    } else if (ret) {
        return ret;
    }

    ret = symlink(target, linkpath);
    if (ret == -1) {
        return -errno;
    }

    return 0;
}

int unreliable_rename(const char *oldpath, const char *newpath)
{
    int ret = error_inject(oldpath, OP_RENAME);
    if (ret == -ERRNO_NOOP) {
        return 0;
    } else if (ret) {
        return ret;
    }

    ret = rename(oldpath, newpath);
    if (ret == -1) {
        return -errno;
    }

    return 0;
}

int unreliable_link(const char *oldpath, const char *newpath)
{
    int ret = error_inject(oldpath, OP_LINK);
    if (ret == -ERRNO_NOOP) {
        return 0;
    } else if (ret) {
        return ret;
    }

    ret = link(oldpath, newpath);
    if (ret < 0) {
        return -errno;
    }

    return 0;
}

int unreliable_chmod(const char *path, mode_t mode)
{
    int ret = error_inject(path, OP_CHMOD);
    if (ret == -ERRNO_NOOP) {
        return 0;
    } else if (ret) {
        return ret;
    }
    
    ret = chmod(path, mode);
    if (ret < 0) {
        return -errno;
    }

    return 0;
}

int unreliable_chown(const char *path, uid_t owner, gid_t group)
{
    int ret = error_inject(path, OP_CHOWN);
    if (ret == -ERRNO_NOOP) {
        return 0;
    } else if (ret) {
        return ret;
    }

    ret = chown(path, owner, group);
    if (ret == -1) {
        return -errno;
    }

    return 0;
}

int unreliable_truncate(const char *path, off_t length)
{
    int ret = error_inject(path, OP_TRUNCATE);
    if (ret == -ERRNO_NOOP) {
        return 0;
    } else if (ret) {
        return ret;
    }

    ret = truncate(path, length); 
    if (ret == -1) {
        return -errno;
    }

    return 0;
}

int unreliable_open(const char *path, struct fuse_file_info *fi)
{
    int ret = error_inject(path, OP_OPEN);
    if (ret == -ERRNO_NOOP) {
        return 0;
    } else if (ret) {
        return ret;
    }

    ret = open(path, fi->flags);
    if (ret == -1) {
        return -errno;
    }
    fi->fh = ret;

    return 0;
}

int unreliable_read(const char *path, char *buf, size_t size, off_t offset,
                    struct fuse_file_info *fi)
{
    int ret = error_inject(path, OP_READ);
    if (ret == -ERRNO_NOOP) {
        return 0;
    } else if (ret) {
        return ret;
    }

    int fd;

    if (fi == NULL) {
	fd = open(path, O_RDONLY);
    } else {
	fd = fi->fh;
    }

    if (fd == -1) {
	return -errno;
    }

    int last = -1; 
    for (int i = 0; i < sizeQueue(&opQueue); i++){
        struct queueNode *ptr = getValueQueue(&opQueue, i);
        if (ptr->fd == fd){
            last = i;
        }
    }
    printf("READ:: last = %d, offset = %ld, size = %ld\n", last, offset,size);
    if (last == -1){
        ret = pread(fd, buf, size, offset);
    }
    else{
        struct queueNode *ptr = getValueQueue(&opQueue, last);
        printf("READ:: last = %d, offset = %ld, size = %ld, ptr->buf = %s, ptr->offset = %ld, ptr->size = %ld\n", last, offset,size, ptr->buf, ptr->offset, ptr->size);
        if (offset == ptr->offset){
            printf("READ:: Using queue\n");
            memcpy(buf, ptr->buf, size);
            ret = ptr->size;
        }
        else{
            ret = pread(ptr->fd, buf, size, offset);
        }
    }
    if (ret == -1) {
        ret = -errno;
    }

    if (fi == NULL) {
	close(fd);
    }

    return ret;
}

int unreliable_write(const char *path, const char *buf, size_t size,
                     off_t offset, struct fuse_file_info *fi)
{
    //int ret = error_inject(path, OP_WRITE);
    
    printf("Start of write\n");
    char *q_buf = (char*)malloc(sizeof(char)*(size+1));
    memcpy(q_buf, buf, size);
    pushQueue(&opQueue, OP_WRITE, q_buf, size, offset, fi->fh); 
    printf("Pushed to queue\n");
    return size;

    /*if (ret == -ERRNO_NOOP) {
        return 0;
    } else if (ret) {
        return ret;
    }

    int fd;
    (void) fi;
    if(fi == NULL) {
	fd = open(path, O_WRONLY);
    } else {
	fd = fi->fh;
    }

    if (fd == -1) {
	return -errno;
    }

    ret = pwrite(fd, buf, size, offset);
    if (ret == -1) {
        ret = -errno;
    }

    if(fi == NULL) {
        close(fd);
    }

    return ret;*/
}

int unreliable_statfs(const char *path, struct statvfs *buf)
{
    int ret = error_inject(path, OP_STATFS);
    if (ret == -ERRNO_NOOP) {
        return 0;
    } else if (ret) {
        return ret;
    }

    ret = statvfs(path, buf);
    if (ret == -1) {
        return -errno;
    }

    return 0;
}

int unreliable_flush(const char *path, struct fuse_file_info *fi)
{
    int ret = error_inject(path, OP_FLUSH);
    if (ret == -ERRNO_NOOP) {
        return 0;
    } else if (ret) {
        return ret;
    }

    ret = close(dup(fi->fh));
    if (ret == -1) {
        return -errno;
    }

    return 0;
}

int unreliable_release(const char *path, struct fuse_file_info *fi)
{
    int ret = error_inject(path, OP_RELEASE);
    if (ret == -ERRNO_NOOP) {
        return 0;
    } else if (ret) {
        return ret;
    }

    ret = close(fi->fh);
    if (ret == -1) {
        return -errno;
    }

    return 0;    
}

int unreliable_fsync(const char *path, int datasync, struct fuse_file_info *fi)
{
    printf("FSYNC: Loop  starting , sizeQueue = %d", sizeQueue(&opQueue));
    struct queueNode *ptr = opQueue;
    while(sizeQueue(&opQueue) != 0){
       int ret = error_inject(path, OP_FSYNC);
       printf("fsync ret = %d\n", ret);
       int index = 0; 
       if (ret == -1){
           //Simple delay
           ptr = getValueQueue(&opQueue, index);
           printf("About to write delay %s\n", ptr->buf);
           int wret = pwrite(ptr->fd, ptr->buf, ptr->size, ptr->offset);
           if (wret == -1){
              printf("ERROR WHILE WRITING\n");
           }
       }
       else {
           //Reorder 
           index = ret;
           ptr = getValueQueue(&opQueue, index);
           printf("About to write reorder %s\n", ptr->buf);
           int wret = pwrite(ptr->fd, ptr->buf, ptr->size, ptr->offset);
           if (wret == -1){
              printf("ERROR WHILE WRITING\n");
           }
       }
       free(ptr->buf);
       delFromQueue(&opQueue, index);
    }

    printf("FSYNC: Loop  completed, sizeQueue = %d", sizeQueue(&opQueue));

    /*if (ret == -ERRNO_NOOP) {
        return 0;
    } else if (ret) {
        return ret;
    }*/
    int ret;
    if (datasync) {
        ret = fdatasync(fi->fh);
        if (ret == -1) {
            return -errno;
        }
    } else {
        ret = fsync(fi->fh);
        if (ret == -1) {
            return -errno;
        }
    }

    return 0;
}

#ifdef HAVE_XATTR
int unreliable_setxattr(const char *path, const char *name,
                        const char *value, size_t size, int flags)
{
    int ret = error_inject(path, OP_SETXATTR);
    if (ret == -ERRNO_NOOP) {
        return 0;
    } else if (ret) {
        return ret;
    }

#ifdef __APPLE__
    ret = setxattr(path, name, value, size, 0, flags);
#else
    ret = setxattr(path, name, value, size, flags);
#endif /* __APPLE__ */
    if (ret == -1) {
        return -errno;
    }

    return 0;
}

int unreliable_getxattr(const char *path, const char *name,
                        char *value, size_t size)
{
    int ret = error_inject(path, OP_GETXATTR);
    if (ret == -ERRNO_NOOP) {
        return 0;
    } else if (ret) {
        return ret;
    }

#ifdef __APPLE__
    ret = getxattr(path, name, value, size, 0, XATTR_NOFOLLOW);
#else
    ret = getxattr(path, name, value, size);
#endif /* __APPLE__ */
    if (ret == -1) {
        return -errno;
    }
    
    return 0;
}

int unreliable_listxattr(const char *path, char *list, size_t size)
{
    int ret = error_inject(path, OP_LISTXATTR);
    if (ret == -ERRNO_NOOP) {
        return 0;
    } else if (ret) {
        return ret;
    }

#ifdef __APPLE__
    ret = listxattr(path, list, size, XATTR_NOFOLLOW);
#else
    ret = listxattr(path, list, size);
#endif /* __APPLE__ */
    if (ret == -1) {
        return -errno;
    }
    
    return ret;
}

int unreliable_removexattr(const char *path, const char *name)
{
    int ret = error_inject(path, OP_REMOVEXATTR);
    if (ret == -ERRNO_NOOP) {
        return 0;
    } else if (ret) {
        return ret;
    }

#ifdef __APPLE__
    ret = removexattr(path, name, XATTR_NOFOLLOW);
#else
    ret = removexattr(path, name);
#endif /* __APPLE__ */
    if (ret == -1) {
        return -errno;
    }
    
    return 0;    
}
#endif /* HAVE_XATTR */

int unreliable_opendir(const char *path, struct fuse_file_info *fi)
{
    int ret = error_inject(path, OP_OPENDIR);
    if (ret == -ERRNO_NOOP) {
        return 0;
    } else if (ret) {
        return ret;
    }

    DIR *dir = opendir(path);

    if (!dir) {
        return -errno;
    }
    fi->fh = (int64_t) dir;

    return 0;    
}

int unreliable_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                       off_t offset, struct fuse_file_info *fi)
{
    int ret = error_inject(path, OP_READDIR);
    if (ret == -ERRNO_NOOP) {
        return 0;
    } else if (ret) {
        return ret;
    }

    DIR *dp = opendir(path);
    if (dp == NULL) {
	return -errno;
    }
    struct dirent *de;

    (void) offset;
    (void) fi;

    while ((de = readdir(dp)) != NULL) {
        struct stat st;
        memset(&st, 0, sizeof(st));
        st.st_ino = de->d_ino;
        st.st_mode = de->d_type << 12;
        if (filler(buf, de->d_name, &st, 0))
            break;
    }
    closedir(dp);

    return 0;
}

int unreliable_releasedir(const char *path, struct fuse_file_info *fi)
{
    int ret = error_inject(path, OP_RELEASEDIR);
    if (ret == -ERRNO_NOOP) {
        return 0;
    } else if (ret) {
        return ret;
    }

    DIR *dir = (DIR *) fi->fh;

    ret = closedir(dir);
    if (ret == -1) {
        return -errno;
    }
    
    return 0;    
}

int unreliable_fsyncdir(const char *path, int datasync, struct fuse_file_info *fi)
{
    int ret = error_inject(path, OP_FSYNCDIR);
    if (ret == -ERRNO_NOOP) {
        return 0;
    } else if (ret) {
        return ret;
    }

    DIR *dir = opendir(path);
    if (!dir) {
        return -errno;
    }

    if (datasync) {
        ret = fdatasync(dirfd(dir));
        if (ret == -1) {
            return -errno;
        }
    } else {
        ret = fsync(dirfd(dir));
        if (ret == -1) {
            return -errno;
        }
    }
    closedir(dir);

    return 0;
}

void *unreliable_init(struct fuse_conn_info *conn)
{
    return NULL;
}

void unreliable_destroy(void *private_data)
{

}

int unreliable_access(const char *path, int mode)
{
    int ret = error_inject(path, OP_ACCESS);
    if (ret == -ERRNO_NOOP) {
        return 0;
    } else if (ret) {
        return ret;
    }

    ret = access(path, mode); 
    if (ret == -1) {
        return -errno;
    }
    
    return 0;
}

int unreliable_create(const char *path, mode_t mode,
                      struct fuse_file_info *fi)
{
    int ret = error_inject(path, OP_CREAT);
    if (ret == -ERRNO_NOOP) {
        return 0;
    } else if (ret) {
        return ret;
    }

    ret = open(path, fi->flags, mode);
    if (ret == -1) {
        return -errno;
    }
    fi->fh = ret;

    return 0;    
}

int unreliable_ftruncate(const char *path, off_t length,
                         struct fuse_file_info *fi)
{
    int ret = error_inject(path, OP_FTRUNCATE);
    if (ret == -ERRNO_NOOP) {
        return 0;
    } else if (ret) {
        return ret;
    }

    ret = truncate(path, length);
    if (ret == -1) {
        return -errno;
    }
    
    return 0;    
}

int unreliable_fgetattr(const char *path, struct stat *buf,
                        struct fuse_file_info *fi)
{
    int ret = error_inject(path, OP_FGETATTR);
    if (ret == -ERRNO_NOOP) {
        return 0;
    } else if (ret) {
        return ret;
    }

    ret = fstat((int) fi->fh, buf);
    if (ret == -1) {
        return -errno;
    }
    
    return 0;    
}

int unreliable_lock(const char *path, struct fuse_file_info *fi, int cmd,
                    struct flock *fl)
{
    int ret = error_inject(path, OP_LOCK);
    if (ret == -ERRNO_NOOP) {
        return 0;
    } else if (ret) {
        return ret;
    }

    ret = fcntl((int) fi->fh, cmd, fl);
    if (ret == -1) {
        return -errno;
    }

    return 0;
}

#if !defined(__OpenBSD__)
int unreliable_ioctl(const char *path, int cmd, void *arg,
                     struct fuse_file_info *fi,
                     unsigned int flags, void *data)
{
    int ret = error_inject(path, OP_IOCTL);
    if (ret == -ERRNO_NOOP) {
        return 0;
    } else if (ret) {
        return ret;
    }

    ret = ioctl(fi->fh, cmd, arg);
    if (ret == -1) {
        return -errno;
    }
    
    return ret;
}
#endif /* __OpenBSD__ */

#ifdef HAVE_FLOCK
int unreliable_flock(const char *path, struct fuse_file_info *fi, int op)
{
    int ret = error_inject(path, OP_FLOCK);
    if (ret == -ERRNO_NOOP) {
        return 0;
    } else if (ret) {
        return ret;
    }

    ret = flock(((int) fi->fh), op);
    if (ret == -1) {
        return -errno;
    }
    
    return 0;    
}
#endif /* HAVE_FLOCK */

#ifdef HAVE_FALLOCATE
int unreliable_fallocate(const char *path, int mode,
                         off_t offset, off_t len,
                         struct fuse_file_info *fi)
{
    int ret = error_inject(path, OP_FALLOCATE);
    if (ret == -ERRNO_NOOP) {
        return 0;
    } else if (ret) {
        return ret;
    }

    int fd;
    (void) fi;

    if (mode) {
	return -EOPNOTSUPP;
    }

    if(fi == NULL) {
	fd = open(path, O_WRONLY);
    } else {
	fd = fi->fh;
    }

    if (fd == -1) {
	return -errno;
    }

    ret = fallocate((int) fi->fh, mode, offset, len);
    if (ret == -1) {
        return -errno;
    }

    if(fi == NULL) {
	close(fd);
    }
    
    return 0;    
}
#endif /* HAVE_FALLOCATE */

#ifdef HAVE_UTIMENSAT
int unreliable_utimens(const char *path, const struct timespec ts[2])
{
    int ret = error_inject(path, OP_UTIMENS);
    if (ret == -ERRNO_NOOP) {
        return 0;
    } else if (ret) {
        return ret;
    }

    /* don't use utime/utimes since they follow symlinks */
    ret = utimensat(0, path, ts, AT_SYMLINK_NOFOLLOW);
    if (ret == -1) {
        return -errno;
    }

    return 0;
}
#endif /* HAVE_UTIMENSAT */
