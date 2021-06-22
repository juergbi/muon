#include "posix.h"

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h> // TODO
#include <string.h>
#include <unistd.h>

#include "log.h"
#include "path.h"

#define PATH_SEP '/'

static char cwd[PATH_MAX + 1];

bool
path_init(void)
{
	if (getcwd(cwd, PATH_MAX) == NULL) {
		LOG_W(log_misc, "getcwd failed: %s", strerror(errno));
		return false;
	}

	return true;
}

static bool
buf_push_s(char *buf, const char *s, uint32_t *i, uint32_t n)
{
	uint32_t si = 0;
	while (*i < n) {
		if (*i && buf[*i - 1] == PATH_SEP && s[si] == PATH_SEP) {
			++si;
			continue;
		}

		buf[*i] = s[si];

		if (!s[si]) {
			return true;
		}

		++(*i);
		++si;
	}

	LOG_W(log_misc, "path '%s' would be truncated", s);
	return false;
}

static bool
buf_push_sn(char *buf, const char *s, uint32_t m, uint32_t *i, uint32_t n)
{
	uint32_t si = 0;

	while (*i < n && si < m) {
		if (*i && buf[*i - 1] == PATH_SEP && s[si] == PATH_SEP) {
			++si;
			continue;
		}

		buf[*i] = s[si];

		if (!s[si]) {
			return true;
		}

		++(*i);
		++si;
	}

	if (*i + 1 >= n) {
		LOG_W(log_misc, "'%s', path '%s' would be truncated", buf, s);
		return false;
	} else {
		buf[*i] = 0;
		return true;
	}
}

static bool
buf_push_c(char *buf, char c, uint32_t *i, uint32_t n)
{
	if (*i < n) {
		if (*i && buf[*i - 1] == PATH_SEP && c == PATH_SEP) {
			return true;
		}

		buf[*i] = c;
		++(*i);
		buf[*i] = 0;
		return true;
	}

	LOG_W(log_misc, "path would be truncated");
	return false;
}

static void
buf_normalize(char *buf, uint32_t i, uint32_t len)
{
	if (i) {
		if (buf[i - 1] == PATH_SEP) {
			buf[i - 1] = 0;
		}
	}
}

static bool
simple_copy(char *buf, uint32_t len, const char *path)
{
	uint32_t i = 0;
	if (!buf_push_s(buf, path, &i, len)) {
		return false;
	}

	buf_normalize(buf, i, len);
	return true;
}

bool
path_cwd(char *buf, uint32_t len)
{
	return simple_copy(buf, len, cwd);
}


bool
path_is_absolute(const char *path)
{
	return *path == PATH_SEP;
}

bool
path_join(char *buf, uint32_t len, const char *a, const char *b)
{
	uint32_t i = 0, a_len;

	if (path_is_absolute(b) || (a_len = strlen(a)) == 0) {
		return simple_copy(buf, len, b);
	} else if (!buf_push_s(buf, a, &i, len)) {
		return false;
	} else if (!buf_push_c(buf, PATH_SEP, &i, len)) {
		return false;
	} else if (!buf_push_s(buf, b, &i, len)) {
		return false;
	}

	buf_normalize(buf, i, len);
	return true;
}

bool
path_make_absolute(char *buf, uint32_t len, const char *path)
{
	if (path_is_absolute(path)) {
		return simple_copy(buf, len, path);
	} else {
		return path_join(buf, len, cwd, path);
	}
}

bool
path_relative_to(char *buf, uint32_t len, const char *base, const char *path)
{
	/*
	 * input: base="/path/to/build/"
	 *        path="/path/to/build/tgt/dir/libfoo.a"
	 * output: "tgt/dir/libfoo.a"
	 *
	 * input: base="/path/to/build"
	 *        path="/path/to/build/libfoo.a"
	 * output: "libfoo.a"
	 *
	 * input: base="/path/to/build"
	 *        path="/path/to/src/asd.c"
	 * output: "../src/asd.c"
	 */

	if (!path_is_absolute(base)) {
		LOG_W(log_misc, "base path '%s' is not absolute", base);
		return false;
	}

	if (!path_is_absolute(path)) {
		LOG_W(log_misc, "path '%s' is not absolute", path);
		return false;
	}

	uint32_t i = 0, j = 0, common_end = 0;

	while (base[i] && path[i] && base[i] == path[i]) {
		if (base[i] == PATH_SEP) {
			common_end = i;
		}

		++i;
	}

	if (!base[i] && path[i] == PATH_SEP) {
		common_end = i;
	}

	assert(i);
	if (i == 1) {
		/* -> base and path match only at root */
		return simple_copy(buf, len, path);
	}

	if (base[common_end] && base[common_end + 1]) {
		bool have_part = true;
		i = common_end + 1;
		do {
			if (have_part) {
				if (!(buf_push_s(buf, "..", &j, len)
				      && buf_push_c(buf, PATH_SEP, &j, len))) {
					return false;
				}
				have_part = false;
			}

			if (base[i] == PATH_SEP) {
				have_part = true;
			}
			++i;
		} while (base[i]);
	}

	if (!buf_push_s(buf, &path[common_end + 1], &j, len)) {
		return false;
	}

	buf_normalize(buf, j, len);
	return true;
}

bool
path_is_basename(const char *path)
{
	return strchr(path, PATH_SEP) == NULL;
}

bool
path_basename(char *buf, uint32_t len, const char *path)
{
	int32_t i;
	uint32_t j;

	assert(len);

	buf[0] = 0;

	if (!*path) {
		return true;
	}

	for (i = strlen(path) - 1; i >= 0; --i) {
		if (path[i] == PATH_SEP) {
			++i;
			break;
		}
	}

	if (!buf_push_s(buf, &path[i], &j, len)) {
		return false;
	}

	buf_normalize(buf, j, len);
	return true;
}

bool
path_dirname(char *buf, uint32_t len, const char *path)
{
	int32_t i;
	uint32_t j = 0;

	if (!*path) {
		goto return_dot;
	}

	for (i = strlen(path) - 1; i >= 0; --i) {
		if (path[i] == PATH_SEP) {
			if (!buf_push_sn(buf, path, i, &j, len)) {
				return false;
			}

			buf_normalize(buf, j, len);
			return true;
		}
	}

return_dot:
	return buf_push_s(buf, ".", &j, len);
}

bool
path_is_subpath(const char *base, const char *sub)
{
	if (!*base) {
		return false;
	}

	uint32_t i = 0;
	while (true) {
		if (!base[i]) {
			assert(i);
			if (sub[i] == PATH_SEP || sub[i - 1] == PATH_SEP) {
				return true;
			}
		}

		if (base[i] == sub[i]) {
			if (!base[i]) {
				return true;
			}
		} else {
			return false;
		}

		assert(base[i] && sub[i]);
		++i;
	}
}

bool
path_add_suffix(char *path, uint32_t len, const char *suff)
{
	uint32_t l = strlen(path), sl = strlen(suff);

	if (l + sl + 1 >= len) {
		return false;
	}

	strcpy(&path[l], suff);
	return true;
}
