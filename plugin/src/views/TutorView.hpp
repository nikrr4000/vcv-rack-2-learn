// The tutor conversation (docked panel and the Tutor module share it): bubbles with streaming,
// FAQ chips answered locally («из урока»), quick actions, a wrapping input. Rack-free.
#pragma once
#include <vector>

#include "Flow.hpp"
#include "Input.hpp"
#include "core/Pack.hpp"

namespace atlas {

struct TutorMessage {
	std::string role;  // "learner" | "tutor"
	std::string text;
	bool local = false;      // prepared answer from the lesson
	bool streaming = false;  // still arriving
	bool error = false;
};

enum TutorAction { TUTOR_FAQ = 1, TUTOR_QUICK, TUTOR_SEND, TUTOR_INPUT, TUTOR_MSG, TUTOR_SCROLL_DOWN };
enum QuickKind { QUICK_SIMPLER = 0, QUICK_EXAMPLE, QUICK_HINT, QUICK_REVIEW };

const char* quickLabel(int k);

struct TutorView {
	const std::vector<TutorMessage>* messages = nullptr;
	const std::vector<Faq>* faq = nullptr;
	bool offline = false;
	bool busy = false;
	bool showQuick = true;
	float time = 0.f;  // seconds, for the typing indicator
	// Input mirror (the Rack TextField owns the keyboard).
	std::string input;
	int cursor = 0, selection = 0;
	bool focused = false, blink = true;

	Rect inputRect;
	Scroll scroll;
	FlowCache cache;
	size_t lastCount = 0;
	size_t lastLen = 0;
	bool newBelow = false;

	/** Draws into r (no panel frame). */
	void draw(NVGcontext* vg, const Style& st, const Rect& r, const Pointer& p, Hits& hits);
	bool wheel(float dy) { return scroll.wheel(dy * 30.f); }
};

} // namespace atlas
