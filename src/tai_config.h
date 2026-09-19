#pragma once
#include <stddef.h>
#define TAI_MAX_LINES 512
#define TAI_LINE_LEN 256
typedef enum { TAI_BLANK, TAI_COMMENT, TAI_SECTION, TAI_PLUGIN, TAI_OTHER } TaiLineType;
typedef struct {
  char text[TAI_LINE_LEN];
  TaiLineType type;
  char section[64];
  int enabled;
} TaiLine;
typedef struct {
  TaiLine lines[TAI_MAX_LINES];
  int count;
} TaiConfig;
int tai_parse_file(const char *path, TaiConfig *cfg);
int tai_write_file(const char *path, const TaiConfig *cfg);
int tai_set_plugin_enabled(TaiConfig *cfg, const char *plugin_path, int enabled);
