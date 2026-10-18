# Course "VCV Rack 2 from zero" — expert sources

Compiled 2026-10-19. Every page was opened and read; links point to specific articles, not home pages. Descriptions are paraphrased. Quotes are verbatim in the original language, under 15 words, at most one per source.

**Quote status.** Quotes were obtained through a page-reading tool that summarises text. Four are marked "verified" — their exact wording was confirmed by a separate check. All others must be checked against the original before use in a lesson; on mismatch, drop the quote and paraphrase (rule 7 of the plan).

**Ids** (`sos-07` etc.) are used in `course.json` → `sources` and in the Sources section of lessons and cards.

## Key facts from the research

- **Versions.** VCV Rack 2 Free — 2.6.6 (CHANGELOG entry dated 2025-11-04). The VCV Free (Fundamental) plugin — 2.6.4, 37 modules, GPL-3.0-or-later.
- **VCV Recorder.** In the VCV Library: version 2.0.3, GPL-3.0-or-later, no price shown; unlike the Fundamental modules, it is marked with an «Add» button, not «Included». So it does not ship with Rack Free but is added from the Library for free. The owner already has it; lesson 0.1 checks that it is installed.
- **Fundamental module slugs** in the Library: `VCO`, `VCO2` (Wavetable VCO), `VCF`, `VCA-1`, `ADSR`, `Mixer` (Mix), `VCMixer` (VCA Mix), `SEQ3`, `SHASR` and others. Exact slugs come from the live Rack snapshot (stage B of the plan).
- **There is no separate manual page for MIDI or the Rack interface.** MIDI is on the Core page; the interface is covered by Getting Started, Menu Bar, Key Commands.
- **Older videos use Rack v0/v1 module names** (VCO-1, VCA-2, Audio-8). Lessons use Rack 2 names only.
- **Gate levels.** The Learning Modular glossary describes 5 V gates (the hardware convention); in VCV a gate is 10 V. The right source for the course is VCV Voltage Standards.
- **Gaps in the sources:** open/closed hi-hat with choke (8.3), stereo and Mid/Side (7.4), Wavetable LFO (4.2), arrangement specifically in VCV (9.4) have little expert material. These lessons rely on the official manual and the few sources below; the lesson must state plainly where something is the course's own method rather than an expert's view.
- **Few Russian-language expert sources**: three (section H), all introductory and partly on Rack v1.

---

## A. Official VCV documentation

**vcv-manual-index** — VCV Manual, table of contents. Author: VCV (Andrew Belt), the developer of Rack.
https://vcvrack.com/manual · EN · free · text. Lists every manual page. Lessons: 0.1, 0.2.

**vcv-manual-installing** — Installing & Running. VCV.
https://vcvrack.com/manual/Installing · EN · free. Installation on macOS/Windows/Linux, the user folder (`-u`, `RACK_USER_DIR`), offline plugin install by copying `plugins-<OS>-<CPU>`. Lessons: 0.1.
Quote: "Plugins are typically installed via the VCV Library."

**vcv-manual-getting-started** — Getting Started. VCV.
https://vcvrack.com/manual/GettingStarted · EN · free. Adding modules from the Library; setting up VCV Audio: driver, device, sample rate, block size. Lessons: 0.1, 0.2, 0.3.
Quote: "The VCV Audio module is the portal between the virtual and physical world."

**vcv-manual-menu-bar** — Menu Bar. VCV.
https://vcvrack.com/manual/MenuBar · EN · free. File, Edit, View menus (zoom, cable opacity and tension, module locking), Engine (sample rate, threads, CPU), Library (login, «Update all»). Lessons: 0.1, 0.2.
Quote: "Engine CPU usage is almost exactly proportional to its sample rate."

**vcv-manual-keys** — Key Commands. VCV.
https://vcvrack.com/manual/KeyCommands · EN · free. Module browser (Enter or right-click), copy, duplicate, delete, zoom. No cable shortcuts on the page. Lessons: 0.2.
Quote: "Launch the Module Browser to add modules to the rack."

**vcv-manual-core** — Core Modules. VCV.
https://vcvrack.com/manual/Core · EN · free. Sections `#Audio`, `#MIDI-CV`, `#MIDI-CC`, `#MIDI-Gate`, `#MIDI-Map`, `#CV-MIDI`, `#CV-CC`, `#CV-Gate`, `#Blank`, `#Notes`. MIDI-CV polyphony modes (Rotate, Reuse, Reset), MIDI-Gate for pads, driver and block-size settings. Lessons: 0.3, 6.1–6.4, 7.5.
Quote (**verified**): "V/OCT generates a 1V/oct pitch signal of the last held MIDI note."

**vcv-manual-polyphony** — Polyphony. VCV.
https://vcvrack.com/manual/Polyphony · EN · free. Up to 16 channels per cable (drawn thicker), a polyphony source such as MIDI-CV; the Split, Merge, Sum, Viz modules. Lessons: 6.4, 8.6.
Quote: "Zero-channel cables are also possible, which make modules think the cable is unpatched."

**vcv-voltage-standards** — Voltage Standards. VCV (developer section, but the main reference on levels).
https://vcvrack.com/manual/VoltageStandards · EN · free. Audio ±5 V; CV 0…10 V or ±5 V; 1V/oct, 0 V = C4; LFOs default to 2 Hz; gate 10 V, trigger 1 ms, Schmitt thresholds; polyphony channel rules; 1 sample of delay per cable. Lessons: 1.2, 1.3, 1.4, 1.5, 5.1, 5.4, 6.4, 7.3, 9.5.
Quote: "Gates should produce 10 V when active."

