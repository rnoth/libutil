#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <vec.h>
#include <util.h>
#include <pat.h>
#include <pat.ih>

static void ctx_prune(struct context *);
static int ctx_step(struct context *ctx, char const);

void
ctx_shift(struct context *ctx)
{
	ctx->thr = ctx->que[0];
	ctx->que[0] = 0;
	ctx->que[1] = 0;
}

int
ctx_next(struct context *ctx, char const ch)
{
	ctx_prune(ctx); 
	if (!ctx->thr) return 0;
	return ctx_step(ctx, ch);
}

int
ctx_step(struct context *ctx, char const ch)
{
	return ctx->thr->ip->op(ctx, ch);
}

void
ctx_rm(struct context *ctx)
{
	thr_mv(ctx->frl, &ctx->thr);
}

void
ctx_prune(struct context *ctx)
{
	while (ctx->thr) {
		if (thr_cmp(ctx->res, ctx->thr) < 0) {
			break;
		} else ctx_rm(ctx);
	}
}

struct thread *
ctx_get(struct context *ctx)
{
	struct thread *ret;

	if (!ctx->frl[0] && thr_alloc(ctx->frl)) {
		return 0;
	}

	ret = ctx->frl[0];
	if (ctx->frl[0] == ctx->frl[1]) {
		ctx->frl[1] = 0;
	}
	ctx->frl[0] = ctx->frl[0]->next;
	ret->next = 0x0;

	return ret;
}

void
ctx_fini(struct context *ctx)
{
	thr_free(ctx->thr);
	thr_free(ctx->que[0]);
	thr_free(ctx->frl[0]);
	thr_free(ctx->res);
}

int
ctx_init(struct context *ctx, struct pattern *pat)
{
	int err = 0;

	ctx->que[0] = ctx_get(ctx);
	if (!ctx->que) return ENOMEM;

	err = thr_init(ctx->que[0], pat->prog);
	if (err) goto fail;

	return 0;

fail:
	ctx_fini(ctx);
	return err;
}

int
do_char(struct context *ctx, char const ch)
{
	if (ctx->thr->ip->arg.b == ch) {
		++ctx->thr->ip;
		thr_mv(ctx->que, &ctx->thr);
	} else thr_mv(ctx->frl, &ctx->thr);

	return ctx_next(ctx, ch);
}

int
do_clss(struct context *ctx, char const ch)
{
	bool res;

	switch (ctx->thr->ip->arg.b) {
	case 0: res = true; break;
	case '.': res = ch != L'\n' && ch != L'\0'; break;
	}

	if (res) {
		++ctx->thr->ip;
		thr_mv(ctx->que, &ctx->thr);
	}
	else thr_mv(ctx->frl, &ctx->thr);

	return ctx_next(ctx, ch);
}

int
do_fork(struct context *ctx, char const ch)
{
	struct thread *new;
	int err = 0;

	new = ctx_get(ctx);
	if (!new) return ENOMEM;

	err = thr_fork(new, ctx->thr);
	if (err) goto fail;
	new->ip += ctx->thr->ip->arg.f;
	new->next = ctx->thr->next;
	ctx->thr->next = new;

	++ctx->thr->ip;
	return ctx->thr->ip->op(ctx, ch);

fail:
	thr_free(new);
	return err;
}

int
do_halt(struct context *ctx, char const ch)
{
	struct thread *th;
	struct patmatch term[] = {{ .off = -1, .ext = -1 }};
	int err;

	if (thr_cmp(ctx->res, ctx->thr) > 0) {
		thr_mv(ctx->frl, &ctx->thr);
		return ctx_next(ctx, ch);
	}

	err = vec_append(&ctx->thr->mat, term);
	if (err) return err;

	// abstract
	if (ctx->res) thr_mv(ctx->frl, &ctx->res);
	th = ctx->thr;
	ctx->thr = ctx->thr->next;
	ctx->res = th;
	ctx->res->next = 0;
	return ctx_next(ctx, ch);
}

int
do_jump(struct context *ctx, char const ch)
{
	ctx->thr->ip += ctx->thr->ip->arg.f;
	return ctx->thr->ip->op(ctx, ch);
}

int
do_mark(struct context *ctx, char const ch)
{
	struct patmatch mat = {0};
	int err = 0;

	mat.off = ctx->pos;
	mat.ext = -1;

	err = vec_append(&ctx->thr->mat, &mat);
	if (err) return err;

	++ctx->thr->ip;
	return ctx->thr->ip->op(ctx, ch);
}

int
do_save(struct context *ctx, char const ch)
{
	size_t off = vec_len(ctx->thr->mat);

	while (ctx->thr->mat[--off].ext != -1UL) continue;

	ctx->thr->mat[off].ext = ctx->pos - ctx->thr->mat[off].off;
	++ctx->thr->ip;
	return ctx->thr->ip->op(ctx, ch);
}

int
get_char(char *dst, void *x)
{
	union {
		void *p;
		struct pos *v;
	} str = { .p = x };
	struct pos *p = str.v;

	if (p->f > p->n) return 0;

	*dst = p->v[p->f];

	++p->f;

	return 1;
}

int
pat_match(struct context *ctx, struct pattern *pat)
{
	int err;
	
	err = pat_exec(ctx);
	if (err) return err;

	if (!ctx->res) return -1;
	return vec_copy(&pat->mat, ctx->res->mat);
}

int
pat_exec(struct context *ctx)
{
	char c;
	int err;

	while (ctx->cb(&c, ctx->cbx)) {

		ctx_shift(ctx);	

		err = ctx_next(ctx, c);
		if (err) break;

		++ctx->pos;
	}
	
	if (err) return err;

	return 0;
}
