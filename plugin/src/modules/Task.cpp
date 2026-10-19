// Task: placeholder until its workstream task.
#include "Base.hpp"

namespace atlas {

struct TaskModule : AtlasModule {
	TaskModule() { config(0, 0, 0, 0); }
};

struct TaskWidget : AtlasWidget {
	TaskWidget(TaskModule* m) : AtlasWidget(m, 10, 2, 40, false, "", "Task") {}
	void drawView(NVGcontext* vg, const Style& st, float w, float h) override { drawMissing(vg, st, w, h, "**Task**"); }
};

} // namespace atlas

Model* modelTask = createModel<atlas::TaskModule, atlas::TaskWidget>("Task");
