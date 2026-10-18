---
id: "p8-02-snare-clap"
num: "8.2"
title: "Малый барабан и клэп"
part: 8
modules: ["Fundamental/Noise", "Fundamental/VCF", "Fundamental/VCO", "Fundamental/ADSR", "Fundamental/Mixer"]
depends_on: ["p2-05-noise", "p3-04-adsr"]
concepts: ["тон и шум в барабане", "баланс тона и шума", "клэп как серия ретриггеров"]
sources: ["sos-35", "sos-36", "ne-clap"]
check: "checks/p8-02-snare-clap.json"
---

# 8.2 Малый барабан и клэп

## Зачем

После урока у ученика есть второй инструмент трека: малый барабан из треугольника и шума с двумя короткими огибающими, плюс клэп на той же основе, но с несколькими быстрыми перезапусками огибающей. Файл `vcv-8.2.vcv` пойдёт в часть 9: в уроке 9.2 малый барабан и клэп встанут на слабые доли ритм-секции. Для патча пригодятся VCA-1 (урок 3.3) и Gates (урок 5.4), хотя в списке модулей урока их нет.

## Пробник

1. Вопрос: почему малый барабан собирают из тона и шума, а не из одного шума?
   - Верно: тон даёт корпус («бум» барабана), шум даёт струны; вместе получается удар с хрустом.
   - Неверно: шум громче тона, поэтому его надо прикрыть тоном. Заблуждение: тон нужен ради громкости.
   - Неверно: Noise в Rack не умеет затухать. Заблуждение: шум нельзя огибать.
2. Вопрос: чему равен Sustain у обоих ADSR в малом барабане?
   - Верно: нулю: барабан не должен держаться, пока играет гейт.
   - Неверно: максимуму, чтобы звук держался до конца гейта. Заблуждение: чем дольше, тем реалистичнее.
   - Неверно: пятидесяти процентам, как по умолчанию. Заблуждение: настройки ADSR по умолчанию годятся для всех инструментов.
3. Вопрос: что отличает клэп от малого барабана в огибающей шума?
   - Верно: огибающая перезапускается три-четыре раза за несколько десятков миллисекунд, потом идёт длинный хвост.
   - Неверно: у клэпа более высокий тон. Заблуждение: клэп отличается высотой, а не формой огибающей.
   - Неверно: у клэпа нет шума. Заблуждение: клэп — это тоновой звук.

## Понятие

