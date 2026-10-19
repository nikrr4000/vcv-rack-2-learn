// A markdown text area that paginates for its box (Page, Note, Task, Quiz explanations).
// Layout is synchronous in draw() and cached by text, box and font scale.
#pragma once
#include <string>
#include <vector>

#include "ui/Draw.hpp"

namespace atlas {

struct PagedText {
	std::string md;
	bool title = false;  // headings at title size
	int page = 0;
	std::string reportId;  // where to report pages/overflow ("" = no report)

	std::vector<text::Page> pages;
	bool splitWords = false;

	/** Lays the text out for a w × h box (no-op when nothing changed). */
	void layout(NVGcontext* vg, const Style& st, float w, float h);
	int count() const { return (int) pages.size(); }
	/** Draws the current page; returns the used height. */
	float draw(NVGcontext* vg, const Style& st, float x, float y);
	/** Height of the whole text laid out in one column of width w (no pagination). */
	static float measureHeight(NVGcontext* vg, const Style& st, const std::string& md, float w, bool title = false);
	/** Glossary term under a point relative to the text origin. */
	std::string termAt(float px, float py) const;

  private:
	std::string key;
	float lw = -1.f, lh = -1.f, lscale = -1.f;
	bool ltitle = false;
};

/** Current font scale of the nanovg transform, quantized like nanovg does (0.01). */
float fontScale(NVGcontext* vg);

enum PagerAction { PAGER_PREV = 1000, PAGER_NEXT = 1001, PAGER_GOTO = 1002 };

/** Footer pager: ← dots →. Adds hits PAGER_*; draws nothing when n <= 1. */
void drawPager(NVGcontext* vg, const Style& st, const Rect& r, int n, int current, const Pointer& p, Hits& hits);

} // namespace atlas
