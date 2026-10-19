// The Quiz module's face: one question at a time, options with local verdicts and
// explanations, «Не знаю», and «Свой ответ» graded by the tutor.
#pragma once
#include <vector>

#include "Flow.hpp"
#include "Input.hpp"
#include "core/Pack.hpp"

namespace atlas {

enum QuizAction { QUIZ_CHOOSE = 1, QUIZ_DONTKNOW, QUIZ_FREE, QUIZ_SEND, QUIZ_CANCEL, QUIZ_RETRY, QUIZ_NEXT, QUIZ_GOTO, QUIZ_INPUT };

const int CHOSEN_NONE = -1;
const int CHOSEN_DONTKNOW = -2;
const int CHOSEN_FREE = -3;

struct QuizAnswer {
	int chosen = CHOSEN_NONE;
	std::string freeText;
	std::string verdict;  // tutor reply for a free answer
	bool busy = false;
};

struct QuizState {
	int current = 0;
	std::vector<QuizAnswer> answers;
	bool freeMode = false;
	void fit(size_t n) {
		if (answers.size() != n)
			answers.resize(n);
		if (current >= (int) n)
			current = 0;
	}
};

struct QuizView {
	std::string header;
	const std::vector<Question>* questions = nullptr;
	QuizState* state = nullptr;
	// The free-answer field (owned by the Rack widget; mirrored here for drawing).
	std::string input;
	int cursor = 0, selection = 0;
	bool focused = false, blink = true;
	bool offline = false;

	Rect inputRect;  // valid while state->freeMode
	Scroll scroll;
	FlowCache cache;

	void draw(NVGcontext* vg, const Style& st, float w, float h, const Pointer& p, Hits& hits);
	/** Local actions (choose, retry, navigation). Returns true when the view changed. SEND is handled by the widget. */
	bool action(int a, int arg);
	bool wheel(float dy) { return scroll.wheel(dy); }
};

/** Letter marker for option k: А Б В Г. */
std::string optionLetter(int k);

} // namespace atlas
