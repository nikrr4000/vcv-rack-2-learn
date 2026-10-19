// Text layout: greedy line breaking by measured widths, pagination, scroll documents.
// No Rack dependency: widths come from a Measurer (nanovg in the plugin and in the
// render harness, opentype.js in the compiler's text-fit tool — same algorithm).
#pragma once
#include <string>
#include <vector>

#include "Markdown.hpp"

namespace atlas {
namespace text {

enum FontId { F_HEADING = 0, F_BODY, F_BOLD, F_ITALIC, F_MONO, F_COUNT };

struct Measurer {
	virtual ~Measurer() {}
	/** Horizontal advance of `s` in font `font` at `size` (Rack px at zoom 1). */
	virtual float advance(FontId font, float size, const std::string& s) = 0;
	/** Ascender and descender (both positive) of the font at `size`. */
	virtual void metrics(FontId font, float size, float* ascender, float* descender) = 0;
};

struct TextStyle {
	float bodySize = 12.f;
	float headingSize = 14.5f;
	float monoSize = 11.f;
	float lineHeight = 1.42f;         // body line height, × size
	float headingLineHeight = 1.25f;  // × headingSize
	float paragraphGap = 7.f;
	float headingGapAfter = 2.f;      // extra space under a heading
	float listIndent = 14.f;
};

struct Run {
	FontId font = F_BODY;
	float size = 12.f;
	float x = 0.f;      // from the left edge of the text box
	float width = 0.f;
	std::string text;
	std::string term;   // glossary key, empty if none
};

struct Line {
	float top = 0.f;       // from the top of the page / document
	float height = 0.f;
	float baseline = 0.f;  // from `top`
	float width = 0.f;     // used width
	std::vector<Run> runs;
	/** List marker («•» or «3.») drawn at x = 0 on the first line of an item; empty otherwise. */
	std::string marker;
	FontId markerFont = F_BODY;
	int block = 0;         // index of the source block
	bool blockStart = false;
};

struct LaidBlock {
	BlockKind kind = BLOCK_PARAGRAPH;
	std::vector<Line> lines;  // tops relative to the block top
	float height = 0.f;
};

struct Document {
	std::vector<LaidBlock> blocks;
	float width = 0.f;
	/** True when some word had to be split with a hyphen. */
	bool splitWords = false;
};

Document layout(const std::vector<Block>& blocks, float width, Measurer& m, const TextStyle& style);

struct Page {
	std::vector<Line> lines;  // tops relative to the page top
	float height = 0.f;       // used height
	/** A line taller than the page itself (cannot happen with sane sizes). */
	bool overflow = false;
};

/** Splits a document into pages of `pageHeight`. Blocks are not split unless one is taller
 * than a page; a heading never ends a page when the next block can follow it. */
std::vector<Page> paginate(const Document& doc, float pageHeight, const TextStyle& style);

/** The whole document as one tall page (scroll mode). */
Page flatten(const Document& doc, const TextStyle& style);

/** Convenience: parse + layout + paginate. */
std::vector<Page> pages(const std::string& markdown, float width, float pageHeight, Measurer& m, const TextStyle& style);

/** UTF-8 helpers. */
size_t utf8Next(const std::string& s, size_t i);
int utf8Length(const std::string& s);

} // namespace text
} // namespace atlas
