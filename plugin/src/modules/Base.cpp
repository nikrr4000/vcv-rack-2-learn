#include "Base.hpp"

#include "host/Host.hpp"

namespace atlas {

json_t* AtlasModule::dataToJson() {
	json_t* r = json_object();
	json_object_set_new(r, "lesson", json_string(lesson.c_str()));
	json_object_set_new(r, "block", json_string(block.c_str()));
	json_object_set_new(r, "item", json_string(item.c_str()));
	if (width > 0)
		json_object_set_new(r, "width", json_integer(width));
	json_t* s = json_object();
	saveState(s);
	json_object_set_new(r, "state", s);
	return r;
}

void AtlasModule::dataFromJson(json_t* r) {
	auto str = [&](const char* k) {
		json_t* v = json_object_get(r, k);
		return v && json_is_string(v) ? std::string(json_string_value(v)) : std::string();
	};
	lesson = str("lesson");
	block = str("block");
	item = str("item");
	json_t* w = json_object_get(r, "width");
	if (w && json_is_number(w))
		width = (int) json_number_value(w);
	json_t* s = json_object_get(r, "state");
	if (s && json_is_object(s))
		loadState(s);
}

AtlasWidget::AtlasWidget(AtlasModule* m, int defaultHp, int minHp, int maxHp, bool resizable, const char* kind, const char* modelSlug)
	: am(m), defaultHp(defaultHp), minHp(minHp), maxHp(maxHp), resizable(resizable) {
	setModule(m);
	int hp = defaultHp;
	if (m) {
		const Item* it = m->I();
		if (m->width > 0)
			hp = m->width;
		else if (it && it->width > 0)
			hp = it->width;
	}
	else {
		// Preview: env selection, else the first item of this kind in the pack.
		std::string ref = previewRef(modelSlug);
		if (!ref.empty()) {
			size_t a = ref.find('/'), b = ref.find('/', a + 1);
			if (a != std::string::npos && b != std::string::npos) {
				pLesson = ref.substr(0, a);
				pBlock = ref.substr(a + 1, b - a - 1);
				pItem = ref.substr(b + 1);
			}
		}
		else if (kind && *kind) {
			std::vector<ItemRef> all = itemsOfKind(kind);
			if (!all.empty()) {
				pLesson = all[0].lesson;
				pBlock = all[0].block;
				pItem = all[0].item;
			}
		}
		const Item* it = itemP();
		if (it && it->width > 0)
			hp = it->width;
	}
	hp = std::max(minHp, std::min(maxHp, hp));
	box.size = Vec(RACK_GRID_WIDTH * hp, RACK_GRID_HEIGHT);
	if (resizable && m) {
		handle = new ResizeHandle;
		handle->box.pos = Vec(box.size.x - handle->box.size.x, 0);
		addChild(handle);
	}
}

AtlasWidget::~AtlasWidget() {
	setTooltip("", "");
}

void AtlasWidget::step() {
	if (am) {
		ensureHost();
		if (am->width > 0) {
			float w = am->width * RACK_GRID_WIDTH;
			if (w != box.size.x)
				box.size.x = w;
		}
	}
	if (handle)
		handle->box.pos.x = box.size.x - handle->box.size.x;
	ModuleWidget::step();
}

void AtlasWidget::draw(const DrawArgs& args) {
	Style st = currentStyle(part());
	if (!st.fonts.ok()) {
		ModuleWidget::draw(args);
		return;
	}
	hits.clear();
	nvgSave(args.vg);
	drawView(args.vg, st, box.size.x, box.size.y);
	nvgRestore(args.vg);
	ModuleWidget::draw(args);  // children (resize handle)
}

void AtlasWidget::drawMissing(NVGcontext* vg, const Style& st, float w, float h, const std::string& what) {
	Rect c = drawPanel(vg, st, w, h, "Atlas");
	text::NvgMeasurer m(vg, st.fonts);
	text::TextStyle ts = st.textStyle();
	std::vector<text::Page> pages = text::pages(what, c.w, c.h, m, ts);
	if (!pages.empty())
		text::drawPage(vg, st.fonts, pages[0], c.x, c.y, st.textColors());
}

void AtlasWidget::onHover(const HoverEvent& e) {
	ptr.x = e.pos.x;
	ptr.y = e.pos.y;
	std::string term = hoverTerm();
	if (term != tooltipKey) {
		std::string def = pack().definition(term);
		setTooltip(term, term.empty() ? "" : term + " — " + (def.empty() ? "термин курса" : def));
	}
	ModuleWidget::onHover(e);
}

void AtlasWidget::onLeave(const LeaveEvent& e) {
	ptr.x = ptr.y = -1e6f;
	setTooltip("", "");
	ModuleWidget::onLeave(e);
}

void AtlasWidget::setTooltip(const std::string& key, const std::string& text) {
	tooltipKey = key;
	if (tooltip) {
		APP->scene->removeChild(tooltip);
		delete tooltip;
		tooltip = nullptr;
	}
	if (!key.empty() && APP && APP->scene) {
		tooltip = new ui::Tooltip;
		tooltip->text = text;
		APP->scene->addChild(tooltip);
	}
}

void AtlasWidget::onButton(const ButtonEvent& e) {
	// Text fields (children) get their own clicks.
	for (widget::Widget* c : children)
		if (c != handle && c->visible && c->box.contains(e.pos)) {
			ModuleWidget::onButton(e);
			return;
		}
	if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_LEFT && (e.mods & RACK_MOD_MASK) == 0) {
		const Hit* h = hits.at(e.pos.x, e.pos.y);
		if (h) {
			int action = h->action, arg = h->arg;
			pressedHit = true;
			onHit(action, arg);
			e.consume(this);
			return;
		}
	}
	ModuleWidget::onButton(e);
}

