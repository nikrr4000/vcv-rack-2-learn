// Схема: a procedural figure from a template plus a caption.
#include "Modules.hpp"
#include "views/SmallViews.hpp"

namespace atlas {

struct FigureModule : AtlasModule {
	FigureModule() { config(0, 0, 0, 0); }
};

struct FigureWidget : AtlasWidget {
	FigureView view;
	FigureWidget(FigureModule* m) : AtlasWidget(m, 16, 8, 24, true, "figure", "Figure") {}
	void drawView(NVGcontext* vg, const Style& st, float w, float h) override {
		const Item* it = itemP();
		if (!it) {
			drawMissing(vg, st, w, h, "**Схема** к уроку курса Atlas.");
			return;
		}
		view.header = header();
		view.figure = it->figure;
		view.caption = it->caption;
		view.draw(vg, st, w, h);
	}
};

} // namespace atlas

Model* modelFigure = createModel<atlas::FigureModule, atlas::FigureWidget>("Figure");
