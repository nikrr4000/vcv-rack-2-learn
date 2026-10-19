// Верстак: marks the practice area of a row (left and right marker).
#include "Modules.hpp"
#include "views/SmallViews.hpp"

namespace atlas {

struct BenchWidget : AtlasWidget {
	BenchView view;
	BenchWidget(BenchModule* m) : AtlasWidget(m, 2, 2, 2, false, "bench", "Bench") {}
	void drawView(NVGcontext* vg, const Style& st, float w, float h) override {
		const Item* it = itemP();
		const Lesson* L = lesson();
		view.left = !it || it->side != "right";
		view.lessonNum = L ? L->num : "";
		view.draw(vg, st, w, h);
	}
};

} // namespace atlas

Model* modelBench = createModel<atlas::BenchModule, atlas::BenchWidget>("Bench");