**vcv-free-manual** — VCV Free (Fundamental), illustrated module manual. VCV.
https://vcvrack.com/Free · EN · free · text and diagrams. One document with anchors for all 37 modules: `#VCO`, `#VCO2`, `#Noise`, `#VCF`, `#VCA-1`, `#ADSR`, `#LFO`, `#LFO2`, `#Delay`, `#Mixer`, `#VCMixer`, `#8vert`, `#Fade`, `#MidSide`, `#Mutes`, `#Pulses`, `#Push`, `#SEQ3`, `#SequentialSwitch1`, `#SequentialSwitch2`, `#Octave`, `#Quantizer`, `#Split`, `#Merge`, `#Sum`, `#Viz`, `#Random`, `#CVMix`, `#Logic`, `#Compare`, `#Gates`, `#Process`, `#Mult`, `#Rescale`, `#RandomValues`, `#SHASR`, `#Scope`. The main source for module cards. Lessons: 2.x–5.x, 6.4, 7.1, 7.3, 7.4.
Quote: "A voltage controlled filter removes a range of frequencies from an audio signal."

**vcv-library-fundamental** — VCV Free in the Library. VCV.
https://library.vcvrack.com/Fundamental · EN. Version, licence, module slugs. Lessons: 0.1.

**vcv-recorder** — VCV Recorder. VCV.
https://vcvrack.com/Recorder, https://library.vcvrack.com/VCV-Recorder · EN. L/MON and RIGHT inputs, a level knob with a VU meter, a record button, GATE and TRIG inputs, the file path in the context menu, automatic file numbering. Formats: WAV, AIFF, FLAC, ALAC, MP3; video: MPEG-2, FFV1/HuffYUV. Lessons: 7.5, 9.5.
Quote: "Click the record button to begin recording."

## B. Synthesis theory: Gordon Reid, "Synth Secrets" (Sound On Sound)

63 articles, 1999–2004, the standard reference on analogue subtractive synthesis. EN, free. List of parts: https://www.soundonsound.com/series/synth-secrets-sound-sound

**sos-01** — Pt 1 "What's In A Sound?", May 1999. https://www.soundonsound.com/techniques/whats-sound
Harmonics as multiples of the fundamental, Fourier analysis using the sawtooth; subtractive synthesis as removing harmonics from a rich wave. Lessons: 1.1, 2.1, 3.1.
Quote: "you attenuate or remove harmonics from harmonically rich waveforms"

**sos-06** — Pt 6 "Of Responses & Resonance", October 1999. https://www.soundonsound.com/techniques/responses-resonance
Filter slopes of 6/12/24 dB/oct, resonance as a peak at the cutoff, self-oscillation — the filter becomes a sine oscillator. Lessons: 3.1, 3.2, 8.1.
Quote: "the filter begins to oscillate at its cutoff frequency"

**sos-07** — Pt 7 "Envelopes, Gates & Triggers", November 1999. https://www.soundonsound.com/techniques/envelopes-gates-triggers
ADSR and other envelope shapes, gate vs trigger, retriggering behaviour. Lessons: 1.2, 3.4, 5.4.
Quote: "Gate remains 'Open' for the whole time that you depress the key"

**sos-09** — Pt 9 "An Introduction To VCAs", January 2000. https://www.soundonsound.com/techniques/introduction-vcas
Gain as multiplication, VCAs in the audio path and in the CV path, headroom and overload. Lessons: 3.3, 4.3, 7.3.
Quote: "Whenever you have more than one amplifier and/or attenuator in series"

**sos-10** — Pt 10 "Modulation", February 2000. https://www.soundonsound.com/techniques/modulation
Vibrato, tremolo, filter sweeps from an LFO and the rates that suit them; pulse-width modulation. Lessons: 2.2, 4.1, 4.3.
Quote: "the magic ingredient that makes all the other elements sound interesting"

**sos-12** — Pt 12 "An Introduction To Frequency Modulation", April 2000. https://www.soundonsound.com/techniques/introduction-frequency-modulation
Carrier and modulator, sidebands, modulation index. Lessons: 2.2.
Quote: "Frequency Modulation is a powerful method of synthesis"

**sos-16** — Pt 16 "From Sample & Hold To Sample-rate Converters (1)", August 2000. https://www.soundonsound.com/techniques/sample-hold-sample-rate-converters-1
S&H fed with noise, random pitch, quantising to semitones (1/12 V) on a 1V/oct scale — a "random arpeggiator". Lessons: 1.3, 4.4, 4.5, 5.3. No quote.

**sos-33** — Pt 33 "Synthesizing Drums: The Bass Drum", January 2002. https://www.soundonsound.com/techniques/synthesizing-drums-bass-drum
A kick model: pitch envelope, beater click (a noise burst or a fast filter envelope), decay through a VCA. Lessons: 8.1.
Quote (**verified**): "the pitch of a typical kick drum can shift by a couple of semitones"

**sos-34** — Pt 34 "Practical Bass Drum Synthesis", February 2002. https://www.soundonsound.com/techniques/practical-bass-drum-synthesis
A kick from a self-oscillating filter "kicked" by an envelope (attack 0, sustain 0); how the TR-808 and TR-909 kicks work. Lessons: 3.2, 8.1, 9.2.
Quote: "the pitch of the drum always swoops down"

