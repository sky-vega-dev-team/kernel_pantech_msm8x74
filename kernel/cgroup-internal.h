#ifndef __CGROUP_INTERNAL_H
#define __CGROUP_INTERNAL_H

#include <linux/cgroup.h>
#include <linux/mutex.h>
#include <linux/spinlock.h>

extern struct mutex cgroup_mutex;
extern spinlock_t css_set_lock;

/* convenient tests for these bits */
static inline bool cgroup_is_dead(const struct cgroup *cgrp)
{
	return !(cgrp->self.flags & CSS_ONLINE);
}

int __cgroup_task_count(const struct cgroup *cgrp);

#endif /* __CGROUP_INTERNAL_H */
