/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_FS_SUPER_TYPES_H
#define _LINUX_FS_SUPER_TYPES_H

#include <linux/fs_dirent.h>
#include <linux/errseq.h>
#include <linux/list_lru.h>
#include <linux/list.h>
#include <linux/list_bl.h>
#include <linux/llist.h>
#include <linux/uidgid.h>
#include <linux/uuid.h>
#include <linux/percpu-rwsem.h>
#include <linux/workqueue_types.h>
#include <linux/quota.h>

struct backing_dev_info;
struct block_device;
struct dentry;
struct dentry_operations;
struct dquot_operations;
struct export_operations;
struct file;
struct file_system_type;
struct fscrypt_operations;
struct fsnotify_sb_info;
struct fsverity_operations;
struct kstatfs;
struct mount;
struct mtd_info;
struct quotactl_ops;
struct shrinker;
struct unicode_map;
struct user_namespace;
struct workqueue_struct;
struct writeback_control;
struct xattr_handler;
struct fserror_event;

extern struct super_block *blockdev_superblock;

/* Possible states of 'frozen' field */
enum {
	SB_UNFROZEN		= 0,	/* FS is unfrozen */
	SB_FREEZE_WRITE		= 1,	/* Writes, dir ops, ioctls frozen */
	SB_FREEZE_PAGEFAULT	= 2,	/* Page faults stopped as well */
	SB_FREEZE_FS		= 3,	/* For internal FS use (e.g. to stop internal threads if needed) */
	SB_FREEZE_COMPLETE	= 4,	/* ->freeze_fs finished successfully */
};

#define SB_FREEZE_LEVELS (SB_FREEZE_COMPLETE - 1)

struct sb_writers {
	unsigned short			frozen;		/* Is sb frozen? */
	int				freeze_kcount;	/* How many kernel freeze requests? */
	int				freeze_ucount;	/* How many userspace freeze requests? */
	const void			*freeze_owner;	/* Owner of the freeze */
	struct percpu_rw_semaphore	rw_sem[SB_FREEZE_LEVELS];
};

/**
 * enum freeze_holder - holder of the freeze
 * @FREEZE_HOLDER_KERNEL: kernel wants to freeze or thaw filesystem
 * @FREEZE_HOLDER_USERSPACE: userspace wants to freeze or thaw filesystem
 * @FREEZE_MAY_NEST: whether nesting freeze and thaw requests is allowed
 * @FREEZE_EXCL: a freeze that can only be undone by the owner
 *
 * Indicate who the owner of the freeze or thaw request is and whether
 * the freeze needs to be exclusive or can nest.
 * Without @FREEZE_MAY_NEST, multiple freeze and thaw requests from the
 * same holder aren't allowed. It is however allowed to hold a single
 * @FREEZE_HOLDER_USERSPACE and a single @FREEZE_HOLDER_KERNEL freeze at
 * the same time. This is relied upon by some filesystems during online
 * repair or similar.
 */
enum freeze_holder {
	FREEZE_HOLDER_KERNEL	= (1U << 0),
	FREEZE_HOLDER_USERSPACE	= (1U << 1),
	FREEZE_MAY_NEST		= (1U << 2),
	FREEZE_EXCL		= (1U << 3),
};

