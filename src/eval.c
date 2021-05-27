#include "posix.h"

#include "eval.h"
#include "filesystem.h"
#include "interpreter.h"
#include "log.h"
#include "mem.h"
#include "parser.h"

bool
eval_entry(enum language_mode mode, struct workspace *wk,
	const char *src, const char *cwd, const char *build_dir)
{
	workspace_init(wk);
	wk->lang_mode = mode;

	struct project *proj = make_project(wk, &wk->cur_project);

	proj->cwd = wk_str_push(wk, cwd);
	proj->build_dir = wk_str_push(wk, build_dir);

	return eval(wk, src);
}

bool
eval(struct workspace *wk, const char *src)
{
	/* L(log_misc, "evaluating '%s'", src); */

	struct tokens *toks = &current_project(wk)->toks;
	struct ast *ast = &current_project(wk)->ast, *old_ast = wk->ast;

	if (!lexer_lex(wk->lang_mode, toks, src)) {
		return false;
	} else if (!parser_parse(ast, toks)) {
		return false;
	}

	wk->ast = ast;
	if (!interpreter_interpret(wk)) {
		wk->ast = old_ast;
		return false;
	}

	wk->ast = old_ast;
	return true;
}

void
error_message(const char *file, uint32_t line, uint32_t col, const char *fmt, va_list args)
{
	const char *label = log_clr() ? "\033[31merror:\033[0m" : "error:";

	log_plain("%s:%d:%d: %s ", file, line, col, label);
	log_plainv(fmt, args);
	log_plain("\n");

	char *buf;
	uint64_t len, i, cl = 1, sol = 0;
	if (fs_read_entire_file(file, &buf, &len)) {
		for (i = 0; i < len; ++i) {
			if (buf[i] == '\n') {
				++cl;
				sol = i + 1;
			}

			if (cl == line) {
				break;
			}
		}

		log_plain("%3d | ", line);
		for (i = sol; buf[i] && buf[i] != '\n'; ++i) {
			if (buf[i] == '\t') {
				log_plain("        ");
			} else {
				putc(buf[i], stderr);
			}
		}
		log_plain("\n");

		log_plain("      ");
		for (i = 0; i < col; ++i) {
			if (buf[sol + i] == '\t') {
				log_plain("        ");
			} else {
				log_plain(i == col - 1 ? "^" : " ");
			}
		}
		log_plain("\n");

		z_free(buf);
	}
}

void
error_messagef(const char *file, uint32_t line, uint32_t col, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	error_message(file, line, col, fmt, ap);
	va_end(ap);
}
