---
id: "p5-05-sequential-switches"
num: "5.5"
title: "Последовательные переключатели"
part: 5
modules: ["Fundamental/SequentialSwitch1", "Fundamental/SequentialSwitch2"]
depends_on: ["p5-02-seq3"]
concepts: ["последовательный переключатель", "канал", "цикл переключателя"]
sources: ["vcv-free-manual", "ne-seq-switches", "loopop-sh-switch"]
check: "checks/p5-05-sequential-switches.json"
---

# 5.5 Последовательные переключатели

## Зачем

После урока ученик умеет по очереди «слушать» разные источники: один тактовый сигнал сам переключается между четырьмя входами или четырьмя выходами. Он собирает четырёхнотную мелодию из четырёх случайных значений. Это основа структуры трека в части 9: так чередуются фразы и слои.

## Пробник

1. Вопрос: какой переключатель раздаёт один вход на четыре выхода?
   - Верно: Sequential Switch 1 to 4.
   - Неверно: Sequential Switch 4 to 1. Заблуждение: направление «из четырёх в один» и «из одного в четыре» одно и то же.
   - Неверно: Mutes. Заблуждение: Mutes включает и выключает, но не выбирает один из четырёх.
2. Вопрос: что делает сигнал на входе Reset у последовательного переключателя?
   - Верно: возвращает его на первый канал; на следующем такте будет второй.
   - Неверно: останавливает и отключает все каналы. Заблуждение: Reset путают с остановкой.
   - Неверно: стирает значения в источниках. Заблуждение: сброс «чистит память», а он двигает только указатель.
3. Вопрос: у Sequential Switch 4 to 1 стоит Steps = 3. Что произойдёт?
   - Верно: цикл из трёх каналов, четвёртый вход пропускается.
   - Неверно: звук станет в три раза громче. Заблуждение: Steps влияет на уровень.
   - Неверно: переключатель сделает три цикла и остановится. Заблуждение: «шаги» — это число повторов.

## Понятие

Идея без модулей. Переключатель по очереди направляет сигнал: каждый тактовый импульс переводит указатель на следующий канал, после последнего он возвращается к первому. Два направления. Раздача (1→4): один вход, четыре выхода; в каждый момент сигнал приходит только на один из выходов. Сбор (4→1): четыре входа, один выход; на выход в каждый момент попадает только один вход. Сбор удобен, чтобы взять четыре источника напряжения и получить из них короткую фразу.

