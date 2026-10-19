// Bench: placeholder until its workstream task.
#include "Base.hpp"

namespace atlas {

struct BenchModule : AtlasModule {
	BenchModule() { config(0, 0, 0, 0); }
};

struct BenchWidget : AtlasWidget {
	BenchWidget(BenchModule* m) : AtlasWidget(m, 10, 2, 40, false, "", "Bench") {}
	void drawView(NVGcontext* vg, const Style& st, float w, float h) override { drawMissing(vg, st, w, h, "**Bench**"); }
};

} // namespace atlas

Model* modelBench = createModel<atlas::BenchModule, atlas::BenchWidget>("Bench");
