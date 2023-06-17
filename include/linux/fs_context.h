/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Filesystem superblock creation and reconfiguration context.
 *
 * Copyright (C) 2018 Red Hat, Inc. All Rights Reserved.
 * Written by David Howells (dhowells@redhat.com)
 */

#ifndef _LINUX_FS_CONTEXT_H
#define _LINUX_FS_CONTEXT_H

#include <linux/kernel.h>
#include <linux/refcount.h>
#include <linux/errno.h>
#include <linux/security.h>
#include <linux/mutex.h>

struct cred;
struct dentry;
struct file_operations;
struct file_system_type;
struct mnt_namespace;
struct net;
struct pid_namespace;
struct super_block;
struct user_namespace;
struct vfsmount;
struct path;

// 目的
enum fs_context_purpose {
    // 创建安装超级块
	FS_CONTEXT_FOR_MOUNT,		/* New superblock for explicit mount */
    // 自动子安装超级块
	FS_CONTEXT_FOR_SUBMOUNT,	/* New superblock for automatic submount */
    // 重新安装超级块
	FS_CONTEXT_FOR_RECONFIGURE,	/* Superblock reconfiguration (remount) */
};

/*
 * Userspace usage phase for fsopen/fspick.
 */
enum fs_context_phase {
	FS_CONTEXT_CREATE_PARAMS,	/* Loading params for sb creation */
	FS_CONTEXT_CREATING,		/* A superblock is being created */
	FS_CONTEXT_AWAITING_MOUNT,	/* Superblock created, awaiting fsmount() */
	FS_CONTEXT_AWAITING_RECONF,	/* Awaiting initialisation for reconfiguration */
	FS_CONTEXT_RECONF_PARAMS,	/* Loading params for reconfiguration */
	FS_CONTEXT_RECONFIGURING,	/* Reconfiguring the superblock */
	FS_CONTEXT_FAILED,		/* Failed to correctly transition a context */
};

/*
 * Type of parameter value.
 */
enum fs_value_type {
	fs_value_is_undefined,
	fs_value_is_flag,		/* Value not given a value */
	fs_value_is_string,		/* Value is a string */
	fs_value_is_blob,		/* Value is a binary blob */
	fs_value_is_filename,		/* Value is a filename* + dirfd */
	fs_value_is_file,		/* Value is a file* */
};

/*
 * Configuration parameter.
 */
struct fs_parameter {
	const char		*key;		/* Parameter name */
	enum fs_value_type	type:8;		/* The type of value here */
	union {
		char		*string;
		void		*blob;
		struct filename	*name;
		struct file	*file;
	};
	size_t	size;
	int	dirfd;
};

struct p_log {
	const char *prefix;
	struct fc_log *log;
};

/*
 * Filesystem context for holding the parameters used in the creation or
 * reconfiguration of a superblock.
 *
 * Superblock creation fills in ->root whereas reconfiguration begins with this
 * already set.
 *
 * See Documentation/filesystems/mount_api.rst
 *
 * 文件系统上下文
 */
