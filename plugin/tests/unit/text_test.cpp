// Text engine: markdown subset, line breaking, glue rules, pagination.
#include "test.hpp"
#include "text/Layout.hpp"
#include "text/NvgText.hpp"
#include "SoftNvg.hpp"
#include "HarnessFonts.hpp"

using namespace atlas::text;

namespace {
/** Every character is 6 px wide (bold 7), spaces 3 px: deterministic line breaks. */
struct FixedMeasurer : Measurer {
	float advance(FontId f, float, const std::string& s) override {
		float w = 0.f;
		for (size_t i = 0; i < s.size(); i = utf8Next(s, i))
			w += s[i] == ' ' ? 3.f : (f == F_BOLD ? 7.f : 6.f);
		return w;
	}
	void metrics(FontId, float size, float* a, float* d) override {
		*a = size * 0.8f;
		*d = size * 0.2f;
	}
};

std::string lineText(const Line& l) {
	std::string s;
	for (const Run& r : l.runs)
		s += r.text;
	return s;
}

std::vector<std::string> lines(const Document& d) {
	std::vector<std::string> out;
	for (const LaidBlock& b : d.blocks)
		for (const Line& l : b.lines)
			out.push_back(lineText(l));
	return out;
}
} // namespace

TEST(markdown_blocks) {
	auto b = parse("# Заголовок\nАбзац один.\n\n- пункт\n2. второй\nещё абзац");
	CHECK_EQ((int) b.size(), 5);
	CHECK(b[0].kind == BLOCK_HEADING);
	CHECK_EQ(b[0].spans[0].text, std::string("Заголовок"));
	CHECK(b[2].kind == BLOCK_BULLET);
	CHECK(b[3].kind == BLOCK_NUMBERED);
	CHECK_EQ(b[3].number, 2);
	CHECK(b[4].kind == BLOCK_PARAGRAPH);
}

TEST(markdown_inline) {
	auto s = parseInline("**Frequency** (частота) и *курсив*, `440 Гц`, [[огибающая|огибающей]] и [[VCA]]");
	CHECK_EQ(s[0].text, std::string("Frequency"));
	CHECK(s[0].style == STYLE_BOLD);
	CHECK_EQ(s[1].text, std::string(" (частота) и "));
	CHECK(s[2].style == STYLE_ITALIC);
	bool mono = false, term = false, term2 = false;
	for (auto& x : s) {
		if (x.style == STYLE_MONO && x.text == "440 Гц")
			mono = true;
		if (x.term == "огибающая" && x.text == "огибающей")
			term = true;
		if (x.term == "VCA" && x.text == "VCA")
			term2 = true;
	}
	CHECK(mono);
	CHECK(term);
	CHECK(term2);
}

TEST(markdown_unclosed_is_literal) {
	auto s = parseInline("5 * 3 и **жирный без конца");
	std::string all;
	for (auto& x : s) {
		all += x.text;
		CHECK(x.style == STYLE_REGULAR);
	}
	CHECK_EQ(all, std::string("5 * 3 и **жирный без конца"));
}

TEST(markdown_plain_and_terms) {
	CHECK_EQ(plain("# Т\n- **a** b\n1. `c`"), std::string("Т\n— a b\n1. c"));
	auto t = termsOf("[[гейт]] и [[огибающая|огибающей]]");
	CHECK_EQ((int) t.size(), 2);
	CHECK_EQ(t[1], std::string("огибающая"));
}

TEST(layout_greedy_breaks) {
	FixedMeasurer m;
	TextStyle st;
	// «aaaa bbbb cccc» = 4*6 + 3 + 4*6 = 51 for two words; width 60 → 2 words per line
	Document d = layout(parse("aaaa bbbb cccc dddd"), 60, m, st);
	auto l = lines(d);
	CHECK_EQ((int) l.size(), 2);
	CHECK_EQ(l[0], std::string("aaaa bbbb"));
	CHECK_EQ(l[1], std::string("cccc dddd"));
	for (auto& b : d.blocks)
		for (auto& ln : b.lines)
			CHECK(ln.width <= 60.f);
}

