// Задание: the practice brief, «Проверить» (the in-Rack checker), results with hint ladders.
#include "Modules.hpp"
#include "host/Host.hpp"

namespace atlas {

struct TaskWidget : AtlasWidget {
	TaskView view;
	TaskState preview;
	TaskWidget(TaskModule* m) : AtlasWidget(m, 14, 14, 14, false, "task", "Task") {}
	TaskState* state() { return am ? &((TaskModule*) am)->state : &preview; }
	void drawView(NVGcontext* vg, const Style& st, float w, float h) override {
		const Item* it = itemP();
		if (!it) {
			drawMissing(vg, st, w, h, "**Задание** к уроку курса Atlas.");
			return;
		}
		const Block* b = blockP();
		view.header = header();
		view.item = it;
		view.title = b ? b->title : "";
		view.state = state();
		view.draw(vg, st, w, h, ptr, hits);
	}
	void onHit(int a, int arg) override {
		if (a == TASK_CHECK) {
			if (am && host())
				host()->startCheck(am->id);
			return;
		}
		view.action(a, arg);
	}
	bool onWheel(float dy) override { return view.wheel(dy); }
};

} // namespace atlas

Model* modelTask = createModel<atlas::TaskModule, atlas::TaskWidget>("Task");
