#include "Input.hpp"

#include <algorithm>

namespace atlas {

static void setFont(NVGcontext* vg, const Style& st, float size) {
	nvgFontFaceId(vg, st.fonts.handle[text::F_BODY]);
	nvgFontSize(vg, size);
	nvgTextLetterSpacing(vg, 0.f);
	nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
}

InputLayout layoutInput(NVGcontext* vg, const Style& st, const std::string& text, float width) {
	InputLayout L;
	L.size = 11.5f;
	L.lineH = std::round(L.size * 1.38f);
	setFont(vg, st, L.size);
	const char* s = text.c_str();
	const char* end = s + text.size();
	NVGtextRow rows[8];
	float y = 0.f;
	const char* p = s;
	while (p < end) {
		int n = nvgTextBreakLines(vg, p, end, std::max(10.f, width), rows, 8);
		if (n <= 0)
			break;
		for (int i = 0; i < n; i++) {
			InputRow r;
			r.start = rows[i].start - s;
			r.end = rows[i].end - s;
			r.y = y;
			L.rows.push_back(r);
			y += L.lineH;
		}
		p = rows[n - 1].next;
	}
	if (L.rows.empty() || (!text.empty() && text.back() == '\n')) {
		InputRow r;
		r.start = r.end = text.size();
		r.y = y;
		L.rows.push_back(r);
	}
	return L;
}

float inputHeight(NVGcontext* vg, const Style& st, const std::string& text, float width) {
	InputLayout L = layoutInput(vg, st, text, width - 2 * INPUT_PAD);
	int n = std::min((int) L.rows.size(), INPUT_MAX_ROWS);
	return n * L.lineH + 2 * INPUT_PAD;
}

static float xAt(NVGcontext* vg, const std::string& text, const InputRow& row, size_t index) {
	size_t i = std::min(std::max(index, row.start), row.end);
	return nvgTextBounds(vg, 0, 0, text.c_str() + row.start, text.c_str() + i, nullptr);
}

static int rowOf(const InputLayout& L, size_t index) {
	for (size_t i = 0; i < L.rows.size(); i++) {
		bool last = i + 1 == L.rows.size();
		if (index >= L.rows[i].start && (index < L.rows[i + (last ? 0 : 1)].start || last))
			return (int) i;
	}
	return (int) L.rows.size() - 1;
}

void drawInput(NVGcontext* vg, const Style& st, const Rect& r, const std::string& text, const std::string& placeholder, int cursor, int selection, bool focused, bool blink, bool disabled) {
	nvgBeginPath(vg);
	nvgRect(vg, r.x + 0.5f, r.y + 0.5f, r.w - 1.f, r.h - 1.f);
	nvgFillColor(vg, st.dark ? nvgRGB(0x15, 0x16, 0x18) : nvgRGB(0xFB, 0xF8, 0xF2));
	nvgFill(vg);
	nvgStrokeColor(vg, focused ? st.accent : st.pal.hairline);
	nvgStrokeWidth(vg, 1.f);
	nvgStroke(vg);
	float tw = r.w - 2 * INPUT_PAD;
	InputLayout L = layoutInput(vg, st, text, tw);
	nvgSave(vg);
	nvgScissor(vg, r.x + 1, r.y + 1, r.w - 2, r.h - 2);
	// Keep the caret row visible when there are more than five rows.
	int crow = rowOf(L, (size_t) std::max(0, cursor));
	float visible = r.h - 2 * INPUT_PAD;
	float off = std::max(0.f, (crow + 1) * L.lineH - visible);
	float ox = r.x + INPUT_PAD, oy = r.y + INPUT_PAD - off;
	setFont(vg, st, L.size);
	if (text.empty()) {
		nvgFillColor(vg, disabled ? st.pal.hairline : st.pal.ink2);
		nvgText(vg, ox, oy + (L.lineH - L.size) / 2, placeholder.c_str(), nullptr);
	}
	else {
		int a = std::min(cursor, selection), b = std::max(cursor, selection);
		for (const InputRow& row : L.rows) {
			if (a != b) {
				size_t s0 = std::max((size_t) a, row.start), s1 = std::min((size_t) b, row.end);
				if (s0 < s1) {
					float x0 = xAt(vg, text, row, s0), x1 = xAt(vg, text, row, s1);
					nvgBeginPath(vg);
					nvgRect(vg, ox + x0, oy + row.y, x1 - x0, L.lineH);
					nvgFillColor(vg, nvgTransRGBAf(st.accent, 0.25f));
					nvgFill(vg);
				}
			}
			nvgFillColor(vg, st.pal.ink);
			nvgText(vg, ox, oy + row.y + (L.lineH - L.size) / 2, text.c_str() + row.start, text.c_str() + row.end);
		}
	}
	if (focused && blink) {
		const InputRow& row = L.rows[crow];
		float cx = ox + xAt(vg, text, row, (size_t) cursor);
		nvgBeginPath(vg);
		nvgRect(vg, cx, oy + row.y + 2.f, 1.f, L.lineH - 4.f);
		nvgFillColor(vg, st.accent);
		nvgFill(vg);
	}
	nvgRestore(vg);
}

int inputIndexAt(NVGcontext* vg, const Style& st, const Rect& r, const std::string& text, float x, float y) {
	float tw = r.w - 2 * INPUT_PAD;
	InputLayout L = layoutInput(vg, st, text, tw);
	setFont(vg, st, L.size);
	float ly = y - r.y - INPUT_PAD;
	int row = std::max(0, std::min((int) L.rows.size() - 1, (int) (ly / L.lineH)));
	const InputRow& R = L.rows[row];
	float lx = x - r.x - INPUT_PAD;
	NVGglyphPosition pos[512];
	int n = nvgTextGlyphPositions(vg, 0, 0, text.c_str() + R.start, text.c_str() + R.end, pos, 512);
	for (int i = 0; i < n; i++) {
		float mid = (pos[i].minx + pos[i].maxx) / 2.f;
		if (lx < mid)
			return (int) (pos[i].str - text.c_str());
	}
	return (int) R.end;
}

} // namespace atlas
