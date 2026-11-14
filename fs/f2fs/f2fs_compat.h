/* SPDX-License-Identifier: GPL-2.0 */
/*
 * The kernel helpers that the f2fs of linux-4.4.y needs on 3.4.
 */
#ifndef _F2FS_COMPAT_H
#define _F2FS_COMPAT_H

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/bio.h>
#include <linux/blkdev.h>
#include <linux/completion.h>
#include <linux/fs.h>
#include <linux/genhd.h>
#include <linux/list.h>
#include <linux/llist.h>
#include <linux/mm.h>
#include <linux/pagemap.h>
#include <linux/pagevec.h>
#include <linux/proc_fs.h>
#include <linux/rbtree.h>
#include <linux/swap.h>
#include <linux/time.h>
#include <linux/backing-dev.h>

/* Tracing: 3.4 can not export the enums to the user space */
#ifndef TRACE_DEFINE_ENUM
#define TRACE_DEFINE_ENUM(a)
#endif

/* linux/overflow.h */
static inline size_t f2fs_compat_array_size(size_t a, size_t b)
{
	if (b && a > SIZE_MAX / b)
		return SIZE_MAX;
	return a * b;
}
#define array_size(a, b)	f2fs_compat_array_size(a, b)

/* linux/list.h */
#ifndef list_last_entry
#define list_last_entry(ptr, type, member) \
	list_entry((ptr)->prev, type, member)
#endif

/* linux/rbtree.h */
#ifndef rb_entry_safe
#define rb_entry_safe(ptr, type, member) \
	({ typeof(ptr) ____ptr = (ptr); \
	   ____ptr ? rb_entry(____ptr, type, member) : NULL; \
	})
#endif

/* linux/llist.h */
#ifndef llist_for_each_entry_safe
#define llist_for_each_entry_safe(pos, n, node, member)			       \
	for (pos = llist_entry((node), typeof(*pos), member);		       \
	     &pos->member != NULL &&					       \
	        (n = llist_entry(pos->member.next, typeof(*n), member), true); \
	     pos = n)
#endif

static inline struct llist_node *f2fs_llist_reverse_order(
						struct llist_node *head)
{
	struct llist_node *new_head = NULL;

	while (head) {
		struct llist_node *tmp = head;

		head = head->next;
		tmp->next = new_head;
		new_head = tmp;
	}

	return new_head;
}
#define llist_reverse_order(head)	f2fs_llist_reverse_order(head)

/* Time */
typedef __s64 time64_t;
#define ktime_get_real_seconds()	((time64_t)get_seconds())

/* Project ids: 3.4 has no project quota */
typedef __kernel_uid32_t projid_t;
typedef struct {
	projid_t val;
} kprojid_t;

static inline kprojid_t make_kprojid(struct user_namespace *from,
							projid_t projid)
{
	return (kprojid_t){ projid };
}

static inline projid_t from_kprojid(struct user_namespace *to,
							kprojid_t kprojid)
{
	return kprojid.val;
}

static inline bool projid_eq(kprojid_t left, kprojid_t right)
{
	return left.val == right.val;
}

/* Encryption is not supported */
#define IS_ENCRYPTED(inode)	0

#ifndef FS_ENCRYPT_FL
#define FS_ENCRYPT_FL			0x00000800 /* Encrypted file */
#endif
#ifndef FS_INLINE_DATA_FL
#define FS_INLINE_DATA_FL		0x10000000 /* Reserved for ext4 */
#endif

#ifndef FS_PROJINHERIT_FL
#define FS_PROJINHERIT_FL		0x20000000 /* Create with parents projid */
#endif

#ifndef S_ENCRYPTED
#define S_ENCRYPTED	0
#endif

/* 3.4 has no IOCB_NOWAIT */
#ifndef FMODE_NOWAIT
#define FMODE_NOWAIT	0
#endif

#ifndef FIEMAP_FLAG_CACHE
#define FIEMAP_FLAG_CACHE	0x00000004 /* request caching of the extents */
#endif

/* The fallocate() of 3.4 refuses the modes except these */
#ifndef FALLOC_FL_COLLAPSE_RANGE
#define FALLOC_FL_COLLAPSE_RANGE	0x08
#endif
#ifndef FALLOC_FL_ZERO_RANGE
#define FALLOC_FL_ZERO_RANGE		0x10
#endif
#ifndef FALLOC_FL_INSERT_RANGE
#define FALLOC_FL_INSERT_RANGE		0x20
#endif

/*
 * The quota of f2fs needs the quota of newer kernels, so it is compiled out
 * (CONFIG_F2FS_QUOTA_COMPAT). Do what quotaops.h of 4.4 does without
 * CONFIG_QUOTA: the quota of 3.4 would reserve the space through the
 * dq_op that f2fs does not set.
 */