struct super_operations {
	/* 被inode_alloc()函数调用用于分配inode内存并进行inode结构初始化
	 * 如果函数未定义，则简单的分配一个'struct inode'
	 * 通常alloc_inode用于底层文件系统分配一个包含inode结构体的更大的结构体（特定的inode结构，如：fuse_inode）*/
	struct inode *(*alloc_inode)(struct super_block *sb);
	/* 被destroy_inode()函数调用用于释放inode相关申请的资源
	 * 只有alloc_inode定义了才需要定义destroy_inode，并且释放的也是alloc_inode里申请的相关资源 */
	void (*destroy_inode)(struct inode *inode);
	void (*free_inode)(struct inode *inode);
	/* 由VFS调用标记inode dirty（元数据信息被修改过并且没有同步到磁盘或服务器） */
	void (*dirty_inode)(struct inode *inode, int flags);
	/* 由VFS调用用于将inode同步到磁盘。第二个参数用于标识是否同步写盘 */
	int (*write_inode)(struct inode *inode, struct writeback_control *wbc);
	/* VFS在当inode的引用计数减为0时，调用该函数 
	 * 调用者已经持有了inode->i_lock
	 * 该函数返回0，则inode将可能被丢到LRU链表里，返回1则会由调用者继续调用evict_inode和destroy_inode 
	 * 如果文件系统不需要缓存inode，则该函数可以设置为NULL或者generic_delete_inode（函数里直接return 1）*/
	int (*drop_inode)(struct inode *inode);
	void (*evict_inode)(struct inode *inode);
	/* VFS想要释放sb时调用（如umount操作）
	 * 调用者已经持有sb的lock */
	void (*put_super)(struct super_block *sb);
	/* VFS想要把该文件系统所有的脏数据刷盘时调用 */
	int (*sync_fs)(struct super_block *sb, int wait);
	/* 目前只有LVM使用。用于冻结文件系统，不能进行写入操作 */
	int (*freeze_super)(struct super_block *sb, enum freeze_holder who,
			    const void *owner);
	int (*freeze_fs)(struct super_block *sb);
	int (*thaw_super)(struct super_block *sb, enum freeze_holder who,
			  const void *owner);
	/* 解冻 */
	int (*unfreeze_fs)(struct super_block *sb);
	/* 用于获取文件系统的统计信息
	 * xfs: xfs_fs_statfs */
	int (*statfs)(struct dentry *dentry, struct kstatfs *kstatfs);
	/* 用于 umount 文件系统 */
	void (*umount_begin)(struct super_block *sb);

	/* 用于/proc/mounts里显示文件系统的mount选项 */
	int (*show_options)(struct seq_file *seq, struct dentry *dentry);
	int (*show_devname)(struct seq_file *seq, struct dentry *dentry);
	int (*show_path)(struct seq_file *seq, struct dentry *dentry);
	int (*show_stats)(struct seq_file *seq, struct dentry *dentry);
#ifdef CONFIG_QUOTA
	ssize_t (*quota_read)(struct super_block *sb, int type, char *data,
			      size_t len, loff_t off);
	ssize_t (*quota_write)(struct super_block *sb, int type,
			       const char *data, size_t len, loff_t off);
	struct dquot __rcu **(*get_dquots)(struct inode *inode);
#endif
	long (*nr_cached_objects)(struct super_block *sb,
				  struct shrink_control *sc);
	long (*free_cached_objects)(struct super_block *sb,
				    struct shrink_control *sc);
	/*
	 * If a filesystem can support graceful removal of a device and
	 * continue read-write operations, implement this callback.
	 *
	 * Return 0 if the filesystem can continue read-write.
	 * Non-zero return value or no such callback means the fs will be shutdown
	 * as usual.
	 */
	int (*remove_bdev)(struct super_block *sb, struct block_device *bdev);
	void (*shutdown)(struct super_block *sb);

	/* Report a filesystem error */
	void (*report_error)(const struct fserror_event *event);
};

struct super_block {
	/* 用于连接到 super_blocks 全局链表上 */
	struct list_head			s_list;		/* Keep this first */
	/* 对应块设备标识 */
	dev_t					s_dev;		/* search index; _not_ kdev_t */
	unsigned char				s_blocksize_bits;
	/* 文件系统块大小 */
	unsigned long				s_blocksize;
	/* 文件系统最大支持文件大小 */
	loff_t					s_maxbytes;	/* Max file size */
	/* 文件系统类型 */
	struct file_system_type			*s_type;
	/* 超级块的操作函数 */
	const struct super_operations		*s_op;
	/* 文件系统限额操作函数 */
	const struct dquot_operations		*dq_op;
	/* 磁盘限额 */
	const struct quotactl_ops		*s_qcop;
	const struct export_operations		*s_export_op;
	/* 挂载标记 */
	unsigned long				s_flags;
	unsigned long				s_iflags;	/* internal SB_I_* flags */
	/*魔术字 */
	unsigned long				s_magic;
	/* 全局根目录的目录项 */
	struct dentry				*s_root;
	struct rw_semaphore			s_umount;
	/* 引用计数 */
	int					s_count;
	/* 使用计数 */
	atomic_t				s_active;
#ifdef CONFIG_SECURITY
	void					*s_security;
#endif
	const struct xattr_handler		*const *s_xattr;
#ifdef CONFIG_FS_ENCRYPTION
	const struct fscrypt_operations		*s_cop;
	struct fscrypt_keyring			*s_master_keys; /* master crypto keys in use */
#endif
#ifdef CONFIG_FS_VERITY
	const struct fsverity_operations	*s_vop;
#endif
#if IS_ENABLED(CONFIG_UNICODE)
	struct unicode_map			*s_encoding;
	__u16					s_encoding_flags;
#endif
	struct hlist_bl_head			s_roots;	/* alternate root dentries for NFS */
	struct mount				*s_mounts;	/* list of mounts; _not_ for fs use */
	/* 对应的块设备 */
	struct block_device			*s_bdev;	/* can go away once we use an accessor for @s_bdev_file */
	struct file				*s_bdev_file;
	/* 超级块对应的设备详情 */
	struct backing_dev_info 		*s_bdi;
	struct mtd_info				*s_mtd;
	/* 连接到 file_system_type 的 fs_supers */
	struct hlist_node			s_instances;
	unsigned int				s_quota_types;	/* Bitmask of supported quota types */
	struct quota_info			s_dquot;	/* Diskquota specific options */

