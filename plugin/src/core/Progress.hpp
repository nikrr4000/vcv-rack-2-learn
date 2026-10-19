// Learner progress (Atlas/progress.json in the Rack user folder), merged with Atlas by the
// latest timestamp. No Rack dependency.
#pragma once
#include <jansson.h>

#include <map>
#include <string>
#include <vector>

#include "Pack.hpp"

namespace atlas {

struct LessonProgress {
	std::string status;  // "" | "learning" | "learned"
	std::string block;   // last block reached
	std::vector<std::string> reached;
	double updatedAt = 0;  // unix seconds
	bool learned() const { return status == "learned"; }
};

struct Progress {
	std::map<std::string, LessonProgress> lessons;
	std::string last;  // the lesson opened last
	double updatedAt = 0;

	bool load(const std::string& path);
	bool save(const std::string& path) const;
	json_t* toJson() const;
	void fromJson(json_t* j);
	/** Takes remote entries that are newer than local ones. Returns true when something changed. */
	bool merge(json_t* remote);
	int learnedCount() const;
	std::string status(const std::string& lesson) const;
	/** What «Продолжить» opens: the last lesson unless learned, else the first unlearned in order. */
	std::string continueLesson(const Pack& pack) const;
	void reach(const std::string& lesson, const std::string& block, double now);
	void complete(const std::string& lesson, double now);
};

} // namespace atlas
