// Note: placeholder until its workstream task.
#include "Base.hpp"

namespace atlas {

struct NoteModule : AtlasModule {
	NoteModule() { config(0, 0, 0, 0); }
};

struct NoteWidget : AtlasWidget {
	NoteWidget(NoteModule* m) : AtlasWidget(m, 10, 2, 40, false, "", "Note") {}
	void drawView(NVGcontext* vg, const Style& st, float w, float h) override { drawMissing(vg, st, w, h, "**Note**"); }
};

} // namespace atlas

Model* modelNote = createModel<atlas::NoteModule, atlas::NoteWidget>("Note");
