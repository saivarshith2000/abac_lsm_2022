# SPDX-License-Identifier: GPL-2.0-only
ccflags-y := -I$(srctree)/security/abac_rules/include/
obj-$(CONFIG_SECURITY_ABAC_RULES) := abac_lsm.o

obj-y :=  obj.o policy.o abacfs.o abac_lsm.o avp.o user.o env.o
