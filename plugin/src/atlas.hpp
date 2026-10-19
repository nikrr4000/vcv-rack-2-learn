#pragma once
#include <rack.hpp>

#include "core/Pack.hpp"
#include "ui/Style.hpp"

using namespace rack;

extern Plugin* pluginInstance;

extern Model* modelNavigator;
extern Model* modelPage;
extern Model* modelNote;
extern Model* modelFigure;
extern Model* modelQuiz;
extern Model* modelTask;
extern Model* modelBench;
extern Model* modelTutor;

namespace atlas {

/** The course pack, loaded once on first use (previews included). */
const Pack& pack();

/** Plugin settings: `Atlas/settings.json` in the Rack user folder; env `ATLAS_URL` wins. */
struct Settings {
	std::string atlasUrl = "http://127.0.0.1:5175";
	bool panelOpen = true;
	void load();
	void save();
	/** The URL in effect (env first), without a trailing slash. */
	std::string url() const;
};
Settings& settings();

/** Style for drawing in the current theme with the accent of `part`. Call from draw(). */
Style currentStyle(int part);

/** Preview selection from env `ATLAS_PREVIEW` (`Page=<lesson>/<block>/<item>;Quiz=…`), or "". */
std::string previewRef(const std::string& modelSlug);

/** Every pack item of `kind` in course order: (lesson, block, item). */
struct ItemRef {
	std::string lesson, block, item;
};
std::vector<ItemRef> itemsOfKind(const std::string& kind);

} // namespace atlas