**sos-35** — Pt 35 "Synthesizing Drums: The Snare Drum", March 2002. https://www.soundonsound.com/techniques/synthesizing-drums-snare-drum
Snare physics: shell modes plus band-limited noise from the snare wires; timbre depends on how hard the drum is struck. Lessons: 8.2.
Quote: "the harder you strike a snare drum, the louder it becomes"

**sos-36** — Pt 36 "Practical Snare Drum Synthesis", April 2002. https://www.soundonsound.com/techniques/practical-snare-drum-synthesis
TR-909 and TR-808 circuits broken down; the recipe: noise + a tonal component, instant attack, no sustain. Lessons: 2.5, 8.2.
Quote (**verified**): "a snare drum should never sustain"

**sos-37** — Pt 37 "Analysing Metallic Percussion", May 2002. https://www.soundonsound.com/techniques/analysing-metallic-percussion
Inharmonic cymbal modes, chaotic vibration. Lessons: 8.3.
Quote: "a cymbal's vibrations become chaotic"

**sos-38** — Pt 38 "Synthesizing Realistic Cymbals", June 2002. https://www.soundonsound.com/techniques/synthesizing-realistic-cymbals
A cymbal from FM of square waves: a band-pass "ping" and a high-pass tail; closed and open hi-hat covered briefly. Lessons: 8.3. No quote.

**sos-39** — Pt 39 "Practical Cymbal Synthesis", July 2002. https://www.soundonsound.com/techniques/practical-cymbal-synthesis
The TR-808 approach: six inharmonically tuned square oscillators split into bands; the author concludes that modulated oscillator pairs in a modular system work better. Lessons: 8.3.
Quote: "six square-wave oscillators tuned enharmonically"

**sos-strings** — "Synthesizing Strings: PWM & String Sounds", March 2003. https://www.soundonsound.com/techniques/synthesizing-strings-pwm-string-sounds
Two detuned saws, one lightly modulated, in place of PWM; octave layering, chorus and reverb — the ensemble pad recipe. Lessons: 2.3, 8.6.
Quote: "Every note you hear is simply four detuned and modulated oscillators"

## C. Chris Meyer, Learning Modular

Chris Meyer: engineer at Sequential Circuits (creator of Vector Synthesis) and chief engineer at Roland R&D US; taught synthesis at UCLA Extension; co-author of "Patch & Tweak". He has no VCV-specific courses.

**lm-basic-concepts** — free course "The Basic Concepts of Synthesis". https://courses.learningmodular.com/courses/basic-concepts · EN · free with signup · 10 captioned videos.
Pitch and loudness, waveforms, noise colours, filter modes and slopes, resonance, waveshaping, envelopes, LFOs, random sources. Lessons: 1.1, 2.1, 2.5, 3.1–3.4, 4.1, 4.4.
Quote: "Demystifying the terminology commonly used in synthesizers."

**lm-glossary** — glossary. https://learningmodular.com/glossary/ · EN · free.
Entries: CV (/glossary/control-voltage/), 1V/oct (/glossary/1-voct/ — the exponential law and tracking drift), attenuator, envelope generator, gates and triggers. Note: gate levels are for hardware (5 V), not VCV. Lessons: 1.2, 1.3, 1.4, 5.4.
Quote: "CV is the most common shorthand to refer to control voltage"

**lm-first-voice** — "Patching a typical subtractive synthesis voice", December 2016. https://learningmodular.com/patching-a-typical-subtractive-synthesis-voice/ · EN · free · video under 10 minutes.
Building a classic "East Coast" voice from scratch. Lessons: 1.5, 3.5.
Quote: "If you're a beginner, here is your first step on the ladder"

**patch-tweak** — Kim Bjørn, Chris Meyer, "Patch & Tweak: Exploring Modular Synthesis" (Bjooks), foreword by Suzanne Ciani. https://learningmodular.com/patch-tweak/ · EN · paid book.
An introduction to modular synthesis, a survey of module types from simple to complex, history and a glossary, patch diagrams, interviews with manufacturers and musicians. Lessons: 1.5, 3.5, 4.x, 9.1. Do not quote the book text.

## D. Doepfer and the classics

**doepfer-tech** — A-100 Technical Details. Dieter Doepfer created the A-100, the origin of the Eurorack format. https://doepfer.de/a100_man/a100t_e.htm · EN · free.
Hardware levels: audio about 10 V peak-to-peak, LFOs about ±2.5 V, ADSR 0…+8 V, 1V/oct, gates and triggers 0…+5 V on the rising edge. A useful contrast to the VCV standards. Lessons: 1.2, 1.3, 5.1.
Quote: "Only rectangle shaped waveforms should be used as Trigger , Gate or Clock sources"

**doepfer-a100** — A-100 overview and complete user's guide. https://doepfer.de/a100e.htm, https://doepfer.de/a100_man/A100_Manual_complete.pdf · EN · free.
The modular as an open system; sections 3.1 "Principles of Voltage Control" and 3.2 "Signals in the A-100". Lessons: 0.2, 1.2, 1.5.
Quote: "A modular is an open system, in which practically anything is possible"