	struct sb_writers			s_writers;

	/*
	 * Keep s_fs_info, s_time_gran, s_fsnotify_mask, and
	 * s_fsnotify_info together for cache efficiency. They are frequently
	 * accessed and rarely modified.
	 *
	 * 文件系统私有数据
	 */
	void					*s_fs_info;	/* Filesystem private info */

	/* Granularity of c/m/atime in ns (cannot be worse than a second) */
	u32					s_time_gran;
	/* Time limits for c/m/atime in seconds */
	time64_t				s_time_min;
	time64_t		   		s_time_max;
#ifdef CONFIG_FSNOTIFY
	u32					s_fsnotify_mask;
	struct fsnotify_sb_info			*s_fsnotify_info;
#endif

	/*
	 * q: why are s_id and s_sysfs_name not the same? both are human
	 * readable strings that identify the filesystem
	 * a: s_id is allowed to change at runtime; it's used in log messages,
	 * and we want to when a device starts out as single device (s_id is dev
	 * name) but then a device is hot added and we have to switch to
	 * identifying it by UUID
	 * but s_sysfs_name is a handle for programmatic access, and can't
	 * change at runtime
	 */
	char					s_id[32];	/* Informational name */
	/* tune2fs -l */
	uuid_t					s_uuid;		/* UUID */
	u8					s_uuid_len;	/* Default 16, possibly smaller for weird filesystems */

	/* if set, fs shows up under sysfs at /sys/fs/$FSTYP/s_sysfs_name */
	char					s_sysfs_name[UUID_STRING_LEN + 1];

	unsigned int				s_max_links;
	unsigned int				s_d_flags;	/* default d_flags for dentries */

	/*
	 * The next field is for VFS *only*. No filesystems have any business
	 * even looking at it. You had been warned.
	 */
	struct mutex				s_vfs_rename_mutex;	/* Kludge */

	/*
	 * Filesystem subtype.  If non-empty the filesystem type field
	 * in /proc/mounts will be "type.subtype"
	 */
	const char				*s_subtype;

	/* 超级块默认的目录项操作函数 */
	const struct dentry_operations		*__s_d_op; /* default d_op for dentries */

	/* 超级块注册的内存回收函数 */
	struct shrinker				*s_shrink;	/* per-sb shrinker handle */

	/* Number of inodes with nlink == 0 but still referenced */
	atomic_long_t				s_remove_count;

	/* Read-only state of the superblock is being changed */
	int					s_readonly_remount;

	/* per-sb errseq_t for reporting writeback errors via syncfs */
	errseq_t s_wb_err;

	/* AIO completions deferred from interrupt context */
	struct workqueue_struct			*s_dio_done_wq;
	struct hlist_head			s_pins;

	/*
	 * Owning user namespace and default context in which to
	 * interpret filesystem uids, gids, quotas, device nodes,
	 * xattrs and security labels.
	 */
	struct user_namespace			*s_user_ns;

	/*
	 * The list_lru structure is essentially just a pointer to a table
	 * of per-node lru lists, each of which has its own spinlock.
	 * There is no need to put them into separate cachelines.
	 */
	/* 超级块对应的未使用 目录项 列表 */
	struct list_lru				s_dentry_lru;
	/* 超级块对应的未使用 索引节点 列表 */
	struct list_lru				s_inode_lru;
	struct rcu_head				rcu;
	struct work_struct			destroy_work;

	struct mutex				s_sync_lock;	/* sync serialisation lock */

	/*
	 * Indicates how deep in a filesystem stack this SB is
	 */
	int s_stack_depth;

	/* s_inode_list_lock protects s_inodes */
	spinlock_t				s_inode_list_lock ____cacheline_aligned_in_smp;
	/* 挂载了所有 索引节点 的链表 */
	struct list_head			s_inodes;	/* all inodes */

