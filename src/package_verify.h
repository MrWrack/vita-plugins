#pragma once
#include <stdint.h>
typedef enum {
 PKG_CHECK_OK=0, PKG_CHECK_NOT_FOUND=-1, PKG_CHECK_EMPTY=-2,
 PKG_CHECK_BAD_ZIP=-3, PKG_CHECK_TOO_SMALL=-4
} PackageCheckResult;
PackageCheckResult mrwrack_check_vpk_container(const char *path, uint64_t *size_out);
