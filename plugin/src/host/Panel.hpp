// The docked panel on the right edge of the Rack window and the pointer overlay in the rack.
#pragma once
#include "Host.hpp"

namespace atlas {

struct AtlasField;

struct DockPanel : widget::OpaqueWidget {
	PanelView view;
	Pointer ptr;
	Hits hits;
	AtlasField* field = nullptr;
	DockPanel();
	void step() override;
	void draw(const DrawArgs& args) override;
	void onButton(const ButtonEvent& e) override;
	void onHover(const HoverEvent& e) override;
	void onLeave(const LeaveEvent& e) override;
	void onHoverScroll(const HoverScrollEvent& e) override;
	void act(int action, int arg);
};

struct PointerOverlay : widget::Widget {
	void step() override;
	void draw(const DrawArgs& args) override;
};

constexpr float PANEL_SCALE_MIN = 1.f, PANEL_SCALE_MAX = 2.f;
/** The side panel's scale in effect: the setting, but the open panel takes at most 60 % of the window. */
float panelScale();
/** The side panel's width on screen (open or collapsed), scale included. */
float panelScreenWidth();

/** Center and radius of a note target in rack coordinates; false when it does not resolve. */
bool resolveTarget(const Target& t, math::Vec* center, float* radius);

} // namespace atlas
