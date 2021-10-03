#include "posix.h"

#include <string.h>

#include "external/pkgconf.h"
#include "functions/common.h"
#include "functions/dependency.h"
#include "lang/interpreter.h"
#include "log.h"

static bool
func_dependency_found(struct workspace *wk, uint32_t rcvr, uint32_t args_node, uint32_t *obj)
{
	if (!interp_args(wk, args_node, NULL, NULL, NULL)) {
		return false;
	}

	struct obj *res = make_obj(wk, obj, obj_bool);
	res->dat.boolean = (get_obj(wk, rcvr)->dat.dep.flags & dep_flag_found)
			   == dep_flag_found;

	return true;
}

static bool
dep_get_pkgconfig_variable(struct workspace *wk, uint32_t dep, uint32_t node, uint32_t var, uint32_t *obj)
{
	if (!(get_obj(wk, dep)->dat.dep.flags & dep_flag_pkg_config)) {
		interp_error(wk, node, "this dependency is not from pkg_config");
		return false;
	}

	uint32_t res;
	if (!muon_pkgconf_get_variable(wk, get_cstr(wk, get_obj(wk, dep)->dat.dep.name), get_cstr(wk, var), &res)) {
		interp_error(wk, node, "undefined pkg_config variable");
		return false;
	}

	make_obj(wk, obj, obj_string)->dat.str = res;
	return true;
}

static bool
func_dependency_get_pkgconfig_variable(struct workspace *wk, uint32_t rcvr,
	uint32_t args_node, uint32_t *obj)
{
	struct args_norm an[] = { { obj_string }, ARG_TYPE_NULL };
	if (!interp_args(wk, args_node, an, NULL, NULL)) {
		return false;
	}

	return dep_get_pkgconfig_variable(wk, rcvr, an[0].node, an[0].val, obj);
}

static bool
func_dependency_get_variable(struct workspace *wk, uint32_t rcvr,
	uint32_t args_node, obj *res)
{
	struct args_norm ao[] = { { obj_string }, ARG_TYPE_NULL };
	enum kwargs {
		kw_pkgconfig,
	};
	struct args_kw akw[] = {
		[kw_pkgconfig] = { "pkgconfig", obj_string },
		0
	};
	if (!interp_args(wk, args_node, NULL, ao, akw)) {
		return false;
	}

	struct obj *dep = get_obj(wk, rcvr);
	if (ao[0].set) {
		if (dep->dat.dep.variables) {
			if (!obj_dict_index(wk, dep->dat.dep.variables, ao[0].val, res)) {
				interp_error(wk, ao[0].node, "undefined variable");
				return false;
			}
			return true;
		} else {
			return dep_get_pkgconfig_variable(wk, rcvr, akw[kw_pkgconfig].node, ao[0].val, res);
		}
	} else if (akw[kw_pkgconfig].set) {
		return dep_get_pkgconfig_variable(wk, rcvr, akw[kw_pkgconfig].node, akw[kw_pkgconfig].val, res);
	} else {
		interp_error(wk, args_node, "I don't know how to get this type of variable");
		return false;
	}
}

static bool
func_dependency_version(struct workspace *wk, uint32_t rcvr, uint32_t args_node, uint32_t *obj)
{
	if (!interp_args(wk, args_node, NULL, NULL, NULL)) {
		return false;
	}

	uint32_t version = get_obj(wk, rcvr)->dat.dep.version;

	if (version) {
		make_obj(wk, obj, obj_string)->dat.str = version;
		return true;
	}

	make_obj(wk, obj, obj_string)->dat.str = wk_str_push(wk, "unknown");
	return true;
}

const struct func_impl_name impl_tbl_dependency[] = {
	{ "found", func_dependency_found },
	{ "get_pkgconfig_variable", func_dependency_get_pkgconfig_variable },
	{ "get_variable", func_dependency_get_variable },
	{ "version", func_dependency_version },
	{ NULL, NULL },
};