TEST(layout_glue_short_words_dashes_units) {
	FixedMeasurer m;
	TextStyle st;
	// «в» sticks to the next word; «—» to the previous; «440 Гц» stays together.
	Document d = layout(parse("aaaaaaa в bbbbbb — cc 440 Гц"), 60, m, st);
	auto l = lines(d);
	for (auto& s : l) {
		CHECK(s != "в");
		CHECK(s.rfind("—", 0) != 0);
		CHECK(s.size() < 3 || s.substr(s.size() - 3) != "440");
		CHECK(s.rfind("Гц", 0) != 0);
	}
	CHECK_EQ(l[0], std::string("aaaaaaa"));
	CHECK_EQ(l[1], std::string("в bbbbbb —"));
}

TEST(layout_mono_value_not_broken) {
	FixedMeasurer m;
	TextStyle st;
	Document d = layout(parse("aaaaa `12 34 56` bb"), 70, m, st);
	bool found = false;
	for (auto& s : lines(d))
		if (s.find("12 34 56") != std::string::npos)
			found = true;
	CHECK(found);
}

TEST(layout_long_word_hyphen) {
	FixedMeasurer m;
	TextStyle st;
	Document d = layout(parse("Сверхдлинноесловобезпробелов"), 60, m, st);
	CHECK(d.splitWords);
	auto l = lines(d);
	CHECK((int) l.size() >= 3);
	CHECK(l[0].substr(l[0].size() - 1) == "-");
	for (auto& b : d.blocks)
		for (auto& ln : b.lines)
			CHECK(ln.width <= 60.f + 0.01f);
}

TEST(layout_list_indent_marker) {
	FixedMeasurer m;
	TextStyle st;
	Document d = layout(parse("- пункт один два три четыре\n3. номер"), 80, m, st);
	CHECK_EQ(d.blocks[0].lines[0].marker, std::string("•"));
	CHECK(d.blocks[0].lines.size() >= 2);
	CHECK_EQ(d.blocks[0].lines[1].marker, std::string(""));
	CHECK(d.blocks[0].lines[1].runs[0].x >= st.listIndent);
	CHECK_EQ(d.blocks[1].lines[0].marker, std::string("3."));
}

TEST(paginate_blocks_not_split_and_heading_kept) {
	FixedMeasurer m;
	TextStyle st;
	float lh = st.bodySize * st.lineHeight;
	// 3 paragraphs of 2 lines each, page fits ~5 lines → paragraph 3 moves whole.
	std::string md = "aaaa bbbb cccc dddd\neeee ffff gggg hhhh\n# Head\niiii jjjj kkkk llll";
	Document d = layout(parse(md), 60, m, st);
	auto p = paginate(d, lh * 5 + st.paragraphGap, st);
	CHECK_EQ((int) p.size(), 2);
	// heading must be on page 2 with its paragraph
	CHECK_EQ(lineText(p[1].lines[0]), std::string("Head"));
	for (auto& pg : p)
		CHECK(pg.height <= lh * 5 + st.paragraphGap + 0.01f);
}

TEST(paginate_tall_paragraph_splits_at_lines) {
	FixedMeasurer m;
	TextStyle st;
	float lh = st.bodySize * st.lineHeight;
	std::string md;
	for (int i = 0; i < 12; i++)
		md += "aaaa bbbb ";
	Document d = layout(parse(md), 60, m, st);  // 12 lines
	auto p = paginate(d, lh * 5, st);
	CHECK_EQ((int) p.size(), 3);
	CHECK_EQ((int) p[0].lines.size(), 5);
	CHECK_EQ((int) p[2].lines.size(), 2);
	for (auto& pg : p)
		CHECK(!pg.overflow);
}

TEST(nanovg_measure_no_line_over_width) {
	NVGcontext* vg = softCreate(400, 400);
	Fonts f = harnessFonts(vg);
	CHECK(f.ok());
	NvgMeasurer m(vg, f);
	TextStyle st;
	std::string md = "# Четыре формы волны\nПеред тобой **VCO**, подключённый к **Scope** и к выходу. Сейчас на экране **пила** (Sawtooth).\nПереключи кабель на другие выходы VCO: `Sine`, `Triangle`, `Square`. Высота тона не меняется — меняется [[тембр]].\n- Частота — высота тона → период.\n- Амплитуда — громкость, ±5 В.";
	for (float w : {120.f, 200.f, 296.f, 420.f}) {
		Document d = layout(parse(md), w, m, st);
		for (auto& b : d.blocks)
			for (auto& ln : b.lines)
				CHECK(ln.width <= w + 0.01f);
	}
	softDelete(vg);
}
