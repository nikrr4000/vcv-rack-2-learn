---
id: "p8-06-pad"
num: "8.6"
title: "Пэд"
part: 8
modules: ["Fundamental/VCO", "Fundamental/LFO", "Valley/Plateau"]
depends_on: ["p6-04-polyphony", "p2-03-two-oscillators", "p7-02-reverb"]
concepts: ["пэд", "медленная атака и долгий релиз", "ансамблевый эффект"]
sources: ["sos-strings", "vcv-manual-polyphony", "valley-plateau"]
check: "checks/p8-06-pad.json"
---

# 8.6 Пэд

## Зачем

После урока у ученика шестой инструмент: полифонический пэд (мягкий фон из нескольких нот аккорда) из двух расстроенных осцилляторов, медленно «дышащей» ширины импульса, плавной атаки и реверберации. Он играет аккордами, звук нарастает и тает сам. Файл `vcv-8.6.vcv` пойдёт в часть 9: в уроке 9.3 пэд даёт гармонию под басом и лидом.

## Пробник

1. Вопрос: зачем два осциллятора на одну ноту, расстроенных на несколько центов?
   - Верно: между ними возникают медленные биения, и звук «шевелится» и кажется широким, как ансамбль.
   - Неверно: чтобы звук был громче. Заблуждение: расстройка — это громкость.
   - Неверно: чтобы получилась октава. Заблуждение: расстройка и октава — одно и то же.
2. Вопрос: что нужно изменить, чтобы пэд не «бил» с первой миллисекунды, а нарастал?
   - Верно: Attack у ADSR поднять до сотен миллисекунд.
   - Неверно: Release поднять до секунд. Заблуждение: Release влияет на начало ноты.
   - Неверно: Level у VCA-1 убавить. Заблуждение: тише — значит плавнее.
3. Вопрос: что делает Sum перед Plateau?
   - Верно: складывает голоса полифонического кабеля в один сигнал, который идёт в реверберацию.
   - Неверно: расстраивает голоса. Заблуждение: Sum меняет высоту.
   - Неверно: делит сигнал на левый и правый. Заблуждение: Sum отвечает за стерео.

## Понятие

Идея без модулей. Пэд — это звук, у которого нет атаки в обычном смысле: ноты плавно вплывают, держатся и долго растворяются. Он занимает место под основной партией и заполняет пространство. Три приёма делают его живым. Первый: два осциллятора на одной ноте, чуть расстроенные: их частоты расходятся на несколько центов, и возникают медленные биения (урок 2.3). Второй: медленная модуляция, например ширины импульса от LFO (урок 2.2): звук не стоит на месте. Третий: реверберация (урок 7.2), которая размывает края.

