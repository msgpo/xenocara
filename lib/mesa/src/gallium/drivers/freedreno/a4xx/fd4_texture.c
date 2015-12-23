/* -*- mode: C; c-file-style: "k&r"; tab-width 4; indent-tabs-mode: t; -*- */

/*
 * Copyright (C) 2014 Rob Clark <robclark@freedesktop.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Authors:
 *    Rob Clark <robclark@freedesktop.org>
 */

#include "pipe/p_state.h"
#include "util/u_string.h"
#include "util/u_memory.h"
#include "util/u_inlines.h"
#include "util/u_format.h"

#include "fd4_texture.h"
#include "fd4_format.h"

/* TODO do we need to emulate clamp-to-edge like a3xx? */
static enum a4xx_tex_clamp
tex_clamp(unsigned wrap)
{
	/* hardware probably supports more, but we can't coax all the
	 * wrap/clamp modes out of the GLESv2 blob driver.
	 *
	 * TODO once we have basics working, go back and just try
	 * different values and see what happens
	 */
	switch (wrap) {
	case PIPE_TEX_WRAP_REPEAT:
		return A4XX_TEX_REPEAT;
	case PIPE_TEX_WRAP_CLAMP:
	case PIPE_TEX_WRAP_CLAMP_TO_EDGE:
		return A4XX_TEX_CLAMP_TO_EDGE;
	case PIPE_TEX_WRAP_CLAMP_TO_BORDER:
// TODO
//		return A4XX_TEX_CLAMP_TO_BORDER;
	case PIPE_TEX_WRAP_MIRROR_CLAMP:
	case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_BORDER:
	case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_EDGE:
// TODO
//		return A4XX_TEX_MIRROR_CLAMP;
	case PIPE_TEX_WRAP_MIRROR_REPEAT:
		return A4XX_TEX_MIRROR_REPEAT;
	default:
		DBG("invalid wrap: %u", wrap);
		return 0;
	}
}

static enum a4xx_tex_filter
tex_filter(unsigned filter, bool aniso)
{
	switch (filter) {
	case PIPE_TEX_FILTER_NEAREST:
		return A4XX_TEX_NEAREST;
	case PIPE_TEX_FILTER_LINEAR:
		return aniso ? A4XX_TEX_ANISO : A4XX_TEX_LINEAR;
	default:
		DBG("invalid filter: %u", filter);
		return 0;
	}
}

static void *
fd4_sampler_state_create(struct pipe_context *pctx,
		const struct pipe_sampler_state *cso)
{
	struct fd4_sampler_stateobj *so = CALLOC_STRUCT(fd4_sampler_stateobj);
	unsigned aniso = util_last_bit(MIN2(cso->max_anisotropy >> 1, 8));
	bool miplinear = false;

	if (!so)
		return NULL;

	if (cso->min_mip_filter == PIPE_TEX_MIPFILTER_LINEAR)
		miplinear = true;

	so->base = *cso;

	so->texsamp0 =
		COND(miplinear, A4XX_TEX_SAMP_0_MIPFILTER_LINEAR_NEAR) |
		A4XX_TEX_SAMP_0_XY_MAG(tex_filter(cso->mag_img_filter, aniso)) |
		A4XX_TEX_SAMP_0_XY_MIN(tex_filter(cso->min_img_filter, aniso)) |
		A4XX_TEX_SAMP_0_ANISO(aniso) |
		A4XX_TEX_SAMP_0_WRAP_S(tex_clamp(cso->wrap_s)) |
		A4XX_TEX_SAMP_0_WRAP_T(tex_clamp(cso->wrap_t)) |
		A4XX_TEX_SAMP_0_WRAP_R(tex_clamp(cso->wrap_r));

	so->texsamp1 =
//		COND(miplinear, A4XX_TEX_SAMP_1_MIPFILTER_LINEAR_FAR) |
		COND(!cso->normalized_coords, A4XX_TEX_SAMP_1_UNNORM_COORDS);

	if (cso->min_mip_filter != PIPE_TEX_MIPFILTER_NONE) {
		so->texsamp0 |= A4XX_TEX_SAMP_0_LOD_BIAS(cso->lod_bias);
		so->texsamp1 |=
			A4XX_TEX_SAMP_1_MIN_LOD(cso->min_lod) |
			A4XX_TEX_SAMP_1_MAX_LOD(cso->max_lod);
	}

	if (cso->compare_mode)
		so->texsamp1 |= A4XX_TEX_SAMP_1_COMPARE_FUNC(cso->compare_func); /* maps 1:1 */

	return so;
}

