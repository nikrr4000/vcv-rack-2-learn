#include "SoftNvg.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <map>
#include <vector>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

namespace {

struct Texture {
	int type = 0;  // NVG_TEXTURE_ALPHA or NVG_TEXTURE_RGBA
	int w = 0, h = 0;
	std::vector<unsigned char> data;
};

struct Canvas {
	int lw = 0, lh = 0;  // logical size
	float dpr = 2.f;
	int ss = 2;
	int w = 0, h = 0;    // sample buffer size
	std::vector<float> px;  // premultiplied RGBA
	std::vector<unsigned char> mask;
	std::map<int, Texture> textures;
	int nextTexture = 1;
	float k() const { return dpr * ss; }
};

Canvas* C(void* u) { return (Canvas*) u; }

void inverse(float* inv, const float* t) {
	double det = (double) t[0] * t[3] - (double) t[2] * t[1];
	if (det > -1e-6 && det < 1e-6) {
		inv[0] = 1; inv[1] = 0; inv[2] = 0; inv[3] = 1; inv[4] = 0; inv[5] = 0;
		return;
	}
	double invdet = 1.0 / det;
	inv[0] = (float) (t[3] * invdet);
	inv[2] = (float) (-t[2] * invdet);
	inv[4] = (float) (((double) t[2] * t[5] - (double) t[3] * t[4]) * invdet);
	inv[1] = (float) (-t[1] * invdet);
	inv[3] = (float) (t[0] * invdet);
	inv[5] = (float) (((double) t[1] * t[4] - (double) t[0] * t[5]) * invdet);
}

void apply(const float* t, float x, float y, float* ox, float* oy) {
	*ox = x * t[0] + y * t[2] + t[4];
	*oy = x * t[1] + y * t[3] + t[5];
}

struct Shader {
	NVGpaint paint;
	float pinv[6];
	bool scissor = false;
	float sinv[6];
	float sext[2];
	const Texture* tex = nullptr;
	bool textTriangles = false;
	Canvas* c = nullptr;

	Shader(Canvas* c, const NVGpaint* p, const NVGscissor* s, bool triangles) : paint(*p), c(c) {
		inverse(pinv, p->xform);
		if (s && s->extent[0] > -0.5f) {
			scissor = true;
			inverse(sinv, s->xform);
			sext[0] = s->extent[0];
			sext[1] = s->extent[1];
		}
		if (p->image) {
			auto it = c->textures.find(p->image);
			if (it != c->textures.end())
				tex = &it->second;
		}
		textTriangles = triangles;
	}

	bool clipped(float x, float y) const {
		if (!scissor)
			return false;
		float sx, sy;
		apply(sinv, x, y, &sx, &sy);
		return std::fabs(sx) > sext[0] || std::fabs(sy) > sext[1];
	}

	static float sdroundrect(float px, float py, float ex, float ey, float rad) {
		float x = std::fabs(px) - (ex - rad), y = std::fabs(py) - (ey - rad);
		float mx = std::max(x, 0.f), my = std::max(y, 0.f);
		return std::min(std::max(x, y), 0.f) + std::sqrt(mx * mx + my * my) - rad;
	}

	float texel(float u, float v, int ch) const {
		// bilinear, clamped
		float fx = u * tex->w - 0.5f, fy = v * tex->h - 0.5f;
		int x0 = (int) std::floor(fx), y0 = (int) std::floor(fy);
		float ax = fx - x0, ay = fy - y0;
		auto at = [&](int x, int y) {
			x = std::max(0, std::min(tex->w - 1, x));
			y = std::max(0, std::min(tex->h - 1, y));
			int n = tex->type == NVG_TEXTURE_ALPHA ? 1 : 4;
			return tex->data[(y * tex->w + x) * n + (n == 1 ? 0 : ch)] / 255.f;
		};
		return (at(x0, y0) * (1 - ax) + at(x0 + 1, y0) * ax) * (1 - ay) + (at(x0, y0 + 1) * (1 - ax) + at(x0 + 1, y0 + 1) * ax) * ay;
	}

