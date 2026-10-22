#include "Panel.hpp"

#include <cmath>

#include "modules/Modules.hpp"
#include "views/SmallViews.hpp"

namespace atlas {

float panelScale() {
	float s = math::clamp(settings().panelScale, PANEL_SCALE_MIN, PANEL_SCALE_MAX);
	float win = APP && APP->scene ? APP->scene->box.size.x : 0.f;
	if (win > 0.f)
		s = std::max(PANEL_SCALE_MIN, std::min(s, win * 0.6f / PanelView::WIDTH));
	return s;
}

float panelScreenWidth() {
	return (settings().panelOpen ? PanelView::WIDTH : PanelView::STRIP) * panelScale();
}

// The panel is laid out at scale 1 (PanelView::WIDTH wide) and drawn scaled: events come in screen
// units relative to the panel and are divided by the scale before they meet the layout's hits.

DockPanel::DockPanel() {
	field = new AtlasField;
	field->onSubmit = [](const std::string& text) {
		if (host() && !text.empty())
			host()->ask("ask", text);
	};
	field->indexAt = [this](math::Vec pos) {
		Style st = currentStyle(0);
		const Rect& r = view.tutor.inputRect;
		float k = panelScale();
		return inputIndexAt(APP->window->vg, st, r, field->text, pos.x / k + r.x, pos.y / k + r.y);
	};
	addChild(field);
}

void DockPanel::step() {
	AtlasHost* h = host();
	visible = h && h->navigatorId >= 0;
	if (visible && parent) {
		float menuH = APP->scene->menuBar ? APP->scene->menuBar->box.size.y : 0.f;
		float w = panelScreenWidth(), k = panelScale();
		box.pos = math::Vec(parent->box.size.x - w, menuH);
		box.size = math::Vec(w, parent->box.size.y - menuH);
		const Rect& r = view.tutor.inputRect;
		field->visible = settings().panelOpen && !view.map && r.w > 0;
		field->box.pos = math::Vec(r.x * k, r.y * k);
		field->box.size = math::Vec(r.w * k, r.h * k);
	}
	OpaqueWidget::step();
}

void DockPanel::draw(const DrawArgs& args) {
	AtlasHost* h = host();
	if (!h)
		return;
	const Lesson* L = h->currentLesson();
	Style st = currentStyle(L ? L->part : 0);
	if (!st.fonts.ok())
		return;
	NavState s = h->navState();
	view.map = h->mapOpen;
	const Block* b = h->currentBlock();
	view.tutor.messages = &h->conversation(h->lesson);
	view.tutor.faq = b ? &b->faq : nullptr;
	view.tutor.busy = h->tutorBusy;
	view.tutor.time = (float) system::getTime();
	view.tutor.input = field->text;
	view.tutor.cursor = field->cursor;
	view.tutor.selection = field->selection;
	view.tutor.focused = field->focused();
	view.tutor.blink = std::fmod(system::getTime(), 1.0) < 0.6;
	hits.clear();
	float k = panelScale();
	nvgSave(args.vg);
	nvgScale(args.vg, k, k);
	view.draw(args.vg, st, box.size.x / k, box.size.y / k, s, !settings().panelOpen, ptr, hits);
	nvgRestore(args.vg);
	Widget::draw(args);
}

void DockPanel::act(int a, int arg) {
	AtlasHost* h = host();
	if (!h)
		return;
	switch (a) {
		case NAV_COLLAPSE:
		case NAV_EXPAND: h->togglePanel(); break;
		case NAV_ZOOM_OUT: h->zoomPanel(-1); break;
		case NAV_ZOOM_IN: h->zoomPanel(1); break;
		case NAV_MAP: h->mapOpen = true; break;
		case NAV_MAP_CLOSE: h->mapOpen = false; break;
		case NAV_OPEN:
			if (arg >= 0 && arg < (int) view.mapRows.size())
				h->openLesson(view.mapRows[arg]);
			break;
		case NAV_GOTO_BLOCK: h->gotoBlock(arg); break;
		case NAV_PREV: h->prev(); break;
		case NAV_NEXT: h->next(); break;
		case NAV_DONE: h->completeLesson(); break;
		case TUTOR_FAQ: h->faq(arg); break;
		case TUTOR_QUICK: h->quick(arg); break;
		case TUTOR_SEND:
			if (!field->text.empty()) {
				h->ask("ask", field->text);
				field->setText("");
			}
			break;
		case TUTOR_SCROLL_DOWN:
			view.tutor.scroll.stickBottom = true;
			view.tutor.newBelow = false;
			break;
	}
}

void DockPanel::onButton(const ButtonEvent& e) {
	if (e.action == GLFW_PRESS) {
		float k = panelScale();
		const Hit* hit = hits.at(e.pos.x / k, e.pos.y / k);
		if (hit && e.button == GLFW_MOUSE_BUTTON_RIGHT && hit->action == TUTOR_MSG && host()) {
			std::vector<TutorMessage>& c = host()->conversation(host()->lesson);
			if (hit->arg >= 0 && hit->arg < (int) c.size())
				glfwSetClipboardString(APP->window->win, c[hit->arg].text.c_str());
			e.consume(this);
			return;
		}
		if (hit && e.button == GLFW_MOUSE_BUTTON_LEFT && hit->action != TUTOR_MSG && hit->action != TUTOR_INPUT) {
			act(hit->action, hit->arg);
			e.consume(this);
			return;
		}
	}
	OpaqueWidget::onButton(e);
}

void DockPanel::onHover(const HoverEvent& e) {
	float k = panelScale();
	ptr.x = e.pos.x / k;
	ptr.y = e.pos.y / k;
	OpaqueWidget::onHover(e);
}

void DockPanel::onLeave(const LeaveEvent& e) {
	ptr.x = ptr.y = -1e6f;
	OpaqueWidget::onLeave(e);
}

void DockPanel::onHoverScroll(const HoverScrollEvent& e) {
	float dy = e.scrollDelta.y;
	if (view.map)
		view.mapScroll.wheel(dy);
	else if (view.tutor.convRect.contains(e.pos.x / panelScale(), e.pos.y / panelScale()))
		view.tutor.wheel(dy);
	else
		view.outlineScroll.wheel(dy);
	e.consume(this);
}

// ── pointers ──

bool resolveTarget(const Target& t, math::Vec* center, float* radius) {
	app::ModuleWidget* mw = APP->scene->rack->getModule(t.moduleId);
	if (!mw)
		return false;
	widget::Widget* w = nullptr;
	if (t.kind == "param")
		w = mw->getParam(t.index);
	else if (t.kind == "input")
		w = mw->getInput(t.index);
	else if (t.kind == "output")
		w = mw->getOutput(t.index);
	else if (t.kind == "light") {
		for (widget::Widget* c : mw->children) {
			app::ModuleLightWidget* lw = dynamic_cast<app::ModuleLightWidget*>(c);
			if (lw && lw->firstLightId == t.index)
				w = lw;
		}
	}
	if (!w)
		return false;
	math::Vec tl = w->getRelativeOffset(math::Vec(0, 0), APP->scene->rack);
	*center = tl.plus(w->box.size.div(2));
	*radius = std::max(w->box.size.x, w->box.size.y) / 2.f + 3.f;
	return true;
}

void PointerOverlay::step() {
	math::Rect bb = APP->scene->rack->getModuleContainer()->getChildrenBoundingBox();
	box.pos = math::Vec(0, 0);
	if (bb.pos.isFinite())
		box.size = bb.getBottomRight().plus(math::Vec(400, 400));
	Widget::step();
}

void PointerOverlay::draw(const DrawArgs& args) {
	AtlasHost* h = host();
	if (!h)
		return;
	const Lesson* L = h->currentLesson();
	const Block* B = h->currentBlock();
	h->missingTargets.clear();
	if (!L || !B)
		return;
	Style st = currentStyle(L->part);
	if (!st.fonts.ok())
		return;
	double now = system::getTime();
	bool pulse = h->pulseBlock == h->block && now - h->pulseStart < 0.9;
	int number = 0;
	for (const Item& it : B->items) {
		if (it.kind != "note")
			continue;
		number++;
		// Where the note itself is (for the leader line).
		math::Vec from;
		bool haveFrom = false;
		for (widget::Widget* w : APP->scene->rack->getModuleContainer()->children) {
			app::ModuleWidget* mw = dynamic_cast<app::ModuleWidget*>(w);
			NoteModule* nm = mw ? dynamic_cast<NoteModule*>(mw->module) : nullptr;
			if (nm && nm->lesson == L->id && nm->block == B->id && nm->item == it.id) {
				from = mw->box.pos.plus(math::Vec(mw->box.size.x < 100.f ? 15.f : 17.f, tokens::panel_accentRule + 19.f));
				haveFrom = true;
			}
		}
		for (const Target& t : it.targets) {
			math::Vec c;
			float r;
			if (!resolveTarget(t, &c, &r)) {
				h->missingTargets.push_back(L->id + "/" + B->id + "/" + it.id + " → " + t.module + "." + t.name);
				continue;
			}
			if (haveFrom) {
				math::Vec d = c.minus(from);
				float len = d.norm();
				if (len > r + 10.f) {
					math::Vec end = c.minus(d.div(len).mult(r));
					nvgBeginPath(args.vg);
					nvgMoveTo(args.vg, from.x, from.y);
					nvgLineTo(args.vg, end.x, end.y);
					nvgStrokeColor(args.vg, nvgTransRGBAf(st.accent, 0.55f));
					nvgStrokeWidth(args.vg, 1.2f);
					nvgStroke(args.vg);
				}
			}
			nvgBeginPath(args.vg);
			nvgCircle(args.vg, c.x, c.y, r);
			nvgStrokeColor(args.vg, st.accent);
			nvgStrokeWidth(args.vg, 2.f);
			nvgStroke(args.vg);
			if (pulse) {
				float k = (float) ((now - h->pulseStart) / 0.9);
				nvgBeginPath(args.vg);
				nvgCircle(args.vg, c.x, c.y, r + 14.f * k);
				nvgStrokeColor(args.vg, nvgTransRGBAf(st.accent, 1.f - k));
				nvgStrokeWidth(args.vg, 2.f);
				nvgStroke(args.vg);
			}
			drawBadge(args.vg, st, c.x + r * 0.75f, c.y - r * 0.75f, number);
		}
	}
}

} // namespace atlas