struct fs_context {
    // 这是一个文件系统上下文实例持续期间，提供给文件系统上下文使用的众多方法
    // 一般由特定文件系统类型的 init_fs_context 方法来对其进行设置。
	const struct fs_context_operations *ops;
	struct mutex		uapi_mutex;	/* Userspace access mutex */
    // 这是一个用来指向即将被挂载（或重新配置）的文件系统所属文件系统类型实例的指针
    // 换句话说，用例中我们要挂载一个XFS，这里就是指向XFS的 file_system_type 结构指针
	struct file_system_type	*fs_type;
    // 这是一个指向文件系统私有数据的指针
    // 常用于存储需要特定文件系统来解析的选项
	void			*fs_private;	/* The filesystem's context */
	void			*sget_key;
    // 这是一个指向一个可挂载文件系统的根节点的指针
    // 同时它也和文件系统的 superblock 关联
    // 这个域一般通过 vfs_get_tree 调用特定文件系统的 ops->get_tree 函数来构建
    // 同时，为了构建SB，也会调用特定文件系统的 fill_super 函数。
	struct dentry		*root;		/* The root and superblock */
	struct user_namespace	*user_ns;	/* The user namespace for this mount */
    // 网络命名空间
	struct net		*net_ns;	/* The network namespace for this mount */
	const struct cred	*cred;		/* The mounter's credentials */
	struct p_log		log;		/* Logging buffer */
    // 这个代表挂载操作的源对象
    // 一般就是一个设备（上面含有文件系统）
    // 如mount /dev/sda1 /mnt中
    // "/dev/sda1"就是这个source的名称
    // 当然它还可以是非本地文件系统时使用的类似url:/path这样的写法
	const char		*source;	/* The source name (eg. dev path) */
	void			*security;	/* LSM options */
    // 这是区别于内存通用 superblock 结构的一个域
    // 用于指向特定文件系统的私有信息
	void			*s_fs_info;	/* Proposed s_fs_info */
    // 这其实是挂载 flag 在 superblock 里的一个镜像
    // 比如SB_RDONLY, SB_NODEV, SB_NOATIME等对分别对应MS_*的flag
	unsigned int		sb_flags;	/* Proposed superblock flags (SB_*) */
	unsigned int		sb_flags_mask;	/* Superblock flags that were changed */
	unsigned int		s_iflags;	/* OR'd with sb->s_iflags */
	unsigned int		lsm_flags;	/* Information flags from the fs to the LSM */
    // 指明当前的文件系统上下文的使用意图
    // 目前有三种使用意图 fs_context_purpose
	enum fs_context_purpose	purpose:8;
    // 指明当前处于文件系统挂载过程的哪个阶段
    // 虽然我们在本文最开始说了现在将挂载过程分成独立的六个步骤
    // 但下面这7个阶段的定义和上面说的并没有直接关系
	enum fs_context_phase	phase:8;	/* The phase the context is in */
	bool			need_free:1;	/* Need to call ops->free() */
	bool			global:1;	/* Goes into &init_user_ns */
	bool			oldapi:1;	/* Coming from mount(2) */
	bool			exclusive:1;    /* create new superblock, reject existing one */
};

struct fs_context_operations {
	void (*free)(struct fs_context *fc);
	int (*dup)(struct fs_context *fc, struct fs_context *src_fc);
	int (*parse_param)(struct fs_context *fc, struct fs_parameter *param);
	int (*parse_monolithic)(struct fs_context *fc, void *data);
	int (*get_tree)(struct fs_context *fc);
	int (*reconfigure)(struct fs_context *fc);
};

/*
 * fs_context manipulation functions.
 */
extern struct fs_context *fs_context_for_mount(struct file_system_type *fs_type,
						unsigned int sb_flags);
extern struct fs_context *fs_context_for_reconfigure(struct dentry *dentry,
						unsigned int sb_flags,
						unsigned int sb_flags_mask);
extern struct fs_context *fs_context_for_submount(struct file_system_type *fs_type,
						struct dentry *reference);

extern struct fs_context *vfs_dup_fs_context(struct fs_context *fc);
extern int vfs_parse_fs_param(struct fs_context *fc, struct fs_parameter *param);
extern int vfs_parse_fs_string(struct fs_context *fc, const char *key,
			       const char *value, size_t v_size);
int vfs_parse_monolithic_sep(struct fs_context *fc, void *data,
			     char *(*sep)(char **));
extern int generic_parse_monolithic(struct fs_context *fc, void *data);
extern int vfs_get_tree(struct fs_context *fc);
extern void put_fs_context(struct fs_context *fc);
extern int vfs_parse_fs_param_source(struct fs_context *fc,
				     struct fs_parameter *param);
extern void fc_drop_locked(struct fs_context *fc);
int reconfigure_single(struct super_block *s,
		       int flags, void *data);

