// SPDX-License-Identifier: GPL-2.0
/*
 *	linux/mm/msync.c
 *
 * Copyright (C) 1994-1999  Linus Torvalds
 */

// ============================================
// 文件说明：msync() 系统调用实现
// 作用：将 mmap 映射的文件数据同步回磁盘
// 核心流程：遍历 VMA 区间，调用 vfs_fsync_range 刷出脏页
// ============================================
//
// msync() 逻辑流图：
//
//   msync(start, len, flags)
//     │
//     ├── 参数校验（对齐、flag 合法性）
//     │
//     ├── MS_ASYNC ──→ 直接返回 0（空操作，脏页由内核自动跟踪）
//     │
//     └── MS_SYNC  ──→ 遍历 [start, end) 所有 VMA
//           │
//           ├── VMA 未映射文件 ──→ 跳过
//           │
//           ├── VMA 非 VM_SHARED ──→ 跳过
//           │
//           └── VMA 映射文件 + VM_SHARED
//                   │
//                   ▼
//             vfs_fsync_range(file, fstart, fend, 1)
//                   │
//                   ├── 文件系统 ->fsync() 实现
//                   ├── 刷出该范围内所有脏页
//                   └── datasync=1 表示等待 I/O 完成
//                   │
//                   ▼
//             继续 / 返回
//

/*
 * The msync() system call.
 */
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/mman.h>
#include <linux/file.h>
#include <linux/syscalls.h>
#include <linux/sched.h>

/*
 * MS_SYNC syncs the entire file - including mappings.
 *
 * MS_ASYNC does not start I/O (it used to, up to 2.5.67).
 * Nor does it marks the relevant pages dirty (it used to up to 2.6.17).
 * Now it doesn't do anything, since dirty pages are properly tracked.
 *
 * The application may now run fsync() to
 * write out the dirty pages and wait on the writeout and check the result.
 * Or the application may run fadvise(FADV_DONTNEED) against the fd to start
 * async writeout immediately.
 * So by _not_ starting I/O in MS_ASYNC we provide complete flexibility to
 * applications.
 */
// 备注：msync 系统调用，将 mmap 映射的脏页同步回磁盘
// 备注：MS_SYNC — 同步刷出并等待完成（调用 vfs_fsync_range）
// 备注：MS_ASYNC — 当前为空操作（脏页由内核自动跟踪，无需在此触发 I/O）
// 备注：MS_INVALIDATE — 使缓存无效，丢弃未持久化的脏页
// 备注：遍历起始地址到结束地址之间的所有 VMA，对 VM_SHARED 映射执行 fsync
SYSCALL_DEFINE3(msync, unsigned long, start, size_t, len, int, flags)
{
	unsigned long end;
	struct mm_struct *mm = current->mm;
	struct vm_area_struct *vma;
	int unmapped_error = 0;
	int error = -EINVAL;

	start = untagged_addr(start);

	if (flags & ~(MS_ASYNC | MS_INVALIDATE | MS_SYNC))
		goto out;
	if (offset_in_page(start))
		goto out;
	if ((flags & MS_ASYNC) && (flags & MS_SYNC))
		goto out;
	error = -ENOMEM;
	len = (len + ~PAGE_MASK) & PAGE_MASK;
	end = start + len;
	if (end < start)
		goto out;
	error = 0;
	if (end == start)
		goto out;
	/*
	 * If the interval [start,end) covers some unmapped address ranges,
	 * just ignore them, but return -ENOMEM at the end. Besides, if the
	 * flag is MS_ASYNC (w/o MS_INVALIDATE) the result would be -ENOMEM
	 * anyway and there is nothing left to do, so return immediately.
	 */
	// 备注：遍历 VMA 链表，对每个映射了文件的共享 VMA 执行同步
	mmap_read_lock(mm);
	vma = find_vma(mm, start);
	for (;;) {
		struct file *file;
		loff_t fstart, fend;

		/* Still start < end. */
		error = -ENOMEM;
		if (!vma)
			goto out_unlock;
		/* Here start < vma->vm_end. */
		if (start < vma->vm_start) {
			if (flags == MS_ASYNC)
				goto out_unlock;
			start = vma->vm_start;
			if (start >= end)
				goto out_unlock;
			unmapped_error = -ENOMEM;
		}
		/* Here vma->vm_start <= start < vma->vm_end. */
		if ((flags & MS_INVALIDATE) &&
				(vma->vm_flags & VM_LOCKED)) {
			error = -EBUSY;
			goto out_unlock;
		}
		file = vma->vm_file;
		fstart = (start - vma->vm_start) +
			 ((loff_t)vma->vm_pgoff << PAGE_SHIFT);
		fend = fstart + (min(end, vma->vm_end) - start) - 1;
		start = vma->vm_end;
		// 备注：MS_SYNC 模式 —— 通过 VFS 层对文件范围执行同步刷出
		// 备注：vfs_fsync_range(file, start, end, 1) 的最后一个参数 1 表示等待 I/O 完成
		if ((flags & MS_SYNC) && file &&
				(vma->vm_flags & VM_SHARED)) {
			get_file(file);
			mmap_read_unlock(mm);
			error = vfs_fsync_range(file, fstart, fend, 1);
			fput(file);
			if (error || start >= end)
				goto out;
			mmap_read_lock(mm);
			vma = find_vma(mm, start);
		} else {
			if (start >= end) {
				error = 0;
				goto out_unlock;
			}
			vma = find_vma(mm, vma->vm_end);
		}
	}
out_unlock:
	mmap_read_unlock(mm);
out:
	return error ? : unmapped_error;
}
