---
id: "p8-07-texture"
num: "8.7"
title: "Текстура"
part: 8
modules: ["AudibleInstruments/Clouds", "Fundamental/Noise", "Fundamental/Random"]
depends_on: ["p2-05-noise", "p4-04-randomness"]
concepts: ["гранулярный синтез", "зерно (grain)", "заморозка буфера"]
sources: ["mi-clouds", "ai-texture"]
check: "checks/p8-07-texture.json"
---

# 8.7 Текстура

## Зачем

После урока у ученика седьмой инструмент: «облако», живая текстура из шума, которая сама медленно меняется. Он понимает, что делают зёрна и ручки Texture Synthesizer, и умеет заморозить звук. Файл `vcv-8.7.vcv` пойдёт в часть 9: в уроке 9.4 текстура будет заполнять паузы между секциями трека и играть роль ризера.

## Пробник

1. Вопрос: что такое «зерно» (grain) в Texture Synthesizer?
   - Верно: крошечный кусочек записанного звука, который воспроизводится сам по себе; из множества зёрен собирается облако.
   - Неверно: одна нота мелодии. Заблуждение: зерно — это нота.
   - Неверно: шумовая волна определённого цвета. Заблуждение: зерно — вариация шума.
2. Вопрос: что делает Freeze?
   - Верно: останавливает запись во входной буфер: зёрна читаются из того, что там уже лежит, и звук застывает.
   - Неверно: отключает выход. Заблуждение: Freeze — это мьют.
   - Неверно: ставит звук на паузу, а потом продолжает с того же места. Заблуждение: Freeze — это пауза воспроизведения.
3. Вопрос: Random в Position и в Texture: что он делает?
   - Верно: медленно двигает положение окна в буфере и характер зёрен, и облако «дышит».
   - Неверно: меняет высоту звука. Заблуждение: любой CV — это высота.
   - Неверно: включает и выключает Freeze. Заблуждение: Random — это переключатель.

## Понятие

