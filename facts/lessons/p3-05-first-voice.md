---
id: "p3-05-first-voice"
num: "3.5"
title: "Первый голос"
part: 3
modules: ["Fundamental/VCO", "Fundamental/VCF", "Fundamental/VCA-1", "Fundamental/ADSR", "Fundamental/Push", "Core/AudioInterface2"]
depends_on: ["p3-01-vcf", "p3-03-vca", "p3-04-adsr"]
concepts: ["голос синтезатора", "путь звука", "путь управления"]
sources: ["lm-first-voice", "samesound-first-patch", "kane-musichackspace"]
check: "checks/p3-05-first-voice.json"
---

# 3.5 Первый голос

## Зачем

После урока у ученика есть собственный голос синтезатора, собранный без подсказок: генератор → фильтр → усилитель с огибающей по нажатию кнопки. Это основа любого инструмента трека: бас, лид и пэд в частях 8 и 9 будут вариациями этого патча.

## Пробник

1. Вопрос: в каком порядке идёт звук в классическом голосе?
   - Верно: VCO → VCF → VCA → выход на устройство.
   - Неверно: VCA → VCF → VCO. Заблуждение: сначала громкость, потом источник.
   - Неверно: VCO → VCA → VCF, причём огибающая на фильтре, а не на VCA. Заблуждение: порядок модулей в цепочке не важен.
2. Вопрос: что получает на вход ADSR и что он отдаёт?
   - Верно: на Gate приходит гейт от клавиши (Push), с выхода Envelope уходит CV на вход CV у VCA.
   - Неверно: ADSR получает звук от VCO и сам его громкость меняет. Заблуждение: огибающая работает со звуком, а не с управлением.
   - Неверно: огибающая идёт во вход Channel у VCA. Заблуждение: все кабели похожи, значит, все гнёзда одинаковы.
3. Вопрос: звук играет без нажатия кнопки и не выключается. В чём причина?
   - Верно: у VCA нет кабеля на входе CV (или он не от огибающей): VCA открыт всегда.
   - Неверно: у ADSR слишком большой Release. Заблуждение: Release не меняет случай, когда нота вообще не заканчивается.
   - Неверно: VCO сломан. Заблуждение: источник сам решает, звучать или нет.

## Понятие

Идея без модулей: голос — это три «этажа». Звук (путь звука): источник → тембр → громкость. Управление (путь управления): кнопка → гейт → огибающая → громкость. Два пути идут рядом и сходятся в одном месте: на CV усилителя. Порядок и разделение этих путей — главное, что нужно понять, а не конкретные ручки.

Что показать (агент рисует): блок-схему с двумя цветами стрелок: синий путь звука (VCO, VCF, VCA, выход на устройство) и жёлтый путь управления (Push, ADSR, CV у VCA).

