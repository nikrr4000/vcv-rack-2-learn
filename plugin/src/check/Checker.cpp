#include "Checker.hpp"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <map>
#include <set>

namespace atlas {
namespace check {

namespace {

const int MAX_ASSIGNMENTS = 20000;

std::string trim(const std::string& s) {
	size_t a = 0, b = s.size();
	while (a < b && std::isspace((unsigned char) s[a]))
		a++;
	while (b > a && std::isspace((unsigned char) s[b - 1]))
		b--;
	return s.substr(a, b - a);
}

std::string str(json_t* o, const char* k) {
	json_t* v = json_object_get(o, k);
	return v && json_is_string(v) ? json_string_value(v) : "";
}

std::string jsNum(double v) {
	char buf[64];
	if (v == std::floor(v) && std::fabs(v) < 1e15)
		std::snprintf(buf, sizeof(buf), "%.0f", v);
	else
		std::snprintf(buf, sizeof(buf), "%.12g", v);
	return buf;
}

/** Number.parseFloat of a display string («1,5» counts as 1.5); false when not a number. */
bool displayNumber(const std::string& s, double* out) {
	std::string t = s;
	for (char& c : t)
		if (c == ',') {
			c = '.';
			break;
		}
	const char* p = t.c_str();
	while (*p && std::isspace((unsigned char) *p))
		p++;
	if (*p == '0' && (p[1] == 'x' || p[1] == 'X')) {
		*out = 0;
		return true;  // parseFloat("0x…") reads the leading 0
	}
	char* end = nullptr;
	double v = std::strtod(p, &end);
	if (end == p || !std::isfinite(v))
		return false;
	*out = v;
	return true;
}

struct SModule {
	std::string ref, plugin, slug;
};
struct SCable {
	std::string fromRef, out, toRef, in;
};
struct SParam {
	std::string module, name;
	bool hasMin = false, hasMax = false, hasIn = false;
	double min = 0, max = 0;
	std::vector<std::string> in;
};
struct SProbe {
	std::string module, output, ask;
	int samples = 10, interval = 50;
	bool hasMaxGt = false, hasMinLt = false, hasRangeGt = false;
	double maxGt = 0, minLt = 0, rangeGt = 0;
};
struct Spec {
	bool fixed = false;
	std::vector<SModule> modules;
	std::vector<SCable> cables;
	std::vector<SParam> params;
	std::vector<SProbe> probes;
	std::map<std::string, std::string> hints;
};

SParam parseParam(json_t* p) {
	SParam s;
	s.module = str(p, "module");
	s.name = str(p, "name");
	json_t* v;
	if ((v = json_object_get(p, "display_min")) && json_is_number(v)) {
		s.hasMin = true;
		s.min = json_number_value(v);
	}
	if ((v = json_object_get(p, "display_max")) && json_is_number(v)) {
		s.hasMax = true;
		s.max = json_number_value(v);
	}
	if ((v = json_object_get(p, "display_in")) && json_is_array(v)) {
		s.hasIn = true;
		size_t i;
		json_t* x;
		json_array_foreach(v, i, x) s.in.push_back(json_is_string(x) ? json_string_value(x) : jsNum(json_number_value(x)));
	}
	return s;
}

SCable cable4(json_t* a) {
	SCable c;
	c.fromRef = json_string_value(json_array_get(a, 0));
	c.out = json_string_value(json_array_get(a, 1));
	c.toRef = json_string_value(json_array_get(a, 2));
	c.in = json_string_value(json_array_get(a, 3));
	return c;
}

Spec parseSpec(json_t* j) {
	Spec s;
	s.fixed = str(j, "kind") == "fixed";
	size_t i;
	json_t* v;
	json_array_foreach(json_object_get(j, "modules"), i, v) s.modules.push_back({str(v, "ref"), str(v, "plugin"), str(v, "slug")});
	if (s.fixed) {
		json_t* ec = json_object_get(j, "expect_cable");
		if (ec && json_is_array(ec) && json_array_size(ec) > 0) {
			if (json_is_array(json_array_get(ec, 0))) {
				json_array_foreach(ec, i, v) s.cables.push_back(cable4(v));
			}
			else
				s.cables.push_back(cable4(ec));
		}
		json_t* ep = json_object_get(j, "expect_param");
		if (ep && json_is_array(ep)) {
			json_array_foreach(ep, i, v) s.params.push_back(parseParam(v));
		}
		else if (ep && json_is_object(ep))
			s.params.push_back(parseParam(ep));
	}
	else {
		json_array_foreach(json_object_get(j, "cables"), i, v) {
			SCable c;
			json_t* f = json_object_get(v, "from");
			json_t* t = json_object_get(v, "to");
			c.fromRef = json_string_value(json_array_get(f, 0));
			c.out = json_string_value(json_array_get(f, 1));
			c.toRef = json_string_value(json_array_get(t, 0));
			c.in = json_string_value(json_array_get(t, 1));
			s.cables.push_back(c);
		}
		json_array_foreach(json_object_get(j, "params"), i, v) s.params.push_back(parseParam(v));
		json_array_foreach(json_object_get(j, "probes"), i, v) {
			SProbe p;
			p.module = str(v, "module");
			p.output = str(v, "output");
			p.ask = str(v, "ask");
			json_t* n;
			if ((n = json_object_get(v, "samples")) && json_is_number(n) && json_number_value(n) > 0)
				p.samples = (int) json_number_value(n);
			if ((n = json_object_get(v, "interval_ms")) && json_is_number(n) && json_number_value(n) > 0)
				p.interval = (int) json_number_value(n);
			json_t* e = json_object_get(v, "expect");
			if ((n = json_object_get(e, "max_gt")) && json_is_number(n)) {
				p.hasMaxGt = true;
				p.maxGt = json_number_value(n);
			}
			if ((n = json_object_get(e, "min_lt")) && json_is_number(n)) {
				p.hasMinLt = true;
				p.minLt = json_number_value(n);
			}
			if ((n = json_object_get(e, "range_gt")) && json_is_number(n)) {
				p.hasRangeGt = true;
				p.rangeGt = json_number_value(n);
			}
			s.probes.push_back(p);
		}
	}
	const char* key;
	json_object_foreach(json_object_get(j, "hints"), key, v) {
		if (json_is_string(v))
			s.hints[key] = json_string_value(v);
		else if (json_is_array(v) && json_array_size(v) > 0 && json_is_string(json_array_get(v, 0)))
			s.hints[key] = json_string_value(json_array_get(v, 0));
	}
	return s;
}

int portIndex(const std::vector<Port>& ports, const std::string& name) {
	std::string n = trim(name);
	for (const Port& p : ports)
		if (trim(p.name) == n)
			return p.id;
	return -1;
}

const Param* findParam(const Module* m, const std::string& name) {
	if (!m)
		return nullptr;
	std::string n = trim(name);
	for (const Param& p : m->params)
		if (trim(p.name) == n)
			return &p;
	return nullptr;
}

bool paramOk(const SParam& s, const Param* p) {
	if (!p)
		return false;
	if (s.hasIn) {
		for (const std::string& x : s.in)
			if (trim(x) == trim(p->display))
				return true;
		return false;
	}
	double v;
	if (!displayNumber(p->display, &v))
		return false;
	if (s.hasMin && v < s.min)
		return false;
	if (s.hasMax && v > s.max)
		return false;
	return true;
}

std::string paramText(const SParam& s) {
	if (s.hasIn) {
		std::string t = "«";
		for (size_t i = 0; i < s.in.size(); i++)
			t += (i ? "» или «" : "") + s.in[i];
		return t + "»";
	}
	if (s.hasMin && s.hasMax)
		return "от " + jsNum(s.min) + " до " + jsNum(s.max);
	if (s.hasMax)
		return "не больше " + jsNum(s.max);
	if (s.hasMin)
		return "не меньше " + jsNum(s.min);
	return "";
}

typedef std::map<std::string, std::string> Assign;  // ref → module id ("" = none)

struct Eval {
	std::vector<bool> cables, params;
	int score = 0;
};

Eval evaluate(const Spec& s, const Assign& a, const Patch& patch) {
	Eval e;
	for (const SCable& c : s.cables) {
		const Module* out = patch.find(a.count(c.fromRef) ? a.at(c.fromRef) : "");
		const Module* in = patch.find(a.count(c.toRef) ? a.at(c.toRef) : "");
		bool ok = false;
		if (out && in) {
			int o = portIndex(out->outputs, c.out), i = portIndex(in->inputs, c.in);
			if (o >= 0 && i >= 0)
				for (const Cable& k : patch.cables)
					if (k.outModule == out->id && k.outId == o && k.inModule == in->id && k.inId == i) {
						ok = true;
						break;
					}
		}
		e.cables.push_back(ok);
		e.score += ok;
	}
	for (const SParam& p : s.params) {
		const Module* m = patch.find(a.count(p.module) ? a.at(p.module) : "");
		bool ok = m && paramOk(p, findParam(m, p.name));
		e.params.push_back(ok);
		e.score += ok;
	}
	return e;
}

/** Same enumeration order as the Node checker's generator. */
void assignments(const std::vector<std::string>& refs, const std::map<std::string, std::vector<std::string>>& cand, size_t i, std::set<std::string>& used, Assign& acc, int& n, const std::function<void(const Assign&)>& yield) {
	if (n >= MAX_ASSIGNMENTS)
		return;
	if (i == refs.size()) {
		yield(acc);
		n++;
		return;
	}
	const std::string& ref = refs[i];
	const std::vector<std::string>& list = cand.at(ref);
	if (list.empty()) {
		acc[ref] = "";
		assignments(refs, cand, i + 1, used, acc, n, yield);
		return;
	}
	for (const std::string& id : list) {
		if (used.count(id))
			continue;
		used.insert(id);
		acc[ref] = id;
		assignments(refs, cand, i + 1, used, acc, n, yield);
		used.erase(id);
		if (n >= MAX_ASSIGNMENTS)
			return;
	}
	bool all = true;
	for (const std::string& id : list)
		if (!used.count(id))
			all = false;
	if (all) {
		acc[ref] = "";
		assignments(refs, cand, i + 1, used, acc, n, yield);
	}
}

Assign best(const Spec& s, const Patch& patch) {
	std::vector<std::string> refs;
	std::map<std::string, std::vector<std::string>> cand;
	for (const SModule& m : s.modules) {
		refs.push_back(m.ref);
		std::vector<std::string>& list = cand[m.ref];
		for (const Module& x : patch.modules)
			if (x.plugin == m.plugin && x.slug == m.slug)
				list.push_back(x.id);
	}
	Assign bestA;
	int bestScore = -1;
	std::set<std::string> used;
	Assign acc;
	int n = 0;
	assignments(refs, cand, 0, used, acc, n, [&](const Assign& a) {
		Eval e = evaluate(s, a, patch);
		if (e.score > bestScore) {
			bestScore = e.score;
			bestA = a;
		}
	});
	for (const std::string& r : refs)
		if (!bestA.count(r))
			bestA[r] = "";
	return bestA;
}

} // namespace

const Module* Patch::find(const std::string& id) const {
	if (id.empty())
		return nullptr;
	for (const Module& m : modules)
		if (m.id == id)
			return &m;
	return nullptr;
}

std::vector<ProbeTarget> probeTargets(json_t* spec, const Patch& patch) {
	Spec s = parseSpec(spec);
	Assign a = best(s, patch);
	std::vector<ProbeTarget> out;
	for (size_t i = 0; i < s.probes.size(); i++) {
		const SProbe& p = s.probes[i];
		const Module* m = patch.find(a[p.module]);
		if (!m)
			continue;
		ProbeTarget t;
		t.probe = (int) i;
		t.moduleId = m->id;
		t.outputId = portIndex(m->outputs, p.output);
		t.samples = p.samples;
		t.intervalMs = p.interval;
		out.push_back(t);
	}
	return out;
}

Result run(json_t* spec, const Patch& patch, ProbeSource* probes) {
	Spec s = parseSpec(spec);
	Assign a = best(s, patch);
	Eval e = evaluate(s, a, patch);
	Result r;
	auto key = [&](const std::string& k) { return s.fixed && k.rfind("module:", 0) != 0 ? std::string("fixed") : k; };
	auto hint = [&](const std::string& k) {
		auto it = s.hints.find(k);
		return it == s.hints.end() ? std::string() : it->second;
	};
	auto nameOf = [&](const std::string& ref) {
		const Module* m = patch.find(a[ref]);
		if (m)
			return m->name.empty() ? m->slug : m->name;
		for (const SModule& x : s.modules)
			if (x.ref == ref)
				return x.slug;
		return ref;
	};
	for (const SModule& m : s.modules) {
		Item it;
		it.key = "module:" + m.ref;
		it.ok = !a[m.ref].empty();
		it.what = "Модуль " + m.slug + " в патче";
		if (!it.ok) {
			it.hint = hint(it.key);
			if (it.hint.empty())
				it.hint = "Не нашёл модуль " + m.slug + " (" + m.plugin + "). Добавь его из браузера модулей.";
		}
		r.items.push_back(it);
	}
	for (size_t i = 0; i < s.cables.size(); i++) {
		const SCable& c = s.cables[i];
		Item it;
		it.key = key("cable:" + std::to_string(i + 1));
		it.ok = e.cables[i];
		it.what = "Кабель " + nameOf(c.fromRef) + " «" + c.out + "» → " + nameOf(c.toRef) + " «" + c.in + "»";
		if (!it.ok) {
			it.hint = hint(it.key);
			if (it.hint.empty())
				it.hint = "Нет кабеля из выхода «" + c.out + "» модуля " + nameOf(c.fromRef) + " во вход «" + c.in + "» модуля " + nameOf(c.toRef) + ".";
		}
		r.items.push_back(it);
	}
	for (size_t i = 0; i < s.params.size(); i++) {
		const SParam& p = s.params[i];
		Item it;
		it.key = key("param:" + std::to_string(i + 1));
		it.ok = e.params[i];
		it.what = trim(nameOf(p.module) + ": «" + p.name + "» " + paramText(p));
		if (!it.ok) {
			it.hint = hint(it.key);
			if (it.hint.empty()) {
				const Param* now = findParam(patch.find(a[p.module]), p.name);
				it.hint = now ? "Сейчас «" + p.name + "» = " + now->display + now->unit + "." : "Ручка «" + p.name + "» не найдена.";
			}
		}
		r.items.push_back(it);
	}
	for (size_t i = 0; i < s.probes.size(); i++) {
		const SProbe& p = s.probes[i];
		Item it;
		it.key = "probe:" + std::to_string(i + 1);
		const Module* m = patch.find(a[p.module]);
		bool ok = false;
		if (m && probes) {
			ProbeTarget t;
			t.probe = (int) i;
			t.moduleId = m->id;
			t.outputId = portIndex(m->outputs, p.output);
			t.samples = p.samples;
			t.intervalMs = p.interval;
			std::vector<float> seen = t.outputId >= 0 ? probes->samples(t) : std::vector<float>();
			if (!seen.empty()) {
				float mx = seen[0], mn = seen[0];
				for (float v : seen) {
					mx = std::max(mx, v);
					mn = std::min(mn, v);
				}
				ok = (!p.hasMaxGt || mx > p.maxGt) && (!p.hasMinLt || mn < p.minLt) && (!p.hasRangeGt || mx - mn > p.rangeGt);
			}
		}
		it.ok = ok;
		it.what = "Сигнал на выходе «" + p.output + "» модуля " + nameOf(p.module);
		if (!ok) {
			it.hint = hint(it.key);
			if (it.hint.empty())
				it.hint = p.ask.empty() ? "Сигнал не меняется." : p.ask;
		}
		r.items.push_back(it);
	}
	r.passed = !r.items.empty();
	for (const Item& it : r.items)
		r.passed = r.passed && it.ok;
	for (const SModule& m : s.modules)
		r.assignment.push_back({m.ref, a[m.ref]});
	return r;
}

std::vector<std::string> keys(json_t* spec) {
	Spec s = parseSpec(spec);
	std::vector<std::string> out;
	for (const SModule& m : s.modules)
		out.push_back("module:" + m.ref);
	if (s.fixed) {
		if (!s.cables.empty() || !s.params.empty())
			out.push_back("fixed");
		return out;
	}
	for (size_t i = 0; i < s.cables.size(); i++)
		out.push_back("cable:" + std::to_string(i + 1));
	for (size_t i = 0; i < s.params.size(); i++)
		out.push_back("param:" + std::to_string(i + 1));
	for (size_t i = 0; i < s.probes.size(); i++)
		out.push_back("probe:" + std::to_string(i + 1));
	return out;
}

} // namespace check
} // namespace atlas
