// Navigator: placeholder until its workstream task.
#include "Base.hpp"

namespace atlas {

struct NavigatorModule : AtlasModule {
	NavigatorModule() { config(0, 0, 0, 0); }
};

struct NavigatorWidget : AtlasWidget {
	NavigatorWidget(NavigatorModule* m) : AtlasWidget(m, 10, 2, 40, false, "", "Navigator") {}
	void drawView(NVGcontext* vg, const Style& st, float w, float h) override { drawMissing(vg, st, w, h, "**Navigator**"); }
};

} // namespace atlas

Model* modelNavigator = createModel<atlas::NavigatorModule, atlas::NavigatorWidget>("Navigator");