#ifndef CONFIG_F2FS_QUOTA_COMPAT
#define dquot_initialize(inode)		({ (void)(inode); 0; })
#define dquot_drop(inode)		do { (void)(inode); } while (0)
#define dquot_alloc_inode(inode)	({ (void)(inode); 0; })
#define dquot_free_inode(inode)		do { (void)(inode); } while (0)
#define dquot_transfer(inode, iattr)	({ (void)(inode); (void)(iattr); 0; })
#define dquot_file_open(inode, file)	generic_file_open(inode, file)

static inline int f2fs_nq_reserve_block(struct inode *inode, qsize_t nr)
{
	return 0;
}
#define dquot_reserve_block(inode, nr)	f2fs_nq_reserve_block(inode, nr)
#define dquot_release_reservation_block(inode, nr) \
	do { (void)(inode); (void)(nr); } while (0)

static inline void f2fs_nq_add_block(struct inode *inode, qsize_t nr)
{
	inode_add_bytes(inode, nr << inode->i_blkbits);
	mark_inode_dirty_sync(inode);
}
#define dquot_claim_block(inode, nr)		f2fs_nq_add_block(inode, nr)
#define dquot_alloc_block_nofail(inode, nr)	f2fs_nq_add_block(inode, nr)

static inline void f2fs_nq_free_block(struct inode *inode, qsize_t nr)
{
	inode_sub_bytes(inode, nr << inode->i_blkbits);
	mark_inode_dirty_sync(inode);
}
#define dquot_free_block(inode, nr)	f2fs_nq_free_block(inode, nr)
#endif

/* The rename of 3.4 has no flags: f2fs never sees these */
#ifndef RENAME_NOREPLACE
#define RENAME_NOREPLACE	(1 << 0)	/* Don't overwrite target */
#define RENAME_EXCHANGE		(1 << 1)	/* Exchange source and dest */
#define RENAME_WHITEOUT		(1 << 2)	/* Whiteout source */
#endif
#ifndef WHITEOUT_DEV
#define WHITEOUT_MODE 0
#define WHITEOUT_DEV 0
#endif

/* The log levels of 3.4 look like "<3>" */
static inline int printk_get_level(const char *buffer)
{
	if (buffer[0] == '<' && buffer[1] && buffer[2] == '>') {
		switch (buffer[1]) {
		case '0' ... '7':
		case 'd':	/* KERN_DEFAULT */
		case 'c':	/* KERN_CONT */
			return buffer[1];
		}
	}
	return 0;
}

static inline const char *printk_skip_level(const char *buffer)
{
	if (printk_get_level(buffer))
		return buffer + 3;
	return buffer;
}

/* 3.4 never dirties only the times of the inodes */
#ifndef I_DIRTY_TIME
#define I_DIRTY_TIME	(1 << 30)
#endif

/* f2fs is built in: evict_inodes() is in fs/internal.h on 3.4 */
extern void evict_inodes(struct super_block *sb);

/* There is no lazytime on 3.4 */
#ifndef MS_LAZYTIME
#define MS_LAZYTIME	0
#endif

/* Inode locks */
static inline void inode_lock(struct inode *inode)
{
	mutex_lock(&inode->i_mutex);
}

static inline void inode_unlock(struct inode *inode)
{
	mutex_unlock(&inode->i_mutex);
}

static inline int inode_trylock(struct inode *inode)
{
	return mutex_trylock(&inode->i_mutex);
}

/* Freeze protection: 3.4 only has vfs_check_frozen() */
#define sb_start_pagefault(sb)	vfs_check_frozen(sb, SB_FREEZE_WRITE)
#define sb_end_pagefault(sb)	do { } while (0)
#define sb_start_intwrite(sb)	do { } while (0)
#define sb_end_intwrite(sb)	do { } while (0)
#define sb_start_write(sb)	vfs_check_frozen(sb, SB_FREEZE_WRITE)
#define sb_end_write(sb)	do { } while (0)
#define sb_start_write_trylock(sb)	((sb)->s_frozen < SB_FREEZE_WRITE)

/* clear_inode() was end_writeback() */
#define clear_inode(inode)	end_writeback(inode)

#define truncate_inode_pages_final(mapping)	truncate_inode_pages(mapping, 0)

/* The old size is not used by truncate_pagecache() of 3.4 */
#define truncate_pagecache(inode, newsize) \
	truncate_pagecache(inode, 0, newsize)

/* 3.4 has no stable pages */
#define wait_for_stable_page(page)	do { } while (0)

#define PDE_DATA(inode)		(PDE(inode)->data)

/* Writeback */
#define wbc_init_bio(wbc, bio)			do { } while (0)
#define wbc_account_io(wbc, page, bytes)	do { } while (0)

