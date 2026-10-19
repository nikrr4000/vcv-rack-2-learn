// Helpers for views that stack blocks vertically: flowing markdown at a width, a cache for it,
// and a wheel scroller with a thin scrollbar (nothing is ever clipped silently).
#pragma once
#include <map>
#include <string>

#include "ui/Draw.hpp"

namespace atlas {

struct FlowOpts {
	float size = 0.f;        // body size override (0 = theme body)
	bool italic = false;     // regular text drawn italic
	bool bold = false;       // regular text drawn bold
	bool title = false;      // headings at title size
	float lineHeight = 0.f;  // override
};

/** Lays out markdown in one column (no pagination). */
text::Page flowText(NVGcontext* vg, const Style& st, const std::string& md, float width, const FlowOpts& o = FlowOpts());

/** Draws a flowed page with optional color override for ink. */
void drawFlow(NVGcontext* vg, const Style& st, const text::Page& p, float x, float y, const NVGcolor* ink = nullptr);

/** Per-view cache of flowed texts keyed by text, width and font scale. */
struct FlowCache {
	std::map<std::string, text::Page> pages;
	const text::Page& get(NVGcontext* vg, const Style& st, const std::string& md, float width, const FlowOpts& o = FlowOpts());
};

struct Scroll {
	float offset = 0.f;
	float content = 0.f;
	float view = 0.f;
	bool stickBottom = false;  // conversation: stay at the bottom while new text arrives
	bool active() const { return content > view + 0.5f; }
	void clamp();
	bool wheel(float dy);
	bool atBottom() const { return offset >= content - view - 2.f; }
};

void drawScrollbar(NVGcontext* vg, const Style& st, const Rect& area, const Scroll& s);

} // namespace atlas
