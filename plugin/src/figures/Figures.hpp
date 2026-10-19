// Procedural figures (plan §6.5): a template name plus parameters, drawn with nanovg in the
// theme. Content writers never draw; new templates are added here only. No Rack dependency.
#pragma once
#include <jansson.h>

#include <string>
#include <vector>

#include "ui/Draw.hpp"

namespace atlas {
namespace figures {

/** Template names, in the order of the gallery. */
const std::vector<std::string>& templates();

/** Checks a figure spec ({template, …}); returns "" or a message in English (for the compiler/tests). */
std::string validate(json_t* spec);

/** Draws a figure into `r`. Unknown template or bad params: a quiet placeholder. */
void draw(NVGcontext* vg, const Style& st, const Rect& r, json_t* spec);

/** Preferred aspect (height / width) of a template for the layout of a Figure module. */
float aspect(json_t* spec);

} // namespace figures
} // namespace atlas