**strange-book** — Allen Strange, "Electronic Music: Systems, Techniques, and Controls" (1972; 2nd ed. 1983). https://www.goodreads.com/book/show/3308580 · EN · out of print, no free chapters.
The classic textbook on patching voltage-controlled systems (Buchla, E-mu examples); recommended by Doepfer. Lessons: 1.2, 1.5, 4.x. Reading recommendation only.

## E. Manufacturer manuals that explain general ideas well

**mi-clouds** — Mutable Instruments Clouds manual. Émilie Gillet designed the Mutable modules; Texture Synthesizer in Audible Instruments is their VCV port. https://pichenettes.github.io/mutable-instruments-documentation/modules/clouds/manual/ · EN · free.
Granular synthesis: buffer, grains, Position, Size, Pitch, Density, Texture, Blend, Freeze, quality modes; a quick-start patch. Lessons: 8.7.
Quote: "Clouds is a granular audio processor."

**mi-marbles** — Mutable Instruments Marbles manual. Émilie Gillet. https://pichenettes.github.io/mutable-instruments-documentation/modules/marbles/manual/ · EN · free.
Controlled randomness: random gates with bias, random voltages with spread and steps (from S&H to smooth), scale quantising, repetition ("Déjà vu"). Lessons: 4.4, 4.5, 5.3. No quote.

**mn-maths** — Make Noise MATHS manual. https://www.makenoise-manuals.com/maths/maths-manual.pdf · EN · free.
One function generator as slew/portamento, AR/ASR envelope, cycling LFO, attenuverters and a SUM/OR mixer. Shows well that a "module type" is really a signal role. Lessons: 1.4, 3.4, 4.3, 4.5, 4.6.
Quote: "Use for Lag, Portamento, ASR (Attack Sustain Release type envelopes)."

## F. VCV Rack: educators and community

**omri-site** — Omri Cohen, website. Producer and educator in analogue and virtual modular; the VCV forum recommends him to beginners first. https://omricohen-music.com · EN · YouTube free, courses paid. Lessons: background for all parts.

**omri-first-minutes** — Omri Cohen, "Your first minutes in VCV 2", 2021-12-22. https://www.youtube.com/watch?v=c-27PRPxwTg · EN · free · video.
The startup template, audio and MIDI setup, a first patch, useful shortcuts. Lessons: 0.1, 0.2, 0.3.

**omri-beginner-playlist** — Omri Cohen, "Beginner-Friendly videos" playlist, 10 videos. https://www.youtube.com/playlist?list=PLiECaNQx239s0Z1WmH5jQAcEZf3mLLHnX · EN · free. Lessons: 1.x–3.x.

**omri-basics-course** — Omri Cohen, "The Basics of Modular Synthesis with VCV Rack 2". https://omricohen.gumroad.com/l/modularbasics · EN · 30 EUR · video.
The building blocks of modular synthesis as a video glossary. Lessons: 1.x–4.x.
Quote: "a sort of a glossary in video format"

**omri-practical-course** — Omri Cohen, "Getting Practical with Modular Synthesis in VCV Rack 2". https://omricohen.gumroad.com/l/practicalmodular · EN · 50 EUR · 40 videos.
Four blocks of 10 videos: keyboard patching (subtractive, FM, sync, AM), sequencing (1V/oct, quantising, clock, switches, S&H), rhythm (drum synthesis, logic, side-chain), generative patching. Lessons: 2.2, 4.4, 5.1–5.5, 8.1–8.5.
Quote: "This series is very much suited for beginners."

**omri-vca-course** — Omri Cohen, "VCAs in the Modular Environment with VCV Rack 2". https://omricohen.gumroad.com/l/vcacourse · EN · 30 EUR · video and PDF.
AM and ring modulation, ducking, panning and crossfading, CV shaping. Lessons: 3.3, 4.6, 7.4.
Quote: "fitting for beginners and advanced users alike"

**omri-303** — Omri Cohen, "Let's build the Roland TB-303 in VCV Rack", 2020-04-02. https://www.youtube.com/watch?v=fpgDMxE-dfc · EN · free · video. The TB-303 voice in VCV; patches on Patchstorage. Lessons: 8.4.

**omri-techno** — Omri Cohen, "Building a Techno patch from scratch with the VCV Drums", 2023-10-23. https://www.youtube.com/watch?v=zjJor81VQNo · EN · free · video.
A track one instrument at a time: kick pattern, master processing, hi-hats, variations, bass, sequence, performance controls. Closest to the structure of part 9. Note: VCV Drums is a paid plugin; the course builds drums from Fundamental. Lessons: 9.1–9.3, 5.6.

**omri-generative** — Omri Cohen, "Let's build a self-playing generative ambient patch", February 2023. https://www.youtube.com/watch?v=P3WgQ4Ez-_o · EN · free · video.
Five voices, a quantiser, counters, chord changes from a comparator, a noise texture, a fade-out — arrangement through counters. Lessons: 5.3, 5.4, 8.6, 9.4.

**omri-moving-bass** — Omri Cohen, "Improvised Patch Tutorial #1 – Moving Bass", 2018. https://www.youtube.com/watch?v=aBCw0-ffQvs · EN · free. Fundamental modules only; Rack v0 interface. Lessons: 8.4.

**vcv-forum-getting-started** — "Getting started with VCV Rack", forum wiki thread, Lars Bjerregaard, since 2019-01-01. https://community.vcvrack.com/t/getting-started-with-vcv-rack/747 · EN · free.
Account, plugin installation (Library → Update all → restart), terminology, curated links. Lessons: 0.1, 0.2.

