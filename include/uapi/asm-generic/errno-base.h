/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
#ifndef _ASM_GENERIC_ERRNO_BASE_H
#define _ASM_GENERIC_ERRNO_BASE_H

#define	EPERM		 1	/* Operation not permitted */
/* 文件或目录不存在,
 * SHM IPC key 指示的共享内存不存在, 没有设置 IPC_CREAT 标志 */
#define	ENOENT		 2	/* No such file or directory */
#define	ESRCH		 3	/* No such process */
/* 中断的系统调用 */
#define	EINTR		 4	/* Interrupted system call */
#define	EIO		 5	/* I/O error */
#define	ENXIO		 6	/* No such device or address */
#define	E2BIG		 7	/* Argument list too long */
#define	ENOEXEC		 8	/* Exec format error */
/* 错误文件编号
 * 文件描述符无效(被关闭或文件描述符超出范围) */
#define	EBADF		 9	/* Bad file number */
/* 没有子进程 */
#define	ECHILD		10	/* No child processes */
/* 重试 */
#define	EAGAIN		11	/* Try again */
/* 内存不足 */
#define	ENOMEM		12	/* Out of memory */
/* 权限不足 */
#define	EACCES		13	/* Permission denied */
#define	EFAULT		14	/* Bad address */
#define	ENOTBLK		15	/* Block device required */
/* 设备或资源繁忙 */
#define	EBUSY		16	/* Device or resource busy */
/* IPC_EXCL | IPC_CREAT 打开已存在的 key 时，异常是此值 */
#define	EEXIST		17	/* File exists */
#define	EXDEV		18	/* Cross-device link */
#define	ENODEV		19	/* No such device */
/* 不是目录 */
#define	ENOTDIR		20	/* Not a directory */
/* 是目录 */
#define	EISDIR		21	/* Is a directory */
/* 参数无效, SHM IPC size 小于 SHMMIN 或大于 SHMMAX */
#define	EINVAL		22	/* Invalid argument */
#define	ENFILE		23	/* File table overflow */
#define	EMFILE		24	/* Too many open files */
#define	ENOTTY		25	/* Not a typewriter */
#define	ETXTBSY		26	/* Text file busy */
/* 文件过大 */
#define	EFBIG		27	/* File too large */
/* 磁盘没有可用空间, SHM IPC 已满 SHMALL */
#define	ENOSPC		28	/* No space left on device */
#define	ESPIPE		29	/* Illegal seek */
/* 只读文件系统 */
#define	EROFS		30	/* Read-only file system */
#define	EMLINK		31	/* Too many links */
#define	EPIPE		32	/* Broken pipe */
#define	EDOM		33	/* Math argument out of domain of func */
#define	ERANGE		34	/* Math result not representable */

#endif
