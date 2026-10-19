#include "Host.hpp"

#include "modules/Base.hpp"

namespace atlas {

AtlasHost* AtlasHost::instance = nullptr;

void ensureHost() {
	if (AtlasHost::instance || !APP || !APP->scene)
		return;
	AtlasHost* h = new AtlasHost;
	if (APP->scene->menuBar)
		APP->scene->addChildBelow(h, APP->scene->menuBar);
	else
		APP->scene->addChild(h);
}

AtlasHost::AtlasHost() {
	instance = this;
}

AtlasHost::~AtlasHost() {
	if (instance == this)
		instance = nullptr;
}

void AtlasHost::post(std::function<void()> fn) {
	std::lock_guard<std::mutex> lock(queueMutex);
	queue.push_back(std::move(fn));
}

void AtlasHost::step() {
	if (parent)
		box = parent->box.zeroPos();
	std::vector<std::function<void()>> run;
	{
		std::lock_guard<std::mutex> lock(queueMutex);
		run.swap(queue);
	}
	for (auto& fn : run)
		fn();
	Widget::step();
}

void AtlasHost::draw(const DrawArgs& args) {
	Widget::draw(args);
}

} // namespace atlas
