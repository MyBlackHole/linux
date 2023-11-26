/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 1995-1997 Olaf Kirch <okir@monad.swb.de>
 */
#ifndef NFSD_EXPORT_H
#define NFSD_EXPORT_H

#include <linux/sunrpc/cache.h>
#include <linux/percpu_counter.h>
#include <uapi/linux/nfsd/export.h>
#include <linux/nfs4.h>

struct knfsd_fh;
struct svc_fh;
struct svc_rqst;

/*
 * FS Locations
 */

#define MAX_FS_LOCATIONS	128

struct nfsd4_fs_location {
	char *hosts; /* colon separated list of hosts */
	char *path;  /* slash separated list of path components */
};

struct nfsd4_fs_locations {
	uint32_t locations_count;
	struct nfsd4_fs_location *locations;
/* If we're not actually serving this data ourselves (only providing a
 * list of replicas that do serve it) then we set "migrated": */
	int migrated;
};

/*
 * We keep an array of pseudoflavors with the export, in order from most
 * to least preferred.  For the foreseeable future, we don't expect more
 * than the eight pseudoflavors null, unix, krb5, krb5i, krb5p, skpm3,
 * spkm3i, and spkm3p (and using all 8 at once should be rare).
 */
#define MAX_SECINFO_LIST	8
#define EX_UUID_LEN		16

struct exp_flavor_info {
	u32	pseudoflavor;
	u32	flags;
};

/* Per-export stats */
enum {
	EXP_STATS_FH_STALE,
	EXP_STATS_IO_READ,
	EXP_STATS_IO_WRITE,
	EXP_STATS_COUNTERS_NUM
};

struct export_stats {
	time64_t		start_time;
	struct percpu_counter	counter[EXP_STATS_COUNTERS_NUM];
};

struct svc_export {
    // cache 缓存
	struct cache_head	h;
	struct auth_domain *	ex_client;
    // 导出文件系统标志位
	int			ex_flags;
    // 导出的文件系统信息
    // fsid=0, ex_fsid=0
	int			ex_fsid;
    // 文件系统根节点路径
	struct path		ex_path;
    // 匿名用户 id
	kuid_t			ex_anon_uid;
    // 匿名用户组 id
	kgid_t			ex_anon_gid;
    // 文件系统 uuid
	unsigned char *		ex_uuid; /* 16 byte fsid */
    // nfs4 文件系统位置
	struct nfsd4_fs_locations ex_fslocs;
	uint32_t		ex_nflavors;
	struct exp_flavor_info	ex_flavors[MAX_SECINFO_LIST];
	u32			ex_layout_types;
	struct nfsd4_deviceid_map *ex_devid_map;
	struct cache_detail	*cd;
	struct rcu_head		ex_rcu;
	unsigned long		ex_xprtsec_modes;
	struct export_stats	*ex_stats;
};

/* an "export key" (expkey) maps a filehandlefragement to an
 * svc_export for a given client.  There can be several per export,
 * for the different fsid types.
 */
/*
“导出密钥”（expkey）将文件句柄片段映射到
给定客户端的 svc_export。 每个导出可以有多个，
对于不同的 fsid 类型。
 */
struct svc_expkey {
    // cache 缓存
	struct cache_head	h;

    // 认证域
	struct auth_domain *	ek_client;
    // 导出的文件系统信息
	int			ek_fsidtype;
	u32			ek_fsid[6];

    // 文件系统根节点的路径.
	struct path		ek_path;
	struct rcu_head		ek_rcu;
};

#define EX_ISSYNC(exp)		(!((exp)->ex_flags & NFSEXP_ASYNC))
#define EX_NOHIDE(exp)		((exp)->ex_flags & NFSEXP_NOHIDE)
#define EX_WGATHER(exp)		((exp)->ex_flags & NFSEXP_GATHERED_WRITES)

int nfsexp_flags(struct svc_rqst *rqstp, struct svc_export *exp);
__be32 check_nfsd_access(struct svc_export *exp, struct svc_rqst *rqstp);

/*
 * Function declarations
 */
int			nfsd_export_init(struct net *);
void			nfsd_export_shutdown(struct net *);
void			nfsd_export_flush(struct net *);
struct svc_export *	rqst_exp_get_by_name(struct svc_rqst *,
					     struct path *);
struct svc_export *	rqst_exp_parent(struct svc_rqst *,
					struct path *);
struct svc_export *	rqst_find_fsidzero_export(struct svc_rqst *);
int			exp_rootfh(struct net *, struct auth_domain *,
					char *path, struct knfsd_fh *, int maxsize);
__be32			exp_pseudoroot(struct svc_rqst *, struct svc_fh *);

static inline void exp_put(struct svc_export *exp)
{
	cache_put(&exp->h, exp->cd);
}

static inline struct svc_export *exp_get(struct svc_export *exp)
{
	cache_get(&exp->h);
	return exp;
}
struct svc_export * rqst_exp_find(struct svc_rqst *, int, u32 *);

#endif /* NFSD_EXPORT_H */
