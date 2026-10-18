---
id: "p8-05-lead-arp"
num: "8.5"
title: "Лид и арпеджио"
part: 8
modules: ["Fundamental/VCO", "Fundamental/VCF", "Fundamental/Quantizer", "Fundamental/Random", "Fundamental/Delay"]
depends_on: ["p5-03-quantizer", "p4-04-randomness", "p7-01-delay"]
concepts: ["арпеджио", "ноты шкалы или аккорда как ограничение", "размах случайной мелодии"]
sources: ["sq-arp", "ne-quantizers", "omri-practical-course"]
check: "checks/p8-05-lead-arp.json"
---

# 8.5 Лид и арпеджио

## Зачем

После урока у ученика пятый инструмент: лид-партия, которая сама перебирает ноты по такту, не выходит за выбранную шкалу или аккорд и звучит с эхом в такт. Меняя шкалу в квантизаторе, он получает то мелодию, то арпеджио. Файл `vcv-8.5.vcv` пойдёт в часть 9: в уроке 9.3 лид встанет в ту же тональность, что бас и пэд.

## Пробник

1. Вопрос: что делает Quantizer в этой партии?
   - Верно: округляет случайное напряжение до ближайшей ноты выбранной шкалы, чтобы партия не фальшивила.
   - Неверно: делает случайные числа более случайными. Заблуждение: квантизатор — генератор случайности.
   - Неверно: удлиняет каждую ноту. Заблуждение: квантизатор управляет временем.
2. Вопрос: как сузить размах случайных нот с десяти октав до двух?
   - Верно: ослабить выход Random ручкой Row 1 gain у 8vert до 10–40%.
   - Неверно: убавить Level у VCA-1. Заблуждение: Level меняет громкость, а не высоту.
   - Неверно: убавить Tempo у Clocked. Заблуждение: размах нот зависит от скорости.
3. Вопрос: как получить арпеджио, а не просто мелодию в тональности?
   - Верно: оставить в квантизаторе только ноты одного аккорда (например, до-ми-бемоль-соль).
   - Неверно: поднять Feedback у Delay. Заблуждение: арпеджио — это эхо.
   - Неверно: поднять Resonance у VCF. Заблуждение: арпеджио — это тембр.

## Понятие

