// AtlasHost: a scene child that outlives patch loads. Owns the action queue (patch loads,
// glides, saves — never run inside a module's event handler), learner progress, the docked
// panel, the pointer overlay, the Atlas client and the dev bridge.
#pragma once
#include <functional>
#include <mutex>
#include <vector>

#include "atlas.hpp"

namespace atlas {

struct AtlasHost : widget::Widget {
	static AtlasHost* instance;
	std::mutex queueMutex;
	std::vector<std::function<void()>> queue;

	AtlasHost();
	~AtlasHost();
	/** Runs `fn` in the next step() on the UI thread. */
	void post(std::function<void()> fn);
	void step() override;
	void draw(const DrawArgs& args) override;
};

} // namespace atlas