static enum a4xx_tex_type
tex_type(unsigned target)
{
	switch (target) {
	default:
		assert(0);
	case PIPE_BUFFER:
	case PIPE_TEXTURE_1D:
	case PIPE_TEXTURE_1D_ARRAY:
		return A4XX_TEX_1D;
	case PIPE_TEXTURE_RECT:
	case PIPE_TEXTURE_2D:
	case PIPE_TEXTURE_2D_ARRAY:
		return A4XX_TEX_2D;
	case PIPE_TEXTURE_3D:
		return A4XX_TEX_3D;
	case PIPE_TEXTURE_CUBE:
	case PIPE_TEXTURE_CUBE_ARRAY:
		return A4XX_TEX_CUBE;
	}
}

static struct pipe_sampler_view *
fd4_sampler_view_create(struct pipe_context *pctx, struct pipe_resource *prsc,
		const struct pipe_sampler_view *cso)
{
	struct fd4_pipe_sampler_view *so = CALLOC_STRUCT(fd4_pipe_sampler_view);
	struct fd_resource *rsc = fd_resource(prsc);
	unsigned lvl = fd_sampler_first_level(cso);
	unsigned miplevels = fd_sampler_last_level(cso) - lvl;

	if (!so)
		return NULL;

	so->base = *cso;
	pipe_reference(NULL, &prsc->reference);
	so->base.texture = prsc;
	so->base.reference.count = 1;
	so->base.context = pctx;

	so->texconst0 =
		A4XX_TEX_CONST_0_TYPE(tex_type(prsc->target)) |
		A4XX_TEX_CONST_0_FMT(fd4_pipe2tex(cso->format)) |
		A4XX_TEX_CONST_0_MIPLVLS(miplevels) |
		fd4_tex_swiz(cso->format, cso->swizzle_r, cso->swizzle_g,
				cso->swizzle_b, cso->swizzle_a);

	if (util_format_is_srgb(cso->format))
		so->texconst0 |= A4XX_TEX_CONST_0_SRGB;

	so->texconst1 =
		A4XX_TEX_CONST_1_WIDTH(u_minify(prsc->width0, lvl)) |
		A4XX_TEX_CONST_1_HEIGHT(u_minify(prsc->height0, lvl));
	so->texconst2 =
		A4XX_TEX_CONST_2_FETCHSIZE(fd4_pipe2fetchsize(cso->format)) |
		A4XX_TEX_CONST_2_PITCH(rsc->slices[lvl].pitch * rsc->cpp);

	switch (prsc->target) {
	case PIPE_TEXTURE_1D_ARRAY:
	case PIPE_TEXTURE_2D_ARRAY:
		so->texconst3 =
			A4XX_TEX_CONST_3_DEPTH(prsc->array_size) |
			A4XX_TEX_CONST_3_LAYERSZ(rsc->layer_size);
		break;
	case PIPE_TEXTURE_CUBE:
	case PIPE_TEXTURE_CUBE_ARRAY:
		so->texconst3 =
			A4XX_TEX_CONST_3_DEPTH(prsc->array_size / 6) |
			A4XX_TEX_CONST_3_LAYERSZ(rsc->layer_size);
		break;
	case PIPE_TEXTURE_3D:
		so->texconst3 =
			A4XX_TEX_CONST_3_DEPTH(u_minify(prsc->depth0, lvl)) |
			A4XX_TEX_CONST_3_LAYERSZ(rsc->slices[0].size0);
		break;
	default:
		so->texconst3 = 0x00000000;
		break;
	}

	return &so->base;
}

void
fd4_texture_init(struct pipe_context *pctx)
{
	pctx->create_sampler_state = fd4_sampler_state_create;
	pctx->bind_sampler_states = fd_sampler_states_bind;
	pctx->create_sampler_view = fd4_sampler_view_create;
	pctx->set_sampler_views = fd_set_sampler_views;
}
