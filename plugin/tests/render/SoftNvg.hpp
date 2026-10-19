// A small software back end for nanovg (tests and visual QA only).
//
// The real nanovg.c and fontstash from VCV's fork do all path building, tessellation and
// text layout; this back end only rasterizes what they hand over: fills (non-zero winding),
// strokes (triangle strips) and text triangles (font atlas), with gradient/solid paints and
// scissors, supersampled for anti-aliasing. Output: PNG via stb_image_write.
#pragma once
#include <nanovg.h>

#include <string>

struct SoftCanvas;

/** A canvas of `width` × `height` logical px rendered at `dpr` with `ss`× supersampling. */
NVGcontext* softCreate(int width, int height, float dpr = 2.f, int ss = 2);
void softDelete(NVGcontext* vg);
void softClear(NVGcontext* vg, NVGcolor c);
/** Starts a frame (nvgBeginFrame with the canvas size and ratio). */
void softBegin(NVGcontext* vg);
void softEnd(NVGcontext* vg);
bool softWritePng(NVGcontext* vg, const std::string& path);
