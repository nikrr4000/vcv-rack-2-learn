// Навигатор: course home and lesson controls. Owns nothing itself: actions go to the host.
#include "Modules.hpp"
#include "host/Host.hpp"
#include "views/NavView.hpp"

namespace atlas {

struct NavigatorWidget : AtlasWidget {
	NavigatorView view;
	bool userHome = false;  // the learner switched to the course home while a lesson is open
	NavigatorWidget(NavigatorModule* m) : AtlasWidget(m, 10, 10, 10, false, "", "Navigator") {}

	NavState state() {
		AtlasHost* h = host();
		if (h)
			return h->navState();
		NavState s;
		s.pack = &pack();
		s.lesson = am ? am->lesson : "";
		return s;
	}

	void drawView(NVGcontext* vg, const Style& st0, float w, float h) override {
		NavState s = state();
		const Lesson* L = pack().lesson(s.lesson);
		Style st = currentStyle(L ? L->part : 0);
		static Progress empty;
		if (!s.progress)
			s.progress = &empty;
		if (view.mode != NAV_MODE_MAP)
			view.mode = L && !userHome ? NAV_MODE_LESSON : NAV_MODE_HOME;
		view.draw(vg, st, w, h, s, ptr, hits);
		(void) st0;
	}

	void onHit(int a, int arg) override {
		AtlasHost* h = host();
		if (!h)
			return;
		switch (a) {
			case NAV_CONTINUE: {
				std::string id = h->progress.continueLesson(pack());
				if (id == h->lesson && h->currentLesson()) {
					userHome = false;
					break;
				}
				if (!id.empty())
					h->openLesson(id);
				userHome = false;
				break;
			}
			case NAV_PREV: h->prev(); break;
			case NAV_NEXT: h->next(); break;
			case NAV_GOTO_BLOCK: h->gotoBlock(arg); break;
			case NAV_DONE:
				h->completeLesson();
				userHome = true;
				break;
			case NAV_MAP:
				view.mode = NAV_MODE_MAP;
				view.mapPart = -1;
				break;
			case NAV_MAP_CLOSE: view.mode = NAV_MODE_HOME; break;
			case NAV_HOME: userHome = false; view.mode = NAV_MODE_LESSON; break;
			case NAV_OPEN:
				if (arg >= 0 && arg < (int) view.mapRows.size()) {
					h->openLesson(view.mapRows[arg]);
					view.mode = NAV_MODE_LESSON;
					userHome = false;
				}
				break;
			case NAV_PART_PREV: view.mapPart--; break;
			case NAV_PART_NEXT: view.mapPart++; break;
			case NAV_PANEL: h->togglePanel(); break;
		}
	}

	void appendContextMenu(ui::Menu* menu) override {
		menu->addChild(createMenuItem("Карта курса", "", [this]() { view.mode = NAV_MODE_MAP; }));
		appendAtlasMenu(menu);
	}
};

} // namespace atlas

Model* modelNavigator = createModel<atlas::NavigatorModule, atlas::NavigatorWidget>("Navigator");