void AtlasWidget::onHoverScroll(const HoverScrollEvent& e) {
	if (onWheel(e.scrollDelta.y)) {
		e.consume(this);
		return;
	}
	ModuleWidget::onHoverScroll(e);
}

void AtlasWidget::onDragStart(const DragStartEvent& e) {
	if (pressedHit)
		return;
	ModuleWidget::onDragStart(e);
}

void AtlasWidget::onDragMove(const DragMoveEvent& e) {
	if (pressedHit)
		return;
	ModuleWidget::onDragMove(e);
}

void AtlasWidget::onDragEnd(const DragEndEvent& e) {
	if (pressedHit) {
		pressedHit = false;
		return;
	}
	ModuleWidget::onDragEnd(e);
}

void ResizeHandle::onDragStart(const DragStartEvent& e) {
	if (e.button != GLFW_MOUSE_BUTTON_LEFT)
		return;
	dragPos = APP->scene->rack->getMousePos();
	ModuleWidget* mw = getAncestorOfType<ModuleWidget>();
	if (mw)
		originalBox = mw->box;
}

void ResizeHandle::onDragMove(const DragMoveEvent& e) {
	AtlasWidget* mw = getAncestorOfType<AtlasWidget>();
	if (!mw || !mw->am)
		return;
	float dx = APP->scene->rack->getMousePos().x - dragPos.x;
	math::Rect next = originalBox;
	math::Rect prev = mw->box;
	float hp = std::round((originalBox.size.x + dx) / RACK_GRID_WIDTH);
	hp = math::clamp(hp, (float) mw->minHp, (float) mw->maxHp);
	next.size.x = hp * RACK_GRID_WIDTH;
	mw->box = next;
	if (!APP->scene->rack->requestModulePos(mw, next.pos))
		mw->box = prev;
	mw->am->width = (int) std::round(mw->box.size.x / RACK_GRID_WIDTH);
}

void ResizeHandle::draw(const DrawArgs& args) {
	// Two short grip lines at the right edge, mid-height.
	for (float x = 1.5f; x <= 4.5f; x += 3.f) {
		nvgBeginPath(args.vg);
		nvgRect(args.vg, x, box.size.y / 2 - 10, 1, 20);
		nvgFillColor(args.vg, nvgRGBA(128, 120, 110, 110));
		nvgFill(args.vg);
	}
}

} // namespace atlas
