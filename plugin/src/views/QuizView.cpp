#include "QuizView.hpp"

#include <algorithm>

namespace atlas {

std::string optionLetter(int k) {
	static const char* l[] = {"А", "Б", "В", "Г", "Д", "Е"};
	return k >= 0 && k < 6 ? l[k] : "?";
}

namespace {
const float GAP = 8.f;
const float OPT_PAD = 6.f;
}

void QuizView::draw(NVGcontext* vg, const Style& st, float w, float h, const Pointer& p, Hits& hits) {
	int n = questions ? (int) questions->size() : 0;
	if (state)
		state->fit(n);
	std::string right = n > 1 && state ? std::to_string(state->current + 1) + "/" + std::to_string(n) : "";
	Rect c = drawPanel(vg, st, w, h, header, right);
	if (!n || !state)
		return;
	const Question& q = (*questions)[state->current];
	QuizAnswer& A = state->answers[state->current];
	bool footer = n > 1;
	Rect area(c.x, c.y, c.w, c.h - (footer ? tokens::panel_footerHeight : 0.f));

	nvgSave(vg);
	nvgScissor(vg, 0, area.y - 2.f, w, area.h + 2.f);
	float y0 = area.y - scroll.offset;
	float y = y0;
	FlowOpts qo;
	qo.bold = true;
	const text::Page& qp = cache.get(vg, st, q.q, area.w, qo);
	drawFlow(vg, st, qp, area.x, y);
	y += qp.height + GAP + 2.f;

	FlowOpts small;
	small.size = 11.f;
	FlowOpts why;
	why.size = 10.5f;
	auto optionBox = [&](int k, NVGcolor border, bool fill, const std::string& prefixGlyph) -> float {
		const Option& o = q.options[k];
		float lw = 16.f;
		const text::Page& op = cache.get(vg, st, o.text, area.w - lw - 2 * OPT_PAD, small);
		float bh = std::max(op.height, 16.f) + 2 * OPT_PAD;
		Rect r(area.x, y, area.w, bh);
		if (fill) {
			nvgBeginPath(vg);
			nvgRect(vg, r.x, r.y, r.w, r.h);
			nvgFillColor(vg, st.pal.paper2);
			nvgFill(vg);
		}
		nvgBeginPath(vg);
		nvgRect(vg, r.x + 0.5f, r.y + 0.5f, r.w - 1.f, r.h - 1.f);
		nvgStrokeColor(vg, border);
		nvgStrokeWidth(vg, 1.f);
		nvgStroke(vg);
		text::drawText(vg, st.fonts.handle[text::F_MONO], 10.5f, st.pal.ink2, r.x + OPT_PAD, r.y + OPT_PAD + 11.5f, optionLetter(k));
		drawFlow(vg, st, op, r.x + OPT_PAD + lw, r.y + OPT_PAD);
		(void) prefixGlyph;
		y += bh;
		return bh;
	};

	if (state->freeMode && A.chosen == CHOSEN_NONE) {
		// Free answer: the learner's own words, graded by the tutor.
		const text::Page& hint = cache.get(vg, st, "Ответь своими словами — репетитор проверит.", area.w, why);
		NVGcolor ink2 = st.pal.ink2;
		drawFlow(vg, st, hint, area.x, y, &ink2);
		y += hint.height + 6.f;
		float ih = std::max(inputHeight(vg, st, input, area.w), 3 * 16.f + 2 * INPUT_PAD);
		inputRect = Rect(area.x, y, area.w, ih);
		drawInput(vg, st, inputRect, input, "Твой ответ…", cursor, selection, focused, blink, offline);
		hits.add(inputRect, QUIZ_INPUT);
		y += ih + GAP;
		float bw = (area.w - 6.f) / 2.f;
		Rect send(area.x, y, bw, 20.f), cancel(area.x + bw + 6.f, y, bw, 20.f);
		ButtonState bs;
		bs.kind = BUTTON_PRIMARY;
		bs.hover = p.over(send);
		bs.disabled = input.empty() || offline;
		drawButton(vg, st, send, "Отправить", bs);
		if (!bs.disabled)
			hits.add(send, QUIZ_SEND);
		ButtonState cs;
		cs.hover = p.over(cancel);
		drawButton(vg, st, cancel, "Отмена", cs);
		hits.add(cancel, QUIZ_CANCEL);
		y += 20.f + GAP;
		if (offline) {
			const text::Page& off = cache.get(vg, st, "Репетитор недоступен: Atlas не запущен. Выбери вариант или нажми «Не знаю».", area.w, why);
			drawFlow(vg, st, off, area.x, y, &ink2);
			y += off.height;
		}
	}
	else if (A.chosen == CHOSEN_NONE) {
		for (size_t k = 0; k < q.options.size(); k++) {
			float top = y;
			Rect probe(area.x, top, area.w, 0);
			// draw with hover border
			const Option& o = q.options[k];
			const text::Page& op = cache.get(vg, st, o.text, area.w - 16.f - 2 * OPT_PAD, small);
			float bh = std::max(op.height, 16.f) + 2 * OPT_PAD;
			Rect r(area.x, top, area.w, bh);
			bool hov = p.over(r) && p.y >= area.y && p.y <= area.bottom();
			optionBox((int) k, hov ? st.accent : st.pal.hairline, false, "");
			hits.add(r, QUIZ_CHOOSE, (int) k);
			y += 5.f;
			(void) probe;
		}
		y += 3.f;
		float bw = (area.w - 6.f) / 2.f;
		Rect dk(area.x, y, q.free ? bw : area.w, 20.f);
		ButtonState ds;
		ds.hover = p.over(dk);
		drawButton(vg, st, dk, "Не знаю", ds);
		hits.add(dk, QUIZ_DONTKNOW);
		if (q.free) {
			Rect fr(area.x + bw + 6.f, y, bw, 20.f);
			ButtonState fs;
			fs.hover = p.over(fr);
			drawButton(vg, st, fr, "Свой ответ", fs);
			hits.add(fr, QUIZ_FREE);
		}
		y += 20.f;
	}
	else {
		// Verdict.
		int correct = -1;
		for (size_t k = 0; k < q.options.size(); k++)
			if (q.options[k].correct)
				correct = (int) k;
		bool right = A.chosen >= 0 && A.chosen == correct;
		NVGcolor vc = A.chosen == CHOSEN_FREE ? st.accent : right ? okColor(st) : A.chosen == CHOSEN_DONTKNOW ? st.pal.ink2 : failColor(st);
		std::string vt = A.chosen == CHOSEN_FREE ? (A.busy ? "Репетитор читает ответ…" : "Свой ответ") : right ? "Верно" : A.chosen == CHOSEN_DONTKNOW ? "Разберём" : "Не совсем";
		Glyph g = A.chosen == CHOSEN_FREE ? G_QUESTION : right ? G_CHECK : A.chosen == CHOSEN_DONTKNOW ? G_QUESTION : G_CROSS;
		drawGlyph(vg, g, area.x + 6.f, y + 8.f, 11.f, vc, 1.8f);
		text::drawText(vg, st.fonts.handle[text::F_BOLD], 12.f, vc, area.x + 18.f, y + 12.f, vt);
		y += 22.f;
		if (A.chosen >= 0) {
			optionBox(A.chosen, vc, true, "");
			y += 4.f;
			const text::Page& wp = cache.get(vg, st, q.options[A.chosen].why, area.w, why);
			drawFlow(vg, st, wp, area.x, y);
			y += wp.height + GAP;
		}
		if (A.chosen == CHOSEN_FREE) {
			const text::Page& mine = cache.get(vg, st, "«" + A.freeText + "»", area.w, why);
			NVGcolor ink2 = st.pal.ink2;
			drawFlow(vg, st, mine, area.x, y, &ink2);
			y += mine.height + 6.f;
			if (A.busy) {
				drawGlyph(vg, G_DOT, area.x + 4.f, y + 6.f, 10.f, st.accent);
				drawGlyph(vg, G_DOT, area.x + 12.f, y + 6.f, 10.f, st.accent);
				drawGlyph(vg, G_DOT, area.x + 20.f, y + 6.f, 10.f, st.accent);
				y += 16.f;
			}
			else if (!A.verdict.empty()) {
				const text::Page& vp = cache.get(vg, st, A.verdict, area.w, small);
				drawFlow(vg, st, vp, area.x, y);
				y += vp.height + GAP;
			}
		}
		if (!right && correct >= 0 && !(A.chosen == CHOSEN_FREE && A.busy)) {
			text::drawText(vg, st.fonts.handle[text::F_MONO], 10.f, st.pal.ink2, area.x, y + 9.f, "Верный ответ");
			y += 14.f;
			optionBox(correct, okColor(st), false, "");
			y += 4.f;
			const text::Page& wp = cache.get(vg, st, q.options[correct].why, area.w, why);
			drawFlow(vg, st, wp, area.x, y);
			y += wp.height + GAP;
		}
		float bw = (area.w - 6.f) / 2.f;
		bool more = state->current < n - 1;
		Rect again(area.x, y, more ? bw : area.w, 20.f);
		ButtonState as;
		as.hover = p.over(again);
		drawButton(vg, st, again, "Ещё раз", as);
		hits.add(again, QUIZ_RETRY);
		if (more) {
			Rect nx(area.x + bw + 6.f, y, bw, 20.f);
			ButtonState ns;
			ns.kind = BUTTON_PRIMARY;
			ns.hover = p.over(nx);
			drawButton(vg, st, nx, "Дальше", ns, -1, G_ARROW_RIGHT);
			hits.add(nx, QUIZ_NEXT);
		}
		y += 20.f;
	}
	nvgRestore(vg);
	scroll.content = y - y0 + 4.f;
	scroll.view = area.h;
	scroll.clamp();
	drawScrollbar(vg, st, Rect(area.x, area.y, area.w + 8.f, area.h), scroll);
	if (scroll.active() && !scroll.atBottom())
		drawGlyph(vg, G_ARROW_DOWN, area.right() - 4.f, area.bottom() - 8.f, 9.f, st.accent, 1.4f);
	// Hits outside the visible area must not fire.
	std::vector<Hit> visible;
	for (const Hit& hh : hits.list)
		if (hh.r.y + hh.r.h > area.y && hh.r.y < area.bottom())
			visible.push_back(hh);
	hits.list = visible;
	if (footer) {
		std::vector<Rect> dots = drawDots(vg, st, c.x + c.w / 2, c.bottom() - 8.f, n, state->current);
		for (size_t i = 0; i < dots.size(); i++)
			hits.add(dots[i], QUIZ_GOTO, (int) i);
	}
}

bool QuizView::action(int a, int arg) {
	if (!state || !questions || questions->empty())
		return false;
	QuizAnswer& A = state->answers[state->current];
	switch (a) {
		case QUIZ_CHOOSE:
			A.chosen = arg;
			scroll.offset = 0;
			return true;
		case QUIZ_DONTKNOW:
			A.chosen = CHOSEN_DONTKNOW;
			scroll.offset = 0;
			return true;
		case QUIZ_FREE:
			state->freeMode = true;
			return true;
		case QUIZ_CANCEL:
			state->freeMode = false;
			return true;
		case QUIZ_RETRY:
			A = QuizAnswer();
			state->freeMode = false;
			scroll.offset = 0;
			return true;
		case QUIZ_NEXT:
			if (state->current < (int) questions->size() - 1)
				state->current++;
			state->freeMode = false;
			scroll.offset = 0;
			return true;
		case QUIZ_GOTO:
			state->current = arg;
			state->freeMode = false;
			scroll.offset = 0;
			return true;
	}
	return false;
}

} // namespace atlas
