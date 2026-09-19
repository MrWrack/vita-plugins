#pragma once
#include <stddef.h>
int mrwrack_sha256_file(const char *path, char out_hex[65]);
int mrwrack_verify_sha256(const char *path, const char *expected_hex);
