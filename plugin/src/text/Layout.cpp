#include "Layout.hpp"

#include <algorithm>
#include <cctype>

namespace atlas {
namespace text {

size_t utf8Next(const std::string& s, size_t i) {
	if (i >= s.size())
		return s.size();
	unsigned char c = (unsigned char) s[i];
	size_t n = c < 0x80 ? 1 : (c >> 5) == 0x6 ? 2 : (c >> 4) == 0xE ? 3 : (c >> 3) == 0x1E ? 4 : 1;
	return std::min(s.size(), i + n);
}

int utf8Length(const std::string& s) {
	int n = 0;
	for (size_t i = 0; i < s.size(); i = utf8Next(s, i))
		n++;
	return n;
}

namespace {

struct Piece {
	std::string text;
	FontId font = F_BODY;
	float size = 12.f;
	std::string term;
	bool space = false;
	float width = 0.f;
};

struct Atom {
	std::vector<Piece> parts;
	float width = 0.f;
	Piece spaceBefore;  // the breakable space in front of this atom (if hasSpace)
	bool hasSpace = false;
	std::string textOf() const {
		std::string t;
		for (const Piece& p : parts)
			t += p.text;
		return t;
	}
};

FontId fontFor(SpanStyle s) {
	switch (s) {
		case STYLE_BOLD: return F_BOLD;
		case STYLE_ITALIC: return F_ITALIC;
		case STYLE_MONO: return F_MONO;
		default: return F_BODY;
	}
}

bool isDash(const std::string& t) {
	return t == "—" || t == "–" || t == "→";
}

/** A one-letter word (Russian preposition or conjunction such as «в», «и», «с»). */
bool isShortWord(const std::string& t) {
	if (utf8Length(t) != 1)
		return false;
	unsigned char c = (unsigned char) t[0];
	if (c < 0x80)
		return std::isalpha(c) != 0;
	return c == 0xD0 || c == 0xD1;  // a Cyrillic letter
}

std::vector<Atom> atomize(const Block& b, Measurer& m, const TextStyle& st) {
	std::vector<Atom> atoms;
	Atom cur;
	bool pendingSpace = false;
	Piece space;
	auto flush = [&]() {
		if (cur.parts.empty())
			return;
		atoms.push_back(cur);
		cur = Atom();
	};
	auto addText = [&](const std::string& t, FontId f, float size, const std::string& term) {
		if (t.empty())
			return;
		if (pendingSpace) {
			flush();
			cur.hasSpace = !atoms.empty();
			cur.spaceBefore = space;
			pendingSpace = false;
		}
		if (!cur.parts.empty() && cur.parts.back().font == f && cur.parts.back().term == term && !cur.parts.back().space)
			cur.parts.back().text += t;
		else {
			Piece p;
			p.text = t;
			p.font = f;
			p.size = size;
			p.term = term;
			cur.parts.push_back(p);
		}
	};
	for (const Span& sp : b.spans) {
		FontId f = b.kind == BLOCK_HEADING ? F_HEADING : fontFor(sp.style);
		float size = b.kind == BLOCK_HEADING ? st.headingSize : sp.style == STYLE_MONO ? st.monoSize : st.bodySize;
		if (sp.style == STYLE_MONO && b.kind != BLOCK_HEADING && utf8Length(sp.text) <= 12) {
			// A short value («440 Гц») is never broken across lines; long names may break at spaces.
			addText(sp.text, f, size, sp.term);
			continue;
		}
		std::string word;
		for (size_t i = 0; i < sp.text.size(); i++) {
			char c = sp.text[i];
			if (c == ' ' || c == '\t') {
				addText(word, f, size, sp.term);
				word.clear();
				if (!pendingSpace) {
					pendingSpace = true;
					space = Piece();
					space.text = " ";
					space.font = f;
					space.size = size;
					space.term = sp.term;
					space.space = true;
				}
			}
			else {
				word += c;
			}
		}
		addText(word, f, size, sp.term);
	}
	flush();

	// Glue: a dash or arrow sticks to the word before it; a one-letter word to the word after it.
	std::vector<Atom> glued;
	for (size_t i = 0; i < atoms.size(); i++) {
		Atom a = atoms[i];
		if (!glued.empty() && a.hasSpace && isDash(a.textOf())) {
			Atom& prev = glued.back();
			prev.parts.push_back(a.spaceBefore);
			prev.parts.insert(prev.parts.end(), a.parts.begin(), a.parts.end());
			continue;
		}
		glued.push_back(a);
	}
	// A one-letter word sticks to the next word; a number sticks to a short unit after it («440 Гц»).
	auto lastWord = [](const Atom& a) {
		std::string t;
		for (auto it = a.parts.rbegin(); it != a.parts.rend(); ++it) {
			if (it->space)
				break;
			t = it->text + t;
		}
		return t;
	};
	auto isNumber = [](const std::string& t) {
		if (t.empty())
			return false;
		for (char c : t)
			if (!std::isdigit((unsigned char) c) && c != ',' && c != '.' && c != '-' && c != '+')
				return false;
		return std::isdigit((unsigned char) t.back()) != 0;
	};
	std::vector<Atom> out;
	for (size_t i = 0; i < glued.size(); i++) {
		Atom a = glued[i];
		while (i + 1 < glued.size() && glued[i + 1].hasSpace) {
			std::string last = lastWord(a);
			const Atom& next = glued[i + 1];
			std::string nt = next.textOf();
			bool unit = isNumber(last) && utf8Length(nt) <= 4 && !isNumber(nt);
			// A panel name and its number stay together: «Ch 1», «Gain 2».
			bool latin = !last.empty() && (unsigned char) last.back() < 0x80 && std::isalpha((unsigned char) last.back());
			bool index = latin && !nt.empty() && nt.size() <= 3 && std::isdigit((unsigned char) nt[0]);
			if (!isShortWord(last) && !unit && !index)
				break;
			a.parts.push_back(next.spaceBefore);
			a.parts.insert(a.parts.end(), next.parts.begin(), next.parts.end());
			i++;
		}
		out.push_back(a);
	}
	for (Atom& a : out) {
		a.width = 0.f;
		for (Piece& p : a.parts) {
			p.width = m.advance(p.font, p.size, p.text);
			a.width += p.width;
		}
		if (a.hasSpace)
			a.spaceBefore.width = m.advance(a.spaceBefore.font, a.spaceBefore.size, " ");
	}
	return out;
}

/** Splits an atom wider than `w` into atoms that fit, each but the last ending in a hyphen. */
std::vector<Atom> splitAtom(const Atom& a, float w, Measurer& m) {
	std::vector<Atom> out;
	Atom cur;
	float curW = 0.f;
	for (const Piece& p : a.parts) {
		size_t i = 0;
		while (i < p.text.size()) {
			size_t j = utf8Next(p.text, i);
			std::string ch = p.text.substr(i, j - i);
			float chW = m.advance(p.font, p.size, ch);
			float hyW = m.advance(p.font, p.size, "-");
			if (!cur.parts.empty() && curW + chW + hyW > w) {
				Piece hy;
				hy.text = "-";
				hy.font = p.font;
				hy.size = p.size;
				hy.term = p.term;
				hy.width = hyW;
				cur.parts.push_back(hy);
				cur.width = curW + hyW;
				out.push_back(cur);
				cur = Atom();
				curW = 0.f;
			}
			if (!cur.parts.empty() && cur.parts.back().font == p.font && cur.parts.back().term == p.term && !cur.parts.back().space) {
				cur.parts.back().text += ch;
				cur.parts.back().width += chW;
			}
			else {
				Piece q = p;
				q.text = ch;
				q.width = chW;
				cur.parts.push_back(q);
			}
			curW += chW;
			i = j;
		}
	}
	if (!cur.parts.empty()) {
		cur.width = curW;
		out.push_back(cur);
	}
	if (!out.empty()) {
		out[0].hasSpace = a.hasSpace;
		out[0].spaceBefore = a.spaceBefore;
	}
	return out;
}

void finishLine(Line& line, const std::vector<Piece>& pieces, Measurer& m, float lineHeight) {
	// Merge pieces into runs; drop leading and trailing spaces.
	size_t a = 0, b = pieces.size();
	while (a < b && pieces[a].space)
		a++;
	while (b > a && pieces[b - 1].space)
		b--;
	float x = 0.f;
	float maxSize = 0.f;
	FontId maxFont = F_BODY;
	for (size_t i = a; i < b; i++) {
		const Piece& p = pieces[i];
		if (!line.runs.empty() && line.runs.back().font == p.font && line.runs.back().size == p.size && line.runs.back().term == p.term) {
			line.runs.back().text += p.text;
			line.runs.back().width += p.width;
		}
		else {
			Run r;
			r.font = p.font;
			r.size = p.size;
			r.x = x;
			r.width = p.width;
			r.text = p.text;
			r.term = p.term;
			line.runs.push_back(r);
		}
		x += p.width;
		if (p.size > maxSize) {
			maxSize = p.size;
			maxFont = p.font;
		}
	}
	line.width = x;
	if (maxSize <= 0.f)
		maxSize = 12.f;
	line.height = lineHeight;
	float asc = maxSize * 0.8f, desc = maxSize * 0.2f;
	m.metrics(maxFont, maxSize, &asc, &desc);
	line.baseline = (lineHeight - (asc + desc)) / 2.f + asc;
}

} // namespace

Document layout(const std::vector<Block>& blocks, float width, Measurer& m, const TextStyle& st) {
	Document doc;
	doc.width = width;
	for (size_t bi = 0; bi < blocks.size(); bi++) {
		const Block& b = blocks[bi];
		LaidBlock lb;
		lb.kind = b.kind;
		bool list = b.kind == BLOCK_BULLET || b.kind == BLOCK_NUMBERED;
		float indent = list ? st.listIndent : 0.f;
		float w = std::max(10.f, width - indent);
		float lh = b.kind == BLOCK_HEADING ? st.headingSize * st.headingLineHeight : st.bodySize * st.lineHeight;
		std::vector<Atom> atoms = atomize(b, m, st);
		std::vector<Atom> fitted;
		for (const Atom& a : atoms) {
			if (a.width > w) {
				std::vector<Atom> parts = splitAtom(a, w, m);
				fitted.insert(fitted.end(), parts.begin(), parts.end());
				doc.splitWords = true;
			}
			else
				fitted.push_back(a);
		}
		std::vector<std::vector<Piece>> linePieces(1);
		float x = 0.f;
		for (const Atom& a : fitted) {
			std::vector<Piece>& cur = linePieces.back();
			if (cur.empty()) {
				cur.insert(cur.end(), a.parts.begin(), a.parts.end());
				x = a.width;
				continue;
			}
			float sw = a.hasSpace ? a.spaceBefore.width : 0.f;
			if (x + sw + a.width <= w + 0.01f) {
				if (a.hasSpace)
					cur.push_back(a.spaceBefore);
				cur.insert(cur.end(), a.parts.begin(), a.parts.end());
				x += sw + a.width;
			}
			else {
				linePieces.push_back(std::vector<Piece>(a.parts.begin(), a.parts.end()));
				x = a.width;
			}
		}
		float top = 0.f;
		for (size_t li = 0; li < linePieces.size(); li++) {
			Line line;
			finishLine(line, linePieces[li], m, lh);
			for (Run& r : line.runs)
				r.x += indent;
			line.width += indent;
			line.top = top;
			line.block = (int) bi;
			line.blockStart = li == 0;
			if (li == 0 && list) {
				line.marker = b.kind == BLOCK_BULLET ? "•" : std::to_string(b.number) + ".";
				line.markerFont = b.kind == BLOCK_BULLET ? F_BODY : F_MONO;
			}
			top += lh;
			lb.lines.push_back(line);
		}
		lb.height = top + (b.kind == BLOCK_HEADING ? st.headingGapAfter : 0.f);
		doc.blocks.push_back(lb);
	}
	return doc;
}

static float gapBetween(BlockKind prev, BlockKind next, const TextStyle& st) {
	bool prevList = prev == BLOCK_BULLET || prev == BLOCK_NUMBERED;
	bool nextList = next == BLOCK_BULLET || next == BLOCK_NUMBERED;
	if (prevList && nextList)
		return st.paragraphGap * 0.45f;
	return st.paragraphGap;
}

std::vector<Page> paginate(const Document& doc, float pageHeight, const TextStyle& st) {
	std::vector<Page> pages(1);
	float y = 0.f;
	int lastKind = -1;
	auto place = [&](const LaidBlock& b, size_t from, size_t to, float at) {
		for (size_t i = from; i < to; i++) {
			Line l = b.lines[i];
			l.top = at + (l.top - b.lines[from].top);
			pages.back().lines.push_back(l);
			if (l.height > pageHeight)
				pages.back().overflow = true;
		}
	};
	for (size_t bi = 0; bi < doc.blocks.size(); bi++) {
		const LaidBlock& b = doc.blocks[bi];
		if (b.lines.empty())
			continue;
		float gap = pages.back().lines.empty() ? 0.f : gapBetween((BlockKind) lastKind, b.kind, st);
		if (y + gap + b.height <= pageHeight + 0.01f) {
			place(b, 0, b.lines.size(), y + gap);
			y += gap + b.height;
		}
		else if (b.height <= pageHeight + 0.01f) {
			// Keep a heading with the block that follows it.
			std::vector<Line> carry;
			if (lastKind == BLOCK_HEADING && !pages.back().lines.empty()) {
				int hb = pages.back().lines.back().block;
				auto& ls = pages.back().lines;
				size_t k = ls.size();
				while (k > 0 && ls[k - 1].block == hb)
					k--;
				if (k > 0) {
					carry.assign(ls.begin() + k, ls.end());
					ls.erase(ls.begin() + k, ls.end());
				}
			}
			pages.back().height = y;
			pages.push_back(Page());
			y = 0.f;
			if (!carry.empty()) {
				float base = carry.front().top;
				for (Line l : carry) {
					l.top -= base;
					pages.back().lines.push_back(l);
				}
				const LaidBlock& hb = doc.blocks[carry.front().block];
				y = hb.height;
				gap = gapBetween(BLOCK_HEADING, b.kind, st);
			}
			else
				gap = 0.f;
			if (y + gap + b.height > pageHeight + 0.01f && !carry.empty()) {
				// heading + block do not fit together: block alone continues below anyway
			}
			place(b, 0, b.lines.size(), y + gap);
			y += gap + b.height;
		}
		else {
			// Taller than a page: split at lines.
			size_t i = 0;
			float at = y + gap;
			while (i < b.lines.size()) {
				size_t j = i;
				float h = 0.f;
				while (j < b.lines.size() && at + h + b.lines[j].height <= pageHeight + 0.01f) {
					h += b.lines[j].height;
					j++;
				}
				if (j == i) {
					if (pages.back().lines.empty()) {
						j = i + 1;  // a single line taller than the page
						h = b.lines[i].height;
					}
					else {
						pages.back().height = y;
						pages.push_back(Page());
						y = 0.f;
						at = 0.f;
						continue;
					}
				}
				place(b, i, j, at);
				y = at + h;
				i = j;
				if (i < b.lines.size()) {
					pages.back().height = y;
					pages.push_back(Page());
					y = 0.f;
					at = 0.f;
				}
			}
		}
		lastKind = b.kind;
	}
	pages.back().height = y;
	if (pages.size() > 1 && pages.back().lines.empty())
		pages.pop_back();
	return pages;
}

Page flatten(const Document& doc, const TextStyle& st) {
	Page p;
	float y = 0.f;
	int lastKind = -1;
	for (const LaidBlock& b : doc.blocks) {
		if (b.lines.empty())
			continue;
		float gap = p.lines.empty() ? 0.f : gapBetween((BlockKind) lastKind, b.kind, st);
		for (Line l : b.lines) {
			l.top += y + gap;
			p.lines.push_back(l);
		}
		y += gap + b.height;
		lastKind = b.kind;
	}
	p.height = y;
	return p;
}

std::vector<Page> pages(const std::string& markdown, float width, float pageHeight, Measurer& m, const TextStyle& st) {
	return paginate(layout(parse(markdown), width, m, st), pageHeight, st);
}

} // namespace text
} // namespace atlas