**vcv-forum-video-tutorials** — "Getting started with VCV Rack – Video tutorials", wiki thread, Lars Bjerregaard and Jens Peter Nielsen. https://community.vcvrack.com/t/getting-started-with-vcv-rack-video-tutorials/11204 · EN · free.
Video lists for Rack 2 and older versions: Omri Cohen, Artem Leonov, Peter O'Neill, Red Means Recording, Jakub Ciupinski and others. Lessons: 0.x–3.x.

**vcv-forum-how-to-start** — "How To Start Out?", July 2020. https://community.vcvrack.com/t/how-to-start-out/10559 · EN · free.
Community advice: master Fundamental first, then the manual and Omri Cohen, experiment freely; recommends Learning Modular and Synth Secrets. Lessons: 0.2, 1.5.

**vcv-forum-song** — "Creating song with multiple sequencers", forum thread with replies from Squinky (author of Seq++). https://community.vcvrack.com/t/creating-song-with-multiple-sequencers/18708 · EN · free.
Ways to arrange a track in VCV: everything runs all the time and parts are opened with mutes or VCAs; Clocked and bar-count dividers; presets; sequencers with counters. Lessons: 5.6, 9.4.
Quote: "There are ways to trigger things when the time is right"

**leonov-seq3** — Artem Leonov (VCV Rack Ideas channel), "Updated SEQ 3 and QUANTIZER patch from scratch in final VCV v2 version", 2021-11-22. https://www.youtube.com/watch?v=MeyvB_87eKM · EN · free · video. SEQ 3 step CV output in Rack 2 and the Quantizer. Lessons: 5.2, 5.3.

**ciupinski-start** — Jakub Ciupinski (teaches at SUNY Purchase, uses Rack in class), "VCV Rack – Getting Started", 2020-09-20. https://www.youtube.com/watch?v=dvimOJpMTGw · EN · free · Rack 1. Lessons: 0.2, 1.5.

**koma-beginner** — koma2000, "VCV Rack Beginner Tutorial (90 Minutes, Full Overview)", 2024-01-12. https://www.youtube.com/watch?v=7xUJmCgCgv4 · EN · free · video. The forum wiki calls it an excellent introduction to Rack 2.
Installation and account, the default patch, VCO/LFO/envelopes, a stereo noise patch, recording. Lessons: 0.1–0.3, 2.5, 7.4, 7.5.

**kane-musichackspace** — Phelan Kane (producer, engineer, educator), Music Hackspace, "Getting Started With VCV Rack 2". https://musichackspace.org/courses/getting-started-with-vcv-rack-2 · EN · 12 videos, 1 h 19 min.
Patching, MIDI to CV, gates and envelopes, two oscillators, filters, modulation, MIDI CC. Lessons: 2.3, 3.1–3.5, 6.1, 6.2.
Quote: "Patch a modular synth in the free VCV Rack 2 software"

**aikin-how-to-rack** — Jim Aikin (long-time synth journalist and author), "How to Rack", PDF, July 2019, Rack 1.1. https://www.musicwords.net/vcv/How_to_Rack.pdf · EN · free.
The big picture, patch concepts, module suggestions, ideas. Lessons: 1.5, 2.x, 3.x.
Quote: "A patch cord carries a signal from an output to an input."

## G. Third-party plugins used in the course

**impromptu-clocked** — Impromptu Modular, Clocked. Marc Boulé, the plugin's author. https://marcboule.github.io/ImpromptuModular/#clocked · EN · free.
Master clock 30–300 BPM, outputs with division and multiplication, swing, per-output delay and pulse width, Run and Reset, sync to an external clock. The README also covers song-mode sequencers (Foundry, PhraseSeq). Lessons: 5.1, 5.2, 9.1, 9.2, 9.4.
Quote: "The clock can also be synchronized to an external clock source."

**valley-plateau** — Valley, Plateau. Dale Johnson (Valley Audio). https://valleyaudio.github.io/rack/plateau/ · EN · free.
A Dattorro plate reverb: Dry/Wet, PreDelay, Size, Diffusion, Decay, filters, Hold/Clear (freeze), Tuned mode on 1V/oct. Lessons: 7.2, 8.6.
Quote: "Sets the overall delay time and apparent 'size' of the reverb."

