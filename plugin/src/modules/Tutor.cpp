// Tutor: placeholder until its workstream task.
#include "Base.hpp"

namespace atlas {

struct TutorModule : AtlasModule {
	TutorModule() { config(0, 0, 0, 0); }
};

struct TutorWidget : AtlasWidget {
	TutorWidget(TutorModule* m) : AtlasWidget(m, 10, 2, 40, false, "", "Tutor") {}
	void drawView(NVGcontext* vg, const Style& st, float w, float h) override { drawMissing(vg, st, w, h, "**Tutor**"); }
};

} // namespace atlas

Model* modelTutor = createModel<atlas::TutorModule, atlas::TutorWidget>("Tutor");