Идея без модулей. Малый барабан — это два звука одновременно. Первый: корпус барабана, затухающий средний тон. Второй: струны под нижней мембраной, которые дребезжат и дают шум. [sos-35](https://www.soundonsound.com/techniques/synthesizing-drums-snare-drum) в этом цикле Sound On Sound разбирает физику: у барабана есть несколько тонов корпуса и полосовой шум струн; чем сильнее удар, тем громче барабан: "the harder you strike a snare drum, the louder it becomes" ([sos-35](https://www.soundonsound.com/techniques/synthesizing-drums-snare-drum)); это то, что в синтезаторе делает уровень удара. Рецепт для синтезатора даёт [sos-36](https://www.soundonsound.com/techniques/practical-snare-drum-synthesis): шум и тоновая составляющая, мгновенная атака, и "a snare drum should never sustain" ([sos-36](https://www.soundonsound.com/techniques/practical-snare-drum-synthesis)).

Клэп (хлопок) — это «много рук»: несколько быстрых всплесков шума с интервалом в миллисекунды, потом общий хвост. По [ne-clap](https://noiseengineering.us/blogs/loquelic-literitas-the-blog/percussion-synthesis-part-3-the-sound-of-one-synth-clapping/) клэп — это шум, пропущенный через резонансный полосовой фильтр, с огибающей, которую перезапускают несколько раз подряд (так сделан и клэп TR-909), а потом добавляют реверберацию. В Rack у VCF нет отдельного полосового выхода, поэтому мы берём Highpass filter и умеренный Resonance: это приближение курса, а не точная копия описанной схемы.

Что показать (агент рисует): две огибающие друг под другом. Малый барабан: тон спадает за 90 мс, шум за 200 мс. Клэп: четыре узких пика с промежутками по 10 мс и затухающий хвост 150 мс.

## Анатомия

Карточки: `Fundamental.Noise.md`, `Fundamental.VCF.md`, `Fundamental.VCO.md`, `Fundamental.ADSR.md`, `Fundamental.Mixer.md`. Все пять знакомы, обход короткий; не больше трёх терминов на шаг:

1. Noise: выход White noise (шум без цвета, нужен верх). Ручек нет. Остальные цвета (Pink, Red, Violet, Blue, Gray, Black) для малого барабана не нужны: вернёмся к ним в 8.3 (Violet) и 8.7 (Pink).
2. VCF как фильтр верхних частот: вход Audio, выход Highpass filter, ручки Cutoff frequency (около 3000 Гц) и Resonance (небольшой). Низ шума срезается: остаётся хруст. Термин: баланс тона и шума (начнём с него в шаге 5).
3. VCO: выход Triangle, Frequency около 180 Гц; вход 1V/octave pitch не подключаем, высота задана ручкой.
4. Два ADSR: один для тона (Decay около 90 мс), другой для шума (Decay около 200 мс); у обоих Sustain 0, Attack минимальный, Decay и Release равны (причина та же, что в 8.1: триггер длится 1 мс, хвост ведёт Release).
5. Два VCA-1 (по одному на ветку) и Mixer. Баланс тона и шума задаём не в Mixer, а ручками Level у двух VCA-1: у Mixer одна общая Level. Термин: клэп как серия ретриггеров.
6. Клэп: ещё один ADSR (Decay 15 мс, Release 150 мс), его вход Retrigger получает частые импульсы от LFO (Square около 100 Гц), а вход Gate — короткий гейт из Gates (около 30 мс). Что именно делает Retrigger при высоком гейте, проверить вручную на Scope (карточка ADSR). Остальные входы ADSR (Attack, Decay, Sustain, Release) пока не используем.

## Демо

Агент строит оба инструмента в своём ряду; ученик смотрит. Перед первым звуком напомнить убавить громкость.

Модули: Fundamental/SEQ3, Fundamental/Noise, Fundamental/VCF, Fundamental/VCO, Fundamental/ADSR (три штуки), Fundamental/VCA-1 (три штуки), Fundamental/Mixer, Fundamental/Gates, Fundamental/LFO, Fundamental/Scope; звук идёт в Audio ученика.

Кабели малого барабана:

1. SEQ 3 `Trigger` → ADSR тона `Gate` и ADSR шума `Gate`.
2. VCO `Triangle` → VCA-1 тона `Channel`; ADSR тона `Envelope` → VCA-1 тона `CV`.
3. Noise `White noise` → VCF `Audio`; VCF `Highpass filter` → VCA-1 шума `Channel`; ADSR шума `Envelope` → VCA-1 шума `CV`.
4. VCA-1 тона `Channel` → Mixer `Channel 1`; VCA-1 шума `Channel` → Mixer `Channel 2`.
5. Mixer `Mix` → `To "device output 1"` и `To "device output 2"`.

Кабели клэпа:

6. SEQ 3 `Step 5` → Gates `Gate`; Gates `Gate` → ADSR клэпа `Gate`; LFO `Square` → ADSR клэпа `Retrigger`.
7. VCF `Highpass filter` → VCA-1 клэпа `Channel`; ADSR клэпа `Envelope` → VCA-1 клэпа `CV`; VCA-1 клэпа `Channel` → Mixer `Channel 3`.
8. ADSR шума `Envelope` → Scope `Ch 1`; ADSR клэпа `Envelope` → Scope `Ch 2`.

Параметры (displayValue):

| Модуль | Параметр | Цель |
| --- | --- | --- |
| SEQ 3 | Tempo | 240 |
| SEQ 3 | Step 3 trigger, Step 7 trigger | 1 (малый барабан); Run не трогать |
| VCO | Frequency | 180 |
| ADSR тона | Attack / Decay / Sustain / Release | 2 / 90 / 0 / 90 |
| ADSR шума | Attack / Decay / Sustain / Release | 2 / 200 / 0 / 200 |
| VCF | Cutoff frequency / Resonance | 3000 / 15 |
| VCA-1 тона | Level | 70 |
| VCA-1 шума | Level | 100 |
| Mixer | Level | 60 |
| Gates | Gate length | 30 |
| LFO | Frequency | 100 |
| ADSR клэпа | Attack / Decay / Sustain / Release | 1 / 15 / 0 / 150 |
| VCA-1 клэпа | Level | 100 |
| Scope | Time | 100 |

Порядок демо: сначала малый барабан без шума (Level у VCA-1 шума 0): ученик слышит «бум» без хруста; затем Level шума 100: появляется хруст. Затем меняем баланс: Level тона 70 → 30 → 100, ученик называет, что меняется. Затем клэп: кабели 6–8, `Step 5 trigger` не нужен (Step 5 выдаёт гейт и без кнопки). Агент читает по `vcvrack_get_module` огибающую клэпа несколько раз подряд; на Scope видно несколько пиков. LFO Frequency 100 → 30 → 200: пиков меньше или больше. В конце агент удаляет свои модули и кабели.

## Покрути сам

1. Cutoff frequency у VCF от 800 до 8000 Гц: шум светлеет, «хруст» превращается в шипение; на Scope форма огибающей та же.
2. Frequency у VCO от 120 до 300 Гц: меняется высота корпуса барабана; шум не меняется.
3. Decay у ADSR шума от 100 до 500 мс: шум тянется дольше или короче; на Scope хвост огибающей длиннее.
4. Decay у ADSR тона от 50 до 200 мс: корпус «бухает» короче или дольше.
5. Level у VCA-1 тона вниз до 20%: барабан превращается в чистый шум (похоже на рассыпанный песок).
6. Frequency у LFO клэпа от 30 до 200 Гц: меньше и больше «ладошек»; на Scope (Ch 2) число пиков.

## Звук-референс

- «хлёстче», «острее», «яркий»: Cutoff frequency у VCF вверх (4000…8000 Гц), Decay у шума вниз.
- «тяжелее», «глуше», «больше тела»: Level у VCA-1 тона вверх, Frequency у VCO вниз (120…150 Гц).
- «рассыпается», «слишком шумный»: Level у VCA-1 шума вниз или Level у VCA-1 тона вверх.
- «короче», «суше»: Decay и Release у обоих ADSR вниз.
- «дольше», «хвост»: Decay и Release у ADSR шума вверх (300…500 мс); тон оставить коротким.
- «клэп жиже», «слишком кучно»: LFO у клэпа вниз (30…60 Гц), ещё и Gate length у Gates вверх (40…60 мс).
- «клэп одним хлопком»: Gate length у Gates вниз (1…5 мс): одна огибающая без перезапусков.
- «хрипит», «громко»: Level у Mixer вниз.

## Практикум

Ученик собирает малый барабан сам, агент ничего не трогает: тон (VCO Triangle → VCA-1 с коротким ADSR), шум (Noise White noise → VCF Highpass filter → VCA-1 со своим ADSR), оба VCA-1 в Mixer, Mix в Audio. Условия: Frequency у VCO от 120 до 300 Гц, Cutoff frequency у VCF не ниже 800 Гц, Sustain у обоих ADSR не выше 10%, Decay у ADSR шума от 100 до 500 мс. Запуск: SEQ 3, Clocked или Push (Hold). Клэп — вторая часть практикума: ученик достраивает его по демо (ADSR, VCA-1, Gates, LFO, Retrigger) и показывает агенту на Scope. Кнопка «Проверить» клэп не проверяет: он зависит от поведения Retrigger, и мы смотрим его вместе на Scope. Подсказки только через результат проверки.

## Проверка

Файл `checks/p8-02-snare-clap.json`. Что проверяет: восемь кабелей малого барабана (VCO `Triangle` → VCA-1 тона `Channel`; ADSR тона `Envelope` → VCA-1 тона `CV`; Noise `White noise` → VCF `Audio`; VCF `Highpass filter` → VCA-1 шума `Channel`; ADSR шума `Envelope` → VCA-1 шума `CV`; VCA-1 тона `Channel` → Mixer `Channel 1`; VCA-1 шума `Channel` → Mixer `Channel 2`; Mixer `Mix` → `To "device output 1"`); Frequency у VCO от 120 до 300 Гц, Cutoff frequency не ниже 800 Гц, Sustain обоих ADSR не выше 10%, Decay шума от 100 до 500 мс; на обеих `Envelope` напряжение выше 1 В. Триггерный выход источника не проверяется: импульс в 1 мс пробой не ловит.

Лесенка подсказок (вопрос, место, ответ):

- Кабели 1–2: «Откуда берётся корпус барабана?» → «VCO, VCA-1 и ADSR» → «Triangle в Channel, Envelope в CV».
- Кабель 3: «Откуда берётся шум?» → «Noise» → «White noise в Audio у VCF».
- Кабель 4: «Что остаётся от шума после фильтра?» → «Выходы VCF» → «Highpass filter в Channel у VCA-1 шума».
- Кабель 5: «Что коротит шум?» → «Второй ADSR» → «Envelope в CV у VCA-1 шума».
- Кабели 6–8: «Как оба VCA-1 попадают в звук?» → «Mixer» → «Channel 1 и Channel 2, Mix в Audio».
- Cutoff frequency ниже 800: «Остался ли в шуме низ?» → «VCF» → «подними».
- Sustain выше 10%: «Держится ли барабан?» → «ADSR» → «Sustain в ноль».
- Огибающая стоит: «Идёт ли ритм?» → «Gate у обоих ADSR» → «запусти SEQ 3, Clocked или Hold у Push».

## Слом

В демо-патче агента (малый барабан играет): поставить Sustain у ADSR шума в 60%. Ученик находит на слух и на Scope: каждый удар заканчивается не затуханием, а ровным шумом, пока держится гейт (на Scope у `Envelope` шума появляется полка). Ответ: Sustain держит звук на уровне, пока гейт высокий; у барабана выдержки быть не должно. Агент возвращает 0 и удаляет демо.

## Итог

Вопросы для повторения:

1. Из каких двух частей собран малый барабан и чем занят каждый фильтр или огибающая?
2. Чем клэп отличается от малого барабана в огибающей шума?
3. Чем регулируется баланс тона и шума, если у Mixer одна общая Level?

Сохрани патч как `vcv-8.2.vcv`: он понадобится в части 9. Следующий урок: 8.3 «Хэты».

Источники урока: [sos-35](https://www.soundonsound.com/techniques/synthesizing-drums-snare-drum), [sos-36](https://www.soundonsound.com/techniques/practical-snare-drum-synthesis), [ne-clap](https://noiseengineering.us/blogs/loquelic-literitas-the-blog/percussion-synthesis-part-3-the-sound-of-one-synth-clapping/).