**ai-texture** — Audible Instruments, Texture Synthesizer (VCV's port of Clouds). https://library.vcvrack.com/AudibleInstruments/Clouds · EN · free. Controls are described in the `mi-clouds` manual. Lessons: 8.7.

## H. Russian-language materials

**zwook-geogenik** — GeoGenik (zwook.ru), «Синтез звука. Базовый курс», 11 lessons; lessons 9–10 are about VCV. https://zwook.ru/material/vcv-rack-nachalo, https://zwook.ru/material/11-oscillator-i-modulacia · RU · video and text.
Lessons 1–8 cover general theory (oscillator, cutoff, ADSR, LFO). Lessons: 0.1, 1.1, 2.1, 4.1.
Quote: "VCV Rack состоит из модулей, которые нужно собирать по отдельности"

**samesound-first-patch** — Кирилл Колотвин, SAMESOUND, «Введение в модульные синтезаторы: создаём первый патч в VCV Rack», 2021-03-05. https://samesound.ru/soft/145529-vvedenie-v-modulnye-sintezatory-sozdayom-pervyj-patch-v-vcv-rack · RU · free · text. Rack v1 module names.
Module types, then a step-by-step patch: VCO, VCA, SEQ 3 with ADSR, VCF with an LFO. Lessons: 1.5, 3.5, 5.2.
Quote: "Вуаля, всё запищит."

**shatnyy-lesson1** — Tim Shatnyy, «VCV Rack – урок 1. Введение. Получаем звук», 2021-11-02. https://www.youtube.com/watch?v=lQqmwcwMmJk · RU · free · video, patch download. Later lessons of the series not checked. Lessons: 0.1, 0.3.

## I. Drums, bass, sequencing, arrangement, mixing

**ne-kick** — Noise Engineering (Eurorack manufacturer, official blog), "Kick out the jams: synthesizing some kicks (for jams)". https://noiseengineering.us/blogs/loquelic-literitas-the-blog/kick-out-the-jams-synthsizing-some-kicks-for-jams · EN · free.
A kick from a sine, a falling pitch envelope and a short VCA envelope; the trigger also resets the oscillator phase so every hit is the same; the click is a noise burst or an envelope mixed in as audio. Lessons: 8.1.

**ne-clap** — Noise Engineering, "Percussion Synthesis, Part 3: The Sound of One Synth Clapping". https://noiseengineering.us/blogs/loquelic-literitas-the-blog/percussion-synthesis-part-3-the-sound-of-one-synth-clapping/ · EN · free.
A clap: noise through a resonant band-pass filter, the envelope retriggered several times in a row (the TR-909 approach), then reverb. Lessons: 8.2.
Quote: "Reverb is always fun on claps"

**ne-hats** — Noise Engineering, "Patching hihats from scratch with FM". https://noiseengineering.us/blogs/loquelic-literitas-the-blog/creating-fm-hats/ · EN · free.
Two inharmonically detuned oscillators with FM, a steep high-pass, a short VCA; long decay — open hat, short — closed. Lessons: 8.3.
Quote: "we'll throw tuning and harmonic relationships to the wind"

**mr-303-guide** — Future Music / MusicRadar, "The producer's guide to the Roland TB-303 (and its clones)", 2024-03-03. https://www.musicradar.com/news/producers-guide-to-the-roland-tb-303-and-clones · EN · free.
What accent does (volume and filter-envelope depth) and slide (portamento into the next note); the 18 dB filter, Env Mod and Decay, high resonance. Lessons: 8.4.
Quote: "add emphasis to notes by increasing the volume and filter envelope depth"

**mr-acid-howto** — Jon Musgrave, Simon Truss, MusicRadar, "How to make an acid bassline like Fatboy Slim's Everybody Needs A 303 (without using a 303)", 2023-02-16. https://www.musicradar.com/how-to/make-acid-bassline-fatboy-slim-303 · EN · free.
A TB-303 on a generic synth: step velocity to cutoff and resonance as accent, held steps as slide, a short filter envelope. Lessons: 8.4.
Quote: "the distinctive TB-303 slide effect"

**ne-acid-repetitor** — Noise Engineering, "Quick patch: Bassline sequencing with Gamut Repetitor". https://noiseengineering.us/blogs/loquelic-literitas-the-blog/gamut-repetitor-acid-basslines/ · EN · free.
An acid line in Eurorack: pitch through a slew limiter, a trigger switches on the slew time, another trigger through an attenuator to the cutoff as accent. Lessons: 4.4, 4.5, 8.4.
Quote: "each time Out 3 goes high, our voice will have a brighter timbre."

**sq-arp** — Squinky Labs, Arpeggiator documentation (SqHarmony for VCV). https://github.com/squinkylabs/SqHarmony/blob/main/docs/arpeggiator.md · EN · free.
An arpeggiator fed by a polyphonic MIDI-CV, one step per clock pulse; modes up, down, up-down, as played, shuffle; Hold; note length = clock pulse width. In the course it is an idea for 8.5; the module is optional. Lessons: 5.1, 6.4, 8.5.
Quote: "Notes are played when the clock is high"

**ne-clocks** — Noise Engineering, "Getting Started: Clocks". https://noiseengineering.us/blogs/loquelic-literitas-the-blog/getting-started-clocks/ · EN · free.
Clock generators, dividers (with reset so they stay in phase), multipliers; two ways sequencers interpret a reset. Lessons: 5.1, 9.1.
Quote (**verified**): "Reset signals tell sequencers when to start over."

**ne-quantizers** — Noise Engineering, "Getting Started: Quantizers". https://noiseengineering.us/blogs/loquelic-literitas-the-blog/getting-started-quantizers/ · EN · free.
A quantiser snaps CV to a scale; noise → S&H → quantiser with an attenuator gives random melodies in key. Lessons: 4.4, 4.5, 5.3.
Quote: "they're easier to use than they are to explain."

**ne-seq-switches** — Noise Engineering, "Getting started: sequential switches". https://noiseengineering.us/blogs/loquelic-literitas-the-blog/getting-started-sequential-switches/ · EN · free.
1→N and N→1 switches, matrices; extending a sequencer, muting voices by switching gates, moving between CV sources. Lessons: 5.5, 9.4.
Quote: "Sequential switches are a utility that was originally designed to extend 8-step sequencers"

**ne-mutes** — Noise Engineering, "Getting Started: Mutes". https://noiseengineering.us/blogs/loquelic-literitas-the-blog/getting-started-mutes/ · EN · free.
What muting audio, gates (the tail decays naturally), pitch CV (the voice drops to the root) or the clock does; a short clock mute plus a periodic reset gives breaks and variations. Lessons: 5.6, 9.4. No quote.

**ne-mixers** — Noise Engineering, "Getting Started: Mixers". https://noiseengineering.us/blogs/loquelic-literitas-the-blog/getting-started-mixers/ · EN · free.
Unity mixers, per-channel levels, stereo and panning, LFO-driven panning, sends to a 100% wet reverb. Lessons: 7.2, 7.3, 7.4.
Quote: "The simplest mixers have a number of inputs, and an output."

**pc-seq-without-seq** — Perfect Circuit Signal, "Sequencing Without Sequencers". https://www.perfectcircuit.com/signal/sequencing-without-sequencers · EN · free.
Sequences from clocks, dividers, logic and comparators, switches; a quantiser to make the result melodic. Lessons: 5.1, 5.4, 5.5.

**pc-learning-synthesis** — Perfect Circuit, "Learning Synthesis" series. Index: https://www.perfectcircuit.com/signal/learn-synthesis · EN · free.
Articles: oscillators (Eldar Tagi, 2019) /signal/learning-synthesis-oscillators — 2.1, 2.2, 2.4; filters /signal/learning-synthesis-filters — 3.1, 3.2; VCAs (Ryan Gaston, 2019) /signal/learning-synthesis-vcas — 3.3; envelopes (Ryan Gaston, 2019) /signal/learning-synthesis-envelopes-1 — 3.4; LFOs /signal/learning-synthesis-lfos — 4.1; noise (Naomi Mitchell, 2020) /signal/learning-synthesis-noise — 2.5, 8.2, 8.3; CV processing /signal/learning-synthesis-cv-processing — 1.4, 4.3; quantisers /signal/learning-synthesis-quantizers — 5.3; logic /signal/learning-synthesis-logic — 4.6, 5.4, 5.5.
Quote (oscillators): "Oscillators or VCOs (Voltage Controlled Oscillators) are sections within synthesizers"

**pc-line-level** — Ryan Gaston, Perfect Circuit, "Eurorack and Line Level Signals", 2020-06-29. https://perfectcircuit.com/signal/eurorack-line-level · EN · free.
Modular level (10 V peak-to-peak) vs line level; why a hot signal eats the headroom of a mixer or interface. Lessons: 7.3, 7.5.

**dewald-gain** — Gahlord Dewald (musician and writer), "Modular live performance: controlling volume", 2022-01-02. https://gahlorddewald.com/gain-staging-modular.html · EN · free.
Gain staging in practice: keep module outputs hot and the next mixer's input low; noise on unbalanced connections. Lessons: 7.3.
Quote: "Set your modular outputs as hot as you can."

**sos-modular-effects** — William Stokes, SOS, "Getting Wet: Using Effects" (Modular Technique column), May 2024. https://www.soundonsound.com/techniques/modular-technique-effects-whats-new · EN · free.
Reverb and delay as a voice of their own on a mixer channel, not just a send: the return through a VCA, filtered under stepped CV, distorted, modulated. Lessons: 7.1, 7.2.

**sos-gigging** — Robin Vincent, SOS, "The SOS Guide To Gigging With Modular Synths", April 2025. https://www.soundonsound.com/techniques/sos-guide-gigging-modular-synths · EN · free.
Set length and pacing, transitions, improvisation vs preparation, a limiter on the output. Lessons: 5.6, 9.4.
Quote: "more notes and ideas do not necessarily make for better music"

**cdm-jw-arrange** — Peter Kirn, CDM, "New VCV Rack modules – including JW Arrange so you can finish tracks", 2025-06-03. https://cdm.link/new-vcv-rack-modules-jw-arrange/ · EN · free.
The free JW Arrange module: clocked lanes switch parts of the patch on and off along a timeline; limitation — on/off only, no CV values. Lessons: 9.4 (as an extra path; not required in the core course).
Quote: "Starting is easy; finishing is hard."

**cdm-multitrack** — Peter Kirn, CDM, "How to make a multitrack recording in VCV Rack modular, free", 2019-03-06. https://cdm.link/how-to-make-a-multitrack-recording-in-vcv-rack-modular-free/ · EN · free · Rack v1.
Recording stems inside the patch (NYSTHI recorders), a limiter before recording, splitting a multichannel WAV. Lessons: 7.5, 9.5 — as an extra.
Quote: "The multitrack is the one I use the most."

**loopop-sh-switch** — Loopop, "Storing generative melodies & rhythms using sample & hold and a sequential switch", 2020-06-29. https://loopopmusic.com/modular-analog-ram-how-to-store-generative-melodies-rhythms-using-sample-hold-and-a-sequential-switch · EN · free · video, demo in VCV Rack.
Several S&H stages through a sequential switch "freeze" a random pattern and replay it. Lessons: 4.4, 4.5, 5.5.
Quote: "freeze randomness once a compelling pattern is presented"

**lsbu-performance** — Hillegonda Rietveld, Justin Randell, "Eurorack to VCV Rack: Modular Synthesis as Compositional Performance", chapter in "Modular Synthesis: Patching Machines and People" (Routledge, 2024). https://openresearch.lsbu.ac.uk/item/9635w · EN · academic chapter.
Patching as embodied performance; hardware Eurorack compared with VCV Rack. For the course's methodological rationale, not for how-to lessons. Lessons: 9.1, 9.4.

---

## Matrix "lesson → main sources"

At least two sources per lesson (a plan rule). The source closest to VCV comes first.

| Lesson | Sources |
| --- | --- |
| 0.1 | vcv-manual-installing, vcv-forum-getting-started, omri-first-minutes, shatnyy-lesson1 |
| 0.2 | vcv-manual-menu-bar, vcv-manual-keys, omri-first-minutes, doepfer-a100 |
| 0.3 | vcv-manual-getting-started, vcv-manual-core, koma-beginner |
| 0.4 | Neural-Harmonics README (facts — appendix D of the plan), vcv-manual-getting-started |
| 1.1 | sos-01, lm-basic-concepts, zwook-geogenik |
| 1.2 | vcv-voltage-standards, sos-07, doepfer-tech, lm-glossary |
| 1.3 | vcv-voltage-standards, lm-glossary, sos-16 |
| 1.4 | vcv-free-manual (#8vert, #CVMix, #Rescale), mn-maths, pc-learning-synthesis (CV processing) |
| 1.5 | lm-first-voice, aikin-how-to-rack, vcv-voltage-standards, samesound-first-patch |
| 2.1 | vcv-free-manual (#VCO), sos-01, pc-learning-synthesis (oscillators) |
| 2.2 | vcv-free-manual (#VCO), sos-10, sos-12, omri-practical-course |
| 2.3 | sos-strings, kane-musichackspace |
| 2.4 | vcv-free-manual (#VCO2), pc-learning-synthesis (oscillators) |
| 2.5 | vcv-free-manual (#Noise), pc-learning-synthesis (noise), lm-basic-concepts |
| 3.1 | vcv-free-manual (#VCF), sos-06, pc-learning-synthesis (filters) |
| 3.2 | sos-06, sos-34, vcv-free-manual (#VCF) |
| 3.3 | vcv-free-manual (#VCA-1, #VCMixer), sos-09, pc-learning-synthesis (VCAs), omri-vca-course |
| 3.4 | vcv-free-manual (#ADSR, #Push, #Pulses), sos-07, pc-learning-synthesis (envelopes) |
| 3.5 | lm-first-voice, samesound-first-patch, kane-musichackspace |
| 4.1 | vcv-free-manual (#LFO), sos-10, pc-learning-synthesis (LFOs) |
| 4.2 | vcv-free-manual (#LFO2), vcv-free-manual (#VCO2) — few expert sources |
| 4.3 | sos-09, sos-10, mn-maths, vcv-free-manual (#CVMix) |
| 4.4 | vcv-free-manual (#Random, #RandomValues), mi-marbles, ne-quantizers |
| 4.5 | sos-16, vcv-free-manual (#Process, #SHASR), mn-maths, loopop-sh-switch |
| 4.6 | vcv-free-manual (#Fade, #Compare), omri-vca-course, pc-learning-synthesis (logic) |
| 5.1 | ne-clocks, impromptu-clocked, vcv-voltage-standards, doepfer-tech |
| 5.2 | vcv-free-manual (#SEQ3), leonov-seq3, samesound-first-patch |
| 5.3 | vcv-free-manual (#Quantizer), ne-quantizers, sos-16, mi-marbles |
| 5.4 | vcv-free-manual (#Logic, #Gates), pc-learning-synthesis (logic), pc-seq-without-seq |
| 5.5 | vcv-free-manual (#SequentialSwitch1/2), ne-seq-switches, loopop-sh-switch |
| 5.6 | vcv-free-manual (#Mutes, #Pulses, #Push), ne-mutes, sos-gigging |
| 6.1 | vcv-manual-core (#MIDI-CV), kane-musichackspace |
| 6.2 | vcv-manual-core (#MIDI-Map, #MIDI-CC), kane-musichackspace |
| 6.3 | vcv-manual-core (#MIDI-Gate), vcv-voltage-standards |
| 6.4 | vcv-manual-polyphony, vcv-manual-core (#MIDI-CV), vcv-voltage-standards |
| 7.1 | vcv-free-manual (#Delay), sos-modular-effects |
| 7.2 | valley-plateau, ne-mixers, sos-modular-effects |
| 7.3 | vcv-voltage-standards, sos-09, dewald-gain, pc-line-level, ne-mixers |
| 7.4 | vcv-free-manual (#Fade, #MidSide), ne-mixers, omri-vca-course — few expert sources |
| 7.5 | vcv-recorder, pc-line-level, cdm-multitrack |
| 8.1 | sos-33, sos-34, ne-kick |
| 8.2 | sos-35, sos-36, ne-clap |
| 8.3 | ne-hats, sos-38, sos-39 |
| 8.4 | mr-303-guide, mr-acid-howto, ne-acid-repetitor, omri-303 |
| 8.5 | sq-arp, ne-quantizers, omri-practical-course |
| 8.6 | sos-strings, vcv-manual-polyphony, valley-plateau |
| 8.7 | mi-clouds, ai-texture |
| 9.1 | ne-clocks, impromptu-clocked, omri-techno, patch-tweak |
| 9.2 | omri-techno, sos-34, impromptu-clocked |
| 9.3 | omri-techno, ne-quantizers |
| 9.4 | vcv-forum-song, ne-mutes, ne-seq-switches, omri-generative, sos-gigging |
| 9.5 | vcv-recorder, vcv-voltage-standards, dewald-gain |
