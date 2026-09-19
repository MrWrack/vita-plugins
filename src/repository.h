#pragma once
#include <stdint.h>
#define REPO_MAX_PLUGINS 64
typedef struct {
 char id[48], name[80], version[24], developer[64], category[48];
 char file_url[256], sha256[65], section[64], description[180];
 uint64_t expected_size;
 int default_enabled;
} RepoPlugin;
typedef struct { RepoPlugin plugins[REPO_MAX_PLUGINS]; int count; } Repository;
int repository_load_local(const char *path, Repository *repo);
