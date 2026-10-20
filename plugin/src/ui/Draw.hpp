// Rack-free drawing primitives of the «Атлас» look: paper panels with an accent rule,
// flat text buttons, page dots, simple vector glyphs (no icon fonts), hit regions.
#pragma once
#include <nanovg.h>

#include <string>
#include <vector>

#include "Style.hpp"

namespace atlas {

struct Rect {
	float x = 0, y = 0, w = 0, h = 0;
	Rect() {}
	Rect(float x, float y, float w, float h) : x(x), y(y), w(w), h(h) {}
	bool contains(float px, float py) const { return px >= x && px <= x + w && py >= y && py <= y + h; }
	float right() const { return x + w; }
	float bottom() const { return y + h; }
	Rect inset(float d) const { return Rect(x + d, y + d, w - 2 * d, h - 2 * d); }
};

/** A clickable region produced while drawing; the Rack widget maps clicks to actions. */
struct Hit {
	Rect r;
	int action = 0;
	int arg = 0;
};

/** Pointer state passed into views: hover highlights and drag targets. */
struct Pointer {
	float x = -1e6f, y = -1e6f;
	bool down = false;
	bool over(const Rect& r) const { return r.contains(x, y); }
};

struct Hits {
	std::vector<Hit> list;
	void clear() { list.clear(); }
	void add(const Rect& r, int action, int arg = 0) {
		// Grow small targets to the minimum hit size around their center.
		Rect g = r;
		float m = tokens::controls_minHit;
		if (g.w < m) {
			g.x -= (m - g.w) / 2;
			g.w = m;
		}
		if (g.h < m) {
			g.y -= (m - g.h) / 2;
			g.h = m;
		}
		Hit h;
		h.r = g;
		h.action = action;
		h.arg = arg;
		list.push_back(h);
	}
	const Hit* at(float x, float y) const {
		for (auto it = list.rbegin(); it != list.rend(); ++it)
			if (it->r.contains(x, y))
				return &*it;
		return nullptr;
	}
};

enum Glyph { G_ARROW_LEFT, G_ARROW_RIGHT, G_CHECK, G_CROSS, G_QUESTION, G_DOT, G_CHEVRON_DOWN, G_CHEVRON_UP, G_CHEVRON_LEFT, G_CHEVRON_RIGHT, G_ARROW_DOWN, G_APPROX, G_PLAY, G_RING };

void drawGlyph(NVGcontext* vg, Glyph g, float cx, float cy, float size, NVGcolor color, float stroke = 1.4f);

/** Paper, accent rule, header line («3.5 · Блок 2» left, `right` aligned right). Returns the content rect. */
Rect drawPanel(NVGcontext* vg, const Style& st, float w, float h, const std::string& header, const std::string& right = "");

/** A hairline across. */
void drawHairline(NVGcontext* vg, const Style& st, float x0, float y, float x1);

enum ButtonKind { BUTTON_NORMAL, BUTTON_PRIMARY, BUTTON_QUIET };

struct ButtonState {
	bool hover = false;
	bool active = false;
	bool disabled = false;
	ButtonKind kind = BUTTON_NORMAL;
};

/** Flat text button with a hairline border. The label may start or end with a glyph via `left`/`right`. */
void drawButton(NVGcontext* vg, const Style& st, const Rect& r, const std::string& label, const ButtonState& s, int leftGlyph = -1, int rightGlyph = -1);

/** Width a button needs for `label` (+ glyph room). */
float buttonWidth(NVGcontext* vg, const Style& st, const std::string& label, bool glyph = false);

/** Page dots centered at cx; returns their rects for clicking. */
std::vector<Rect> drawDots(NVGcontext* vg, const Style& st, float cx, float cy, int n, int current);

/** A round status light. `state`: 0 off, 1 ok (accent green), 2 fail, 3 busy. */
void drawLight(NVGcontext* vg, const Style& st, float cx, float cy, float r, int state);

/** Single-line text truncated with «…» to fit `maxW`. */
std::string ellipsize(NVGcontext* vg, int font, float size, const std::string& s, float maxW);
/** How many strings ellipsize() has shortened so far (visual QA counts truncated figure labels). */
int& ellipsizedCount();
/** How many flow-figure edges were drawn straight through another box (visual QA). */
int& hiddenEdgeCount();

/** The accent mixed into the panel color (opaque), e.g. 0.15 for active fills. */
NVGcolor tint(const Style& st, float amount);

/** Semantic colors that read on both themes. */
NVGcolor okColor(const Style& st);
NVGcolor failColor(const Style& st);
NVGcolor busyColor(const Style& st);

} // namespace atlas
