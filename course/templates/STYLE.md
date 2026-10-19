## Appendix B. Style guide for learner text

- Address the learner as «ты». Short sentences (≤ 18 words preferred, 22 max). One idea per paragraph, 1–3 sentences per paragraph.
- Start with what the learner sees or hears, then the term. Bold a term on first use in the lesson or link it as `[[термин]]`.
- Rack names exactly as printed on the panel (Latin), with a Russian gloss on first use: «**Frequency** (частота)». Port names in the form «выход **Sawtooth**», «вход **V/Oct**».
- Numbers with units: «440 Гц», «±5 В», «1 мс», «120 BPM».
- No filler: banned — «давай разберёмся», «важно отметить», «стоит отметить», «как мы видим», «итак», «в этом уроке мы», «следует», «является», «осуществляет», «данный», exclamation marks.
- Never say what the learner will feel («это просто»). Never claim what something sounds like in a way the learner cannot verify; describe what to compare.
- Captions are imperative or descriptive, ≤ 40 words: «Поверни вправо — фильтр открывается».
- FAQ questions are written as a beginner would ask them, in their words, including confused ones («Почему ничего не звучит?»).
- Sources: names of authors and works on the recap page, in plain text. Quotes only from `quotes.json`, at most one per lesson.

## Appendix C. FAQ and hints — so the tutor rarely needs to think

- Every text-bearing block: 2–4 FAQ items. Sources for questions: the old lesson's misconceptions («Пробник» wrong options), «Проверка» failure comments, «Слом» symptoms, the card's «Типовые ошибки», and the obvious «что будет, если…» about the block's knobs.
- Each FAQ answer ≤ 70 words, answers the question in the first sentence, points to something on screen when possible.
- Every practice check item: a 3-step hint ladder (vague → where to look → exact fix with port/knob names). The `Task` module shows step 1 on the first failure, step 2 on «Ещё подсказка», step 3 on the next.
- Every quiz option: `why` ≤ 40 words.
- `simpler` text per story block (optional, ≤ 60 words): an everyday analogy. The «Объясни проще» button uses it locally when present; only if absent does it go to the tutor.

## Appendix D. Mapping old lessons to blocks
