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

/** Center and radius of a note target in rack coordinates; false when it does not resolve. */
bool resolveTarget(const Target& t, math::Vec* center, float* radius);

} // namespace atlas
