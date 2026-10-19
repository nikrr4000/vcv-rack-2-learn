// What every Rack-free view needs to draw: palette, part accent, fonts. Built from the Rack
// theme in the plugin (Theme.hpp) and directly in the render harness.
#pragma once
#include <nanovg.h>

#include "ThemeTokens.hpp"
#include "text/NvgText.hpp"

namespace atlas {

struct Style {
	tokens::Palette pal;
	NVGcolor accent;
	text::Fonts fonts;
	bool dark = false;
	int part = 0;

	text::TextStyle textStyle(bool titleHeadings = false) const {
		text::TextStyle t;
		t.bodySize = tokens::type_body;
		t.headingSize = titleHeadings ? tokens::type_title : tokens::type_heading;
		t.monoSize = tokens::type_mono;
		t.lineHeight = tokens::type_lineHeight;
		t.paragraphGap = tokens::type_paragraphGap;
		t.listIndent = tokens::type_listIndent;
		return t;
	}
	text::TextColors textColors() const {
		text::TextColors c;
		c.ink = pal.ink;
		c.ink2 = pal.ink2;
		c.term = pal.ink2;
		c.accent = accent;
		return c;
	}
};

inline Style makeStyle(bool dark, int part, const text::Fonts& fonts) {
	Style s;
	s.pal = dark ? tokens::darkPalette() : tokens::lightPalette();
	s.accent = tokens::accent(part);
	s.fonts = fonts;
	s.dark = dark;
	s.part = part;
	return s;
}

} // namespace atlas
