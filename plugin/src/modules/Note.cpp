// Подпись: a short caption with numbered pointer rings on its targets (drawn by the overlay).
#include "Modules.hpp"
#include "host/Panel.hpp"
#include "views/SmallViews.hpp"

namespace atlas {

struct NoteWidget : AtlasWidget {
	NoteView view;
	NoteWidget(NoteModule* m) : AtlasWidget(m, 8, 4, 12, true, "note", "Note") {
		if (m)
			view.text.page = m->page;
	}
	void drawView(NVGcontext* vg, const Style& st, float w, float h) override {
		const Item* it = itemP();
		const Block* b = blockP();
		if (!it || !b) {
			drawMissing(vg, st, w, h, "**Подпись** к уроку курса Atlas.");
			return;
		}
		int n = 0;
		for (const Item& x : b->items) {
			if (x.kind == "note")
				n++;
			if (&x == it)
				break;
		}
		view.number = it->targets.empty() ? 0 : n;
		view.missingTarget = false;
		if (am && host())
			for (const Target& t : it->targets) {
				math::Vec c;
				float r;
				if (!resolveTarget(t, &c, &r))
					view.missingTarget = true;
			}
		view.header = header();
		view.text.md = it->text;
		view.text.reportId = lessonId() + "/" + blockId() + "/" + itemId();
		view.draw(vg, st, w, h, ptr, hits);
		if (am)
			((NoteModule*) am)->page = view.text.page;
	}
	void onHit(int a, int arg) override { view.action(a, arg); }
	std::string hoverTerm() override { return view.termAt(ptr.x, ptr.y); }
};

} // namespace atlas

Model* modelNote = createModel<atlas::NoteModule, atlas::NoteWidget>("Note");
