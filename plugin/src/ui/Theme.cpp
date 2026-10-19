#include "Theme.hpp"

#include <set>

#include "atlas.hpp"

namespace atlas {

int fontHandle(FontRole role) {
	if (!APP || !APP->window)
		return -1;
	static const char* files[FONT_COUNT] = {tokens::font_heading, tokens::font_body, tokens::font_bodyBold, tokens::font_bodyItalic, tokens::font_mono};
	std::shared_ptr<window::Font> f = APP->window->loadFontWithoutFallbacks(asset::plugin(pluginInstance, files[role]));
	if (!f)
		return -1;
	// Symbols the paper fonts lack (→ ✓ ✗ ≈) come from Rack's own DejaVu Sans.
	static std::set<int> withFallback;
	if (!withFallback.count(f->handle)) {
		std::shared_ptr<window::Font> fb = APP->window->loadFontWithoutFallbacks(asset::system("res/fonts/DejaVuSans.ttf"));
		if (fb)
			nvgAddFallbackFontId(APP->window->vg, f->handle, fb->handle);
		withFallback.insert(f->handle);
	}
	return f->handle;
}

Style currentStyle(int part) {
	text::Fonts fonts;
	for (int i = 0; i < FONT_COUNT; i++)
		fonts.handle[i] = fontHandle((FontRole) i);
	return makeStyle(rack::settings::preferDarkPanels, part, fonts);
}

} // namespace atlas
