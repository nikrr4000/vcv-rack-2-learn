// Репетитор: the tutor as a module (same engine and look as the docked panel).
#include <cmath>

#include "Modules.hpp"
#include "host/Host.hpp"

namespace atlas {

struct TutorWidget : AtlasWidget {
	TutorView view;
	AtlasField* field = nullptr;
	std::vector<TutorMessage> previewMessages;
	TutorWidget(TutorModule* m) : AtlasWidget(m, 18, 14, 30, true, "", "Tutor") {
		if (m) {
			field = new AtlasField;
			field->onSubmit = [](const std::string& t) {
				if (host())
					host()->ask("ask", t);
			};
			field->indexAt = [this](math::Vec pos) {
				Style st = currentStyle(part());
				const Rect& r = view.inputRect;
				return inputIndexAt(APP->window->vg, st, r, field->text, pos.x + r.x, pos.y + r.y);
			};
			addChild(field);
		}
		else {
			TutorMessage q;
			q.role = "learner";
			q.text = "Почему пила звучит ярче синуса?";
			previewMessages.push_back(q);
			TutorMessage a;
			a.role = "tutor";
			a.local = true;
			a.text = "В пиле много **гармоник** — призвуков, кратных основной частоте. В синусе только одна.";
			previewMessages.push_back(a);
		}
	}
	void step() override {
		if (field) {
			const Rect& r = view.inputRect;
			field->box.pos = math::Vec(r.x, r.y);
			field->box.size = math::Vec(r.w, r.h);
		}
		AtlasWidget::step();
	}
	void drawView(NVGcontext* vg, const Style& st0, float w, float h) override {
		AtlasHost* hs = host();
		const Lesson* L = hs ? hs->currentLesson() : nullptr;
		Style st = currentStyle(L ? L->part : 0);
		Rect c = drawPanel(vg, st, w, h, (L ? L->num + " · " : std::string()) + "Репетитор");
		const Block* b = hs ? hs->currentBlock() : nullptr;
		view.messages = hs ? &hs->conversation(hs->lesson) : &previewMessages;
		view.faq = b ? &b->faq : nullptr;
		view.offline = hs ? !hs->client.connected : false;
		view.busy = hs && hs->tutorBusy;
		view.time = (float) system::getTime();
		if (field) {
			view.input = field->text;
			view.cursor = field->cursor;
			view.selection = field->selection;
			view.focused = field->focused();
			view.blink = std::fmod(system::getTime(), 1.0) < 0.6;
		}
		view.draw(vg, st, c, ptr, hits);
		(void) st0;
	}
	void onHit(int a, int arg) override {
		AtlasHost* h = host();
		if (!h)
			return;
		if (a == TUTOR_FAQ)
			h->faq(arg);
		else if (a == TUTOR_QUICK)
			h->quick(arg);
		else if (a == TUTOR_SEND && field && !field->text.empty()) {
			h->ask("ask", field->text);
			field->setText("");
		}
		else if (a == TUTOR_SCROLL_DOWN) {
			view.scroll.stickBottom = true;
			view.newBelow = false;
		}
	}
	void onButton(const ButtonEvent& e) override {
		if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_RIGHT && host()) {
			const Hit* hit = hits.at(e.pos.x, e.pos.y);
			if (hit && hit->action == TUTOR_MSG) {
				std::vector<TutorMessage>& c = host()->conversation(host()->lesson);
				if (hit->arg >= 0 && hit->arg < (int) c.size())
					glfwSetClipboardString(APP->window->win, c[hit->arg].text.c_str());
				e.consume(this);
				return;
			}
		}
		AtlasWidget::onButton(e);
	}
	bool onWheel(float dy) override { return view.convRect.contains(ptr.x, ptr.y) && view.wheel(dy); }
	void appendContextMenu(ui::Menu* menu) override { appendAtlasMenu(menu); }
};

} // namespace atlas

Model* modelTutor = createModel<atlas::TutorModule, atlas::TutorWidget>("Tutor");
