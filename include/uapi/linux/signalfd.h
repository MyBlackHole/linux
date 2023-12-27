/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
/*
 *  include/linux/signalfd.h
 *
 *  Copyright (C) 2007  Davide Libenzi <davidel@xmailserver.org>
 *
 */

#ifndef _UAPI_LINUX_SIGNALFD_H
#define _UAPI_LINUX_SIGNALFD_H

#include <linux/types.h>
/* For O_CLOEXEC and O_NONBLOCK */
#include <linux/fcntl.h>

/* Flags for signalfd4.  */
#define SFD_CLOEXEC O_CLOEXEC
#define SFD_NONBLOCK O_NONBLOCK

struct signalfd_siginfo {
    // 信号编号
	__u32 ssi_signo;
    // 错误码
	__s32 ssi_errno;
    // 信号码
	__s32 ssi_code;
    // 发送的 pid
	__u32 ssi_pid;
    // 发送的真实 uid
	__u32 ssi_uid;
    // 文件描述符 (sigio)
	__s32 ssi_fd;
    // 内核定时器 id (posix 定时器)
	__u32 ssi_tid;
    // 波动事件 (sigio)
	__u32 ssi_band;
    // posix 定时器超过次数
	__u32 ssi_overrun;
    // 导致信号的陷阱号
	__u32 ssi_trapno;
    // 退出状态或信号
	__s32 ssi_status;
    // sigqueue 发送的整数
	__s32 ssi_int;
    // sigqueue 发送的指针
	__u64 ssi_ptr;
    // 用户 cpu 消耗
	__u64 ssi_utime;
    // 系统 cpu 消耗
	__u64 ssi_stime;
    // 产生信号的地址(对硬件信号)
	__u64 ssi_addr;
	__u16 ssi_addr_lsb;
	__u16 __pad2;
	__s32 ssi_syscall;
	__u64 ssi_call_addr;
	__u32 ssi_arch;

	/*
	 * Pad strcture to 128 bytes. Remember to update the
	 * pad size when you add new members. We use a fixed
	 * size structure to avoid compatibility problems with
	 * future versions, and we leave extra space for additional
	 * members. We use fixed size members because this strcture
	 * comes out of a read(2) and we really don't want to have
	 * a compat on read(2).
     *
     * 保留
	 */
	__u8 __pad[28];
};



#endif /* _UAPI_LINUX_SIGNALFD_H */
