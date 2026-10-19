#include "Progress.hpp"

#include <algorithm>

namespace atlas {

json_t* Progress::toJson() const {
	json_t* r = json_object();
	json_object_set_new(r, "last", json_string(last.c_str()));
	json_object_set_new(r, "updatedAt", json_real(updatedAt));
	json_t* ls = json_object();
	for (const auto& kv : lessons) {
		json_t* l = json_object();
		json_object_set_new(l, "status", json_string(kv.second.status.c_str()));
		json_object_set_new(l, "block", json_string(kv.second.block.c_str()));
		json_t* rs = json_array();
		for (const std::string& b : kv.second.reached)
			json_array_append_new(rs, json_string(b.c_str()));
		json_object_set_new(l, "reached", rs);
		json_object_set_new(l, "updatedAt", json_real(kv.second.updatedAt));
		json_object_set_new(ls, kv.first.c_str(), l);
	}
	json_object_set_new(r, "lessons", ls);
	return r;
}

static std::string s(json_t* o, const char* k) {
	json_t* v = json_object_get(o, k);
	return v && json_is_string(v) ? json_string_value(v) : "";
}

static double num(json_t* o, const char* k) {
	json_t* v = json_object_get(o, k);
	if (v && json_is_number(v))
		return json_number_value(v);
	return 0;
}

static LessonProgress parse(json_t* l) {
	LessonProgress p;
	p.status = s(l, "status");
	p.block = s(l, "block");
	p.updatedAt = num(l, "updatedAt");
	size_t i;
	json_t* v;
	json_array_foreach(json_object_get(l, "reached"), i, v) if (json_is_string(v)) p.reached.push_back(json_string_value(v));
	return p;
}

void Progress::fromJson(json_t* r) {
	lessons.clear();
	last = s(r, "last");
	updatedAt = num(r, "updatedAt");
	const char* key;
	json_t* v;
	json_object_foreach(json_object_get(r, "lessons"), key, v) lessons[key] = parse(v);
}

bool Progress::load(const std::string& path) {
	json_error_t err;
	json_t* r = json_load_file(path.c_str(), 0, &err);
	if (!r)
		return false;
	fromJson(r);
	json_decref(r);
	return true;
}

bool Progress::save(const std::string& path) const {
	json_t* r = toJson();
	int rc = json_dump_file(r, path.c_str(), JSON_INDENT(2));
	json_decref(r);
	return rc == 0;
}

bool Progress::merge(json_t* remote) {
	bool changed = false;
	const char* key;
	json_t* v;
	json_object_foreach(json_object_get(remote, "lessons"), key, v) {
		LessonProgress r = parse(v);
		auto it = lessons.find(key);
		if (it == lessons.end() || r.updatedAt > it->second.updatedAt) {
			// Never downgrade «learned» to an older status.
			if (it != lessons.end() && it->second.learned() && !r.learned())
				continue;
			lessons[key] = r;
			changed = true;
		}
	}
	return changed;
}

int Progress::learnedCount() const {
	int n = 0;
	for (const auto& kv : lessons)
		n += kv.second.learned();
	return n;
}

std::string Progress::status(const std::string& lesson) const {
	auto it = lessons.find(lesson);
	return it == lessons.end() ? "" : it->second.status;
}

std::string Progress::continueLesson(const Pack& pack) const {
	if (!last.empty() && pack.lesson(last) && status(last) != "learned")
		return last;
	for (const std::string& id : pack.order())
		if (status(id) != "learned")
			return id;
	std::vector<std::string> o = pack.order();
	return o.empty() ? "" : o.front();
}

void Progress::reach(const std::string& lesson, const std::string& block, double now) {
	LessonProgress& p = lessons[lesson];
	if (p.status.empty())
		p.status = "learning";
	p.block = block;
	if (std::find(p.reached.begin(), p.reached.end(), block) == p.reached.end())
		p.reached.push_back(block);
	p.updatedAt = now;
	last = lesson;
	updatedAt = now;
}

void Progress::complete(const std::string& lesson, double now) {
	LessonProgress& p = lessons[lesson];
	p.status = "learned";
	p.updatedAt = now;
	updatedAt = now;
}

} // namespace atlas