Идея без модулей. Мелодию можно строить как случайные ноты, пропущенные через фильтр правил. Случайный источник даёт новое напряжение на каждый такт (урок 4.4); квантизатор (урок 5.3) округляет его до ближайшей разрешённой ноты, и всё, что получилось, укладывается в тональность. Чем меньше разрешённых нот, тем «музыкальнее» и повторяемее партия; если разрешены только ноты одного аккорда, получается арпеджио (ноты аккорда одна за другой). [ne-quantizers](https://noiseengineering.us/blogs/loquelic-literitas-the-blog/getting-started-quantizers/) пишет о квантизаторах, что "they're easier to use than they are to explain." ([ne-quantizers](https://noiseengineering.us/blogs/loquelic-literitas-the-blog/getting-started-quantizers/)).

В железных синтезаторах арпеджиатор ещё и берёт ноты из зажатых клавиш. Для VCV есть отдельный модуль: [sq-arp](https://github.com/squinkylabs/SqHarmony/blob/main/docs/arpeggiator.md) (Squinky Labs) берёт аккорд из полифонического MIDI-CV и играет по шагу на импульс такта; автор подчёркивает: "Notes are played when the clock is high" ([sq-arp](https://github.com/squinkylabs/SqHarmony/blob/main/docs/arpeggiator.md)). Мы его не ставим: он не входит в курс, но принцип тот же (такт, ноты, длительность). Приём «случайные ноты через квантизатор» разбирают в блоках про секвенирование и генеративные патчи курса [omri-practical-course](https://omricohen.gumroad.com/l/practicalmodular) (платный, мы его не пересказываем).

Эхо в такт: Delay в 7.1 умеет синхронизироваться с клоком, и повторы ложатся на ту же сетку, что ноты.

Что показать (агент рисует): три строки: случайные значения ступеньками (Random Stepped), те же значения, прижатые к нотам шкалы (Quantizer Pitch), и стрелка такта, которая запускает и Random, и огибающую.

## Анатомия

Карточки: `ImpromptuModular.Clocked.md` (урок 5.1), `Fundamental.Random.md`, `Fundamental.8vert.md`, `Fundamental.Quantizer.md`, `Fundamental.VCO.md`, `Fundamental.VCF.md`, `Fundamental.Delay.md`. Не больше трёх терминов на шаг:

1. Clocked (из 5.1): выход Clock 1 — ровный такт; Master clock (в bpm) задаёт скорость. Ручки swing, ratio, delay не трогаем. Термин: арпеджио.
2. Random (из 4.4): вход Trigger (сюда такт), выход Stepped (ступенчатое случайное напряжение), ручки Shape (для чистых ступеней ноль), Random spread (насколько новая нота далека от предыдущей), Trigger probability (вероятность новой ноты), Offset (Bipolar: ±5 В). Остальные выходы (Linear, Smooth, Exponential, Trigger) и входы CV пока не нужны.
3. 8vert (из 1.4): Row 1 вход и Row 1 выход, ручка Row 1 gain (в процентах, потому что кабель в Row 1): ослабляет ±5 В до ±1 В (две октавы). Термин: размах случайной мелодии.
4. Quantizer (из 5.3): вход 1V/octave pitch, выход Pitch, ручка Pre-offset (в полутонах). Шкала и разрешённые ноты выбираются в контекстном меню (пункты проверить вручную: где выбор шкалы, корня и отдельных нот). Термин: ноты шкалы или аккорда как ограничение.
5. VCO и VCF: как в 3.5; VCO Sawtooth, Frequency на C4, VCF Cutoff frequency около 2500 Гц, Resonance небольшой.
6. ADSR и VCA-1: короткая нота (Decay около 200 мс), Gate от тех же импульсов такта.
7. Delay (из 7.1): входы Audio и Clock, выходы Mix и Wet, ручки Time, Feedback, Tone, Mix. С кабелем в Clock Time перестаёт быть секундами и множит такт (что показывает дисплей, проверить вручную).

## Демо

Агент строит лид в своём ряду; ученик смотрит. Перед первым звуком напомнить убавить громкость.

Модули: ImpromptuModular/Clocked, Fundamental/Random, Fundamental/8vert, Fundamental/Quantizer, Fundamental/VCO, Fundamental/VCF, Fundamental/ADSR, Fundamental/VCA-1, Fundamental/Delay, Fundamental/Scope; звук идёт в Audio ученика.

Кабели:

1. Clocked `Clock 1` → Random `Trigger`.
2. Clocked `Clock 1` → ADSR `Gate`.
3. Random `Stepped` → 8vert `Row 1`.
4. 8vert `Row 1` (выход) → Quantizer `1V/octave pitch`.
5. Quantizer `Pitch` → VCO `1V/octave pitch`.
6. VCO `Sawtooth` → VCF `Audio`.
7. VCF `Lowpass filter` → VCA-1 `Channel`.
8. ADSR `Envelope` → VCA-1 `CV`.
9. VCA-1 `Channel` → Delay `Audio`.
10. Clocked `Clock 1` → Delay `Clock` (подключить позже, по порядку демо).
11. Delay `Mix` → `To "device output 1"` и `To "device output 2"` (Audio ученика).
12. Random `Stepped` → Scope `Ch 1`; Quantizer `Pitch` → Scope `Ch 2`.

Параметры (displayValue):

| Модуль | Параметр | Цель |
| --- | --- | --- |
| Clocked | Master clock | 240; Run не трогать: новый Clocked уже идёт |
| Random | Shape / Random spread / Trigger probability | 0 / 60 / 100 |
| Random | Offset | Bipolar |
| 8vert | Row 1 gain | 20 (проценты, после кабеля 3) |
| Quantizer | Pre-offset | 0 |
| VCF | Cutoff frequency / Resonance | 2500 / 10 |
| ADSR | Attack / Decay / Sustain / Release | 2 / 200 / 0 / 200 |
| VCA-1 | Level | 80 |
| Delay | Mix / Feedback / Tone | 30 / 40 / −30 |
| Delay | Time | 0.375 (до кабеля 10) |
| Scope | Time | 2000 |

Порядок демо: сначала кабели 1–9 и 11 (Delay без Clock): ученик слышит случайную мелодию; на Scope Ch 1 даёт все возможные значения, а Ch 2 — прижатые к нотам. Просить ученика выбрать в меню Quantizer шкалу (например, минорная пентатоника; пункты проверить вручную): мелодия перестаёт фальшивить. Затем ученик оставляет в меню только три ноты аккорда (до, ми-бемоль, соль): слышно арпеджио. Агент читает `Pitch` несколько раз: значения только из этого набора. Затем Row 1 gain 20 → 40 → 10: размах шире или уже. Затем Random spread 60 → 100 → 20: от скачков к шагам по соседним нотам. Затем кабель 10: эхо ложится на сетку такта; агент читает displayValue у Time и сообщает ученику, как теперь читается ручка. В конце агент удаляет свои модули и кабели.

## Покрути сам

1. Master clock от 90 до 300: арпеджио быстрее или медленнее; на Scope чаще или реже ступеньки.
2. Row 1 gain от 10 до 40%: размах от узкого (две-три ноты) до широкого (две октавы); на Scope размах ступенек шире.
3. Random spread от 100 до 20%: от прыжков через весь диапазон к шагам по соседним нотам; Scope Ch 1 меньше скачков.
4. Cutoff frequency у VCF от 800 до 5000 Гц: партия темнее или ярче.
5. Feedback у Delay от 20 до 60% и Mix от 15 до 45%: эхо длиннее и заметнее.
6. Tone у Delay от −60 до 60: повторы темнее (минус) или тоньше (плюс).

## Звук-референс

- «арпеджио», «аккордом»: в меню Quantizer оставить только ноты одного аккорда.
- «мелодичнее», «шагами»: Random spread вниз.
- «хаотичнее», «прыгает»: Random spread вверх.
- «шире», «выше-ниже»: Row 1 gain вверх; «уже»: вниз.
- «быстрее», «медленнее»: Master clock у Clocked.
- «короче», «стаккато»: Decay и Release вниз; «тянется»: вверх.
- «ярче»/«глуше»: Cutoff frequency у VCF.
- «больше эха», «эха мало»: Mix у Delay; «дольше хвост эха»: Feedback.
- «темнее эхо»: Tone у Delay влево.
- «повторяются одни и те же ноты»: Trigger probability ниже 100%: на части тактов новая нота не берётся.

## Практикум

Ученик собирает лид сам, агент ничего не трогает: Clocked, Random (Stepped, Trigger от Clock 1), 8vert (Row 1 gain), Quantizer, VCO Sawtooth → VCF → VCA-1 с ADSR от Clock 1, эхо через Delay с тактом на Clock, Mix в Audio. Условия: Master clock не ниже 90, Row 1 gain от 10 до 40%, Sustain не выше 40%, Feedback не выше 70%, Mix у Delay от 15 до 60%. Шкалу или аккорд ученик выбирает сам. Подсказки только через результат проверки.

## Проверка

Файл `checks/p8-05-lead-arp.json`. Что проверяет: одиннадцать кабелей (Clocked `Clock 1` → Random `Trigger`; Clocked `Clock 1` → ADSR `Gate`; Random `Stepped` → 8vert `Row 1`; 8vert `Row 1` → Quantizer `1V/octave pitch`; Quantizer `Pitch` → VCO `1V/octave pitch`; VCO `Sawtooth` → VCF `Audio`; VCF `Lowpass filter` → VCA-1 `Channel`; ADSR `Envelope` → VCA-1 `CV`; VCA-1 `Channel` → Delay `Audio`; Clocked `Clock 1` → Delay `Clock`; Delay `Mix` → `To "device output 1"`); параметры из условий, а также Shape у Random не выше 20%; на `Pitch` у Quantizer размах не меньше 0,2 В за две секунды; на `Envelope` выше 1 В. Шкала в меню Quantizer через REST не видна: её агент проверяет с учеником по слуху и по значениям `Pitch`.

Лесенка подсказок (вопрос, место, ответ):

- Кабель 1: «Откуда Random знает, когда брать новую ноту?» → «Clocked» → «Clock 1 в Trigger у Random».
- Кабель 2: «Что запускает огибающую?» → «Clocked» → «Clock 1 в Gate у ADSR».
- Кабель 3–5: «Как случайное напряжение становится нотой?» → «8vert, Quantizer, VCO» → «Stepped в Row 1, Row 1 в Quantizer, Pitch в VCO».
- Кабели 6–8: как в 3.5: пила → фильтр → усилитель, огибающая в CV.
- Кабель 9: «Куда идёт звук до эха?» → «Delay» → «Channel у VCA-1 в Audio у Delay».
- Кабель 10: «Эхо не в такт?» → «Clocked» → «Clock 1 в Clock у Delay».
- Shape выше 20%: «Партия съезжает между нотами?» → «Shape у Random» → «в ноль для чистых ступеней».
- `Pitch` стоит: «Идёт ли Clocked, получает ли Random такт?» → «Clock 1, Trigger» → «проверь кабель 1 и Row 1 gain выше нуля».

## Слом

В демо-патче агента (лид играет, Shape = 0): поставить Shape у Random в 100%. Ученик находит на слух и на Scope: между нотами появляются «проходные» (глиссандо через соседние ноты), ступеньки на Ch 1 превращаются в лесенки. Ответ: при Shape 100% Stepped делает до шестнадцати промежуточных ступеней к новому значению; для чистых прыжков нужен 0%. Агент возвращает 0 и удаляет демо.

## Итог

Вопросы для повторения:

1. Как из случайного напряжения получить ноту, и как из мелодии получить арпеджио?
2. Чем ограничивается размах партии и чем её «прыгучесть»?
3. Что меняется в эхе, когда в Clock у Delay приходит такт?

Сохрани патч как `vcv-8.5.vcv`: он понадобится в части 9. Следующий урок: 8.6 «Пэд».

Источники урока: [sq-arp](https://github.com/squinkylabs/SqHarmony/blob/main/docs/arpeggiator.md), [ne-quantizers](https://noiseengineering.us/blogs/loquelic-literitas-the-blog/getting-started-quantizers/), [omri-practical-course](https://omricohen.gumroad.com/l/practicalmodular).
