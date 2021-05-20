#include "posix.h"

#include <stdarg.h>
#include <string.h>

#include "interpreter.h"
#include "log.h"
#include "workspace.h"

struct obj *
get_obj(struct workspace *wk, uint32_t id)
{
	return darr_get(&wk->objs, id);
}

bool
get_obj_id(struct workspace *wk, const char *name, uint32_t *id, uint32_t proj_id)
{
	uint64_t *idp;
	struct project *proj = darr_get(&wk->projects, proj_id);
	L(log_misc, "%d", proj_id);

	if ((idp = hash_get(&proj->scope, name))) {
		*id = *idp;
		return true;
	} else if ((idp = hash_get(&wk->scope, name))) {
		*id = *idp;
		return true;
	} else {
		LOG_W(log_interp, "unknown object name '%s'", name);
		return false;
	}
}

struct obj *
make_obj(struct workspace *wk, uint32_t *id, enum obj_type type)
{
	*id = darr_push(&wk->objs, &(struct obj){ .type = type });
	return darr_get(&wk->objs, *id);
}

#define BUF_SIZE 2048

uint32_t
wk_str_push(struct workspace *wk, const char *str)
{
	uint32_t len, ret;

	char buf[BUF_SIZE + 1] = { 0 };
	strncpy(buf, str, BUF_SIZE);

	len = strlen(buf) + 1;
	ret = wk->strs.len;

	darr_grow_by(&wk->strs, len);
	strncpy(darr_get(&wk->strs, ret), buf, len);

	return ret;
}

uint32_t
wk_str_vpushf(struct workspace *wk, const char *fmt, va_list args)
{
	uint32_t len, ret;

	char buf[BUF_SIZE + 1] = { 0 };
	vsnprintf(buf, BUF_SIZE, fmt,  args);

	len = strlen(buf) + 1;
	ret = wk->strs.len;

	darr_grow_by(&wk->strs, len);
	strncpy(darr_get(&wk->strs, ret), buf, len);

	return ret;
}

uint32_t
wk_str_pushf(struct workspace *wk, const char *fmt, ...)
{
	uint32_t ret;
	va_list args;
	va_start(args, fmt);
	ret = wk_str_vpushf(wk, fmt, args);
	va_end(args);
	return ret;
}

void
wk_strappf(struct workspace *wk, uint32_t *id, const char *fmt, ...)
{
	uint32_t curlen;
	const char *cur = wk_str(wk, *id);

	curlen = strlen(cur);

	if (*id + curlen + 1 == wk->strs.len) {
		/* L(log_misc, "str '%s' is already at the end of pool", cur); */
	} else {
		/* L(log_misc, "moving '%s' to the end of pool", cur); */
		*id = wk_str_push(wk, cur);
	}

	assert(wk->strs.len);
	--wk->strs.len;

	va_list args;
	va_start(args, fmt);
	wk_str_vpushf(wk, fmt, args);
	va_end(args);
}

char *
wk_str(struct workspace *wk, uint32_t id)
{
	return darr_get(&wk->strs, id);
}

const char *
wk_objstr(struct workspace *wk, uint32_t id)
{
	struct obj *obj = get_obj(wk, id);
	assert(obj->type == obj_string);
	return wk_str(wk, obj->dat.str);
}

static void
project_init(struct workspace *wk, struct project *proj)
{
	hash_init(&proj->scope, 128);
	darr_init(&proj->tgts, 64, sizeof(uint32_t));

	make_obj(wk, &proj->cfg.args, obj_array);
}

struct project *
make_project(struct workspace *wk, uint32_t *id)
{
	*id = darr_push(&wk->projects, &(struct project){ 0 });
	struct project *proj = darr_get(&wk->projects, *id);
	project_init(wk, proj);
	return proj;
}

struct project *
current_project(struct workspace *wk)
{
	return darr_get(&wk->projects, wk->cur_project);
}

void
workspace_init(struct workspace *wk)
{
	*wk = (struct workspace){ 0 };
	darr_init(&wk->projects, 16, sizeof(struct project));
	darr_init(&wk->objs, 1024, sizeof(struct obj));
	darr_init(&wk->strs, 2048, sizeof(char));
	hash_init(&wk->scope, 32);

	uint32_t id;
	make_obj(wk, &id, obj_null);
	assert(id == 0);

	make_obj(wk, &id, obj_meson);
	hash_set(&wk->scope, "meson", id);
}
