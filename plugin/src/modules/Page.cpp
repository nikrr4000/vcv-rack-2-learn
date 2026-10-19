// Страница: the narrative text of a block. Content comes from the pack by reference.
#include "Base.hpp"
#include "views/PageView.hpp"

namespace atlas {

struct PageModule : AtlasModule {
	int page = 0;
	PageModule() { config(0, 0, 0, 0); }
	void saveState(json_t* s) override { json_object_set_new(s, "page", json_integer(page)); }
	void loadState(json_t* s) override {
		json_t* p = json_object_get(s, "page");
		if (p)
			page = (int) json_integer_value(p);
	}
};

struct PageWidget : AtlasWidget {
	PageView view;
	PageWidget(PageModule* m) : AtlasWidget(m, 24, 8, 40, true, "page", "Page") {
		if (m)
			view.text.page = m->page;
	}
	PageModule* pm() { return (PageModule*) am; }

	void drawView(NVGcontext* vg, const Style& st, float w, float h) override {
		const Item* it = itemP();
		if (!it) {
			drawMissing(vg, st, w, h, "**Страница**\nТекст блока появится, когда модуль привязан к уроку курса Atlas.");
			return;
		}
		view.header = header();
		view.text.md = it->text;
		view.text.title = it->title;
		view.text.reportId = lessonId() + "/" + blockId() + "/" + itemId();
		view.draw(vg, st, w, h, ptr, hits);
		if (pm())
			pm()->page = view.text.page;
	}
	void onAction(int a, int arg) override { view.action(a, arg); }
	bool onWheel(float dy) override { return false; }
	std::string hoverTerm() override { return view.termAt(ptr.x, ptr.y); }
};

} // namespace atlas

Model* modelPage = createModel<atlas::PageModule, atlas::PageWidget>("Page");
