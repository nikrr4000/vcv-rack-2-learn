// Quiz: placeholder until its workstream task.
#include "Base.hpp"

namespace atlas {

struct QuizModule : AtlasModule {
	QuizModule() { config(0, 0, 0, 0); }
};

struct QuizWidget : AtlasWidget {
	QuizWidget(QuizModule* m) : AtlasWidget(m, 10, 2, 40, false, "", "Quiz") {}
	void drawView(NVGcontext* vg, const Style& st, float w, float h) override { drawMissing(vg, st, w, h, "**Quiz**"); }
};

} // namespace atlas

Model* modelQuiz = createModel<atlas::QuizModule, atlas::QuizWidget>("Quiz");
