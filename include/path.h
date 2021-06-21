#ifndef MUON_PATH_H
#define MUON_PATH_H
#include <stdbool.h>
#include <stdint.h>

bool path_init(void);
bool path_cwd(char *buf, uint32_t len);
bool path_is_absolute(const char *path);
bool path_join(char *buf, uint32_t len, const char *a, const char *b);
bool path_make_absolute(char *buf, uint32_t len, const char *path);
bool path_relative_to(char *buf, uint32_t len, const char *base, const char *path);
bool path_is_basename(const char *path);
bool path_basename(char *buf, uint32_t len, const char *path);
bool path_dirname(char *buf, uint32_t len, const char *path);
bool path_is_subpath(const char *base, const char *sub);
bool path_add_suffix(char *path, uint32_t len, const char *suff);
#endif
