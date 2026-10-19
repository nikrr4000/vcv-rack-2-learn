#include "Markdown.hpp"

#include <cctype>

namespace atlas {
namespace text {

static void push(std::vector<Span>& out, std::string& buf, SpanStyle style, const std::string& term = "") {
	if (buf.empty())
		return;
	if (!out.empty() && term.empty() && out.back().term.empty() && out.back().style == style) {
		out.back().text += buf;
	}
	else {
		Span s;
		s.text = buf;
		s.style = style;
		s.term = term;
		out.push_back(s);
	}
	buf.clear();
}

std::vector<Span> parseInline(const std::string& line) {
	std::vector<Span> out;
	std::string buf;
	bool bold = false, italic = false;
	auto style = [&]() { return bold ? STYLE_BOLD : italic ? STYLE_ITALIC : STYLE_REGULAR; };
	size_t i = 0;
	const size_t n = line.size();
	while (i < n) {
		char c = line[i];
		if (c == '`') {
			size_t end = line.find('`', i + 1);
			if (end != std::string::npos) {
				push(out, buf, style());
				std::string v = line.substr(i + 1, end - i - 1);
				push(out, v, STYLE_MONO);
				i = end + 1;
				continue;
			}
		}
		else if (c == '[' && i + 1 < n && line[i + 1] == '[') {
			size_t end = line.find("]]", i + 2);
			if (end != std::string::npos) {
				push(out, buf, style());
				std::string inner = line.substr(i + 2, end - i - 2);
				std::string key = inner, shown = inner;
				size_t bar = inner.find('|');
				if (bar != std::string::npos) {
					key = inner.substr(0, bar);
					shown = inner.substr(bar + 1);
				}
				push(out, shown, style(), key);
				i = end + 2;
				continue;
			}
		}
		else if (c == '*' && i + 1 < n && line[i + 1] == '*') {
			if (bold || line.find("**", i + 2) != std::string::npos) {
				push(out, buf, style());
				bold = !bold;
				i += 2;
				continue;
			}
		}
		else if (c == '*') {
			// A single star: italic when a closing single star exists (not part of **).
			bool closes = false;
			if (italic) {
				closes = true;
			}
			else {
				for (size_t j = i + 1; j < n; j++) {
					if (line[j] == '*') {
						if (j + 1 < n && line[j + 1] == '*') {
							j++;
							continue;
						}
						closes = true;
						break;
					}
				}
			}
			if (closes) {
				push(out, buf, style());
				italic = !italic;
				i += 1;
				continue;
			}
		}
		buf += c;
		i++;
	}
	push(out, buf, style());
	return out;
}

static std::string trim(const std::string& s) {
	size_t a = 0, b = s.size();
	while (a < b && (s[a] == ' ' || s[a] == '\t' || s[a] == '\r'))
		a++;
	while (b > a && (s[b - 1] == ' ' || s[b - 1] == '\t' || s[b - 1] == '\r'))
		b--;
	return s.substr(a, b - a);
}

std::vector<Block> parse(const std::string& markdown) {
	std::vector<Block> out;
	size_t start = 0;
	while (start <= markdown.size()) {
		size_t end = markdown.find('\n', start);
		if (end == std::string::npos)
			end = markdown.size();
		std::string line = trim(markdown.substr(start, end - start));
		start = end + 1;
		if (line.empty()) {
			if (end >= markdown.size())
				break;
			continue;
		}
		Block b;
		if (line.rfind("# ", 0) == 0 || line.rfind("## ", 0) == 0) {
			b.kind = BLOCK_HEADING;
			line = trim(line.substr(line.find(' ') + 1));
		}
		else if (line.rfind("- ", 0) == 0) {
			b.kind = BLOCK_BULLET;
			line = trim(line.substr(2));
		}
		else if (std::isdigit((unsigned char) line[0])) {
			size_t k = 0;
			while (k < line.size() && std::isdigit((unsigned char) line[k]))
				k++;
			if (k < line.size() - 1 && line[k] == '.' && line[k + 1] == ' ') {
				b.kind = BLOCK_NUMBERED;
				b.number = std::stoi(line.substr(0, k));
				line = trim(line.substr(k + 2));
			}
		}
		b.spans = parseInline(line);
		out.push_back(b);
		if (end >= markdown.size())
			break;
	}
	return out;
}

std::string plain(const std::string& markdown) {
	std::string out;
	for (const Block& b : parse(markdown)) {
		if (!out.empty())
			out += "\n";
		if (b.kind == BLOCK_BULLET)
			out += "— ";
		if (b.kind == BLOCK_NUMBERED)
			out += std::to_string(b.number) + ". ";
		for (const Span& s : b.spans)
			out += s.text;
	}
	return out;
}

std::vector<std::string> termsOf(const std::string& markdown) {
	std::vector<std::string> out;
	for (const Block& b : parse(markdown))
		for (const Span& s : b.spans)
			if (!s.term.empty())
				out.push_back(s.term);
	return out;
}

} // namespace text
} // namespace atlas
