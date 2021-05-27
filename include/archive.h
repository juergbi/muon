#ifndef BOSON_ARCHIVE_H
#define BOSON_ARCHIVE_H

#include <stdbool.h>
#include <stdint.h>

bool archive_extract(uint8_t *data, uint64_t len, const char *destdir);
#endif