extern int get_tree_nodev(struct fs_context *fc,
			 int (*fill_super)(struct super_block *sb,
					   struct fs_context *fc));
extern int get_tree_single(struct fs_context *fc,
			 int (*fill_super)(struct super_block *sb,
					   struct fs_context *fc));
extern int get_tree_keyed(struct fs_context *fc,
			 int (*fill_super)(struct super_block *sb,
					   struct fs_context *fc),
			 void *key);

int setup_bdev_super(struct super_block *sb, int sb_flags,
		struct fs_context *fc);
extern int get_tree_bdev(struct fs_context *fc,
			       int (*fill_super)(struct super_block *sb,
						 struct fs_context *fc));

extern const struct file_operations fscontext_fops;

/*
 * Mount error, warning and informational message logging.  This structure is
 * shareable between a mount and a subordinate mount.
 */
struct fc_log {
	refcount_t	usage;
	u8		head;		/* Insertion index in buffer[] */
	u8		tail;		/* Removal index in buffer[] */
	u8		need_free;	/* Mask of kfree'able items in buffer[] */
	struct module	*owner;		/* Owner module for strings that don't then need freeing */
	char		*buffer[8];
};

extern __attribute__((format(printf, 4, 5)))
void logfc(struct fc_log *log, const char *prefix, char level, const char *fmt, ...);

#define __logfc(fc, l, fmt, ...) logfc((fc)->log.log, NULL, \
					l, fmt, ## __VA_ARGS__)
#define __plog(p, l, fmt, ...) logfc((p)->log, (p)->prefix, \
					l, fmt, ## __VA_ARGS__)
/**
 * infof - Store supplementary informational message
 * @fc: The context in which to log the informational message
 * @fmt: The format string
 *
 * Store the supplementary informational message for the process if the process
 * has enabled the facility.
 */
#define infof(fc, fmt, ...) __logfc(fc, 'i', fmt, ## __VA_ARGS__)
#define info_plog(p, fmt, ...) __plog(p, 'i', fmt, ## __VA_ARGS__)
#define infofc(p, fmt, ...) __plog((&(fc)->log), 'i', fmt, ## __VA_ARGS__)

/**
 * warnf - Store supplementary warning message
 * @fc: The context in which to log the error message
 * @fmt: The format string
 *
 * Store the supplementary warning message for the process if the process has
 * enabled the facility.
 */
#define warnf(fc, fmt, ...) __logfc(fc, 'w', fmt, ## __VA_ARGS__)
#define warn_plog(p, fmt, ...) __plog(p, 'w', fmt, ## __VA_ARGS__)
#define warnfc(fc, fmt, ...) __plog((&(fc)->log), 'w', fmt, ## __VA_ARGS__)

/**
 * errorf - Store supplementary error message
 * @fc: The context in which to log the error message
 * @fmt: The format string
 *
 * Store the supplementary error message for the process if the process has
 * enabled the facility.
 */
#define errorf(fc, fmt, ...) __logfc(fc, 'e', fmt, ## __VA_ARGS__)
#define error_plog(p, fmt, ...) __plog(p, 'e', fmt, ## __VA_ARGS__)
#define errorfc(fc, fmt, ...) __plog((&(fc)->log), 'e', fmt, ## __VA_ARGS__)

/**
 * invalf - Store supplementary invalid argument error message
 * @fc: The context in which to log the error message
 * @fmt: The format string
 *
 * Store the supplementary error message for the process if the process has
 * enabled the facility and return -EINVAL.
 */
#define invalf(fc, fmt, ...) (errorf(fc, fmt, ## __VA_ARGS__), -EINVAL)
#define inval_plog(p, fmt, ...) (error_plog(p, fmt, ## __VA_ARGS__), -EINVAL)
#define invalfc(fc, fmt, ...) (errorfc(fc, fmt, ## __VA_ARGS__), -EINVAL)

#endif /* _LINUX_FS_CONTEXT_H */