	spinlock_t				s_inode_wblist_lock;
	/* 正在回写 索引节点 的链表 */
	struct list_head			s_inodes_wb;	/* writeback inodes */
	long					s_min_writeback_pages;

	/* number of fserrors that are being sent to fsnotify/filesystems */
	refcount_t				s_pending_errors;
} __randomize_layout;

/*
 * sb->s_flags.  Note that these mirror the equivalent MS_* flags where
 * represented in both.
 */
/* 只读挂载 */
#define SB_RDONLY       BIT(0)	/* Mount read-only */
#define SB_NOSUID       BIT(1)	/* Ignore suid and sgid bits */
#define SB_NODEV        BIT(2)	/* Disallow access to device special files */
#define SB_NOEXEC       BIT(3)	/* Disallow program execution */
#define SB_SYNCHRONOUS  BIT(4)	/* Writes are synced at once */
#define SB_MANDLOCK     BIT(6)	/* Allow mandatory locks on an FS */
#define SB_DIRSYNC      BIT(7)	/* Directory modifications are synchronous */
#define SB_NOATIME      BIT(10)	/* Do not update access times. */
#define SB_NODIRATIME   BIT(11)	/* Do not update directory access times */
#define SB_SILENT       BIT(15)
#define SB_POSIXACL     BIT(16)	/* Supports POSIX ACLs */
#define SB_INLINECRYPT  BIT(17)	/* Use blk-crypto for encrypted files */
#define SB_KERNMOUNT    BIT(22)	/* this is a kern_mount call */
#define SB_I_VERSION    BIT(23)	/* Update inode I_version field */
#define SB_LAZYTIME     BIT(25)	/* Update the on-disk [acm]times lazily */

/* These sb flags are internal to the kernel */
/* 这些 sb 标志是内核内部的 */
#define SB_DEAD         BIT(21)
#define SB_DYING        BIT(24)
#define SB_FORCE        BIT(27)
#define SB_NOSEC        BIT(28)
#define SB_BORN         BIT(29)
#define SB_ACTIVE       BIT(30)
#define SB_NOUSER       BIT(31)

/* These flags relate to encoding and casefolding */
#define SB_ENC_STRICT_MODE_FL		(1 << 0)
#define SB_ENC_NO_COMPAT_FALLBACK_FL	(1 << 1)

#define sb_has_strict_encoding(sb) \
	(sb->s_encoding_flags & SB_ENC_STRICT_MODE_FL)

#if IS_ENABLED(CONFIG_UNICODE)
#define sb_no_casefold_compat_fallback(sb) \
	(sb->s_encoding_flags & SB_ENC_NO_COMPAT_FALLBACK_FL)
#else
#define sb_no_casefold_compat_fallback(sb) (1)
#endif

/* sb->s_iflags */
#define SB_I_CGROUPWB	0x00000001	/* cgroup-aware writeback enabled */
#define SB_I_NOEXEC	0x00000002	/* Ignore executables on this fs */
#define SB_I_NODEV	0x00000004	/* Ignore devices on this fs */
#define SB_I_STABLE_WRITES 0x00000008	/* don't modify blks until WB is done */

/* sb->s_iflags to limit user namespace mounts */
#define SB_I_USERNS_VISIBLE		0x00000010 /* fstype already mounted */
#define SB_I_IMA_UNVERIFIABLE_SIGNATURE	0x00000020
#define SB_I_UNTRUSTED_MOUNTER		0x00000040
#define SB_I_EVM_HMAC_UNSUPPORTED	0x00000080

#define SB_I_SKIP_SYNC	0x00000100	/* Skip superblock at global sync */
#define SB_I_PERSB_BDI	0x00000200	/* has a per-sb bdi */
#define SB_I_TS_EXPIRY_WARNED 0x00000400 /* warned about timestamp range expiry */
#define SB_I_RETIRED	0x00000800	/* superblock shouldn't be reused */
#define SB_I_NOUMASK	0x00001000	/* VFS does not apply umask */
#define SB_I_NOIDMAP	0x00002000	/* No idmapped mounts on this superblock */
#define SB_I_ALLOW_HSM	0x00004000	/* Allow HSM events on this superblock */
#define SB_I_NO_DATA_INTEGRITY	0x00008000 /* fs cannot guarantee data persistence on sync */

#endif /* _LINUX_FS_SUPER_TYPES_H */
