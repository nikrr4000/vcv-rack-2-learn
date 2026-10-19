// Figure: placeholder until its workstream task.
#include "Base.hpp"

namespace atlas {

struct FigureModule : AtlasModule {
	FigureModule() { config(0, 0, 0, 0); }
};

struct FigureWidget : AtlasWidget {
	FigureWidget(FigureModule* m) : AtlasWidget(m, 10, 2, 40, false, "", "Figure") {}
	void drawView(NVGcontext* vg, const Style& st, float w, float h) override { drawMissing(vg, st, w, h, "**Figure**"); }
};

} // namespace atlas

Model* modelFigure = createModel<atlas::FigureModule, atlas::FigureWidget>("Figure");
