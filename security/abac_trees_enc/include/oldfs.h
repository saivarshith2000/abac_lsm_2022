#ifndef _ABAC_FS_H_
#define _ABAC_FS_H_

#include "avp.h"
#include "env.h"
#include "obj.h"
#include "policy.h"
#include "user.h"

/* Pointer to the abac policy. Initialized in abacfs */
extern abac_policy *policy;

/* Pointer to the user attribute list. Initialized in abacfs */
extern abac_user *user_attr;

/* Pointer to the object attribute list. Initialized in abacfs */
extern abac_obj *obj_attr;

/* Pointer to the environment attribute list. Initialized in abacfs */
extern avp *env_attr;

#endif /* _ABAC_FS_H */