Первый голос показывают почти все курсы по модулярному синтезу. [lm-first-voice](https://learningmodular.com/patching-a-typical-subtractive-synthesis-voice/) называет его первой ступенью лестницы: "If you're a beginner, here is your first step on the ladder" ([lm-first-voice](https://learningmodular.com/patching-a-typical-subtractive-synthesis-voice/)). Русскоязычный разбор такого патча есть у [samesound-first-patch](https://samesound.ru/soft/145529-vvedenie-v-modulnye-sintezatory-sozdayom-pervyj-patch-v-vcv-rack): "Вуаля, всё запищит." ([samesound-first-patch](https://samesound.ru/soft/145529-vvedenie-v-modulnye-sintezatory-sozdayom-pervyj-patch-v-vcv-rack)); там имена модулей из Rack v1 (в Rack 2 VCA называется VCA, а аудио-выход Audio 2). Курс Music Hackspace ([kane-musichackspace](https://musichackspace.org/courses/getting-started-with-vcv-rack-2)) даёт тот же голос на видео. Различия сводятся к названиям; идея голоса одна.

## Анатомия

Карточки: `Fundamental.VCO.md`, `Fundamental.VCF.md`, `Fundamental.VCA-1.md`, `Fundamental.ADSR.md`, `Fundamental.Push.md`, `Core.AudioInterface2.md`. Здесь новых ручек нет, поэтому анатомия — это обход пути. Не больше трёх терминов на шаг:

1. Путь звука: выход Sawtooth у VCO, вход Audio и выход Lowpass filter у VCF, вход Channel и выход Channel у VCA, вход `To "device output 1"` у Audio. Термины: голос синтезатора, путь звука.
2. Путь управления: Push Gate → ADSR Gate → ADSR Envelope → VCA CV. Термин: путь управления.
3. Ручки, которые стоит держать в голове: VCO Frequency, VCF Cutoff frequency и Resonance, ADSR Attack, Decay, Sustain, Release. Resonance держим не выше 50%, иначе фильтр начнёт звучать сам (урок 3.2).
4. Hold у Push: «нота» держится сама, пока ты крутишь ручки. Вернёмся к Hold в части 5, когда гейт будет приходить от секвенсера.

## Демо

Агент строит голос в своём ряду; ученик в это время смотрит. Перед первым звуком напомнить убавить громкость.

Модули: Fundamental/VCO, Fundamental/VCF, Fundamental/VCA-1, Fundamental/ADSR, Fundamental/Push; звук идёт в Audio ученика.

Кабели:

1. VCO `Sawtooth` → VCF `Audio`.
2. VCF `Lowpass filter` → VCA-1 `Channel`.
3. ADSR `Envelope` → VCA-1 `CV`.
4. VCA-1 `Channel` (выход) → `To "device output 1"` (Audio ученика).
5. Push `Gate` → ADSR `Gate`.

Параметры (displayValue, читать до и после):

| Модуль | Параметр | Цель |
| --- | --- | --- |
| VCO | Frequency | 130.81 (C3; допустимо 100–300) |
| VCF | Cutoff frequency | 800 |
| VCF | Resonance | 30 |
| ADSR | Attack | 5 |
| ADSR | Decay | 300 |
| ADSR | Sustain | 40 |
| ADSR | Release | 400 |
| VCA-1 | Level | 100 |

Порядок демо: собрать без кабеля 3 и 5, спросить, что слышно (должна быть тишина или постоянный звук: зависит от того, подключён ли CV; без CV звук идёт всегда). Затем добавить кабели 3 и 5, нажать Push (Push = 1, затем 0) и спросить, что слышно. Затем включить Hold = 1, подкрутить VCF Cutoff frequency с 800 до 2000 и обратно (спрашивать, что меняется), снять Hold. Показать огибающую: агент читает `vcvrack_get_module` выхода ADSR `Envelope` несколько раз при Hold = 1: напряжение поднимается, держится около четырёх вольт (Sustain 40% от 10 В) и после снятия Hold падает к нулю.

В конце агент удаляет свои модули и кабели.

## Покрути сам

1. Cutoff frequency на VCF от 500 до 3000 Гц при Hold = 1: звук от тёмного к яркому; на Scope (подключи к VCA-1 `Channel`) пила становится острее.
2. Attack от 5 мс до 500 мс: нота «вплывает», а не бьёт; Scope показывает медленный подъём размаха.
3. Sustain от 0 до 100%: при 0 нота после спада затихает, при 100% держится без спада.
4. Release от 50 мс до 2 с: после отпускания хвост всё длиннее.
5. VCF Resonance от 0 до 50%: тембр «окрашивается»; выше 50% не поднимать (по условию проверки).
6. VCO Frequency меняет высоту, а не форму огибающей: убедись, что огибающая не зависит от ноты.

## Практикум

Ученик собирает голос сам, без подсказок (агент только читает патч и ничего не трогает). Условия: источник Sawtooth, фильтр Lowpass, огибающая ADSR на CV усилителя, гейт от Push, выход на первый выход Audio. Resonance не больше 50%. Звук должен появляться только по Push (или Hold), а не постоянно. Подсказки даются только через результат проверки.

## Проверка

Файл `checks/p3-05-first-voice.json`. Что проверяет: пять кабелей (VCO Sawtooth → VCF Audio; VCF Lowpass filter → VCA Channel; ADSR Envelope → VCA CV; VCA Channel → `To "device output 1"`; Push Gate → ADSR Gate); Resonance не больше 50; на выходе Envelope у ADSR напряжение выше 1 В (для этого Hold у Push должен быть включён или кнопка нажата).

Лесенка подсказок (вопрос, место, ответ):

- Кабель 1: «Какой выход VCO даёт пилу?» → «Правая часть VCO» → «Sawtooth в Audio у VCF».
- Кабель 2: «Фильтр стоит, но до VCA звук не доходит» → «Какой выход VCF ведёт к VCA?» → «Lowpass filter в Channel».
- Кабель 3: «Звук есть всегда? Что управляет VCA?» → «Нужен вход CV, не Channel» → «Envelope в CV».
- Кабель 4: «Голос собран, а тишина» → «Откуда звук выходит из VCA?» → «Channel в первый выход Audio».
- Кабель 5: «Огибающая не знает, когда начинать» → «Что подаёт гейт?» → «Gate у Push в Gate у ADSR».
- Resonance выше 50: «Почему фильтр свистит?» → «Сколько процентов?» → «не больше 50».
- Огибающая стоит: «Идёт ли гейт?» → «Hold у Push» → «Hold = 1, потом проверить ещё раз».

## Слом

В демо-патче агента (Hold = 1, звук звучит с огибающей): удалить кабель ADSR `Envelope` → VCA-1 `CV` (кабель 3). Ученик находит на слух: огибающая больше не влияет, звук держится постоянно и не выключается от Hold; на Scope размах пилы постоянный. Ответ: без кабеля на CV усилитель открыт всегда. Агент возвращает кабель и удаляет демо.

## Итог

Вопросы для повторения:

1. Какие два пути сходятся в голосе и где именно они встречаются?
2. Почему гейт идёт в ADSR, а не в VCA?
3. Что сделает голос, если у VCA нет кабеля на входе CV?

Сохрани патч как `vcv-3.5.vcv`. Следующий урок: 4.1 «LFO».

Источники урока: [lm-first-voice](https://learningmodular.com/patching-a-typical-subtractive-synthesis-voice/), [samesound-first-patch](https://samesound.ru/soft/145529-vvedenie-v-modulnye-sintezatory-sozdayom-pervyj-patch-v-vcv-rack), [kane-musichackspace](https://musichackspace.org/courses/getting-started-with-vcv-rack-2).