[ne-seq-switches](https://noiseengineering.us/blogs/loquelic-literitas-the-blog/getting-started-sequential-switches/) объясняет, откуда это взялось: "Sequential switches are a utility that was originally designed to extend 8-step sequencers" ([ne-seq-switches](https://noiseengineering.us/blogs/loquelic-literitas-the-blog/getting-started-sequential-switches/)). Второе, более весёлое применение показывает видео [loopop-sh-switch](https://loopopmusic.com/modular-analog-ram-how-to-store-generative-melodies-rhythms-using-sample-hold-and-a-sequential-switch): "freeze randomness once a compelling pattern is presented" ([loopop-sh-switch](https://loopopmusic.com/modular-analog-ram-how-to-store-generative-melodies-rhythms-using-sample-hold-and-a-sequential-switch)); то есть четыре случайных значения сохраняются и проигрываются по кругу, пока не понадобится новая случайность. Сами модули описывает [vcv-free-manual](https://vcvrack.com/Free) (разделы SequentialSwitch1 и SequentialSwitch2).

Что показать (агент рисует): две схемы. 1→4: один входящий кабель и четыре выхода, один из которых подсвечен и «бежит» по кругу. 4→1: четыре входа, один подсвеченный бежит по кругу, выход один.

## Анатомия

Карточки: `Fundamental.SequentialSwitch1.md`, `Fundamental.SequentialSwitch2.md`. Не больше трёх терминов на шаг:

1. Sequential Switch 1 to 4: входы `Clock` (такт переключения), `Reset` (вернуть на первый), `Main` (то, что раздаём); выходы `Channel 1`…`Channel 4`. Термины: последовательный переключатель, канал.
2. Ручка Steps (показание 2, 3 или 4): сколько каналов в цикле. Термин: цикл переключателя.
3. Sequential Switch 4 to 1: входы `Clock`, `Reset`, `Channel 1`…`Channel 4`; выход `Main`. Та же ручка Steps.
4. Правило, которое нельзя забыть: активен один канал; остальные выходы 1→4 стоят на 0 В, неактивные входы 4→1 игнорируются.
5. Откуда берём «четыре источника»: Random Values (карточку пишет часть 4): `Random 1`…`Random 4` — четыре случайных напряжения, которые меняются при нажатии Push. Подробнее в уроке 4.4.

## Демо

Агент строит демо в своём ряду; ученик смотрит. Перед первым звуком напомнить убавить громкость.

Часть 1, раздача (1→4):

Модули: ImpromptuModular/Clocked, Fundamental/SequentialSwitch1, Fundamental/Scope.

1. Clocked `Clock 1` → Switch `Clock`.
2. Clocked `Master clock` → Switch `Main`.
3. Switch `Channel 1` → Scope `Ch 1`; Switch `Channel 2` → Scope `Ch 2`.

Параметры: Clocked Master clock 120, Run 1 (raw), Clk 1 ratio raw 1; Switch Steps = 4 (показание 4). Агент читает Switch через `vcvrack_get_module`: импульсы появляются по очереди только на одном из каналов; ученик говорит, на каком канале импульсы сейчас, и сверяет с Scope. Затем Steps = 2: импульсы только в Channel 1 и Channel 2 по кругу.

Часть 2, сбор (4→1) и «замороженная случайность»:

Модули: ImpromptuModular/Clocked, Fundamental/RandomValues, Fundamental/SequentialSwitch2, Fundamental/VCO, Fundamental/Scope; звук идёт в Audio ученика.

Кабели:

1. Clocked `Clock 1` → Switch `Clock`.
2. Random Values `Random 1` → Switch `Channel 1`.
3. Random Values `Random 2` → Switch `Channel 2`.
4. Random Values `Random 3` → Switch `Channel 3`.
5. Random Values `Random 4` → Switch `Channel 4`.
6. Switch `Main` → VCO `1V/octave pitch`.
7. VCO `Triangle` → `To "device output 1"` (Audio ученика).
8. Switch `Main` → Scope `Ch 1`.

Параметры (displayValue, читать до и после):

| Модуль | Параметр | Цель |
| --- | --- | --- |
| Clocked | Master clock | 120 |
| Clocked | Run | 1 (raw) |
| Clocked | Clk 1 ratio | raw 1 |
| Random Values | Push | 1, затем 0 (получить четыре значения) |
| Sequential Switch 4 to 1 | Steps | 4 |
| VCO | Frequency | 130.81 |

Порядок демо: нажать Push у Random Values; прочитать несколько раз `Main` у переключателя: четыре разных значения, повторяющихся по кругу. Спросить ученика: что слышно (четырёхнотная фраза) и сколько в ней нот. Нажать Push ещё раз: фраза новая, но снова четырёхнотная; значит, значения хранятся, пока не нажали Push. Steps = 4 → 3: фраза из трёх нот, четвёртое значение молчит. Если диапазон случайных напряжений слишком широкий для слуха, сказать ученику, что в уроке 5.3 такие значения сводят в одну тональность. В конце агент удаляет свои модули и кабели.

## Покрути сам

1. Steps у 4→1 от 4 до 2: фраза короче, остаются первые каналы; на Scope видны два уровня.
2. Нажми Push у Random Values: новая четырёхнотная фраза; на Scope четыре новых ступеньки.
3. Подай Pulses `Row 1 trigger` (или Push `Trigger`) в Reset у переключателя: нажми в середине фразы, и она начнётся сначала.
4. Clocked Master clock от 60 до 180: фраза быстрее; значения те же.
5. Для 1→4: Steps от 4 до 2: сигнал бежит только по двум каналам; на Scope видно, что Channel 3 и Channel 4 молчат.

## Практикум

Ученик собирает сам, агент ничего не трогает: Clocked, Random Values, Sequential Switch 4 to 1, VCO, выход. Четыре значения Random 1–4 в Channel 1–4, Clock от Clocked, Main в 1V/octave pitch у VCO, Steps = 4. Нажми Push у Random Values, чтобы значения стали разными. Подсказки только через результат проверки.

## Проверка

Файл `checks/p5-05-sequential-switches.json`. Что проверяет: семь кабелей (Clocked `Clock 1` → `Clock`; Random Values `Random 1`…`Random 4` → `Channel 1`…`Channel 4`; `Main` → VCO `1V/octave pitch`; VCO `Triangle` → `To "device output 1"`); Steps равен 4; на `Main` напряжение меняется не менее чем на 0,2 В за полторы секунды (значит, Run включён и у Random Values есть разные значения). Если у ученика уже был Clocked, проверка найдёт любой.

Лесенка подсказок (вопрос, место, ответ):

- Кабель 1: «Что говорит переключателю, когда менять канал?» → «Вход Clock» → «Clock 1 в Clock».
- Кабели 2–5: «Все ли четыре источника подключены?» → «Каналы 1–4» → «Random 1 в Channel 1 и так далее по порядку».
- Кабель 6: «Куда уходит результат?» → «Main» → «Main в 1V/octave pitch у VCO».
- Кабель 7: «Куда звук?» → «VCO в Audio» → «Triangle в первый выход Audio».
- Steps не 4: «Сколько каналов в цикле?» → «Ручка Steps» → «поставь 4».
- `Main` стоит: «Идёт ли такт и разные ли значения?» → «Run у Clocked и Push у Random Values» → «включи Run, нажми Push».

## Слом

В демо-патче агента (фраза из четырёх нот идёт): удалить кабель Clocked `Clock 1` → Switch `Clock`. Ученик находит на слух и на Scope: мелодия остановилась на одной ноте, на Scope прямая линия. Ответ: без такта переключатель стоит на одном канале. Агент возвращает кабель и удаляет демо.

## Итог

Вопросы для повторения:

1. Чем Sequential Switch 1 to 4 отличается от 4 to 1?
2. Что делает вход Reset?
3. Как из четырёх случайных значений получить повторяющуюся фразу?

Сохрани патч как `vcv-5.5.vcv`. Следующий урок: 5.6 «Живое управление».

Источники урока: [vcv-free-manual](https://vcvrack.com/Free), [ne-seq-switches](https://noiseengineering.us/blogs/loquelic-literitas-the-blog/getting-started-sequential-switches/), [loopop-sh-switch](https://loopopmusic.com/modular-analog-ram-how-to-store-generative-melodies-rhythms-using-sample-hold-and-a-sequential-switch).