	/** Premultiplied color at logical point (x, y); (u, v) for text triangles. */
	void color(float x, float y, float u, float v, float* out) const {
		NVGcolor col;
		if (textTriangles && tex) {
			float a = tex->type == NVG_TEXTURE_ALPHA ? texel(u, v, 0) : texel(u, v, 3);
			col = paint.innerColor;
			out[0] = col.r * col.a * a;
			out[1] = col.g * col.a * a;
			out[2] = col.b * col.a * a;
			out[3] = col.a * a;
			return;
		}
		float px, py;
		apply(pinv, x, y, &px, &py);
		if (tex) {
			float uu = px / paint.extent[0], vv = py / paint.extent[1];
			float r = texel(uu, vv, 0), g = texel(uu, vv, 1), b = texel(uu, vv, 2), a = tex->type == NVG_TEXTURE_ALPHA ? r : texel(uu, vv, 3);
			if (tex->type == NVG_TEXTURE_ALPHA)
				r = g = b = 1.f;
			float ia = paint.innerColor.a;
			out[0] = r * a * ia;
			out[1] = g * a * ia;
			out[2] = b * a * ia;
			out[3] = a * ia;
			return;
		}
		float f = std::max(paint.feather, 1e-4f);
		float d = (sdroundrect(px, py, paint.extent[0], paint.extent[1], paint.radius) + paint.feather * 0.5f) / f;
		d = std::max(0.f, std::min(1.f, d));
		const NVGcolor& a = paint.innerColor;
		const NVGcolor& b = paint.outerColor;
		float r = a.r + (b.r - a.r) * d, g = a.g + (b.g - a.g) * d, bl = a.b + (b.b - a.b) * d, al = a.a + (b.a - a.a) * d;
		out[0] = r * al;
		out[1] = g * al;
		out[2] = bl * al;
		out[3] = al;
	}
};

void blend(Canvas* c, int i, const float* s) {
	float* d = &c->px[i * 4];
	float ia = 1.f - s[3];
	d[0] = s[0] + d[0] * ia;
	d[1] = s[1] + d[1] * ia;
	d[2] = s[2] + d[2] * ia;
	d[3] = s[3] + d[3] * ia;
}

void fillPolygons(Canvas* c, const Shader& sh, const NVGpath* paths, int npaths) {
	float k = c->k();
	float miny = 1e9f, maxy = -1e9f;
	struct Edge { float x0, y0, x1, y1; int dir; };
	std::vector<Edge> edges;
	for (int p = 0; p < npaths; p++) {
		const NVGpath& path = paths[p];
		int n = path.nfill;
		for (int i = 0; i < n; i++) {
			const NVGvertex& a = path.fill[i];
			const NVGvertex& b = path.fill[(i + 1) % n];
			Edge e{a.x * k, a.y * k, b.x * k, b.y * k, 1};
			if (e.y0 == e.y1)
				continue;
			if (e.y0 > e.y1) {
				std::swap(e.x0, e.x1);
				std::swap(e.y0, e.y1);
				e.dir = -1;
			}
			miny = std::min(miny, e.y0);
			maxy = std::max(maxy, e.y1);
			edges.push_back(e);
		}
	}
	if (edges.empty())
		return;
	int j0 = std::max(0, (int) std::floor(miny)), j1 = std::min(c->h - 1, (int) std::ceil(maxy));
	std::vector<std::pair<float, int>> xs;
	float col[4];
	for (int j = j0; j <= j1; j++) {
		float y = j + 0.5f;
		xs.clear();
		for (const Edge& e : edges) {
			if (y < e.y0 || y >= e.y1)
				continue;
			float t = (y - e.y0) / (e.y1 - e.y0);
			xs.push_back({e.x0 + (e.x1 - e.x0) * t, e.dir});
		}
		if (xs.empty())
			continue;
		std::sort(xs.begin(), xs.end());
		int wind = 0;
		for (size_t q = 0; q + 1 < xs.size(); q++) {
			wind += xs[q].second;
			if (wind == 0)
				continue;
			int i0 = std::max(0, (int) std::ceil(xs[q].first - 0.5f));
			int i1 = std::min(c->w - 1, (int) std::ceil(xs[q + 1].first - 0.5f) - 1);
			for (int i = i0; i <= i1; i++) {
				float lx = (i + 0.5f) / k, ly = y / k;
				if (sh.clipped(lx, ly))
					continue;
				sh.color(lx, ly, 0, 0, col);
				blend(c, j * c->w + i, col);
			}
		}
	}
}

/** Rasterizes triangles; `strip` = triangle strip. With `unionMask`, overlapping samples blend once. */
void rasterTriangles(Canvas* c, const Shader& sh, const NVGvertex* v, int n, bool strip, bool unionMask) {
	float k = c->k();
	float col[4];
	if (unionMask)
		std::fill(c->mask.begin(), c->mask.end(), 0);
	int count = strip ? n - 2 : n / 3;
	for (int t = 0; t < count; t++) {
		const NVGvertex *a, *b, *d;
		if (strip) {
			a = &v[t];
			b = &v[t + 1];
			d = &v[t + 2];
		}
		else {
			a = &v[t * 3];
			b = &v[t * 3 + 1];
			d = &v[t * 3 + 2];
		}
		float ax = a->x * k, ay = a->y * k, bx = b->x * k, by = b->y * k, dx = d->x * k, dy = d->y * k;
		float area = (bx - ax) * (dy - ay) - (by - ay) * (dx - ax);
		if (std::fabs(area) < 1e-6f)
			continue;
		int i0 = std::max(0, (int) std::floor(std::min({ax, bx, dx})));
		int i1 = std::min(c->w - 1, (int) std::ceil(std::max({ax, bx, dx})));
		int j0 = std::max(0, (int) std::floor(std::min({ay, by, dy})));
		int j1 = std::min(c->h - 1, (int) std::ceil(std::max({ay, by, dy})));
		for (int j = j0; j <= j1; j++) {
			float y = j + 0.5f;
			for (int i = i0; i <= i1; i++) {
				float x = i + 0.5f;
				float w0 = ((bx - x) * (dy - y) - (by - y) * (dx - x)) / area;
				float w1 = ((dx - x) * (ay - y) - (dy - y) * (ax - x)) / area;
				float w2 = 1.f - w0 - w1;
				if (w0 < 0 || w1 < 0 || w2 < 0)
					continue;
				int idx = j * c->w + i;
				if (unionMask) {
					if (c->mask[idx])
						continue;
					c->mask[idx] = 1;
				}
				float lx = x / k, ly = y / k;
				if (sh.clipped(lx, ly))
					continue;
				float u = w0 * a->u + w1 * b->u + w2 * d->u;
				float vv = w0 * a->v + w1 * b->v + w2 * d->v;
				sh.color(lx, ly, u, vv, col);
				blend(c, idx, col);
			}
		}
	}
}

int renderCreate(void*, void*) { return 1; }

int renderCreateTexture(void* u, int type, int w, int h, int, const unsigned char* data) {
	Canvas* c = C(u);
	Texture t;
	t.type = type;
	t.w = w;
	t.h = h;
	int n = type == NVG_TEXTURE_ALPHA ? 1 : 4;
	t.data.assign((size_t) w * h * n, 0);
	if (data)
		std::memcpy(t.data.data(), data, t.data.size());
	int id = c->nextTexture++;
	c->textures[id] = t;
	return id;
}

int renderDeleteTexture(void* u, int image) {
	C(u)->textures.erase(image);
	return 1;
}

int renderUpdateTexture(void* u, int image, int x, int y, int w, int h, const unsigned char* data) {
	Canvas* c = C(u);
	auto it = c->textures.find(image);
	if (it == c->textures.end())
		return 0;
	Texture& t = it->second;
	int n = t.type == NVG_TEXTURE_ALPHA ? 1 : 4;
	// nanovg passes the whole texture's data pointer; copy rows y..y+h (full width like the GL back end).
	(void) x;
	(void) w;
	for (int row = y; row < y + h; row++)
		std::memcpy(&t.data[(size_t) row * t.w * n], &data[(size_t) row * t.w * n], (size_t) t.w * n);
	return 1;
}

int renderGetTextureSize(void* u, int image, int* w, int* h) {
	auto it = C(u)->textures.find(image);
	if (it == C(u)->textures.end())
		return 0;
	*w = it->second.w;
	*h = it->second.h;
	return 1;
}

void renderViewport(void*, float, float, float) {}
void renderCancel(void*) {}
void renderFlush(void*) {}

void renderFill(void* u, NVGpaint* paint, NVGcompositeOperationState, NVGscissor* scissor, float, const float*, const NVGpath* paths, int npaths) {
	Shader sh(C(u), paint, scissor, false);
	fillPolygons(C(u), sh, paths, npaths);
}

void renderStroke(void* u, NVGpaint* paint, NVGcompositeOperationState, NVGscissor* scissor, float, float, const NVGpath* paths, int npaths) {
	Canvas* c = C(u);
	Shader sh(c, paint, scissor, false);
	std::fill(c->mask.begin(), c->mask.end(), 0);
	for (int i = 0; i < npaths; i++)
		rasterTriangles(c, sh, paths[i].stroke, paths[i].nstroke, true, false);
	(void) c;
}

void renderTriangles(void* u, NVGpaint* paint, NVGcompositeOperationState, NVGscissor* scissor, const NVGvertex* verts, int nverts, float) {
	Shader sh(C(u), paint, scissor, true);
	rasterTriangles(C(u), sh, verts, nverts, false, false);
}

void renderDelete(void* u) { delete C(u); }

} // namespace

