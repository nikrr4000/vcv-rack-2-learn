#include "Draw.hpp"

#include <cmath>

#include "text/Layout.hpp"

namespace atlas {

using namespace tokens;

void drawGlyph(NVGcontext* vg, Glyph g, float cx, float cy, float s, NVGcolor c, float sw) {
	float h = s / 2.f;
	nvgSave(vg);
	nvgStrokeColor(vg, c);
	nvgFillColor(vg, c);
	nvgStrokeWidth(vg, sw);
	nvgLineCap(vg, NVG_ROUND);
	nvgLineJoin(vg, NVG_ROUND);
	nvgBeginPath(vg);
	switch (g) {
		case G_ARROW_LEFT:
			nvgMoveTo(vg, cx + h, cy);
			nvgLineTo(vg, cx - h, cy);
			nvgMoveTo(vg, cx - h + s * 0.35f, cy - s * 0.35f);
			nvgLineTo(vg, cx - h, cy);
			nvgLineTo(vg, cx - h + s * 0.35f, cy + s * 0.35f);
			nvgStroke(vg);
			break;
		case G_ARROW_RIGHT:
			nvgMoveTo(vg, cx - h, cy);
			nvgLineTo(vg, cx + h, cy);
			nvgMoveTo(vg, cx + h - s * 0.35f, cy - s * 0.35f);
			nvgLineTo(vg, cx + h, cy);
			nvgLineTo(vg, cx + h - s * 0.35f, cy + s * 0.35f);
			nvgStroke(vg);
			break;
		case G_ARROW_DOWN:
			nvgMoveTo(vg, cx, cy - h);
			nvgLineTo(vg, cx, cy + h);
			nvgMoveTo(vg, cx - s * 0.35f, cy + h - s * 0.35f);
			nvgLineTo(vg, cx, cy + h);
			nvgLineTo(vg, cx + s * 0.35f, cy + h - s * 0.35f);
			nvgStroke(vg);
			break;
		case G_CHECK:
			nvgMoveTo(vg, cx - h, cy);
			nvgLineTo(vg, cx - h * 0.25f, cy + h * 0.7f);
			nvgLineTo(vg, cx + h, cy - h * 0.7f);
			nvgStroke(vg);
			break;
		case G_CROSS:
			nvgMoveTo(vg, cx - h * 0.75f, cy - h * 0.75f);
			nvgLineTo(vg, cx + h * 0.75f, cy + h * 0.75f);
			nvgMoveTo(vg, cx + h * 0.75f, cy - h * 0.75f);
			nvgLineTo(vg, cx - h * 0.75f, cy + h * 0.75f);
			nvgStroke(vg);
			break;
		case G_APPROX:
			for (int k = -1; k <= 1; k += 2) {
				float y = cy + k * s * 0.18f;
				nvgMoveTo(vg, cx - h, y + s * 0.06f);
				nvgBezierTo(vg, cx - h * 0.4f, y - s * 0.2f, cx + h * 0.4f, y + s * 0.2f, cx + h, y - s * 0.06f);
			}
			nvgStroke(vg);
			break;
		case G_QUESTION:
			nvgArc(vg, cx, cy - s * 0.18f, s * 0.26f, NVG_PI * 1.05f, NVG_PI * 2.4f, NVG_CW);
			nvgLineTo(vg, cx, cy + s * 0.15f);
			nvgStroke(vg);
			nvgBeginPath(vg);
			nvgCircle(vg, cx, cy + s * 0.42f, sw * 0.7f);
			nvgFill(vg);
			break;
		case G_DOT:
			nvgCircle(vg, cx, cy, s * 0.22f);
			nvgFill(vg);
			break;
		case G_RING:
			nvgCircle(vg, cx, cy, s * 0.4f);
			nvgStroke(vg);
			break;
		case G_CHEVRON_DOWN:
			nvgMoveTo(vg, cx - h * 0.7f, cy - h * 0.3f);
			nvgLineTo(vg, cx, cy + h * 0.4f);
			nvgLineTo(vg, cx + h * 0.7f, cy - h * 0.3f);
			nvgStroke(vg);
			break;
		case G_CHEVRON_UP:
			nvgMoveTo(vg, cx - h * 0.7f, cy + h * 0.3f);
			nvgLineTo(vg, cx, cy - h * 0.4f);
			nvgLineTo(vg, cx + h * 0.7f, cy + h * 0.3f);
			nvgStroke(vg);
			break;
		case G_CHEVRON_LEFT:
			nvgMoveTo(vg, cx + h * 0.3f, cy - h * 0.7f);
			nvgLineTo(vg, cx - h * 0.4f, cy);
			nvgLineTo(vg, cx + h * 0.3f, cy + h * 0.7f);
			nvgStroke(vg);
			break;
		case G_CHEVRON_RIGHT:
			nvgMoveTo(vg, cx - h * 0.3f, cy - h * 0.7f);
			nvgLineTo(vg, cx + h * 0.4f, cy);
			nvgLineTo(vg, cx - h * 0.3f, cy + h * 0.7f);
			nvgStroke(vg);
			break;
		case G_PLAY:
			nvgMoveTo(vg, cx - h * 0.6f, cy - h * 0.8f);
			nvgLineTo(vg, cx + h * 0.8f, cy);
			nvgLineTo(vg, cx - h * 0.6f, cy + h * 0.8f);
			nvgClosePath(vg);
			nvgFill(vg);
			break;
	}
	nvgRestore(vg);
}

void drawHairline(NVGcontext* vg, const Style& st, float x0, float y, float x1) {
	nvgBeginPath(vg);
	nvgRect(vg, x0, y, x1 - x0, 1.f);
	nvgFillColor(vg, st.pal.hairline);
	nvgFill(vg);
}

Rect drawPanel(NVGcontext* vg, const Style& st, float w, float h, const std::string& header, const std::string& right) {
	nvgBeginPath(vg);
	nvgRect(vg, 0, 0, w, h);
	nvgFillColor(vg, st.pal.panel);
	nvgFill(vg);
	// Edges: a hairline frame so neighbouring paper panels stay apart.
	nvgBeginPath(vg);
	nvgRect(vg, 0.5f, 0.5f, w - 1.f, h - 1.f);
	nvgStrokeColor(vg, st.pal.hairline);
	nvgStrokeWidth(vg, 1.f);
	nvgStroke(vg);
	nvgBeginPath(vg);
	nvgRect(vg, 0, 0, w, panel_accentRule);
	nvgFillColor(vg, st.accent);
	nvgFill(vg);
	float y = panel_accentRule + panel_padTop - 2.f;
	if (!header.empty() || !right.empty()) {
		float x0 = panel_padLeft, x1 = w - panel_padRight;
		float rw = right.empty() ? 0.f : text::textWidth(vg, st.fonts.handle[text::F_MONO], type_small, right);
		std::string left = ellipsize(vg, st.fonts.handle[text::F_MONO], type_small, header, x1 - x0 - rw - 8.f);
		text::drawText(vg, st.fonts.handle[text::F_MONO], type_small, st.pal.ink2, x0, y + 8.f, left);
		if (!right.empty())
			text::drawText(vg, st.fonts.handle[text::F_MONO], type_small, st.pal.ink2, x1, y + 8.f, right, NVG_ALIGN_RIGHT | NVG_ALIGN_BASELINE);
		y += panel_headerHeight;
	}
	return Rect(panel_padLeft, y, w - panel_padLeft - panel_padRight, h - y - panel_padBottom);
}

float buttonWidth(NVGcontext* vg, const Style& st, const std::string& label, bool glyph) {
	return text::textWidth(vg, st.fonts.handle[text::F_BOLD], type_small, label) + 16.f + (glyph ? 14.f : 0.f);
}

void drawButton(NVGcontext* vg, const Style& st, const Rect& r, const std::string& label, const ButtonState& s, int leftGlyph, int rightGlyph) {
	NVGcolor border = s.hover && !s.disabled ? st.pal.ink2 : st.pal.hairline;
	NVGcolor ink = s.disabled ? st.pal.hairline : st.pal.ink;
	if (s.kind == BUTTON_PRIMARY && !s.disabled) {
		border = st.accent;
		ink = st.accent;
	}
	if (s.active && !s.disabled) {
		NVGcolor fill = st.accent;
		NVGcolor p = st.pal.panel;
		float a = controls_activeFill;
		nvgBeginPath(vg);
		nvgRect(vg, r.x, r.y, r.w, r.h);
		nvgFillColor(vg, nvgRGBf(p.r + (fill.r - p.r) * a, p.g + (fill.g - p.g) * a, p.b + (fill.b - p.b) * a));
		nvgFill(vg);
		border = st.accent;
		ink = st.accent;
	}
	if (s.kind != BUTTON_QUIET || s.hover || s.active) {
		nvgBeginPath(vg);
		nvgRect(vg, r.x + 0.5f, r.y + 0.5f, r.w - 1.f, r.h - 1.f);
		nvgStrokeColor(vg, border);
		nvgStrokeWidth(vg, controls_border);
		nvgStroke(vg);
	}
	int font = st.fonts.handle[s.kind == BUTTON_QUIET ? text::F_BODY : text::F_BOLD];
	float tw = text::textWidth(vg, font, type_small, label);
	float gw = 9.f, gap = label.empty() ? 0.f : 5.f;
	float total = tw + (leftGlyph >= 0 ? gw + gap : 0.f) + (rightGlyph >= 0 ? gw + gap : 0.f);
	float x = r.x + (r.w - total) / 2.f;
	float cy = r.y + r.h / 2.f;
	if (leftGlyph >= 0) {
		drawGlyph(vg, (Glyph) leftGlyph, x + gw / 2.f, cy, gw, ink, 1.3f);
		x += gw + gap;
	}
	text::drawText(vg, font, type_small, ink, x, cy + type_small * 0.36f, label);
	x += tw + gap;
	if (rightGlyph >= 0)
		drawGlyph(vg, (Glyph) rightGlyph, x + gw / 2.f, cy, gw, ink, 1.3f);
}

std::vector<Rect> drawDots(NVGcontext* vg, const Style& st, float cx, float cy, int n, int current) {
	std::vector<Rect> out;
	if (n <= 1)
		return out;
	float gap = 9.f;
	float x0 = cx - gap * (n - 1) / 2.f;
	for (int i = 0; i < n; i++) {
		float x = x0 + gap * i;
		nvgBeginPath(vg);
		nvgCircle(vg, x, cy, i == current ? 2.6f : 2.f);
		nvgFillColor(vg, i == current ? st.accent : st.pal.hairline);
		nvgFill(vg);
		out.push_back(Rect(x - 4.5f, cy - 6.f, 9.f, 12.f));
	}
	return out;
}

NVGcolor okColor(const Style& st) { return st.dark ? nvgRGB(0x7F, 0xB0, 0x82) : nvgRGB(0x3E, 0x7A, 0x45); }
NVGcolor failColor(const Style& st) { return st.dark ? nvgRGB(0xE0, 0x7A, 0x5F) : nvgRGB(0xB0, 0x45, 0x2E); }
NVGcolor busyColor(const Style& st) { return st.dark ? nvgRGB(0xE0, 0xB0, 0x5A) : nvgRGB(0xB0, 0x7A, 0x1E); }

void drawLight(NVGcontext* vg, const Style& st, float cx, float cy, float r, int state) {
	nvgBeginPath(vg);
	nvgCircle(vg, cx, cy, r);
	if (state == 0) {
		nvgStrokeColor(vg, st.pal.ink2);
		nvgStrokeWidth(vg, 1.f);
		nvgStroke(vg);
		return;
	}
	nvgFillColor(vg, state == 1 ? okColor(st) : state == 2 ? failColor(st) : busyColor(st));
	nvgFill(vg);
}

std::string ellipsize(NVGcontext* vg, int font, float size, const std::string& s, float maxW) {
	if (text::textWidth(vg, font, size, s) <= maxW)
		return s;
	std::string out;
	for (size_t i = 0; i < s.size(); i = text::utf8Next(s, i)) {
		std::string next = s.substr(0, text::utf8Next(s, i));
		if (text::textWidth(vg, font, size, next + "…") > maxW)
			break;
		out = next;
	}
	return out + "…";
}

} // namespace atlas
