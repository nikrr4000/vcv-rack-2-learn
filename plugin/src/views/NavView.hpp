// The Navigator module's face (course home, lesson controls, course map) and the docked
// panel on the right edge of the Rack window (outline, block counter, tutor). Rack-free.
#pragma once
#include "TutorView.hpp"
#include "core/Progress.hpp"

namespace atlas {

enum NavAction {
	NAV_CONTINUE = 1, NAV_PREV, NAV_NEXT, NAV_DONE, NAV_MAP, NAV_MAP_CLOSE, NAV_OPEN, NAV_PART_PREV, NAV_PART_NEXT,
	NAV_PANEL, NAV_GOTO_BLOCK, NAV_COLLAPSE, NAV_EXPAND, NAV_HOME
};

enum NavMode { NAV_MODE_HOME, NAV_MODE_LESSON, NAV_MODE_MAP };

/** What the course controls show (filled by the host each frame). */
struct NavState {
	const Pack* pack = nullptr;
	const Progress* progress = nullptr;
	std::string lesson;  // current lesson ("" = none open)
	int block = 0;       // 0-based
	bool connected = false;
	bool panelOpen = true;
	bool panelExists = false;
	std::string notice;  // one-line message (e.g. where the previous patch went)
};

struct NavigatorView {
	NavMode mode = NAV_MODE_HOME;
	int mapPart = -1;  // -1 = the part of the current/continue lesson
	FlowCache cache;
	/** Lesson ids of the visible map rows, for NAV_OPEN arg. */
	std::vector<std::string> mapRows;
	void draw(NVGcontext* vg, const Style& st, float w, float h, const NavState& s, const Pointer& p, Hits& hits);
};

struct PanelView {
	bool map = false;
	Scroll outlineScroll, mapScroll;
	FlowCache cache;
	TutorView tutor;
	std::vector<std::string> mapRows;
	static constexpr float WIDTH = 360.f;
	static constexpr float STRIP = 28.f;
	/** Draws the panel in a w × h box (w = WIDTH or STRIP when collapsed). */
	void draw(NVGcontext* vg, const Style& st, float w, float h, const NavState& s, bool collapsed, const Pointer& p, Hits& hits);
};

/** Glyph + color for a lesson status. */
void drawStatus(NVGcontext* vg, const Style& st, float cx, float cy, const std::string& status, bool current);

} // namespace atlas
