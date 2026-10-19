#include "NvgText.hpp"

namespace atlas {
namespace text {

float NvgMeasurer::advance(FontId font, float size, const std::string& s) {
	if (s.empty())
		return 0.f;
	nvgFontFaceId(vg, fonts.handle[font]);
	nvgFontSize(vg, size);
	nvgTextLetterSpacing(vg, 0.f);
	nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_BASELINE);
	return nvgTextBounds(vg, 0.f, 0.f, s.c_str(), s.c_str() + s.size(), nullptr);
}

void NvgMeasurer::metrics(FontId font, float size, float* ascender, float* descender) {
	nvgFontFaceId(vg, fonts.handle[font]);
	nvgFontSize(vg, size);
	float a = 0.f, d = 0.f, lh = 0.f;
	nvgTextMetrics(vg, &a, &d, &lh);
	*ascender = a;
	*descender = d < 0.f ? -d : d;
}

float drawText(NVGcontext* vg, int font, float size, NVGcolor color, float x, float y, const std::string& s, int align) {
	if (s.empty() || font < 0)
		return 0.f;
	nvgFontFaceId(vg, font);
	nvgFontSize(vg, size);
	nvgTextLetterSpacing(vg, 0.f);
	nvgTextAlign(vg, align);
	nvgFillColor(vg, color);
	return nvgText(vg, x, y, s.c_str(), s.c_str() + s.size()) - x;
}

float textWidth(NVGcontext* vg, int font, float size, const std::string& s) {
	if (s.empty() || font < 0)
		return 0.f;
	nvgFontFaceId(vg, font);
	nvgFontSize(vg, size);
	nvgTextLetterSpacing(vg, 0.f);
	nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_BASELINE);
	return nvgTextBounds(vg, 0.f, 0.f, s.c_str(), s.c_str() + s.size(), nullptr);
}

void drawPage(NVGcontext* vg, const Fonts& fonts, const Page& page, float x, float y, const TextColors& colors, float maxHeight) {
	nvgSave(vg);
	for (const Line& l : page.lines) {
		if (maxHeight > 0.f && l.top + l.height > maxHeight + 0.5f)
			break;
		float by = y + l.top + l.baseline;
		if (!l.marker.empty()) {
			float ms = l.runs.empty() ? 12.f : l.runs.front().size;
			drawText(vg, fonts.handle[l.markerFont], l.markerFont == F_MONO ? ms * 0.9f : ms, colors.ink2, x, by, l.marker);
		}
		for (const Run& r : l.runs) {
			NVGcolor c = (r.font == F_HEADING && colors.accentHeadings) ? colors.accent : colors.ink;
			drawText(vg, fonts.handle[r.font], r.size, c, x + r.x, by, r.text);
			if (!r.term.empty()) {
				// Dotted underline: a glossary term with a tooltip.
				nvgBeginPath(vg);
				float uy = by + 2.5f;
				for (float dx = 0.5f; dx < r.width; dx += 2.5f)
					nvgRect(vg, x + r.x + dx, uy, 1.f, 1.f);
				nvgFillColor(vg, colors.term);
				nvgFill(vg);
			}
		}
	}
	nvgRestore(vg);
}

bool termAt(const Page& page, float px, float py, std::string* term, float rect[4]) {
	for (const Line& l : page.lines) {
		if (py < l.top || py > l.top + l.height)
			continue;
		for (const Run& r : l.runs) {
			if (r.term.empty() || px < r.x || px > r.x + r.width)
				continue;
			if (term)
				*term = r.term;
			if (rect) {
				rect[0] = r.x;
				rect[1] = l.top;
				rect[2] = r.width;
				rect[3] = l.height;
			}
			return true;
		}
	}
	return false;
}

} // namespace text
} // namespace atlas
