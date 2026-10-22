// C++ checker parity: replays every fixture exported by `live-check.mjs --fixtures` and
// compares each item (key, ok, hint) with the Node checker's result on the same patch.
#include <dirent.h>

#include <algorithm>

#include "check/Checker.hpp"
#include "test.hpp"

using namespace atlas::check;

namespace {

std::string fixturesDir() {
	const char* d = std::getenv("ATLAS_FIXTURES");
	return d ? d : "plugin/tests/fixtures/checks";
}

struct FixtureProbes : ProbeSource {
	json_t* data;  // {moduleId: [[v per output] × 10]}
	const Patch* patch;
	std::vector<float> samples_(const ProbeTarget& t) {
		std::vector<float> out;
		json_t* rows = json_object_get(data, t.moduleId.c_str());
		const Module* m = patch->find(t.moduleId);
		if (!rows || !m)
			return out;
		int idx = -1;
		for (size_t i = 0; i < m->outputs.size(); i++)
			if (m->outputs[i].id == t.outputId)
				idx = (int) i;
		if (idx < 0)
			return out;
		size_t n = json_array_size(rows);
		for (int k = 0; k < t.samples && n > 0; k++)
			out.push_back((float) json_number_value(json_array_get(json_array_get(rows, k % n), idx)));
		return out;
	}
	std::vector<float> samples(const ProbeTarget& t) override { return samples_(t); }
};

Patch patchOf(json_t* c) {
	Patch p;
	size_t i;
	json_t* v;
	json_array_foreach(json_object_get(c, "modules"), i, v) {
		Module m;
		m.id = json_string_value(json_object_get(v, "id"));
		m.plugin = json_string_value(json_object_get(v, "plugin"));
		m.slug = json_string_value(json_object_get(v, "slug"));
		json_t* n = json_object_get(v, "name");
		m.name = n && json_is_string(n) ? json_string_value(n) : "";
		size_t k;
		json_t* x;
		json_array_foreach(json_object_get(v, "params"), k, x) {
			Param q;
			q.id = (int) json_integer_value(json_object_get(x, "id"));
			q.name = json_string_value(json_object_get(x, "name"));
			json_t* d = json_object_get(x, "displayValue");
			q.display = d && json_is_string(d) ? json_string_value(d) : "";
			json_t* u = json_object_get(x, "unit");
			q.unit = u && json_is_string(u) ? json_string_value(u) : "";
			m.params.push_back(q);
		}
		json_array_foreach(json_object_get(v, "inputs"), k, x) m.inputs.push_back({(int) json_integer_value(json_object_get(x, "id")), json_string_value(json_object_get(x, "name"))});
		json_array_foreach(json_object_get(v, "outputs"), k, x) m.outputs.push_back({(int) json_integer_value(json_object_get(x, "id")), json_string_value(json_object_get(x, "name"))});
		p.modules.push_back(m);
	}
	json_array_foreach(json_object_get(c, "cables"), i, v) {
		Cable k;
		k.outModule = json_string_value(json_object_get(v, "outputModuleId"));
		k.outId = (int) json_integer_value(json_object_get(v, "outputId"));
		k.inModule = json_string_value(json_object_get(v, "inputModuleId"));
		k.inId = (int) json_integer_value(json_object_get(v, "inputId"));
		p.cables.push_back(k);
	}
	return p;
}

std::vector<std::string> fixtureFiles(const std::string& dir) {
	std::vector<std::string> out;
	DIR* d = opendir(dir.c_str());
	if (!d)
		return out;
	while (dirent* e = readdir(d)) {
		std::string n = e->d_name;
		if (n.size() > 5 && n.substr(n.size() - 5) == ".json")
			out.push_back(dir + "/" + n);
	}
	closedir(d);
	std::sort(out.begin(), out.end());
	return out;
}

} // namespace

TEST(checker_matches_node_on_every_fixture) {
	std::vector<std::string> files = fixtureFiles(fixturesDir());
	CHECK(files.size() > 0);
	int cases = 0;
	for (const std::string& f : files) {
		json_error_t err;
		json_t* root = json_load_file(f.c_str(), 0, &err);
		CHECK(root != nullptr);
		if (!root)
			continue;
		json_t* spec = json_object_get(root, "check");
		size_t i;
		json_t* c;
		json_array_foreach(json_object_get(root, "cases"), i, c) {
			Patch p = patchOf(c);
			FixtureProbes probes;
			probes.data = json_object_get(c, "samples");
			probes.patch = &p;
			Result r = run(spec, p, &probes);
			json_t* exp = json_object_get(c, "expected");
			json_t* items = json_object_get(exp, "items");
			bool same = json_is_true(json_object_get(exp, "passed")) == r.passed && json_array_size(items) == r.items.size();
			for (size_t k = 0; same && k < r.items.size(); k++) {
				json_t* e = json_array_get(items, k);
				json_t* h = json_object_get(e, "hint");
				std::string hint = h && json_is_string(h) ? json_string_value(h) : "";
				if (r.items[k].key != json_string_value(json_object_get(e, "key")) || r.items[k].ok != json_is_true(json_object_get(e, "ok")) || r.items[k].hint != hint) {
					std::fprintf(stderr, "  %s [%s] item %zu: cpp {%s %d «%s»} node {%s %d «%s»}\n", f.c_str(), json_string_value(json_object_get(c, "name")), k, r.items[k].key.c_str(), r.items[k].ok, r.items[k].hint.c_str(), json_string_value(json_object_get(e, "key")), json_is_true(json_object_get(e, "ok")), hint.c_str());
					same = false;
				}
			}
			if (!same)
				std::fprintf(stderr, "  mismatch: %s [%s]\n", f.c_str(), json_string_value(json_object_get(c, "name")));
			CHECK(same);
			cases++;
		}
		json_decref(root);
	}
	std::printf("  %d fixture cases\n", cases);
}

TEST(checker_fixed_kind) {
	const char* specText = "{\"kind\":\"fixed\",\"modules\":[{\"ref\":\"env\",\"plugin\":\"Fundamental\",\"slug\":\"ADSR\"},{\"ref\":\"vca\",\"plugin\":\"Fundamental\",\"slug\":\"VCA-1\"}],"
	                       "\"expect_cable\":[\"env\",\"Envelope\",\"vca\",\"CV\"],\"hints\":{\"fixed\":\"Проследи, куда идёт выход ADSR.\"}}";
	json_t* spec = json_loads(specText, 0, nullptr);
	Patch p;
	Module env;
	env.id = "1";
	env.plugin = "Fundamental";
	env.slug = "ADSR";
	env.name = "ADSR";
	env.outputs.push_back({0, "Envelope"});
	Module vca;
	vca.id = "2";
	vca.plugin = "Fundamental";
	vca.slug = "VCA-1";
	vca.name = "VCA";
	vca.inputs.push_back({0, "CV"});
	vca.inputs.push_back({1, "Channel"});
	p.modules = {env, vca};
	Result r = run(spec, p, nullptr);
	CHECK(!r.passed);
	CHECK_EQ(r.items.back().key, std::string("fixed"));
	CHECK_EQ(r.items.back().hint, std::string("Проследи, куда идёт выход ADSR."));
	p.cables.push_back({"1", 0, "2", 0});
	r = run(spec, p, nullptr);
	CHECK(r.passed);
	std::vector<std::string> k = keys(spec);
	CHECK_EQ(k.back(), std::string("fixed"));
	json_decref(spec);
}
