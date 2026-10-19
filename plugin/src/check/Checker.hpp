// The practice checker, ported from app/server/vcv/checker.mjs (same semantics, same Russian
// texts, same item keys). No Rack dependency: the patch comes in as a plain model, probe
// samples from a ProbeSource. The Rack adapter (RackPatch) builds the model from the engine.
//
// Check spec (jansson, the format of vcv/course/checks/*.json):
//   modules [{ref, plugin, slug}], cables [{from: [ref, out], to: [ref, in]}],
//   params [{module, name, display_min?, display_max?, display_in?}],
//   probes [{module, output, ask, samples, interval_ms, expect: {max_gt?, min_lt?, range_gt?}}],
//   hints {"module:<ref>" | "cable:<i>" | "param:<i>" | "probe:<i>" | "fixed": text}
// Kind `fixed` (debug blocks): {kind: "fixed", modules, expect_cable: [[ref, out, ref, in], …],
//   expect_param: [{module, name, display_*}]} — every cable/param item has the key "fixed".
#pragma once
#include <jansson.h>

#include <string>
#include <vector>

namespace atlas {
namespace check {

struct Port {
	int id = 0;
	std::string name;
};

struct Param {
	int id = 0;
	std::string name;
	std::string display;
	std::string unit;
};

struct Module {
	std::string id;
	std::string plugin, slug, name;
	std::vector<Param> params;
	std::vector<Port> inputs, outputs;
};

struct Cable {
	std::string outModule;
	int outId = 0;
	std::string inModule;
	int inId = 0;
};

struct Patch {
	std::vector<Module> modules;
	std::vector<Cable> cables;
	const Module* find(const std::string& id) const;
};

struct ProbeTarget {
	int probe = 0;  // 0-based index into spec.probes
	std::string moduleId;
	int outputId = -1;
	int samples = 10;
	int intervalMs = 50;
};

struct ProbeSource {
	virtual ~ProbeSource() {}
	/** Voltages sampled for a probe target (may be fewer than asked; empty = no port). */
	virtual std::vector<float> samples(const ProbeTarget& t) = 0;
};

struct Item {
	std::string key;
	bool ok = false;
	std::string what;
	std::string hint;
};

struct Result {
	bool passed = false;
	std::vector<Item> items;
	/** The chosen ref → module id assignment ("" = none). */
	std::vector<std::pair<std::string, std::string>> assignment;
};

/** Probe targets of the best assignment: what must be sampled before `run`. */
std::vector<ProbeTarget> probeTargets(json_t* spec, const Patch& patch);

/** Runs the check. Without a ProbeSource every probe fails. */
Result run(json_t* spec, const Patch& patch, ProbeSource* probes);

/** Keys a check produces, in order (for hint ladders): module:<ref>, cable:<i>, … or fixed. */
std::vector<std::string> keys(json_t* spec);

} // namespace check
} // namespace atlas
