#ifndef __CGROUP_INTERNAL_H
#define __CGROUP_INTERNAL_H

#include <linux/cgroup.h>

int __cgroup_task_count(const struct cgroup *cgrp);

#endif /* __CGROUP_INTERNAL_H */
