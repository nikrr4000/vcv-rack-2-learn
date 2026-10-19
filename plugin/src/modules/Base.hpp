// Common ground of all Atlas modules: references into the course pack (never content),
// per-learner state, a resizable width, the paper panel, hit regions and term tooltips.
#pragma once
#include "atlas.hpp"
#include "ui/Draw.hpp"

namespace atlas {

struct AtlasModule : Module {
	std::string lesson, block, item;
	int width = 0;  // HP; 0 = the widget's default

	json_t* dataToJson() override;
	void dataFromJson(json_t* rootJ) override;
	/** Per-learner state of the concrete module. */
	virtual void saveState(json_t*) {}
	virtual void loadState(json_t*) {}

	const Lesson* L() const { return pack().lesson(lesson); }
	const Block* B() const {
		const Lesson* l = L();
		return l ? l->block(block) : nullptr;
	}
	const Item* I() const { return pack().item(lesson, block, item); }
};

struct ResizeHandle;

struct AtlasWidget : ModuleWidget {
	AtlasModule* am = nullptr;
	int defaultHp = 10, minHp = 4, maxHp = 40;
	bool resizable = false;
	Pointer ptr;
	Hits hits;
	ResizeHandle* handle = nullptr;
	ui::Tooltip* tooltip = nullptr;
	std::string tooltipKey;
	/** Preview reference when module == nullptr (browser, screenshots). */
	std::string pLesson, pBlock, pItem;

	AtlasWidget(AtlasModule* m, int defaultHp, int minHp, int maxHp, bool resizable, const char* kind, const char* modelSlug);
	~AtlasWidget();

	std::string lessonId() const { return am ? am->lesson : pLesson; }
	std::string blockId() const { return am ? am->block : pBlock; }
	std::string itemId() const { return am ? am->item : pItem; }
	const Lesson* lesson() const { return pack().lesson(lessonId()); }
	const Block* blockP() const {
		const Lesson* l = lesson();
		return l ? l->block(blockId()) : nullptr;
	}
	const Item* itemP() const { return pack().item(lessonId(), blockId(), itemId()); }
	int part() const {
		const Lesson* l = lesson();
		return l ? l->part : 0;
	}
	std::string header() const { return blockHeader(lesson(), blockP()); }

	void step() override;
	void draw(const DrawArgs& args) override;
	void onHover(const HoverEvent& e) override;
	void onLeave(const LeaveEvent& e) override;
	void onButton(const ButtonEvent& e) override;
	void onHoverScroll(const HoverScrollEvent& e) override;

	/** Draws the module in a w × h box with hits registered in `hits`. */
	virtual void drawView(NVGcontext* vg, const Style& st, float w, float h) = 0;
	/** A click on a hit region. */
	virtual void onHit(int action, int arg) {}
	/** Wheel over the module; return true when used. */
	virtual bool onWheel(float dy) { return false; }
	/** Glossary term under the pointer (for the tooltip), or "". */
	virtual std::string hoverTerm() { return ""; }
	/** Text when the module's reference does not resolve. */
	void drawMissing(NVGcontext* vg, const Style& st, float w, float h, const std::string& what);
	void setTooltip(const std::string& key, const std::string& text);
};

/** Drag the right edge to resize (snaps to HP, respects neighbours). */
struct ResizeHandle : OpaqueWidget {
	Vec dragPos;
	math::Rect originalBox;
	ResizeHandle() { box.size = Vec(6.f, RACK_GRID_HEIGHT); }
	void onDragStart(const DragStartEvent& e) override;
	void onDragMove(const DragMoveEvent& e) override;
	void draw(const DrawArgs& args) override;
};

/** Ensures the scene host exists (called by every Atlas widget with a module). */
void ensureHost();

} // namespace atlas