NVGcontext* softCreate(int width, int height, float dpr, int ss) {
	Canvas* c = new Canvas;
	c->lw = width;
	c->lh = height;
	c->dpr = dpr;
	c->ss = ss;
	c->w = (int) std::ceil(width * dpr * ss);
	c->h = (int) std::ceil(height * dpr * ss);
	c->px.assign((size_t) c->w * c->h * 4, 0.f);
	c->mask.assign((size_t) c->w * c->h, 0);
	NVGparams p;
	std::memset(&p, 0, sizeof(p));
	p.userPtr = c;
	p.edgeAntiAlias = 0;
	p.renderCreate = renderCreate;
	p.renderCreateTexture = renderCreateTexture;
	p.renderDeleteTexture = renderDeleteTexture;
	p.renderUpdateTexture = renderUpdateTexture;
	p.renderGetTextureSize = renderGetTextureSize;
	p.renderViewport = renderViewport;
	p.renderCancel = renderCancel;
	p.renderFlush = renderFlush;
	p.renderFill = renderFill;
	p.renderStroke = renderStroke;
	p.renderTriangles = renderTriangles;
	p.renderDelete = renderDelete;
	return nvgCreateInternal(&p, nullptr);
}

void softDelete(NVGcontext* vg) { nvgDeleteInternal(vg); }