Идея без модулей. Гранулярный синтез — это способ собрать звук из очень коротких кусочков (зёрен) длиной в десятки миллисекунд. Модуль записывает входной звук в буфер, потом читает из него множество зёрен и накладывает их друг на друга; зёрна можно брать из разных мест буфера, сдвигать по высоте, прореживать или сгущать. Из одного и того же материала получается и рассыпь трещащих капель, и сплошная ткань. По определению из руководства: "Clouds is a granular audio processor." ([mi-clouds](https://pichenettes.github.io/mutable-instruments-documentation/modules/clouds/manual/)). Руководство Mutable Instruments описывает ручки Position, Size, Pitch, Density, Texture, Blend, кнопку Freeze, режимы и короткий стартовый патч; страница [ai-texture](https://library.vcvrack.com/AudibleInstruments/Clouds) показывает, как модуль называется в VCV Library (Texture Synthesizer) и что он повторяет панель Clouds.

Что здесь принадлежит курсу. Рецепт «розовый шум в Left, Random на Position и Texture, Dry/wet почти в максимум» — наш выбор для текстуры трека, а не рецепт из руководства. Шум здесь нужен, потому что он даёт материал для зёрен, не привязанный к высоте: облако получается ветром, дождём или шипящим ковром. Если нужно услышать именно зёрна, подай в Left простой тон (например, пилу из VCO): тогда видно, как они разбирают звук.

Что показать (агент рисует): горизонтальную ленту буфера, на ней окно Position и множество маленьких пиков-зёрен, берущих кусочки из окна; рядом ручки Size (ширина пика), Density (частота пиков), Pitch (высота зёрен).

## Анатомия

Карточка: `AudibleInstruments.Clouds.md` (обходим по ней: все ручки и входы). Остальные карточки: `Fundamental.Noise.md`, `Fundamental.Random.md`. Не больше трёх терминов на шаг:

1. Входы звука: Left (основной), Right (для стерео), ручка Audio input gain (уровень записи в буфер). Термин: зерно (grain).
2. Ручки зёрен: Grain position (где в буфере), Grain size (длина зерна), Grain pitch (высота зёрен), Grain density (сколько зёрен), Grain texture (характер зёрен). У каждой есть CV-вход: Position, Size, Pitch (1V/oct), Density, Texture. Мы используем только Position и Texture.
3. Кнопка Freeze и вход Freeze (гейт): заморозка буфера. Термин: заморозка буфера.
4. Кнопки Mode и Load/save: режимы и служебная функция. Режимы и работа Load/save в этой версии — проверить вручную; в уроке остаёмся на режиме по умолчанию.
5. Блок Blend: Dry/wet, Stereo spread, Feedback amount, Reverb amount (четыре значения одной ручки Blend с кнопкой выбора) и вход Blend; какое из четырёх сейчас крутится на панели, проверить вручную. Вход Trigger (запуск зерна) пока не используем.
6. Выходы Left и Right: оба в Audio.
7. Noise: берём Pink noise (мягче белого).
8. Random: Smooth (плавное случайное напряжение), Linear, Stepped, Exponential и Trigger; ручки Internal trigger rate (скорость), Shape, Random spread, Offset. Вход Trigger у Random пуст: работает внутренний такт.

## Демо

Агент строит текстуру в своём ряду; ученик смотрит. Перед первым звуком напомнить убавить громкость.

Модули: Fundamental/VCO (только для первого эксперимента), Fundamental/Noise, Fundamental/Random, AudibleInstruments/Clouds, Fundamental/Scope; звук идёт в Audio ученика.

Кабели:

1. VCO `Sawtooth` → Clouds `Left` (первый эксперимент; потом заменить на Noise).
2. Noise `Pink noise` → Clouds `Left`.
3. Random `Smooth` → Clouds `Position`.
4. Random `Linear` → Clouds `Texture`.
5. Clouds `Left` → `To "device output 1"`; Clouds `Right` → `To "device output 2"` (Audio ученика).
6. Random `Smooth` → Scope `Ch 1`; Clouds `Left` → Scope `Ch 2`.

Параметры (displayValue):

| Модуль | Параметр | Цель |
| --- | --- | --- |
| Clouds | Audio input gain | 0.6 |
| Clouds | Grain position / Grain size / Grain pitch | 0.5 / 0.6 / 0 |
| Clouds | Grain density / Grain texture | 0.6 / 0.5 |
| Clouds | Dry/wet | 0.9 |
| Clouds | Stereo spread / Feedback amount / Reverb amount | 0.7 / 0.2 / 0.3 |
| Random | Internal trigger rate | 0.5 |
| Random | Shape / Random spread / Offset | 100 / 50 / Bipolar |
| Scope | Time | 5000 |

Порядок демо: сначала кабели 1 и 5 (пила в облако): ученик слышит, как нота рассыпается на зёрна; Grain density 0.6 → 0.2 → 0.9: от редких «капель» к сплошной массе. Затем Freeze в 1 (сырое значение): звук застывает; вернуть в 0. Затем заменить кабель 1 на кабель 2 (розовый шум): ученик слышит ветер или дождь. Затем кабели 3 и 4: облако начинает «ходить»; агент читает `Smooth` несколько раз, на Scope медленная кривая. Затем Grain pitch 0 → −1 → 1 (единицы на дисплее не подписаны, ученик сообщает на слух). Затем Feedback amount 0.2 → 0.6 (ненадолго, ученик слышит нарастание) → 0.2. В конце агент удаляет свои модули и кабели.

## Покрути сам

1. Grain density от 0.2 до 0.9: от отдельных капель к сплошной ткани; на Scope (Ch 2) огибающая выхода становится ровнее.
2. Grain size от 0.2 до 0.8: от «хруста» к плавному гулу.
3. Grain pitch от −1 до 1: облако темнеет или светлеет, «цвет» шума меняется.
4. Dry/wet от 0.5 до 1: от смеси с сухим шумом к чистому облаку.
5. Freeze в 1 на несколько секунд и обратно в 0: звук застывает, потом снова меняется.
6. Internal trigger rate у Random от 0.1 до 1: облако «ходит» медленнее или быстрее; на Scope (Ch 1) кривая Smooth быстрее или медленнее.

## Звук-референс

- «плотнее», «гуще»: Grain density вверх.
- «реже», «капли», «потрескивает»: Grain density вниз и Grain size вниз.
- «плавнее», «ткань»: Grain size вверх.
- «выше», «светлее» / «ниже», «темнее»: Grain pitch вверх / вниз.
- «шире», «объёмнее»: Stereo spread вверх; «больше пространства»: Reverb amount вверх.
- «застыло», «держи»: Freeze.
- «живее», «шевелится»: Internal trigger rate у Random вверх; «статичнее»: вниз.
- «накапливается», «разрослось»: Feedback amount слишком велик; убавить.
- «слышно сухой шум», «не облако»: Dry/wet вверх.
- «тихо»: Audio input gain вверх; «хрипит»: вниз.

## Практикум

Ученик собирает текстуру сам, агент ничего не трогает: Noise `Pink noise` в Left у Clouds, Random `Smooth` в Position, Random `Linear` в Texture, оба выхода Clouds в Audio. Условия: Dry/wet не ниже 0,7, Audio input gain не ниже 0,3, Feedback amount не выше 0,5, Reverb amount не выше 0,7, Internal trigger rate у Random не выше 1 Гц. Дальше ученик подбирает текстуру для трека: ветер, дождь, ковёр. Подсказки только через результат проверки.

## Проверка

Файл `checks/p8-07-texture.json`. Что проверяет: пять кабелей (Noise `Pink noise` → Clouds `Left`; Random `Smooth` → Clouds `Position`; Random `Linear` → Clouds `Texture`; Clouds `Left` → `To "device output 1"`; Clouds `Right` → `To "device output 2"`); параметры из условий; на `Smooth` у Random размах не меньше 0,3 В за четыре секунды; на `Left` у Clouds размах не меньше 0,1 В. Кнопки Freeze, Mode и Load/save не проверяются: у них нет `displayValue`.

Лесенка подсказок (вопрос, место, ответ):

- Кабель 1: «Что даёт материал для облака?» → «Noise» → «Pink noise в Left у Clouds».
- Кабели 2–3: «Чем двигать облако?» → «Random» → «Smooth в Position, Linear в Texture».
- Кабели 4–5: «Куда идёт облако?» → «Audio» → «Left в первый выход, Right во второй».
- Dry/wet мал: «Слышен ли сухой шум?» → «Clouds» → «подними до 0,7».
- Feedback выше 0,5: «Облако разрастается?» → «Feedback amount» → «убавь».
- Internal trigger rate выше 1 Гц: «Не слишком ли быстро ходит облако?» → «Random» → «до 1 Гц и ниже».
- Probes молчат: «Звучит ли облако и идёт ли внутренний такт у Random?» → «Left у Clouds, Trigger у Random» → «не должно быть кабеля в Trigger, Audio input gain выше нуля».

## Слом

В демо-патче агента (облако играет): поставить Grain density в 0. Ученик находит на слух и на Scope: облако почти замолкает: остаётся редкая россыпь, Ch 2 почти ровный. Ответ: Density отвечает за число зёрен; без зёрен нет и облака. Агент возвращает 0.6 и удаляет демо.

## Итог

Вопросы для повторения:

1. Что такое зерно и какие три ручки задают, где, какой длины и сколько зёрен играет?
2. Чем Freeze отличается от паузы и мьюта?
3. Зачем Random подключён к Position и Texture, а не к высоте?

Сохрани патч как `vcv-8.7.vcv`: он понадобится в части 9. Следующий урок: 9.1 «Архитектура патча-трека».

Источники урока: [mi-clouds](https://pichenettes.github.io/mutable-instruments-documentation/modules/clouds/manual/), [ai-texture](https://library.vcvrack.com/AudibleInstruments/Clouds).
