#include "PageView.hpp"

namespace atlas {

void PageView::draw(NVGcontext* vg, const Style& st, float w, float h, const Pointer& p, Hits& hits) {
	Rect c = drawPanel(vg, st, w, h, header, text.count() > 1 ? std::to_string(text.page + 1) + "/" + std::to_string(text.count()) : "");
	text.layout(vg, st, c.w, c.h);
	if (text.count() > 1)
		text.layout(vg, st, c.w, c.h - tokens::panel_footerHeight);
	content = c;
	text.draw(vg, st, c.x, c.y);
	if (text.count() > 1)
		drawPager(vg, st, Rect(c.x, c.bottom() - tokens::panel_footerHeight + 4.f, c.w, tokens::panel_footerHeight - 4.f), text.count(), text.page, p, hits);
}

bool PageView::action(int a, int arg) {
	if (a == PAGER_PREV && text.page > 0) {
		text.page--;
		return true;
	}
	if (a == PAGER_NEXT && text.page < text.count() - 1) {
		text.page++;
		return true;
	}
	if (a == PAGER_GOTO) {
		text.page = arg;
		return true;
	}
	return false;
}

std::string PageView::termAt(float x, float y) const {
	return text.termAt(x - content.x, y - content.y);
}

} // namespace atlas
