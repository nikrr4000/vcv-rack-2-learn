// The Task module's face: the practice brief (or «Покрути сам» steps), «Проверить», results
// with a hint ladder for the first failing item.
#pragma once
#include <map>
#include <vector>

#include "Flow.hpp"
#include "check/Checker.hpp"
#include "core/Pack.hpp"

namespace atlas {

enum TaskAction { TASK_CHECK = 1, TASK_MOREHINT, TASK_BRIEF, TASK_RESULT, TASK_STEP };

struct TaskState {
	int page = 0;  // 0 brief, 1 result
	bool checking = false;
	bool hasResult = false;
	check::Result result;
	std::map<std::string, int> hintLevel;
	std::vector<bool> stepsDone;
	std::string error;  // e.g. «нет верстака»
};

struct TaskView {
	std::string header;
	const Item* item = nullptr;
	std::string title;
	TaskState* state = nullptr;
	Scroll scroll;
	FlowCache cache;

	void draw(NVGcontext* vg, const Style& st, float w, float h, const Pointer& p, Hits& hits);
	bool action(int a, int arg);
	bool wheel(float dy) { return scroll.wheel(dy); }
	/** The hint shown for the first failing item at its current level, or "". */
	std::string currentHint(int* level = nullptr, int* levels = nullptr) const;
	const check::Item* firstFailing() const;
	/** Light state: 0 none, 1 passed, 2 failed, 3 checking. */
	int light() const;
};

} // namespace atlas
