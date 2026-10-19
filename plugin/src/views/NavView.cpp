#include "NavView.hpp"

#include <algorithm>

namespace atlas {

void drawStatus(NVGcontext* vg, const Style& st, float cx, float cy, const std::string& status, bool current) {
	if (status == "learned")
		drawGlyph(vg, G_CHECK, cx, cy, 8.f, okColor(st), 1.6f);
	else if (current || status == "learning") {
		nvgBeginPath(vg);
		nvgCircle(vg, cx, cy, 3.f);
		nvgFillColor(vg, st.accent);
		nvgFill(vg);
	}
	else {
		nvgBeginPath(vg);
		nvgCircle(vg, cx, cy, 2.6f);
		nvgStrokeColor(vg, st.pal.ink2);
		nvgStrokeWidth(vg, 1.f);
		nvgStroke(vg);
	}
}

static int partOf(const Pack& pk, const std::string& lesson) {
	for (size_t i = 0; i < pk.parts.size(); i++)
		for (const std::string& l : pk.parts[i].lessons)
			if (l == lesson)
				return (int) i;
	return 0;
}

static void connection(NVGcontext* vg, const Style& st, const Rect& c, bool connected) {
	float y = c.bottom() - 4.f;
	drawLight(vg, st, c.x + 4.f, y - 3.5f, 3.5f, connected ? 1 : 0);
	text::drawText(vg, st.fonts.handle[text::F_MONO], 9.5f, st.pal.ink2, c.x + 12.f, y, connected ? "Atlas на связи" : "Atlas не запущен");
}

void NavigatorView::draw(NVGcontext* vg, const Style& st, float w, float h, const NavState& s, const Pointer& p, Hits& hits) {
	if (!s.pack)
		return;
	const Pack& pk = *s.pack;
	const Lesson* L = pk.lesson(s.lesson);
	NavMode m = mode;
	if (m == NAV_MODE_LESSON && !L)
		m = NAV_MODE_HOME;
	std::string header = m == NAV_MODE_MAP ? "Карта курса" : L ? L->num + " · Урок" : "Атлас";
	Rect c = drawPanel(vg, st, w, h, header);
	float y = c.y;
	auto button = [&](const std::string& label, int action, int arg, ButtonKind kind, bool disabled, int lg = -1, int rg = -1, float hgt = 22.f) {
		Rect r(c.x, y, c.w, hgt);
		ButtonState bs;
		bs.kind = kind;
		bs.hover = p.over(r);
		bs.disabled = disabled;
		drawButton(vg, st, r, label, bs, lg, rg);
		if (!disabled)
			hits.add(r, action, arg);
		y += hgt + 5.f;
	};
	if (m == NAV_MODE_HOME) {
		FlowOpts t;
		const text::Page& tp = cache.get(vg, st, "# " + (pk.title.empty() ? std::string("Модульный синтез с нуля") : pk.title), c.w, t);
		drawFlow(vg, st, tp, c.x, y);
		y += tp.height + 8.f;
		int total = (int) pk.order().size();
		int done = s.progress ? s.progress->learnedCount() : 0;
		text::drawText(vg, st.fonts.handle[text::F_MONO], 10.f, st.pal.ink2, c.x, y + 9.f, "Пройдено " + std::to_string(done) + " из " + std::to_string(total));
		y += 14.f;
		nvgBeginPath(vg);
		nvgRect(vg, c.x, y, c.w, 3.f);
		nvgFillColor(vg, st.pal.hairline);
		nvgFill(vg);
		nvgBeginPath(vg);
		nvgRect(vg, c.x, y, total ? c.w * done / total : 0.f, 3.f);
		nvgFillColor(vg, st.accent);
		nvgFill(vg);
		y += 14.f;
		std::string next = s.progress ? s.progress->continueLesson(pk) : (pk.order().empty() ? "" : pk.order()[0]);
		const Lesson* N = pk.lesson(next);
		if (N) {
			text::drawText(vg, st.fonts.handle[text::F_MONO], 9.5f, st.pal.ink2, c.x, y + 9.f, s.progress && s.progress->status(next) == "learning" ? "Ты остановился на" : "Дальше");
			y += 14.f;
			FlowOpts b;
			b.bold = true;
			b.size = 11.5f;
			const text::Page& np = cache.get(vg, st, N->num + " " + N->title, c.w, b);
			drawFlow(vg, st, np, c.x, y);
			y += np.height + 4.f;
			if (!N->builds.empty()) {
				FlowOpts sm;
				sm.size = 10.5f;
				sm.italic = true;
				const text::Page& bp = cache.get(vg, st, N->builds, c.w, sm);
				if (y + bp.height < c.bottom() - 90.f) {
					NVGcolor i2 = st.pal.ink2;
					drawFlow(vg, st, bp, c.x, y, &i2);
					y += bp.height + 6.f;
				}
			}
		}
		y = std::max(y + 6.f, c.bottom() - 80.f);
		if (L)
			y -= 23.f;
		button("Продолжить", NAV_CONTINUE, 0, BUTTON_PRIMARY, !N, -1, G_ARROW_RIGHT, 24.f);
		button("Карта курса", NAV_MAP, 0, BUTTON_NORMAL, false);
		if (L)
			button("К уроку " + L->num, NAV_HOME, 0, BUTTON_QUIET, false, G_ARROW_LEFT, -1, 18.f);
		connection(vg, st, c, s.connected);
		return;
	}
	if (m == NAV_MODE_LESSON) {
		FlowOpts t;
		const text::Page& tp = cache.get(vg, st, "# " + L->title, c.w, t);
		drawFlow(vg, st, tp, c.x, y);
		y += tp.height + 6.f;
		int n = (int) L->blocks.size();
		int b = std::max(0, std::min(n - 1, s.block));
		text::drawText(vg, st.fonts.handle[text::F_MONO], 10.f, st.accent, c.x, y + 9.f, "Блок " + std::to_string(b + 1) + " из " + std::to_string(n));
		y += 14.f;
		if (n > 0) {
			FlowOpts sm;
			sm.size = 11.f;
			const text::Page& bp = cache.get(vg, st, L->blocks[b].title, c.w, sm);
			drawFlow(vg, st, bp, c.x, y);
			y += bp.height + 6.f;
		}
		// Block dots: click to glide.
		float dx = std::min(12.f, c.w / std::max(1, n));
		for (int i = 0; i < n; i++) {
			float cx = c.x + dx * i + 4.f;
			nvgBeginPath(vg);
			nvgCircle(vg, cx, y + 4.f, i == b ? 3.f : 2.2f);
			nvgFillColor(vg, i == b ? st.accent : i < b ? st.pal.ink2 : st.pal.hairline);
			nvgFill(vg);
			hits.add(Rect(cx - dx / 2, y - 2.f, dx, 12.f), NAV_GOTO_BLOCK, i);
		}
		y += 16.f;
		y = std::max(y, c.bottom() - 150.f);
		button("Назад", NAV_PREV, 0, BUTTON_NORMAL, b <= 0, G_ARROW_LEFT);
		button("Дальше", NAV_NEXT, 0, b < n - 1 ? BUTTON_PRIMARY : BUTTON_NORMAL, b >= n - 1, -1, G_ARROW_RIGHT);
		bool learned = s.progress && s.progress->status(L->id) == "learned";
		button(learned ? "Урок пройден ✓" : "Урок пройден", NAV_DONE, 0, b >= n - 1 && !learned ? BUTTON_PRIMARY : BUTTON_QUIET, false);
		button("Карта курса", NAV_MAP, 0, BUTTON_QUIET, false, -1, -1, 18.f);
		if (s.panelExists)
			button(s.panelOpen ? "Скрыть панель" : "Показать панель", NAV_PANEL, 0, BUTTON_QUIET, false, -1, -1, 18.f);
		connection(vg, st, c, s.connected);
		return;
	}
	// Map: one part per page.
	int nparts = (int) pk.parts.size();
	int part = mapPart >= 0 ? mapPart : partOf(pk, s.lesson.empty() && s.progress ? s.progress->continueLesson(pk) : s.lesson);
	part = std::max(0, std::min(nparts - 1, part));
	mapPart = part;
	if (nparts == 0)
		return;
	const Part& P = pk.parts[part];
	Style ps = st;
	ps.accent = tokens::accent(P.n);
	text::drawText(vg, st.fonts.handle[text::F_MONO], 10.f, ps.accent, c.x, y + 9.f, "Часть " + std::to_string(P.n));
	y += 14.f;
	FlowOpts pt;
	pt.bold = true;
	pt.size = 11.5f;
	const text::Page& tp = cache.get(vg, st, P.title, c.w, pt);
	drawFlow(vg, st, tp, c.x, y);
	y += tp.height + 6.f;
	mapRows.clear();
	for (const std::string& id : P.lessons) {
		const Lesson* l = pk.lesson(id);
		if (!l)
			continue;
		Rect row(c.x - 4.f, y, c.w + 8.f, 30.f);
		bool hov = p.over(row);
		if (hov || id == s.lesson) {
			nvgBeginPath(vg);
			nvgRect(vg, row.x, row.y, row.w, row.h);
			nvgFillColor(vg, st.pal.paper2);
			nvgFill(vg);
		}
		drawStatus(vg, ps, c.x + 4.f, y + 9.f, s.progress ? s.progress->status(id) : "", id == s.lesson);
		text::drawText(vg, st.fonts.handle[text::F_MONO], 10.f, st.pal.ink2, c.x + 12.f, y + 12.f, l->num);
		FlowOpts lo;
		lo.size = 10.5f;
		const text::Page& lp = cache.get(vg, st, l->title, c.w - 34.f, lo);
		text::Page clipped = lp;
		if (clipped.lines.size() > 2)
			clipped.lines.resize(2);
		drawFlow(vg, st, clipped, c.x + 34.f, y + 1.f);
		hits.add(row, NAV_OPEN, (int) mapRows.size());
		mapRows.push_back(id);
		y += 32.f;
	}
	Rect pager(c.x, c.bottom() - 46.f, c.w, 18.f);
	if (part > 0)
		hits.add(Rect(pager.x, pager.y, 22.f, 18.f), NAV_PART_PREV);
	if (part < nparts - 1)
		hits.add(Rect(pager.right() - 22.f, pager.y, 22.f, 18.f), NAV_PART_NEXT);
	drawGlyph(vg, G_CHEVRON_LEFT, pager.x + 8.f, pager.y + 9.f, 10.f, part > 0 ? st.pal.ink : st.pal.hairline, 1.5f);
	drawGlyph(vg, G_CHEVRON_RIGHT, pager.right() - 8.f, pager.y + 9.f, 10.f, part < nparts - 1 ? st.pal.ink : st.pal.hairline, 1.5f);
	text::drawText(vg, st.fonts.handle[text::F_MONO], 10.f, st.pal.ink2, pager.x + pager.w / 2, pager.y + 13.f, std::to_string(part + 1) + " / " + std::to_string(nparts), NVG_ALIGN_CENTER | NVG_ALIGN_BASELINE);
	y = c.bottom() - 22.f;
	button(L ? "Вернуться к уроку" : "Закрыть карту", NAV_MAP_CLOSE, 0, BUTTON_QUIET, false, G_ARROW_LEFT);
}

void PanelView::draw(NVGcontext* vg, const Style& st, float w, float h, const NavState& s, bool collapsed, const Pointer& p, Hits& hits) {
	nvgBeginPath(vg);
	nvgRect(vg, 0, 0, w, h);
	nvgFillColor(vg, st.pal.panel);
	nvgFill(vg);
	nvgBeginPath(vg);
	nvgRect(vg, 0, 0, 1.f, h);
	nvgFillColor(vg, st.pal.hairline);
	nvgFill(vg);
	nvgBeginPath(vg);
	nvgRect(vg, 0, 0, w, 3.f);
	nvgFillColor(vg, st.accent);
	nvgFill(vg);
	const Pack* pk = s.pack;
	const Lesson* L = pk ? pk->lesson(s.lesson) : nullptr;
	if (collapsed) {
		Rect b(0, 6.f, w, 24.f);
		drawGlyph(vg, G_CHEVRON_LEFT, w / 2, b.y + 12.f, 11.f, p.over(b) ? st.accent : st.pal.ink, 1.6f);
		hits.add(Rect(0, 0, w, h), NAV_EXPAND);
		if (L) {
			nvgSave(vg);
			nvgTranslate(vg, w / 2 + 4.f, 44.f);
			nvgRotate(vg, NVG_PI / 2);
			text::drawText(vg, st.fonts.handle[text::F_BOLD], 11.f, st.pal.ink, 0, 0, ellipsize(vg, st.fonts.handle[text::F_BOLD], 11.f, L->num + " " + L->title, h - 60.f));
			nvgRestore(vg);
		}
		return;
	}
	float pad = 14.f;
	Rect c(pad, 10.f, w - 2 * pad, h - 20.f);
	float y = c.y;
	// Header: «1.1 · Урок» and the collapse chevron.
	text::drawText(vg, st.fonts.handle[text::F_MONO], 10.5f, st.pal.ink2, c.x, y + 10.f, map ? "Карта курса" : L ? L->num + " · Урок" : "Атлас");
	Rect col(c.right() - 22.f, y - 2.f, 22.f, 20.f);
	drawGlyph(vg, G_CHEVRON_RIGHT, col.x + 11.f, col.y + 10.f, 11.f, p.over(col) ? st.accent : st.pal.ink, 1.6f);
	hits.add(col, NAV_COLLAPSE);
	Rect mapBtn(c.right() - 104.f, y - 2.f, 78.f, 18.f);
	ButtonState ms;
	ms.kind = BUTTON_QUIET;
	ms.hover = p.over(mapBtn);
	ms.active = map;
	drawButton(vg, st, mapBtn, map ? "К уроку" : "Карта курса", ms);
	hits.add(mapBtn, map ? NAV_MAP_CLOSE : NAV_MAP);
	y += 24.f;
	if (map || !L) {
		// Course map: every part, scrollable.
		Rect area(c.x, y, c.w, c.bottom() - y);
		nvgSave(vg);
		nvgScissor(vg, area.x - 4.f, area.y, area.w + 8.f, area.h);
		float y0 = area.y - mapScroll.offset, yy = y0;
		mapRows.clear();
		if (pk) {
			FlowOpts t;
			t.title = true;
			const text::Page& tp = cache.get(vg, st, "# " + (pk->title.empty() ? std::string("Модульный синтез с нуля") : pk->title), area.w, t);
			drawFlow(vg, st, tp, area.x, yy);
			yy += tp.height + 4.f;
			int total = (int) pk->order().size(), done = s.progress ? s.progress->learnedCount() : 0;
			text::drawText(vg, st.fonts.handle[text::F_MONO], 10.f, st.pal.ink2, area.x, yy + 9.f, "Пройдено " + std::to_string(done) + " из " + std::to_string(total));
			yy += 20.f;
			for (const Part& P : pk->parts) {
				Style ps = st;
				ps.accent = tokens::accent(P.n);
				nvgBeginPath(vg);
				nvgRect(vg, area.x, yy + 6.f, 14.f, 2.f);
				nvgFillColor(vg, ps.accent);
				nvgFill(vg);
				text::drawText(vg, st.fonts.handle[text::F_BOLD], 11.5f, st.pal.ink, area.x + 20.f, yy + 11.f, std::to_string(P.n) + ". " + P.title);
				yy += 20.f;
				for (const std::string& id : P.lessons) {
					const Lesson* l = pk->lesson(id);
					if (!l)
						continue;
					Rect row(area.x - 4.f, yy, area.w + 8.f, 19.f);
					bool hov = p.over(row) && p.y > area.y && p.y < area.bottom();
					if (hov || id == s.lesson) {
						nvgBeginPath(vg);
						nvgRect(vg, row.x, row.y, row.w, row.h);
						nvgFillColor(vg, st.pal.paper2);
						nvgFill(vg);
					}
					drawStatus(vg, ps, area.x + 6.f, yy + 9.5f, s.progress ? s.progress->status(id) : "", id == s.lesson);
					text::drawText(vg, st.fonts.handle[text::F_MONO], 10.f, st.pal.ink2, area.x + 16.f, yy + 13.f, l->num);
					text::drawText(vg, st.fonts.handle[text::F_BODY], 11.5f, st.pal.ink, area.x + 44.f, yy + 13.f, ellipsize(vg, st.fonts.handle[text::F_BODY], 11.5f, l->title, area.w - 48.f));
					hits.add(row, NAV_OPEN, (int) mapRows.size());
					mapRows.push_back(id);
					yy += 19.f;
				}
				yy += 8.f;
			}
		}
		nvgRestore(vg);
		mapScroll.content = yy - y0;
		mapScroll.view = area.h;
		mapScroll.clamp();
		drawScrollbar(vg, st, Rect(area.x, area.y, area.w + 10.f, area.h), mapScroll);
		std::vector<Hit> visible;
		for (const Hit& hh : hits.list)
			if (hh.action != NAV_OPEN || (hh.r.y + hh.r.h > area.y && hh.r.y < area.bottom()))
				visible.push_back(hh);
		hits.list = visible;
		return;
	}
	// Lesson title and outline.
	FlowOpts t;
	t.title = true;
	const text::Page& tp = cache.get(vg, st, "# " + L->title, c.w, t);
	drawFlow(vg, st, tp, c.x, y);
	y += tp.height + 8.f;
	int n = (int) L->blocks.size();
	int b = std::max(0, std::min(n - 1, s.block));
	float rowH = 20.f;
	float outlineH = std::min(n * rowH, std::max(rowH * 3, (c.bottom() - y) * 0.36f));
	Rect outline(c.x, y, c.w, outlineH);
	nvgSave(vg);
	nvgScissor(vg, outline.x - 4.f, outline.y, outline.w + 8.f, outline.h);
	outlineScroll.content = n * rowH;
	outlineScroll.view = outline.h;
	if (outlineScroll.active()) {
		// Keep the current block in view.
		float top = b * rowH, bottom = top + rowH;
		if (top < outlineScroll.offset)
			outlineScroll.offset = top;
		if (bottom > outlineScroll.offset + outline.h)
			outlineScroll.offset = bottom - outline.h;
	}
	outlineScroll.clamp();
	const LessonProgress* lp = nullptr;
	if (s.progress) {
		auto it = s.progress->lessons.find(L->id);
		if (it != s.progress->lessons.end())
			lp = &it->second;
	}
	for (int i = 0; i < n; i++) {
		float ry = outline.y + i * rowH - outlineScroll.offset;
		Rect row(c.x - 4.f, ry, c.w + 8.f, rowH);
		bool cur = i == b;
		bool hov = p.over(row) && p.y >= outline.y && p.y <= outline.bottom();
		if (cur || hov) {
			nvgBeginPath(vg);
			nvgRect(vg, row.x, row.y, row.w, row.h);
			nvgFillColor(vg, cur ? tint(st, 0.12f) : st.pal.paper2);
			nvgFill(vg);
		}
		if (cur) {
			nvgBeginPath(vg);
			nvgRect(vg, row.x, row.y, 3.f, row.h);
			nvgFillColor(vg, st.accent);
			nvgFill(vg);
		}
		bool reached = lp && std::find(lp->reached.begin(), lp->reached.end(), L->blocks[i].id) != lp->reached.end();
		text::drawText(vg, st.fonts.handle[text::F_MONO], 10.f, cur ? st.accent : st.pal.ink2, c.x + 4.f, ry + 14.f, std::to_string(i + 1));
		int f = st.fonts.handle[cur ? text::F_BOLD : text::F_BODY];
		text::drawText(vg, f, 11.5f, cur ? st.pal.ink : reached ? st.pal.ink2 : st.pal.ink, c.x + 24.f, ry + 14.f, ellipsize(vg, f, 11.5f, L->blocks[i].title, c.w - 40.f));
		if (reached && !cur)
			drawGlyph(vg, G_CHECK, c.right() - 6.f, ry + 10.f, 7.f, okColor(st), 1.3f);
		if (ry + rowH > outline.y && ry < outline.bottom())
			hits.add(row, NAV_GOTO_BLOCK, i);
	}
	nvgRestore(vg);
	drawScrollbar(vg, st, Rect(outline.x, outline.y, outline.w + 10.f, outline.h), outlineScroll);
	y += outline.h + 8.f;
	// Block counter with back / next.
	Rect back(c.x, y, 84.f, 22.f), next(c.right() - 92.f, y, 92.f, 22.f);
	ButtonState bs;
	bs.hover = p.over(back);
	bs.disabled = b <= 0;
	drawButton(vg, st, back, "Назад", bs, G_ARROW_LEFT);
	if (!bs.disabled)
		hits.add(back, NAV_PREV);
	ButtonState ns;
	ns.kind = BUTTON_PRIMARY;
	ns.hover = p.over(next);
	bool last = b >= n - 1;
	bool learned = s.progress && s.progress->status(L->id) == "learned";
	drawButton(vg, st, next, last ? (learned ? "Пройден ✓" : "Урок пройден") : "Дальше", ns, -1, last ? -1 : G_ARROW_RIGHT);
	hits.add(next, last ? NAV_DONE : NAV_NEXT);
	text::drawText(vg, st.fonts.handle[text::F_MONO], 10.f, st.pal.ink2, (back.right() + next.x) / 2, y + 15.f, "Блок " + std::to_string(b + 1) + " из " + std::to_string(n), NVG_ALIGN_CENTER | NVG_ALIGN_BASELINE);
	y += 32.f;
	if (!s.notice.empty()) {
		FlowOpts no;
		no.size = 10.5f;
		no.italic = true;
		const text::Page& np = cache.get(vg, st, s.notice, c.w, no);
		NVGcolor i2 = st.pal.ink2;
		drawFlow(vg, st, np, c.x, y, &i2);
		y += np.height + 6.f;
	}
	drawHairline(vg, st, c.x, y, c.right());
	text::drawText(vg, st.fonts.handle[text::F_MONO], 10.f, st.pal.ink2, c.x, y + 16.f, "Репетитор");
	drawLight(vg, st, c.right() - 4.f, y + 12.f, 3.5f, s.connected ? 1 : 0);
	y += 26.f;
	tutor.offline = !s.connected;
	tutor.draw(vg, st, Rect(c.x, y, c.w, c.bottom() - y), p, hits);
}

} // namespace atlas
