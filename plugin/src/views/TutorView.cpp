#include "TutorView.hpp"

#include <algorithm>
#include <cmath>

namespace atlas {

const char* quickLabel(int k) {
	static const char* l[] = {"Объясни проще", "Пример", "Подсказка", "Проверь мой патч"};
	return k >= 0 && k < 4 ? l[k] : "";
}

void TutorView::draw(NVGcontext* vg, const Style& st, const Rect& r, const Pointer& p, Hits& hits) {
	float y = r.bottom();
	// Input with a send arrow.
	float ih = inputHeight(vg, st, input, r.w - 24.f);
	inputRect = Rect(r.x, y - ih, r.w - 24.f, ih);
	drawInput(vg, st, inputRect, input, offline ? "Репетитор недоступен: Atlas не запущен" : "Спроси про этот блок…", cursor, selection, focused, blink, offline);
	hits.add(inputRect, TUTOR_INPUT);
	Rect send(r.right() - 20.f, y - ih, 20.f, ih);
	bool canSend = !input.empty() && !offline && !busy;
	drawGlyph(vg, G_ARROW_RIGHT, send.x + 10.f, send.y + send.h / 2, 11.f, canSend ? (p.over(send) ? st.pal.ink : st.accent) : st.pal.hairline, 1.6f);
	if (canSend)
		hits.add(send, TUTOR_SEND);
	y -= ih + 6.f;
	// Quick actions: 2 × 2.
	if (showQuick) {
		float bw = (r.w - 6.f) / 2.f, bh = 19.f;
		for (int k = 3; k >= 0; k--) {
			int row = k / 2, col = k % 2;
			Rect b(r.x + col * (bw + 6.f), y - (2 - row) * (bh + 4.f) + 4.f, bw, bh);
			ButtonState s;
			s.hover = p.over(b);
			s.disabled = busy || (offline && k != QUICK_HINT && k != QUICK_SIMPLER);
			drawButton(vg, st, b, quickLabel(k), s);
			if (!s.disabled)
				hits.add(b, TUTOR_QUICK, k);
		}
		y -= 2 * (bh + 4.f) + 4.f;
	}
	// FAQ chips (prepared answers, instant).
	if (faq && !faq->empty()) {
		FlowOpts o;
		o.size = 10.5f;
		std::vector<float> hs;
		float total = 0.f;
		for (const Faq& f : *faq) {
			const text::Page& fp = cache.get(vg, st, f.q, r.w - 26.f, o);
			hs.push_back(fp.height + 8.f);
			total += fp.height + 8.f + 4.f;
		}
		float cy = y - total;
		for (size_t i = 0; i < faq->size(); i++) {
			const text::Page& fp = cache.get(vg, st, (*faq)[i].q, r.w - 26.f, o);
			Rect chip(r.x, cy, r.w, hs[i]);
			bool hov = p.over(chip);
			nvgBeginPath(vg);
			nvgRect(vg, chip.x + 0.5f, chip.y + 0.5f, chip.w - 1.f, chip.h - 1.f);
			nvgStrokeColor(vg, hov ? st.accent : st.pal.hairline);
			nvgStrokeWidth(vg, 1.f);
			nvgStroke(vg);
			drawGlyph(vg, G_QUESTION, chip.x + 10.f, chip.y + chip.h / 2, 9.f, st.accent, 1.3f);
			drawFlow(vg, st, fp, chip.x + 20.f, chip.y + 4.f);
			hits.add(chip, TUTOR_FAQ, (int) i);
			cy += hs[i] + 4.f;
		}
		y -= total + 4.f;
	}
	// Conversation.
	Rect conv(r.x, r.y, r.w, std::max(0.f, y - r.y));
	drawHairline(vg, st, r.x, y + 1.f, r.right());
	if (conv.h < 20.f)
		return;
	FlowOpts mo;
	mo.size = 11.5f;
	float maxW = conv.w * 0.86f;
	struct Laid {
		const text::Page* page;
		float h;
	};
	std::vector<Laid> laid;
	float total = 6.f;
	const std::vector<TutorMessage> empty;
	const std::vector<TutorMessage>& ms = messages ? *messages : empty;
	for (const TutorMessage& m : ms) {
		std::string t = m.text.empty() && m.streaming ? " " : m.text;
		const text::Page& pg = cache.get(vg, st, t, maxW - 16.f, mo);
		float h = pg.height + 12.f + (m.local ? 13.f : 0.f) + (m.streaming ? 10.f : 0.f);
		laid.push_back({&pg, h});
		total += h + 8.f;
	}
	size_t len = 0;
	for (const TutorMessage& m : ms)
		len += m.text.size();
	scroll.view = conv.h;
	scroll.content = total;
	if (ms.size() != lastCount || len != lastLen) {
		if (scroll.atBottom() || ms.size() > lastCount || scroll.content <= scroll.view)
			scroll.stickBottom = true;
		else
			newBelow = true;
		lastCount = ms.size();
		lastLen = len;
	}
	if (ms.empty())
		scroll.stickBottom = true;
	scroll.clamp();
	if (scroll.atBottom())
		newBelow = false;
	nvgSave(vg);
	nvgScissor(vg, conv.x - 2.f, conv.y, conv.w + 4.f, conv.h);
	if (ms.empty()) {
		FlowOpts eo;
		eo.size = 11.f;
		eo.italic = true;
		const text::Page& ep = cache.get(vg, st, offline ? "Atlas не запущен: готовые вопросы ниже работают и без него." : "Спроси про этот блок — или выбери готовый вопрос ниже.", conv.w, eo);
		NVGcolor i2 = st.pal.ink2;
		drawFlow(vg, st, ep, conv.x, conv.y + 6.f, &i2);
	}
	float my = conv.y + 6.f - scroll.offset;
	for (size_t i = 0; i < ms.size(); i++) {
		const TutorMessage& m = ms[i];
		const text::Page& pg = *laid[i].page;
		float bw = std::max(40.f, std::min(maxW, pg.height > 18.f ? maxW : 0.f) );
		float textW = 0.f;
		for (const text::Line& l : pg.lines)
			textW = std::max(textW, l.width);
		bw = std::min(maxW, textW + 16.f);
		bool learner = m.role == "learner";
		float bx = learner ? conv.right() - bw : conv.x;
		float top = my;
		if (m.local) {
			text::drawText(vg, st.fonts.handle[text::F_MONO], 9.f, st.accent, bx, top + 9.f, "из урока");
			top += 13.f;
		}
		float bh = pg.height + 12.f + (m.streaming ? 10.f : 0.f);
		nvgBeginPath(vg);
		nvgRect(vg, bx, top, bw, bh);
		if (learner) {
			nvgFillColor(vg, st.pal.paper2);
			nvgFill(vg);
		}
		else {
			nvgFillColor(vg, st.pal.panel);
			nvgFill(vg);
			nvgBeginPath(vg);
			nvgRect(vg, bx + 0.5f, top + 0.5f, bw - 1.f, bh - 1.f);
			nvgStrokeColor(vg, m.error ? failColor(st) : st.pal.hairline);
			nvgStrokeWidth(vg, 1.f);
			nvgStroke(vg);
			nvgBeginPath(vg);
			nvgRect(vg, bx, top, 2.f, bh);
			nvgFillColor(vg, m.local ? st.accent : st.pal.ink2);
			nvgFill(vg);
		}
		drawFlow(vg, st, pg, bx + 8.f, top + 6.f);
		if (m.streaming) {
			for (int k = 0; k < 3; k++) {
				float ph = std::fmod(time * 2.f + k * 0.25f, 1.f);
				nvgBeginPath(vg);
				nvgCircle(vg, bx + 12.f + k * 7.f, top + bh - 8.f, 2.f);
				nvgFillColor(vg, nvgTransRGBAf(st.accent, 0.3f + 0.7f * (ph < 0.5f ? ph * 2 : 2 - ph * 2)));
				nvgFill(vg);
			}
		}
		hits.add(Rect(bx, top, bw, bh), TUTOR_MSG, (int) i);
		my += laid[i].h + 8.f;
	}
	nvgRestore(vg);
	drawScrollbar(vg, st, Rect(conv.x, conv.y, conv.w + 6.f, conv.h), scroll);
	if (newBelow) {
		Rect chip(conv.right() - 64.f, conv.bottom() - 22.f, 60.f, 18.f);
		nvgBeginPath(vg);
		nvgRect(vg, chip.x, chip.y, chip.w, chip.h);
		nvgFillColor(vg, st.accent);
		nvgFill(vg);
		text::drawText(vg, st.fonts.handle[text::F_BOLD], 10.f, st.pal.panel, chip.x + chip.w / 2, chip.y + 12.5f, "вниз ↓", NVG_ALIGN_CENTER | NVG_ALIGN_BASELINE);
		hits.add(chip, TUTOR_SCROLL_DOWN);
	}
	// Message hits outside the conversation area must not fire.
	std::vector<Hit> visible;
	for (const Hit& hh : hits.list)
		if (hh.action != TUTOR_MSG || (hh.r.y + hh.r.h > conv.y && hh.r.y < conv.bottom()))
			visible.push_back(hh);
	hits.list = visible;
}

} // namespace atlas
