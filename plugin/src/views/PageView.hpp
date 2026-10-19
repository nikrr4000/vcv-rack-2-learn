// The Page module's face: a paper page with the block text, paginated with a footer pager.
#pragma once
#include "PagedText.hpp"
#include "core/Pack.hpp"

namespace atlas {

struct PageView {
	std::string header;
	PagedText text;
	void draw(NVGcontext* vg, const Style& st, float w, float h, const Pointer& p, Hits& hits);
	/** Handles pager actions; true when handled. */
	bool action(int action, int arg);
	std::string termAt(float x, float y) const;

  private:
	Rect content;
};

} // namespace atlas