/* Write life time hints */
enum rw_hint {
	WRITE_LIFE_NOT_SET	= 0,
	WRITE_LIFE_NONE		= 1,
	WRITE_LIFE_SHORT	= 2,
	WRITE_LIFE_MEDIUM	= 3,
	WRITE_LIFE_LONG		= 4,
	WRITE_LIFE_EXTREME	= 5,
};

/* bio */
#ifndef bio_for_each_segment_all
#define bio_for_each_segment_all(bvl, bio, i)	\
	__bio_for_each_segment(bvl, bio, i, 0)
#endif

/* submit_bio_wait() of 3.10 */
struct f2fs_submit_bio_ret {
	struct completion event;
	int error;
};

static void f2fs_submit_bio_wait_endio(struct bio *bio, int error)
{
	struct f2fs_submit_bio_ret *ret = bio->bi_private;

	ret->error = error;
	complete(&ret->event);
}

static inline int f2fs_submit_bio_wait(int rw, struct bio *bio)
{
	struct f2fs_submit_bio_ret ret;

	rw |= REQ_SYNC;
	init_completion(&ret.event);
	bio->bi_private = &ret;
	bio->bi_end_io = f2fs_submit_bio_wait_endio;
	submit_bio(rw, bio);
	wait_for_completion(&ret.event);

	return ret.error;
}
#define submit_bio_wait(rw, bio)	f2fs_submit_bio_wait(rw, bio)

/* bdev_discard_alignment() of 3.7 */
static inline int f2fs_bdev_discard_alignment(struct block_device *bdev)
{
	struct request_queue *q = bdev_get_queue(bdev);

	if (bdev != bdev->bd_contains)
		return bdev->bd_part->discard_alignment;

	return q->limits.discard_alignment;
}
#define bdev_discard_alignment(bdev)	f2fs_bdev_discard_alignment(bdev)

/*
 * The iov_iter of 3.4 only walks the iovecs and does not know the direction
 * of the I/O: carry it next to the iterator. Only the aio_write and
 * direct_IO of f2fs make these.
 */
struct f2fs_iov_iter {
	struct iov_iter iter;
	int rw;
};

static inline void f2fs_iov_iter_init(struct f2fs_iov_iter *i, int rw,
		const struct iovec *iov, unsigned long nr_segs, size_t count)
{
	iov_iter_init(&i->iter, iov, nr_segs, count, 0);
	i->rw = (rw & WRITE) ? WRITE : READ;
}

#define iov_iter_rw(i)	(container_of(i, struct f2fs_iov_iter, iter)->rw)

/* iov_iter_alignment() of 3.16 for the iovecs */
static inline unsigned long iov_iter_alignment(const struct iov_iter *i)
{
	const struct iovec *iov = i->iov;
	size_t size = i->count;
	size_t skip = i->iov_offset;
	unsigned long res = 0;

	while (size) {
		size_t len = min(iov->iov_len - skip, size);

		if (len) {
			res |= (unsigned long)iov->iov_base + skip;
			res |= len;
			size -= len;
		}
		skip = 0;
		iov++;
	}

	return res;
}

/* 3.4 tells the direct I/O with O_DIRECT and can not do it without waiting */
#define f2fs_iocb_direct(iocb)	((iocb)->ki_filp->f_flags & O_DIRECT)

/* readdir: the dir_context and dir_emit() of 3.11 on top of filldir */
struct dir_context {
	filldir_t actor;
	loff_t pos;
	void *dirent;
};

static inline bool dir_emit(struct dir_context *ctx, const char *name,
			int namelen, u64 ino, unsigned type)
{
	return ctx->actor(ctx->dirent, name, namelen, ctx->pos, ino, type) == 0;
}

/* Page cache: pagecache_get_page() of 3.16 */
#define FGP_ACCESSED		0x00000001
#define FGP_LOCK		0x00000002
#define FGP_CREAT		0x00000004
#define FGP_WRITE		0x00000008
#define FGP_NOFS		0x00000010
#define FGP_NOWAIT		0x00000020