[sos-strings](https://www.soundonsound.com/techniques/synthesizing-strings-pwm-string-sounds) описывает рецепт струнных пэдов: две расстроенные пилы вместо PWM, одна слегка модулирована, плюс октавное удвоение, хорус и реверберация. Один из итогов там: "Every note you hear is simply four detuned and modulated oscillators" ([sos-strings](https://www.soundonsound.com/techniques/synthesizing-strings-pwm-string-sounds)). Наша версия проще: один осциллятор с прямоугольной волной и PWM от LFO, второй с пилой, оба расстроены в разные стороны; хоруса нет. [vcv-manual-polyphony](https://vcvrack.com/manual/Polyphony) объясняет, как один кабель несёт до 16 голосов: модули в цепочке сами работают по каналам, а Sum сводит их (урок 6.4). Реверберацию [valley-plateau](https://valleyaudio.github.io/rack/plateau/) описывает так: "Sets the overall delay time and apparent 'size' of the reverb." ([valley-plateau](https://valleyaudio.github.io/rack/plateau/)) — это ручка Size.

Что показать (агент рисует): две волны на одной оси с почти одинаковой частотой и биения на их сумме (огибающая медленно растёт и спадает); под ними огибающая с длинной атакой и долгим релизом; справа дымка реверберации.

## Анатомия

Карточки: `Fundamental.VCO.md`, `Fundamental.LFO.md`, `Fundamental.Mixer.md`, `Fundamental.VCF.md`, `Fundamental.ADSR.md`, `Fundamental.VCA-1.md`, `Fundamental.Sum.md`, `Valley.Plateau.md`. Не больше трёх терминов на шаг:

1. Два VCO. Первый: выход Square, ручка Pulse width (50%), ручка Pulse width modulation (глубина входа), вход Pulse width modulation, ручка Frequency чуть ниже C4. Второй: выход Sawtooth, Frequency чуть выше C4. Оба получают одну и ту же высоту на входе 1V/octave pitch: в демо от аккорда, в практикуме от MIDI. Термин: ансамблевый эффект.
2. LFO: выход Triangle, Frequency (около 0,3 Гц), Offset в положении Bipolar (так ширина импульса качается вокруг 50%). Остальные ручки и выходы не нужны.
3. Mixer: Channel 1 (прямоугольник) и Channel 2 (пила), выход Mix. Полифонические кабели Mixer складывает по каналам; убедиться на Viz (проверить вручную).
4. VCF: Lowpass filter, Cutoff frequency около 1800 Гц, Resonance небольшой. Для пэда фильтр обычно мягкий: резонанс выше 40% начинает свистеть.
5. ADSR: Attack около 500 мс, Decay около 1000 мс, Sustain 80%, Release около 2000 мс. Термин: медленная атака и долгий релиз.
6. VCA-1 и Sum: Channel, CV, Level; Sum сводит голоса (урок 6.4), Level у него не выше 60%.
7. Plateau: входы Left, Right, выходы Left, Right, ручки Dry level, Wet level, Size, Decay, Pre-delay, Hold (и остальные, по 7.2). Термин: пэд.

## Демо

Агент строит пэд в своём ряду; ученик смотрит. Аккорд агент берёт не от MIDI, а из констант: так демо не зависит от того, настроен ли у ученика MIDI-вход. Перед первым звуком напомнить убавить громкость.

Модули: Fundamental/8vert, Fundamental/Merge, Fundamental/VCO (две штуки), Fundamental/LFO, Fundamental/Mixer, Fundamental/VCF, Fundamental/ADSR, Fundamental/VCA-1, Fundamental/Sum, Fundamental/Push, Fundamental/Viz, Valley/Plateau; звук идёт в Audio ученика.

Кабели:

1. 8vert `Row 1`, `Row 2`, `Row 3` (выходы) → Merge `Channel 1`, `Channel 2`, `Channel 3`.
2. Merge `Polyphonic` → VCO A `1V/octave pitch` и VCO B `1V/octave pitch`.
3. LFO `Triangle` → VCO A `Pulse width modulation` (вход).
4. VCO A `Square` → Mixer `Channel 1`; VCO B `Sawtooth` → Mixer `Channel 2`.
5. Mixer `Mix` → VCF `Audio`; VCF `Lowpass filter` → VCA-1 `Channel`.
6. Push `Gate` → ADSR `Gate`; ADSR `Envelope` → VCA-1 `CV`.
7. VCA-1 `Channel` → Sum `Polyphonic`; Sum `Monophonic` → Plateau `Left`.
8. Plateau `Left` → `To "device output 1"`; Plateau `Right` → `To "device output 2"`.
9. Mixer `Mix` → Viz `Polyphonic`.

Параметры (displayValue):

| Модуль | Параметр | Цель |
| --- | --- | --- |
| 8vert | Row 1 gain / Row 2 gain / Row 3 gain | 0 / 0.25 / 0.58 (вольты: входы Row пусты, поэтому дисплей в вольтах; ноты C, Es, G) |
| VCO A | Frequency | 260.12 (−10 центов) |
| VCO B | Frequency | 263.15 (+10 центов) |
| VCO A | Pulse width modulation | 40 |
| LFO | Frequency / Offset | 0.3 / Bipolar |
| VCF | Cutoff frequency / Resonance | 1800 / 15 |
| ADSR | Attack / Decay / Sustain / Release | 500 / 1000 / 80 / 2000 |
| Sum | Level | 30 |
| Plateau | Dry level / Wet level | 70 / 50 |
| Plateau | Size / Decay | 60 / 65 |

Порядок демо: сначала кабели 1, 2, 4–7 и 8, без LFO и без второго осциллятора (Mixer Channel 2 пока пуст): ученик слышит три ноты, Push держать (Hold). Агент читает по Viz три канала. Затем второй осциллятор: биения, звук оживает. Затем кабель 3: ширина импульса «дышит»; агент читает `Square` несколько раз подряд. Затем Attack 500 → 50 → 1500: на Viz видно, как быстро нарастает уровень. Затем Wet level 50 → 0 → 80: без реверберации пэд «сухой». В конце агент удаляет свои модули и кабели.

## Покрути сам

1. Frequency у второго VCO от 261 до 266 Гц: от почти унисона к заметным биениям и «квинтету»; выше 270 Гц уже фальшивит.
2. Pulse width modulation у первого VCO от 0 до 60%: от неподвижного тембра к «дышащему»; на Scope форма прямоугольника то уже, то шире.
3. Frequency у LFO от 0,1 до 1 Гц: покачивание быстрее или медленнее.
4. Attack от 100 до 2000 мс: от лёгкого входа к долгому нарастанию; Release от 500 до 4000 мс: от обрыва к растворению.
5. Cutoff frequency у VCF от 600 до 4000 Гц: пэд темнее или ярче.
6. Wet level и Size у Plateau: от сухого пэда к «облаку».

## Звук-референс

- «шире», «объёмнее», «как хор»: расстройка VCO (разница в несколько центов) и Pulse width modulation вверх; хвост реверберации (Size, Decay) вверх.
- «слишком фальшивит», «плывёт»: расстройка слишком велика; вернуть к ±10 центам.
- «дышит медленнее»: Frequency у LFO вниз.
- «мягче вход», «плавнее»: Attack вверх (500…1500 мс).
- «обрывается»: Release вверх.
- «темнее», «глуше»: Cutoff frequency вниз; «ярче»: вверх.
- «больше пространства», «хвост»: Wet level, Size, Decay вверх.
- «мутно», «заливает»: Wet level вниз, Dry level вверх.
- «хрипит» на аккорде: Level у Sum вниз (урок 6.4).

## Практикум

Ученик собирает пэд сам, агент ничего не трогает: MIDI to CV с Channels = 4 (как в 6.4) подаёт высоту на оба VCO и Gate на ADSR; первый VCO Square с PWM от LFO Triangle, второй VCO Sawtooth, оба в Mixer, дальше VCF, VCA-1 с ADSR, Sum, Plateau Left, оба выхода Plateau в Audio. Условия: первый VCO ниже 261 Гц, второй выше 262,3 Гц, Pulse width modulation от 20%, Frequency у LFO не выше 1 Гц, Attack не меньше 400 мс, Release не меньше 800 мс, Resonance не выше 40%, Level у Sum не выше 60%, Wet level у Plateau не ниже 25%, Hold выключен. Подсказки только через результат проверки.

## Проверка

Файл `checks/p8-06-pad.json`. Что проверяет: десять кабелей (LFO `Triangle` → VCO A `Pulse width modulation`; VCO A `Square` → Mixer `Channel 1`; VCO B `Sawtooth` → Mixer `Channel 2`; Mixer `Mix` → VCF `Audio`; VCF `Lowpass filter` → VCA-1 `Channel`; ADSR `Envelope` → VCA-1 `CV`; VCA-1 `Channel` → Sum `Polyphonic`; Sum `Monophonic` → Plateau `Left`; Plateau `Left` → `To "device output 1"`; Plateau `Right` → `To "device output 2"`); параметры из условий; Offset у LFO в положении Bipolar; на обоих выходах Plateau размах выше 0,05 В (на левом выше 0,3 В), пока держится нота. MIDI не приходит в проверку, поэтому MIDI-часть (кабели от MIDI to CV, число каналов) агент проверяет вместе с учеником: ученик берёт аккорд, агент смотрит на Viz (по линии на голос). Для проверки ученик держит клавишу или Hold у Push на Gate у ADSR; атака медленная, поэтому подождать секунду-две.

Лесенка подсказок (вопрос, место, ответ):

- Кабель 1: «Что заставляет ширину импульса дышать?» → «LFO» → «Triangle в Pulse width modulation у первого VCO».
- Кабели 2–3: «Откуда звук идёт в сумму?» → «Mixer» → «Square в Channel 1, Sawtooth в Channel 2».
- Кабели 4–6: как в 3.5: сумма → фильтр → усилитель, огибающая в CV.
- Кабели 7–8: «Как голоса попадают в реверберацию?» → «Sum» → «Channel в Polyphonic у Sum, Monophonic в Left у Plateau».
- Кабели 9–10: «Куда идёт реверберация?» → «Audio» → «Left в первый выход, Right во второй».
- Расстройка: «Звучат ли оба осциллятора одинаково?» → «ручки Frequency» → «один чуть ниже C4, другой чуть выше».
- Атака или Release вне диапазона: «Нарастает ли пэд?» → «ADSR» → «Attack от 400 мс, Release от 800 мс».
- Wet level мал: «Слышна ли реверберация?» → «Plateau» → «подними до 25% и выше».
- Probes молчат: «Держится ли нота и прошла ли атака?» → «клавиша или Hold у Push» → «подожди секунду-две и нажми ещё раз».

## Слом

В демо-патче агента (пэд играет аккордом): поставить Frequency у второго VCO в 260.12 (как у первого: расстройки между осцилляторами нет) и Pulse width modulation у первого VCO в 0. Ученик находит на слух: биения исчезают и тембр застывает; на Scope волна больше не «дышит». Ответ: расстройка и медленная модуляция оживляют пэд. Агент возвращает 263.15 и 40 и удаляет демо.

## Итог

Вопросы для повторения:

1. Откуда берутся биения между двумя осцилляторами и почему расстройка небольшая?
2. Что делают медленная атака и долгий релиз с ощущением звука?
3. Зачем Sum стоит перед Plateau, и что бы случилось без него?

Сохрани патч как `vcv-8.6.vcv`: он понадобится в части 9. Следующий урок: 8.7 «Текстура».

Источники урока: [sos-strings](https://www.soundonsound.com/techniques/synthesizing-strings-pwm-string-sounds), [vcv-manual-polyphony](https://vcvrack.com/manual/Polyphony), [valley-plateau](https://valleyaudio.github.io/rack/plateau/).
