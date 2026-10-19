// Вопрос: options graded locally with explanations; «Свой ответ» graded by the tutor.
#include <cmath>

#include "Modules.hpp"
#include "host/Host.hpp"

namespace atlas {

struct QuizWidget : AtlasWidget {
	QuizView view;
	QuizState preview;
	AtlasField* field = nullptr;
	QuizWidget(QuizModule* m) : AtlasWidget(m, 14, 14, 14, false, "quiz", "Quiz") {
		if (m) {
			field = new AtlasField;
			field->visible = false;
			field->onSubmit = [this](const std::string& t) { send(t); };
			field->indexAt = [this](math::Vec pos) {
				Style st = currentStyle(part());
				const Rect& r = view.inputRect;
				return inputIndexAt(APP->window->vg, st, r, field->text, pos.x + r.x, pos.y + r.y);
			};
			addChild(field);
		}
	}
	QuizState* state() { return am ? &((QuizModule*) am)->state : &preview; }

	void step() override {
		if (field) {
			const Rect& r = view.inputRect;
			field->visible = state()->freeMode && r.w > 0;
			field->box.pos = math::Vec(r.x, r.y);
			field->box.size = math::Vec(r.w, r.h);
		}
		AtlasWidget::step();
	}

	void drawView(NVGcontext* vg, const Style& st, float w, float h) override {
		const Item* it = itemP();
		if (!it || it->questions.empty()) {
			drawMissing(vg, st, w, h, "**Вопрос** к уроку курса Atlas.");
			return;
		}
		view.header = header();
		view.questions = &it->questions;
		view.state = state();
		view.offline = !(host() && host()->client.connected);
		if (field) {
			view.input = field->text;
			view.cursor = field->cursor;
			view.selection = field->selection;
			view.focused = field->focused();
			view.blink = std::fmod(system::getTime(), 1.0) < 0.6;
		}
		view.draw(vg, st, w, h, ptr, hits);
	}

	std::string context(const Question& q, const std::string& answer) {
		std::string s = "Вопрос: " + q.q + "\nВарианты:";
		for (size_t k = 0; k < q.options.size(); k++)
			s += "\n" + optionLetter((int) k) + ") " + q.options[k].text + (q.options[k].correct ? " (верный)" : "");
		s += "\nОтвет ученика: " + answer;
		return s;
	}

	void send(const std::string& text) {
		AtlasHost* h = host();
		const Item* it = itemP();
		if (!h || !it || text.empty())
			return;
		QuizState* s = state();
		int qi = s->current;
		if (qi < 0 || qi >= (int) it->questions.size())
			return;
		QuizAnswer& A = s->answers[qi];
		A.chosen = CHOSEN_FREE;
		A.freeText = text;
		A.busy = true;
		A.verdict.clear();
		s->freeMode = false;
		int64_t mid = am->id;
		h->ask("grade", text, context(it->questions[qi], text), [mid, qi](const std::string& reply) {
			QuizModule* m = dynamic_cast<QuizModule*>(APP->engine->getModule(mid));
			if (m && qi < (int) m->state.answers.size()) {
				m->state.answers[qi].verdict = reply;
				m->state.answers[qi].busy = false;
			}
		});
		if (field)
			field->setText("");
	}

	void onHit(int a, int arg) override {
		if (a == QUIZ_SEND) {
			if (field)
				send(field->text);
			return;
		}
		if (a == QUIZ_INPUT)
			return;
		view.action(a, arg);
		const Item* it = itemP();
		AtlasHost* h = host();
		if (h && it && (a == QUIZ_CHOOSE || a == QUIZ_DONTKNOW)) {
			QuizState* s = state();
			const Question& q = it->questions[s->current];
			json_t* d = json_object();
			json_object_set_new(d, "item", json_string(itemId().c_str()));
			json_object_set_new(d, "question", json_string(q.q.c_str()));
			json_object_set_new(d, "answer", json_string(a == QUIZ_DONTKNOW ? "не знаю" : q.options[arg].text.c_str()));
			json_object_set_new(d, "correct", json_boolean(a == QUIZ_CHOOSE && q.options[arg].correct));
			h->event("quiz", d);
		}
		if (a == QUIZ_FREE && field)
			APP->event->setSelectedWidget(field);
	}
	bool onWheel(float dy) override { return view.wheel(dy); }
};

} // namespace atlas

Model* modelQuiz = createModel<atlas::QuizModule, atlas::QuizWidget>("Quiz");
