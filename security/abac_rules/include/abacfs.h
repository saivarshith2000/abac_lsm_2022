#ifndef _ABAC_FS_H_
#define _ABAC_FS_H_

#include "linux/time.h"
#include "avp.h"
#include "env.h"
#include "user.h"
#include "obj.h"
#include "policy.h"

/* Pointer to the environment attribute list. Initialized in abacfs */
extern avp *env_attr;

/* Recording performance variables. Initialized in abacfs */
extern int recording;
extern char perf_buf[64];
extern u64 prev_access_time;

#endif /* _ABAC_FS_H */