static inline struct page *f2fs_compat_pagecache_get_page(
			struct address_space *mapping, pgoff_t offset,
			int fgp_flags, gfp_t gfp_mask)
{
	struct page *page;

repeat:
	page = find_get_page(mapping, offset);
	if (!page)
		goto no_page;

	if (fgp_flags & FGP_LOCK) {
		if (fgp_flags & FGP_NOWAIT) {
			if (!trylock_page(page)) {
				page_cache_release(page);
				return NULL;
			}
		} else {
			lock_page(page);
		}

		/* Has the page been truncated? */
		if (unlikely(page->mapping != mapping)) {
			unlock_page(page);
			page_cache_release(page);
			goto repeat;
		}
		VM_BUG_ON(page->index != offset);
	}

	if (fgp_flags & FGP_ACCESSED)
		mark_page_accessed(page);

no_page:
	if (!page && (fgp_flags & FGP_CREAT)) {
		int err;

		if ((fgp_flags & FGP_WRITE) &&
				mapping_cap_account_dirty(mapping))
			gfp_mask |= __GFP_WRITE;
		if (fgp_flags & FGP_NOFS)
			gfp_mask &= ~__GFP_FS;

		page = __page_cache_alloc(gfp_mask);
		if (!page)
			return NULL;

		if (WARN_ON_ONCE(!(fgp_flags & FGP_LOCK)))
			fgp_flags |= FGP_LOCK;

		err = add_to_page_cache_lru(page, mapping, offset,
					gfp_mask & ~__GFP_HIGHMEM);
		if (unlikely(err)) {
			page_cache_release(page);
			page = NULL;
			if (err == -EEXIST)
				goto repeat;
		}

		if (page && (fgp_flags & FGP_ACCESSED))
			mark_page_accessed(page);
	}

	return page;
}
#define pagecache_get_page(m, o, f, g) \
	f2fs_compat_pagecache_get_page(m, o, f, g)
#define find_get_page_flags(m, o, f) \
	f2fs_compat_pagecache_get_page(m, o, f, 0)

/* pagevec_lookup_tag() takes the number of the pages on 3.4 */
static inline unsigned f2fs_pagevec_lookup_range_tag(struct pagevec *pvec,
		struct address_space *mapping, pgoff_t *index, pgoff_t end,
		int tag)
{
	unsigned nr_pages = PAGEVEC_SIZE;
	unsigned i, j;

	if (end != (pgoff_t)-1) {
		if (*index > end)
			return 0;
		nr_pages = min_t(pgoff_t, end - *index, PAGEVEC_SIZE - 1) + 1;
	}

	nr_pages = (pagevec_lookup_tag)(pvec, mapping, index, tag, nr_pages);

	/* The tagged lookup of 3.4 does not stop at the end of the range */
	for (i = 0; i < nr_pages; i++)
		if (pvec->pages[i]->index > end)
			break;
	if (i < nr_pages) {
		for (j = i; j < nr_pages; j++)
			page_cache_release(pvec->pages[j]);
		pvec->nr = i;
		nr_pages = i;
		*index = end + 1;
	}

	return nr_pages;
}
#define pagevec_lookup_range_tag(pvec, mapping, index, end, tag) \
	f2fs_pagevec_lookup_range_tag(pvec, mapping, index, end, tag)
#define pagevec_lookup_tag(pvec, mapping, index, tag) \
	(pagevec_lookup_tag)(pvec, mapping, index, tag, PAGEVEC_SIZE)

/* 3.4 counts the dirty exceeding in the bdi */
#define f2fs_bdi_dirty_exceeded(sb)	((sb)->s_bdi->dirty_exceeded)

/* percpu_counter_init() of 3.4 can not take the gfp mask */
#define f2fs_percpu_counter_init(fbc, value, gfp) \
	percpu_counter_init(fbc, value)

/* The set of the inode flags: inode_set_flags() of 3.15 */
static inline void inode_set_flags(struct inode *inode, unsigned int flags,
				   unsigned int mask)
{
	unsigned int old_flags, new_flags;

	WARN_ON_ONCE(flags & ~mask);
	do {
		old_flags = ACCESS_ONCE(inode->i_flags);
		new_flags = (old_flags & ~mask) | flags;
	} while (unlikely(cmpxchg(&inode->i_flags, old_flags,
				  new_flags) != old_flags));
}

/* vfs_ioc_setflags_prepare() of 5.3 */
static inline int vfs_ioc_setflags_prepare(struct inode *inode,
					unsigned int oldflags,
					unsigned int flags)
{
	/*
	 * The IMMUTABLE and APPEND_ONLY flags can only be changed by the
	 * relevant capability.
	 */
	if ((flags ^ oldflags) & (FS_APPEND_FL | FS_IMMUTABLE_FL) &&
	    !capable(CAP_LINUX_IMMUTABLE))
		return -EPERM;

	return 0;
}

/* vfs_setpos() of 3.11 */
static inline loff_t vfs_setpos(struct file *file, loff_t offset,
				loff_t maxsize)
{
	if (offset < 0 && !(file->f_mode & FMODE_UNSIGNED_OFFSET))
		return -EINVAL;
	if (offset > maxsize)
		return -EINVAL;

	if (offset != file->f_pos) {
		file->f_pos = offset;
		file->f_version = 0;
	}
	return offset;
}

#endif /* _F2FS_COMPAT_H */
