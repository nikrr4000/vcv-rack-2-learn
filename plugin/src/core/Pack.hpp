// The course pack (res/course/pack.json, written by rack/compiler): every lesson, block and
// item, the glossary and the course map. No Rack dependency (jansson only), so the render
// harness and unit tests read the same file the plugin reads.
#pragma once
#include <jansson.h>

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace atlas {

struct Faq {
	std::string q, a;
};

/** A pointer target of a Note: a param, input, output or light of a module in the patch. */
struct Target {
	int64_t moduleId = -1;
	std::string module;  // the lesson ref (for messages)
	std::string kind;    // "param" | "input" | "output" | "light"
	int index = -1;
	std::string name;
};

struct Option {
	std::string text;
	bool correct = false;
	std::string why;
};

struct Question {
	std::string q;
	std::vector<Option> options;
	bool free = false;
};

struct Step {
	std::string doText, see;
};

struct Item {
	std::string id;
	std::string kind;  // page | note | figure | quiz | task | bench
	std::string text;  // page / note markdown; task brief
	bool title = false;  // page: headings at title size (cover)
	std::vector<Target> targets;
	json_t* figure = nullptr;  // borrowed from the pack root
	std::string caption;
	std::vector<Question> questions;
	std::string mode;  // task: practice | debug | try
	std::vector<Step> steps;
	json_t* check = nullptr;  // borrowed; Node checker format (+ kind: fixed)
	std::map<std::string, std::vector<std::string>> hints;
	std::vector<int64_t> given;   // practice: modules that count besides the bench
	std::vector<int64_t> scope;   // debug: the block's own modules
	std::string side;             // bench: left | right
	int width = 0;                // HP the compiler chose
};

struct Block {
	std::string id, type, title;
	int index = 0;  // 0-based
	int row = 0;    // rack row (block n = row n)
	std::vector<Item> items;
	std::vector<Faq> faq;
	std::string simpler;
	std::string plain;  // block text for the tutor header
	float x = 0, y = 0, w = 0, h = 0;  // the row's module bounds in rack px
	/** Demo bus channels of this block (one per bus: mono 1, stereo 2); opened only while it is active. */
	struct Channel {
		int64_t mixerId = -1;
		int param = -1;
		float level = 1.f;
	};
	std::vector<Channel> channels;
};

struct Lesson {
	std::string id, num, title, goal, builds, next;
	int part = 0;
	int minutes = 0;
	std::vector<std::string> requires;
	std::string patch;
	std::string layoutHash;
	std::vector<Block> blocks;
	const Block* block(const std::string& id) const;
	int blockIndex(const std::string& id) const;
};

struct Part {
	int n = 0;
	std::string title;
	std::vector<std::string> lessons;
};

struct Pack {
	json_t* root = nullptr;
	std::string title;
	std::vector<Part> parts;
	std::map<std::string, Lesson> lessons;
	std::map<std::string, std::string> glossary;
	std::string error;

	Pack() {}
	~Pack();
	Pack(const Pack&) = delete;
	Pack& operator=(const Pack&) = delete;

	bool loadFile(const std::string& path);
	bool loadJson(json_t* root);  // takes ownership

	const Lesson* lesson(const std::string& id) const;
	const Item* item(const std::string& lesson, const std::string& block, const std::string& item) const;
	/** All lesson ids in course order. */
	std::vector<std::string> order() const;
	/** The lesson after `id` in course order, or "". */
	std::string nextLesson(const std::string& id) const;
	std::string definition(const std::string& term) const;
};

/** «1.1 · Блок 2» */
std::string blockHeader(const Lesson* l, const Block* b);

/** «Часть 1 · Язык модульного синтеза» for a lesson's cover. */
std::string partHeader(const Pack& pack, const Lesson* l);

} // namespace atlas
