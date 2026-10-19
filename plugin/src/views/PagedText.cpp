#include "PagedText.hpp"

#include <cmath>

#include "core/Report.hpp"

namespace atlas {

float fontScale(NVGcontext* vg) {
	float xf[6];
	nvgCurrentTransform(vg, xf);
	float s = std::sqrt(xf[0] * xf[0] + xf[1] * xf[1]);
	return std::round(s * 100.f) / 100.f;
}

void PagedText::layout(NVGcontext* vg, const Style& st, float w, float h) {
	float sc = fontScale(vg);
	if (md == key && w == lw && h == lh && sc == lscale && title == ltitle && !pages.empty())
		return;
	text::NvgMeasurer m(vg, st.fonts);
	text::TextStyle ts = st.textStyle(title);
	text::Document d = text::layout(text::parse(md), w, m, ts);
	pages = text::paginate(d, h, ts);
	splitWords = d.splitWords;
	key = md;
	lw = w;
	lh = h;
	lscale = sc;
	ltitle = title;
	if (page >= (int) pages.size())
		page = (int) pages.size() - 1;
	if (page < 0)
		page = 0;
	if (!reportId.empty()) {
		TextReport r;
		r.pages = (int) pages.size();
		r.splitWords = splitWords;
		r.width = w;
		for (const text::Page& p : pages)
			r.overflow = r.overflow || p.overflow;
		reportText(reportId, r);
	}
}

float PagedText::draw(NVGcontext* vg, const Style& st, float x, float y) {
	if (pages.empty())
		return 0.f;
	const text::Page& p = pages[page];
	text::drawPage(vg, st.fonts, p, x, y, st.textColors());
	return p.height;
}

float PagedText::measureHeight(NVGcontext* vg, const Style& st, const std::string& md, float w, bool title) {
	text::NvgMeasurer m(vg, st.fonts);
	text::TextStyle ts = st.textStyle(title);
	return text::flatten(text::layout(text::parse(md), w, m, ts), ts).height;
}

std::string PagedText::termAt(float px, float py) const {
	if (pages.empty())
		return "";
	std::string t;
	float r[4];
	if (text::termAt(pages[page], px, py, &t, r))
		return t;
	return "";
}

void drawPager(NVGcontext* vg, const Style& st, const Rect& r, int n, int current, const Pointer& p, Hits& hits) {
	if (n <= 1)
		return;
	float cy = r.y + r.h / 2.f;
	Rect prev(r.x, cy - 9.f, 22.f, 18.f), next(r.right() - 22.f, cy - 9.f, 22.f, 18.f);
	bool canPrev = current > 0, canNext = current < n - 1;
	drawGlyph(vg, G_CHEVRON_LEFT, prev.x + 11.f, cy, 10.f, canPrev ? (p.over(prev) ? st.accent : st.pal.ink) : st.pal.hairline, 1.5f);
	drawGlyph(vg, G_CHEVRON_RIGHT, next.x + 11.f, cy, 10.f, canNext ? (p.over(next) ? st.accent : st.pal.ink) : st.pal.hairline, 1.5f);
	if (canPrev)
		hits.add(prev, PAGER_PREV);
	if (canNext)
		hits.add(next, PAGER_NEXT);
	std::vector<Rect> dots = drawDots(vg, st, r.x + r.w / 2.f, cy, n, current);
	for (size_t i = 0; i < dots.size(); i++)
		hits.add(dots[i], PAGER_GOTO, (int) i);
}

} // namespace atlas
