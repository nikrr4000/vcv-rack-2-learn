// nanovg adapter for the text engine: measuring and drawing laid-out pages.
// Depends only on nanovg, so the render harness uses it unchanged.
#pragma once
#include <nanovg.h>

#include "Layout.hpp"

namespace atlas {
namespace text {

struct Fonts {
	int handle[F_COUNT] = {-1, -1, -1, -1, -1};
	bool ok() const {
		for (int h : handle)
			if (h < 0)
				return false;
		return true;
	}
};

struct NvgMeasurer : Measurer {
	NVGcontext* vg;
	Fonts fonts;
	NvgMeasurer(NVGcontext* vg, const Fonts& fonts) : vg(vg), fonts(fonts) {}
	float advance(FontId font, float size, const std::string& s) override;
	void metrics(FontId font, float size, float* ascender, float* descender) override;
};

struct TextColors {
	NVGcolor ink;
	NVGcolor ink2;    // list markers
	NVGcolor term;    // dotted underline of glossary terms
	NVGcolor accent;  // headings (optional: same as ink)
	bool accentHeadings = false;
};

/** Draws a page with its top-left corner at (x, y). Lines below `maxHeight` (if > 0) are skipped. */
void drawPage(NVGcontext* vg, const Fonts& fonts, const Page& page, float x, float y, const TextColors& colors, float maxHeight = 0.f);

/** Draws one line of plain text in a font; returns its advance. */
float drawText(NVGcontext* vg, int font, float size, NVGcolor color, float x, float y, const std::string& s, int align = NVG_ALIGN_LEFT | NVG_ALIGN_BASELINE);

/** Advance of one line of plain text. */
float textWidth(NVGcontext* vg, int font, float size, const std::string& s);

/** A glossary term under (px, py) on a page drawn at the origin; `rect` gets its box. */
bool termAt(const Page& page, float px, float py, std::string* term, float rect[4]);

} // namespace text
} // namespace atlas
