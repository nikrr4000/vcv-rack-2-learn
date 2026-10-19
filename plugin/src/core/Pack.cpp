#include "Pack.hpp"

namespace atlas {

namespace {

std::string str(json_t* o, const char* k) {
	json_t* v = json_object_get(o, k);
	return v && json_is_string(v) ? json_string_value(v) : "";
}

int integer(json_t* o, const char* k, int def = 0) {
	json_t* v = json_object_get(o, k);
	return v && json_is_number(v) ? (int) json_number_value(v) : def;
}

int64_t id64(json_t* v) {
	if (!v)
		return -1;
	if (json_is_integer(v))
		return (int64_t) json_integer_value(v);
	if (json_is_real(v))
		return (int64_t) json_real_value(v);
	if (json_is_string(v))
		return (int64_t) std::stoll(json_string_value(v));
	return -1;
}

float real(json_t* o, const char* k, float def = 0.f) {
	json_t* v = json_object_get(o, k);
	return v && json_is_number(v) ? (float) json_number_value(v) : def;
}

bool boolean(json_t* o, const char* k) {
	json_t* v = json_object_get(o, k);
	return v && json_is_true(v);
}

std::vector<int64_t> ids(json_t* a) {
	std::vector<int64_t> out;
	size_t i;
	json_t* v;
	json_array_foreach(a, i, v) out.push_back(id64(v));
	return out;
}

Item parseItem(const std::string& id, json_t* j) {
	Item it;
	it.id = id;
	it.kind = str(j, "kind");
	it.text = str(j, "text");
	it.title = boolean(j, "title");
	it.caption = str(j, "caption");
	it.mode = str(j, "mode");
	it.side = str(j, "side");
	it.width = integer(j, "width");
	it.figure = json_object_get(j, "figure");
	it.check = json_object_get(j, "check");
	if (it.check && json_is_null(it.check))
		it.check = nullptr;
	size_t i;
	json_t* v;
	json_array_foreach(json_object_get(j, "targets"), i, v) {
		Target t;
		t.moduleId = id64(json_object_get(v, "moduleId"));
		t.module = str(v, "module");
		t.kind = str(v, "kind");
		t.index = integer(v, "index", -1);
		t.name = str(v, "name");
		it.targets.push_back(t);
	}
	json_array_foreach(json_object_get(j, "questions"), i, v) {
		Question q;
		q.q = str(v, "q");
		q.free = boolean(v, "free");
		size_t k;
		json_t* o;
		json_array_foreach(json_object_get(v, "options"), k, o) {
			Option op;
			op.text = str(o, "text");
			op.correct = boolean(o, "correct");
			op.why = str(o, "why");
			q.options.push_back(op);
		}
		it.questions.push_back(q);
	}
	json_array_foreach(json_object_get(j, "steps"), i, v) {
		Step s;
		s.doText = str(v, "do");
		s.see = str(v, "see");
		it.steps.push_back(s);
	}
	const char* key;
	json_object_foreach(json_object_get(j, "hints"), key, v) {
		std::vector<std::string> ladder;
		if (json_is_string(v))
			ladder.push_back(json_string_value(v));
		size_t k;
		json_t* h;
		json_array_foreach(v, k, h) if (json_is_string(h)) ladder.push_back(json_string_value(h));
		it.hints[key] = ladder;
	}
	it.given = ids(json_object_get(j, "given"));
	it.scope = ids(json_object_get(j, "scope"));
	return it;
}

} // namespace

const Block* Lesson::block(const std::string& id) const {
	for (const Block& b : blocks)
		if (b.id == id)
			return &b;
	return nullptr;
}

int Lesson::blockIndex(const std::string& id) const {
	for (size_t i = 0; i < blocks.size(); i++)
		if (blocks[i].id == id)
			return (int) i;
	return -1;
}

Pack::~Pack() {
	if (root)
		json_decref(root);
}

bool Pack::loadFile(const std::string& path) {
	json_error_t err;
	json_t* r = json_load_file(path.c_str(), 0, &err);
	if (!r) {
		error = std::string("pack.json: ") + err.text + " (line " + std::to_string(err.line) + ")";
		return false;
	}
	return loadJson(r);
}

bool Pack::loadJson(json_t* r) {
	if (root)
		json_decref(root);
	root = r;
	parts.clear();
	lessons.clear();
	glossary.clear();
	json_t* course = json_object_get(r, "course");
	title = str(course, "title");
	size_t i;
	json_t* v;
	json_array_foreach(json_object_get(course, "parts"), i, v) {
		Part p;
		p.n = integer(v, "n");
		p.title = str(v, "title");
		size_t k;
		json_t* l;
		json_array_foreach(json_object_get(v, "lessons"), k, l) p.lessons.push_back(json_string_value(l));
		parts.push_back(p);
	}
	const char* key;
	json_object_foreach(json_object_get(r, "glossary"), key, v) glossary[key] = json_is_string(v) ? json_string_value(v) : "";
	json_object_foreach(json_object_get(r, "lessons"), key, v) {
		Lesson L;
		L.id = key;
		L.num = str(v, "num");
		L.title = str(v, "title");
		L.goal = str(v, "goal");
		L.builds = str(v, "builds");
		L.next = str(v, "next");
		L.part = integer(v, "part");
		L.minutes = integer(v, "minutes");
		L.patch = str(v, "patch");
		L.layoutHash = str(v, "layoutHash");
		size_t k;
		json_t* x;
		json_array_foreach(json_object_get(v, "requires"), k, x) L.requires.push_back(json_string_value(x));
		json_array_foreach(json_object_get(v, "blocks"), k, x) {
			Block b;
			b.id = str(x, "id");
			b.type = str(x, "type");
			b.title = str(x, "title");
			b.index = (int) k;
			b.row = integer(x, "row", (int) k + 1);
			b.simpler = str(x, "simpler");
			b.plain = str(x, "plain");
			json_t* bounds = json_object_get(x, "bounds");
			b.x = real(bounds, "x");
			b.y = real(bounds, "y");
			b.w = real(bounds, "w");
			b.h = real(bounds, "h");
			size_t c;
			json_t* ch;
			json_array_foreach(json_object_get(x, "channels"), c, ch) {
				Block::Channel bc;
				bc.mixerId = id64(json_object_get(ch, "mixerId"));
				bc.param = integer(ch, "param", -1);
				bc.level = real(ch, "level", 1.f);
				b.channels.push_back(bc);
			}
			size_t m;
			json_t* f;
			json_array_foreach(json_object_get(x, "faq"), m, f) b.faq.push_back({str(f, "q"), str(f, "a")});
			// Items keep the compiler's order (an array of {id, ...}).
			json_array_foreach(json_object_get(x, "items"), m, f) b.items.push_back(parseItem(str(f, "id"), f));
			L.blocks.push_back(b);
		}
		lessons[key] = L;
	}
	return true;
}

const Lesson* Pack::lesson(const std::string& id) const {
	auto it = lessons.find(id);
	return it == lessons.end() ? nullptr : &it->second;
}

const Item* Pack::item(const std::string& lesson, const std::string& block, const std::string& item) const {
	const Lesson* l = this->lesson(lesson);
	if (!l)
		return nullptr;
	const Block* b = l->block(block);
	if (!b)
		return nullptr;
	for (const Item& it : b->items)
		if (it.id == item)
			return &it;
	return nullptr;
}

std::vector<std::string> Pack::order() const {
	std::vector<std::string> out;
	for (const Part& p : parts)
		for (const std::string& l : p.lessons)
			if (lessons.count(l))
				out.push_back(l);
	return out;
}

std::string Pack::nextLesson(const std::string& id) const {
	std::vector<std::string> o = order();
	for (size_t i = 0; i + 1 < o.size(); i++)
		if (o[i] == id)
			return o[i + 1];
	return "";
}

std::string Pack::definition(const std::string& term) const {
	auto it = glossary.find(term);
	return it == glossary.end() ? "" : it->second;
}

std::string blockHeader(const Lesson* l, const Block* b) {
	if (!l)
		return "";
	std::string s = l->num;
	if (b)
		s += " · Блок " + std::to_string(b->index + 1);
	return s;
}

std::string partHeader(const Pack& pack, const Lesson* l) {
	if (!l)
		return "";
	for (const Part& p : pack.parts)
		if (p.n == l->part)
			return "Часть " + std::to_string(p.n) + " · " + p.title;
	return "Часть " + std::to_string(l->part);
}

} // namespace atlas
