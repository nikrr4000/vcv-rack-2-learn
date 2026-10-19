#include "SmallViews.hpp"

#include "figures/Figures.hpp"

namespace atlas {

void drawBadge(NVGcontext* vg, const Style& st, float cx, float cy, int n) {
	nvgBeginPath(vg);
	nvgCircle(vg, cx, cy, 7.f);
	nvgFillColor(vg, st.accent);
	nvgFill(vg);
	text::drawText(vg, st.fonts.handle[text::F_BOLD], 9.5f, st.pal.panel, cx, cy + 3.3f, std::to_string(n), NVG_ALIGN_CENTER | NVG_ALIGN_BASELINE);
}

void NoteView::draw(NVGcontext* vg, const Style& st, float w, float h, const Pointer& p, Hits& hits) {
	// Narrow paper: tighter padding than a page.
	nvgBeginPath(vg);
	nvgRect(vg, 0, 0, w, h);
	nvgFillColor(vg, st.pal.panel);
	nvgFill(vg);
	nvgBeginPath(vg);
	nvgRect(vg, 0.5f, 0.5f, w - 1.f, h - 1.f);
	nvgStrokeColor(vg, st.pal.hairline);
	nvgStrokeWidth(vg, 1.f);
	nvgStroke(vg);
	nvgBeginPath(vg);
	nvgRect(vg, 0, 0, w, tokens::panel_accentRule);
	nvgFillColor(vg, st.accent);
	nvgFill(vg);
	float pad = w < 100.f ? 8.f : 10.f;
	float y = tokens::panel_accentRule + 12.f;
	if (number > 0) {
		drawBadge(vg, st, pad + 7.f, y + 7.f, number);
		if (missingTarget)
			text::drawText(vg, st.fonts.handle[text::F_MONO], 9.f, st.pal.ink2, pad + 19.f, y + 10.f, "нет цели");
		y += 22.f;
	}
	content = Rect(pad, y, w - 2 * pad, h - y - 10.f);
	text.layout(vg, st, content.w, content.h);
	if (text.count() > 1)
		text.layout(vg, st, content.w, content.h - tokens::panel_footerHeight);
	text.draw(vg, st, content.x, content.y);
	if (text.count() > 1)
		drawPager(vg, st, Rect(content.x, content.bottom() - tokens::panel_footerHeight + 4.f, content.w, tokens::panel_footerHeight - 4.f), text.count(), text.page, p, hits);
	// Vertical header along the bottom-left edge would crowd narrow notes: header goes small at the bottom.
	if (!header.empty())
		text::drawText(vg, st.fonts.handle[text::F_MONO], 8.5f, st.pal.ink2, pad, h - 6.f, ellipsize(vg, st.fonts.handle[text::F_MONO], 8.5f, header, w - 2 * pad));
}

bool NoteView::action(int a, int arg) {
	if (a == PAGER_PREV && text.page > 0)
		text.page--;
	else if (a == PAGER_NEXT && text.page < text.count() - 1)
		text.page++;
	else if (a == PAGER_GOTO)
		text.page = arg;
	else
		return false;
	return true;
}

std::string NoteView::termAt(float x, float y) const {
	return text.termAt(x - content.x, y - content.y);
}

void FigureView::draw(NVGcontext* vg, const Style& st, float w, float h) {
	Rect c = drawPanel(vg, st, w, h, header);
	float capH = 0.f;
	text::Page capPage;
	if (!caption.empty()) {
		text::NvgMeasurer m(vg, st.fonts);
		text::TextStyle ts = st.textStyle();
		ts.bodySize = tokens::type_small;
		std::vector<text::Block> blocks = text::parse(caption);
		for (text::Block& b : blocks)
			for (text::Span& s : b.spans)
				if (s.style == text::STYLE_REGULAR)
					s.style = text::STYLE_ITALIC;
		capPage = text::flatten(text::layout(blocks, c.w, m, ts), ts);
		capH = capPage.height + 8.f;
	}
	float avail = c.h - capH;
	float fh = figure ? std::min(avail, std::max(80.f, c.w * figures::aspect(figure))) : 0.f;
	if (figure)
		figures::draw(vg, st, Rect(c.x, c.y + 4.f, c.w, fh - 4.f), figure);
	if (!caption.empty()) {
		text::TextColors tc = st.textColors();
		tc.ink = st.pal.ink2;
		text::drawPage(vg, st.fonts, capPage, c.x, c.y + fh + 8.f, tc);
	}
}

void BenchView::draw(NVGcontext* vg, const Style& st, float w, float h) {
	nvgBeginPath(vg);
	nvgRect(vg, 0, 0, w, h);
	nvgFillColor(vg, st.pal.panel);
	nvgFill(vg);
	// The marker: an accent bar on the bench side, a hairline on the other.
	float bar = 4.f;
	nvgBeginPath(vg);
	nvgRect(vg, left ? w - bar : 0, 0, bar, h);
	nvgFillColor(vg, st.accent);
	nvgFill(vg);
	nvgBeginPath(vg);
	nvgRect(vg, left ? 0 : w - 1.f, 0, 1.f, h);
	nvgFillColor(vg, st.pal.hairline);
	nvgFill(vg);
	// Vertical label, reading bottom to top.
	nvgSave(vg);
	nvgTranslate(vg, w / 2 - (left ? 2.f : -2.f) + 4.f, h / 2);
	nvgRotate(vg, -NVG_PI / 2);
	std::string label = left ? "ВЕРСТАК  →" : "←  ВЕРСТАК";
	text::drawText(vg, st.fonts.handle[text::F_BOLD], 10.f, st.accent, 0, 0, label, NVG_ALIGN_CENTER | NVG_ALIGN_BASELINE);
	nvgRestore(vg);
	if (!lessonNum.empty())
		text::drawText(vg, st.fonts.handle[text::F_MONO], 8.f, st.pal.ink2, w / 2 - (left ? 2.f : -2.f), h - 8.f, lessonNum, NVG_ALIGN_CENTER | NVG_ALIGN_BASELINE);
}

} // namespace atlas
