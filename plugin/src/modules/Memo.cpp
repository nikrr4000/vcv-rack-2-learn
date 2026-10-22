// Памятка: a reference card that works in any patch, with or without a lesson. The text lives in
// the plugin (res/memo/notes-voltage.md), paginated like a Page, with the page kept per module.
#include "Base.hpp"
#include "views/PageView.hpp"

namespace atlas {

/** The memo text, read once from the plugin's resources. */
static const std::string& memoText() {
	static std::string text;
	static bool loaded = false;
	if (!loaded) {
		loaded = true;
		std::string path = asset::plugin(pluginInstance, "res/memo/notes-voltage.md");
		FILE* f = std::fopen(path.c_str(), "rb");
		if (f) {
			char buf[4096];
			size_t n;
			while ((n = std::fread(buf, 1, sizeof(buf), f)) > 0)
				text.append(buf, n);
			std::fclose(f);
		}
		if (text.empty())
			text = "# Памятка\nНе найден файл res/memo/notes-voltage.md в плагине Atlas.";
	}
	return text;
}

struct MemoModule : AtlasModule {
	int page = 0;
	MemoModule() { config(0, 0, 0, 0); }
	void saveState(json_t* s) override { json_object_set_new(s, "page", json_integer(page)); }
	void loadState(json_t* s) override {
		json_t* p = json_object_get(s, "page");
		if (p)
			page = (int) json_integer_value(p);
	}
};

struct MemoWidget : AtlasWidget {
	PageView view;
	MemoWidget(MemoModule* m) : AtlasWidget(m, 20, 12, 40, true, "memo", "Memo") {
		if (m)
			view.text.page = m->page;
	}
	MemoModule* mm() { return (MemoModule*) am; }

	void drawView(NVGcontext* vg, const Style& st, float w, float h) override {
		view.header = "Памятка · звук и вольты";
		view.text.md = memoText();
		view.text.title = true;
		view.text.reportId = "memo/notes-voltage";
		view.draw(vg, st, w, h, ptr, hits);
		if (mm())
			mm()->page = view.text.page;
	}
	void onHit(int a, int arg) override { view.action(a, arg); }
	std::string hoverTerm() override { return view.termAt(ptr.x, ptr.y); }
};

} // namespace atlas

Model* modelMemo = createModel<atlas::MemoModule, atlas::MemoWidget>("Memo");
