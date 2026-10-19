#include "Flow.hpp"

#include <algorithm>

#include "PagedText.hpp"

namespace atlas {

text::Page flowText(NVGcontext* vg, const Style& st, const std::string& md, float width, const FlowOpts& o) {
	text::NvgMeasurer m(vg, st.fonts);
	text::TextStyle ts = st.textStyle(o.title);
	if (o.size > 0.f)
		ts.bodySize = o.size;
	if (o.lineHeight > 0.f)
		ts.lineHeight = o.lineHeight;
	std::vector<text::Block> blocks = text::parse(md);
	if (o.italic || o.bold)
		for (text::Block& b : blocks)
			for (text::Span& s : b.spans)
				if (s.style == text::STYLE_REGULAR)
					s.style = o.bold ? text::STYLE_BOLD : text::STYLE_ITALIC;
	return text::flatten(text::layout(blocks, width, m, ts), ts);
}

void drawFlow(NVGcontext* vg, const Style& st, const text::Page& p, float x, float y, const NVGcolor* ink) {
	text::TextColors c = st.textColors();
	if (ink)
		c.ink = *ink;
	text::drawPage(vg, st.fonts, p, x, y, c);
}

const text::Page& FlowCache::get(NVGcontext* vg, const Style& st, const std::string& md, float width, const FlowOpts& o) {
	char k[96];
	std::snprintf(k, sizeof(k), "|%.1f|%.2f|%.1f|%d%d%d|%.2f|", width, fontScale(vg), o.size, o.italic, o.bold, o.title, o.lineHeight);
	std::string key = md + k;
	auto it = pages.find(key);
	if (it != pages.end())
		return it->second;
	if (pages.size() > 400)
		pages.clear();
	return pages[key] = flowText(vg, st, md, width, o);
}

void Scroll::clamp() {
	float maxOff = std::max(0.f, content - view);
	if (stickBottom)
		offset = maxOff;
	offset = std::max(0.f, std::min(maxOff, offset));
}

bool Scroll::wheel(float dy) {
	if (!active())
		return false;
	offset -= dy;
	stickBottom = false;
	clamp();
	if (atBottom())
		stickBottom = true;
	return true;
}

void drawScrollbar(NVGcontext* vg, const Style& st, const Rect& a, const Scroll& s) {
	if (!s.active())
		return;
	float h = std::max(16.f, a.h * s.view / s.content);
	float y = a.y + (a.h - h) * (s.offset / std::max(1.f, s.content - s.view));
	nvgBeginPath(vg);
	nvgRect(vg, a.right() - 2.f, y, 2.f, h);
	nvgFillColor(vg, st.pal.ink2);
	nvgFill(vg);
}

} // namespace atlas
