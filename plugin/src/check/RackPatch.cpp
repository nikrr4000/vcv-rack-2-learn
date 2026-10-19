#include "RackPatch.hpp"

namespace atlas {

static check::Port port(int id, const std::string& name) {
	check::Port p;
	p.id = id;
	p.name = name;
	return p;
}

check::Patch rackPatch(const std::set<int64_t>& ids) {
	check::Patch p;
	std::vector<int64_t> all = APP->engine->getModuleIds();
	std::set<std::string> keep;
	for (int64_t id : all) {
		if (!ids.empty() && !ids.count(id))
			continue;
		engine::Module* m = APP->engine->getModule(id);
		if (!m || !m->model)
			continue;
		check::Module cm;
		cm.id = std::to_string(id);
		cm.plugin = m->model->plugin ? m->model->plugin->slug : "";
		cm.slug = m->model->slug;
		cm.name = m->model->name;
		for (size_t i = 0; i < m->paramQuantities.size(); i++) {
			ParamQuantity* q = m->paramQuantities[i];
			if (!q)
				continue;
			check::Param cp;
			cp.id = (int) i;
			cp.name = q->name;
			cp.display = q->getDisplayValueString();
			cp.unit = q->unit;
			cm.params.push_back(cp);
		}
		for (size_t i = 0; i < m->inputInfos.size(); i++)
			if (m->inputInfos[i])
				cm.inputs.push_back(port((int) i, m->inputInfos[i]->name));
		for (size_t i = 0; i < m->outputInfos.size(); i++)
			if (m->outputInfos[i])
				cm.outputs.push_back(port((int) i, m->outputInfos[i]->name));
		keep.insert(cm.id);
		p.modules.push_back(cm);
	}
	for (int64_t cid : APP->engine->getCableIds()) {
		engine::Cable* c = APP->engine->getCable(cid);
		if (!c || !c->outputModule || !c->inputModule)
			continue;
		check::Cable k;
		k.outModule = std::to_string(c->outputModule->id);
		k.outId = c->outputId;
		k.inModule = std::to_string(c->inputModule->id);
		k.inId = c->inputId;
		if (keep.count(k.outModule) && keep.count(k.inModule))
			p.cables.push_back(k);
	}
	return p;
}

float outputVoltage(int64_t moduleId, int outputId, bool* ok) {
	engine::Module* m = APP->engine->getModule(moduleId);
	if (!m || outputId < 0 || outputId >= (int) m->outputs.size()) {
		if (ok)
			*ok = false;
		return 0.f;
	}
	if (ok)
		*ok = true;
	return m->outputs[outputId].getVoltage();
}

} // namespace atlas