void softClear(NVGcontext* vg, NVGcolor col) {
	Canvas* c = C(nvgInternalParams(vg)->userPtr);
	for (size_t i = 0; i < c->px.size(); i += 4) {
		c->px[i] = col.r * col.a;
		c->px[i + 1] = col.g * col.a;
		c->px[i + 2] = col.b * col.a;
		c->px[i + 3] = col.a;
	}
}

void softBegin(NVGcontext* vg) {
	Canvas* c = C(nvgInternalParams(vg)->userPtr);
	nvgBeginFrame(vg, (float) c->lw, (float) c->lh, c->dpr);
}

void softEnd(NVGcontext* vg) { nvgEndFrame(vg); }

bool softWritePng(NVGcontext* vg, const std::string& path) {
	Canvas* c = C(nvgInternalParams(vg)->userPtr);
	int ow = c->w / c->ss, oh = c->h / c->ss;
	std::vector<unsigned char> out((size_t) ow * oh * 4);
	for (int y = 0; y < oh; y++)
		for (int x = 0; x < ow; x++) {
			float acc[4] = {0, 0, 0, 0};
			for (int sy = 0; sy < c->ss; sy++)
				for (int sx = 0; sx < c->ss; sx++) {
					const float* p = &c->px[(((size_t) y * c->ss + sy) * c->w + (size_t) x * c->ss + sx) * 4];
					for (int k = 0; k < 4; k++)
						acc[k] += p[k];
				}
			float n = (float) (c->ss * c->ss);
			float a = acc[3] / n;
			for (int k = 0; k < 3; k++) {
				float v = a > 1e-6f ? (acc[k] / n) / a : 0.f;
				out[((size_t) y * ow + x) * 4 + k] = (unsigned char) std::lround(std::max(0.f, std::min(1.f, v)) * 255.f);
			}
			out[((size_t) y * ow + x) * 4 + 3] = (unsigned char) std::lround(std::max(0.f, std::min(1.f, a)) * 255.f);
		}
	return stbi_write_png(path.c_str(), ow, oh, 4, out.data(), ow * 4) != 0;
}
