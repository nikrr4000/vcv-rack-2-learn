// Markdown subset for all Atlas text (plan §6.3.3). No Rack dependency.
//
//   # heading      - item      1. item      any other line: a paragraph
//   **bold**  *italic*  `value` (mono)  [[термин]]  [[термин|форма]]
//
// Every non-empty line is its own block (as in the lesson templates); blank lines are
// ignored. Unclosed markers are kept as literal text. Nothing else is markup.
#pragma once
#include <string>
#include <vector>

namespace atlas {
namespace text {

enum SpanStyle { STYLE_REGULAR = 0, STYLE_BOLD, STYLE_ITALIC, STYLE_MONO };

struct Span {
	std::string text;
	SpanStyle style = STYLE_REGULAR;
	/** Glossary key for [[термин]] spans (the text shown may be an inflected form). */
	std::string term;
};

enum BlockKind { BLOCK_PARAGRAPH, BLOCK_HEADING, BLOCK_BULLET, BLOCK_NUMBERED };

struct Block {
	BlockKind kind = BLOCK_PARAGRAPH;
	int number = 0;  // for BLOCK_NUMBERED
	std::vector<Span> spans;
};

std::vector<Block> parse(const std::string& markdown);

/** Inline markup only (one paragraph). */
std::vector<Span> parseInline(const std::string& line);

/** Plain text of a document: markup removed, blocks joined by newlines. */
std::string plain(const std::string& markdown);

/** Glossary terms used in a document, in order (keys, duplicates kept). */
std::vector<std::string> termsOf(const std::string& markdown);

} // namespace text
} // namespace atlas
