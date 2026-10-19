#include "TaskView.hpp"

#include <algorithm>

namespace atlas {

const check::Item* TaskView::firstFailing() const {
	if (!state || !state->hasResult)
		return nullptr;
	for (const check::Item& it : state->result.items)
		if (!it.ok)
			return &it;
	return nullptr;
}

std::string TaskView::currentHint(int* level, int* levels) const {
	const check::Item* f = firstFailing();
	if (!f)
		return "";
	std::vector<std::string> ladder;
	if (item) {
		auto it = item->hints.find(f->key);
		if (it != item->hints.end())
			ladder = it->second;
	}
	if (ladder.empty())
		ladder.push_back(f->hint);
	int lv = 0;
	auto h = state->hintLevel.find(f->key);
	if (h != state->hintLevel.end())
		lv = h->second;
	lv = std::max(0, std::min((int) ladder.size() - 1, lv));
	if (level)
		*level = lv;
	if (levels)
		*levels = (int) ladder.size();
	return ladder[lv];
}

int TaskView::light() const {
	if (!state)
		return 0;
	if (state->checking)
		return 3;
	if (!state->hasResult)
		return 0;
	return state->result.passed ? 1 : 2;
}

void TaskView::draw(NVGcontext* vg, const Style& st, float w, float h, const Pointer& p, Hits& hits) {
	std::string mode = item ? item->mode : "practice";
	std::string kind = mode == "try" ? "Покрути сам" : mode == "debug" ? "Найди поломку" : "Практика";
	Rect c = drawPanel(vg, st, w, h, header, kind);
	if (!item || !state)
		return;
	bool checkable = item->check != nullptr;
	float footerH = checkable ? 30.f : 0.f;
	Rect area(c.x, c.y, c.w, c.h - footerH);
	nvgSave(vg);
	nvgScissor(vg, 0, area.y - 2.f, w, area.h + 2.f);
	float y0 = area.y - scroll.offset, y = y0;
	FlowOpts titleOpts;
	if (state->page == 0 || !state->hasResult) {
		if (!title.empty()) {
			const text::Page& tp = cache.get(vg, st, "# " + title, area.w, titleOpts);
			drawFlow(vg, st, tp, area.x, y);
			y += tp.height + 6.f;
		}
		if (mode == "try" && !item->steps.empty()) {
			if (state->stepsDone.size() != item->steps.size())
				state->stepsDone.resize(item->steps.size());
			for (size_t i = 0; i < item->steps.size(); i++) {
				const Step& s = item->steps[i];
				float bx = area.x, top = y;
				bool done = state->stepsDone[i];
				FlowOpts small;
				small.size = 11.5f;
				const text::Page& dp = cache.get(vg, st, s.doText, area.w - 20.f, small);
				FlowOpts see;
				see.size = 10.5f;
				see.italic = true;
				const text::Page& sp = cache.get(vg, st, s.see.empty() ? "" : "→ " + s.see, area.w - 20.f, see);
				Rect box(bx, top + 2.f, 12.f, 12.f);
				nvgBeginPath(vg);
				nvgRect(vg, box.x + 0.5f, box.y + 0.5f, box.w - 1.f, box.h - 1.f);
				nvgStrokeColor(vg, done ? st.accent : st.pal.ink2);
				nvgStrokeWidth(vg, 1.f);
				nvgStroke(vg);
				if (done)
					drawGlyph(vg, G_CHECK, box.x + 6.f, box.y + 6.f, 8.f, st.accent, 1.6f);
				NVGcolor ink = done ? st.pal.ink2 : st.pal.ink;
				drawFlow(vg, st, dp, bx + 20.f, top, &ink);
				float hh = dp.height;
				if (!s.see.empty()) {
					NVGcolor i2 = st.pal.ink2;
					drawFlow(vg, st, sp, bx + 20.f, top + dp.height + 1.f, &i2);
					hh += sp.height + 1.f;
				}
				hits.add(Rect(bx, top, area.w, hh), TASK_STEP, (int) i);
				y += hh + 8.f;
			}
		}
		if (!item->text.empty()) {
			const text::Page& bp = cache.get(vg, st, item->text, area.w);
			drawFlow(vg, st, bp, area.x, y);
			y += bp.height + 6.f;
		}
		if (!state->error.empty()) {
			NVGcolor fc = failColor(st);
			FlowOpts o;
			o.size = 10.5f;
			const text::Page& ep = cache.get(vg, st, state->error, area.w, o);
			drawFlow(vg, st, ep, area.x, y, &fc);
			y += ep.height + 4.f;
		}
	}
	else {
		const check::Result& r = state->result;
		NVGcolor vc = r.passed ? okColor(st) : failColor(st);
		drawGlyph(vg, r.passed ? G_CHECK : G_CROSS, area.x + 6.f, y + 8.f, 11.f, vc, 1.8f);
		text::drawText(vg, st.fonts.handle[text::F_BOLD], 12.f, vc, area.x + 18.f, y + 12.f, r.passed ? "Всё работает" : "Пока не всё");
		y += 22.f;
		int ok = 0;
		for (const check::Item& it : r.items)
			ok += it.ok;
		text::drawText(vg, st.fonts.handle[text::F_MONO], 10.f, st.pal.ink2, area.x, y + 9.f, std::to_string(ok) + " из " + std::to_string(r.items.size()) + " на месте");
		y += 18.f;
		FlowOpts small;
		small.size = 10.5f;
		int shown = 0;
		for (const check::Item& it : r.items) {
			if (it.ok)
				continue;
			if (shown == 4) {
				int rest = 0;
				for (const check::Item& x : r.items)
					rest += !x.ok;
				text::drawText(vg, st.fonts.handle[text::F_ITALIC], 10.5f, st.pal.ink2, area.x + 14.f, y + 10.f, "и ещё " + std::to_string(rest - 4));
				y += 16.f;
				break;
			}
			const text::Page& ip = cache.get(vg, st, it.what, area.w - 14.f, small);
			drawGlyph(vg, G_CROSS, area.x + 4.f, y + 7.f, 7.f, failColor(st), 1.4f);
			drawFlow(vg, st, ip, area.x + 14.f, y);
			y += ip.height + 3.f;
			shown++;
		}
		int level = 0, levels = 0;
		std::string hint = currentHint(&level, &levels);
		if (!hint.empty()) {
			y += 6.f;
			FlowOpts ho;
			ho.size = 11.5f;
			const text::Page& hp = cache.get(vg, st, hint, area.w - 16.f, ho);
			float bh = hp.height + 30.f + (level < levels - 1 ? 24.f : 0.f);
			nvgBeginPath(vg);
			nvgRect(vg, area.x, y, area.w, bh);
			nvgFillColor(vg, st.pal.paper2);
			nvgFill(vg);
			nvgBeginPath(vg);
			nvgRect(vg, area.x, y, 3.f, bh);
			nvgFillColor(vg, st.accent);
			nvgFill(vg);
			text::drawText(vg, st.fonts.handle[text::F_MONO], 9.5f, st.pal.ink2, area.x + 10.f, y + 13.f, "Подсказка " + std::to_string(level + 1) + "/" + std::to_string(levels));
			drawFlow(vg, st, hp, area.x + 10.f, y + 20.f);
			if (level < levels - 1) {
				Rect more(area.x + 10.f, y + 24.f + hp.height, std::min(area.w - 20.f, 120.f), 18.f);
				ButtonState ms;
				ms.kind = BUTTON_QUIET;
				ms.hover = p.over(more);
				drawButton(vg, st, more, "Ещё подсказка", ms);
				hits.add(more, TASK_MOREHINT);
			}
			y += bh + 6.f;
		}
		Rect back(area.x, y, 90.f, 18.f);
		ButtonState bs;
		bs.kind = BUTTON_QUIET;
		bs.hover = p.over(back);
		drawButton(vg, st, back, "Задание", bs, G_ARROW_LEFT);
		hits.add(back, TASK_BRIEF);
		y += 22.f;
	}
	nvgRestore(vg);
	scroll.content = y - y0;
	scroll.view = area.h;
	scroll.clamp();
	drawScrollbar(vg, st, Rect(area.x, area.y, area.w + 8.f, area.h), scroll);
	std::vector<Hit> visible;
	for (const Hit& hh : hits.list)
		if (hh.r.y + hh.r.h > area.y && hh.r.y < area.bottom())
			visible.push_back(hh);
	hits.list = visible;
	if (checkable) {
		float fy = c.bottom() - 22.f;
		drawHairline(vg, st, c.x, fy - 6.f, c.right());
		bool showResultLink = state->hasResult && state->page == 0;
		float bw = showResultLink ? c.w - 92.f : c.w - 22.f;
		Rect btn(c.x, fy, bw, 22.f);
		ButtonState s;
		s.kind = BUTTON_PRIMARY;
		s.hover = p.over(btn);
		s.disabled = state->checking;
		drawButton(vg, st, btn, state->checking ? "Проверяю…" : state->hasResult ? "Проверить ещё раз" : "Проверить", s);
		if (!state->checking)
			hits.add(btn, TASK_CHECK);
		if (showResultLink) {
			Rect rl(c.x + bw + 4.f, fy, 66.f, 22.f);
			ButtonState rs;
			rs.kind = BUTTON_QUIET;
			rs.hover = p.over(rl);
			drawButton(vg, st, rl, "Итог", rs, -1, G_ARROW_RIGHT);
			hits.add(rl, TASK_RESULT);
		}
		drawLight(vg, st, c.right() - 6.f, fy + 11.f, 5.f, light());
	}
}

bool TaskView::action(int a, int arg) {
	if (!state)
		return false;
	switch (a) {
		case TASK_MOREHINT: {
			const check::Item* f = firstFailing();
			if (f)
				state->hintLevel[f->key]++;
			return true;
		}
		case TASK_BRIEF:
			state->page = 0;
			scroll.offset = 0;
			return true;
		case TASK_RESULT:
			state->page = 1;
			scroll.offset = 0;
			return true;
		case TASK_STEP:
			if (arg >= 0 && arg < (int) state->stepsDone.size())
				state->stepsDone[arg] = !state->stepsDone[arg];
			return true;
	}
	return false;
}

} // namespace atlas
