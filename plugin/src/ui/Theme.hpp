// The «Атлас» visual identity at runtime: palette by Rack's «Prefer dark panels»,
// part accents, type scale and the bundled fonts. Numbers come from ThemeTokens.hpp,
// which is generated from rack/course/design-tokens.json (the text-fit tool reads the same file).
#pragma once
#include <rack.hpp>
#include "ThemeTokens.hpp"

namespace atlas {

using tokens::Palette;

/** Current palette: dark when the user prefers dark panels. Previews use it too. */
inline const Palette& palette() {
	static const Palette light = tokens::lightPalette();
	static const Palette dark = tokens::darkPalette();
	return rack::settings::preferDarkPanels ? dark : light;
}

inline bool darkTheme() { return rack::settings::preferDarkPanels; }

inline NVGcolor accent(int part) { return tokens::accent(part); }

/** A color with alpha (0–1). */
inline NVGcolor alpha(NVGcolor c, float a) {
	c.a = a;
	return c;
}

/** Mix of the accent into the panel color (for the 15 % active fill), opaque. */
inline NVGcolor tint(NVGcolor accentColor, float amount) {
	NVGcolor p = palette().panel;
	return nvgRGBf(p.r + (accentColor.r - p.r) * amount, p.g + (accentColor.g - p.g) * amount, p.b + (accentColor.b - p.b) * amount);
}

enum FontRole { FONT_HEADING, FONT_BODY, FONT_BOLD, FONT_ITALIC, FONT_MONO, FONT_COUNT };

/** nanovg font handle for a role; -1 when the window cannot load fonts (headless). Call from draw() only. */
int fontHandle(FontRole role);

} // namespace atlas
