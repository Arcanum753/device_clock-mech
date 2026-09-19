# device_clock-mech — механические часы (шаговый двигатель)

> **Опциональное устройство.** Подключается через `src_filter` + `build_flags`.
> **Работоспособно только в составе сборки, содержащей ядро** (см. `../../TRS.md` §2.1).
> **Для сборки необходимо клонировать само устройство и все указанные в его `.ini` модули**
> (см. `../../TRS.md` §2.2).

Управление механизмом часов через шаговый двигатель (STEP/DIR/EN драйвера A4988),
парковка/обнуление часовой и минутной стрелок по двум оптическим датчикам и подсветке
датчиков, подсчёт шагов на оборот (калибровка), опрос реального времени и вывод стрелок в
целевое положение.

- **Репозиторий:** https://github.com/Arcanum753/device_clock-mech
- **Папка:** `src/device_clock-mech/`
- **Флаг активации:** `-D DEVICE_CLOCKMECH=1`
- **Registry:** `object=device_clock_mech`, `define=DEVICE_CLOCKMECH`, `web=1`, `loop=0` (без `namespace`/`res`/`prio`)
- **Env:** `esp32_clock-mech` (extends `env:esp32`); `src_filter`: `+<module_udp/> +<module_otaclient/> +<device_clock-mech/> +<module_ds3231/>`; флаги: `MODULE_UDP=1`, `MODULE_OTACLIENT=1`, `MODULE_DS3231=1`, `DS3231_SQW_PIN=13`, `DEVICE_CLOCKMECH=1`, `DEBUG_*`
- **Только для платформы:** ESP32
- **Зависит от модулей:** `module_udp`, `module_otaclient`, `module_ds3231`
- **Зависит от ядра:** `core_web`, `core_sys`, `core_json`, `core_ntp`, `core_led`

## Назначение

Управление механизмом часов через шаговый двигатель (STEP/DIR/EN драйвера A4988),
парковка/обнуление часовой и минутной стрелок по двум оптическим датчикам и подсветке
датчиков, подсчёт шагов на оборот (калибровка), опрос реального времени и вывод стрелок в
целевое положение.

## Функциональные требования

- FR-CLOCKMECH-1: Два режима `enable_status`: DEBUG (ручное управление) и WORK (автономная работа).
- FR-CLOCKMECH-2: Калибровка `stepsPerRevolution` и лимит ошибки `errorLimitSteps`.
- FR-CLOCKMECH-3: Источник времени `timeSource`: `ds3231` (I2C RTC + SQW пин 13) или `ntp` (`now()`).
- FR-CLOCKMECH-4: Установка стрелок в целевое время (`cmdSetArrows`, `/clock-mech/set-xx00`, `/clock-mech/set-12xx`).
- FR-CLOCKMECH-5: Управление драйвером: `cmdStep`, `cmdDir`, `cmdEn`, `cmdSled` (подсветка датчиков), `cmdSens` (чтение датчиков).
- FR-CLOCKMECH-6: Периодический опрос `pollInterval`; при превышении `errorLimitSteps` — индикация ошибки (`ledMacrosClockMechError`).
- FR-CLOCKMECH-7: Сохранение конфига — только web `handleSave` и терминал `c-save` (bus-функции `save` нет).
- FR-CLOCKMECH-8: Выводы: DIR=33, STEP=12, EN=14, SENS_LED=27, SENS_MIN=26, SENS_HOUR=25; `CONNECTION_LED=2`.

## Аппаратные интерфейсы

| Интерфейс | Выводы по умолчанию | Примечание |
|-----------|---------------------|------------|
| Шаговый драйвер | STEP=12, DIR=33, EN=14 | драйвер A4988 |
| Датчики часов | SENS_MIN=26, SENS_HOUR=25 | оптические датчики |
| Подсветка датчиков | SENS_LED=27 | — |
| DS3231 SQW/INT# | 13 (`DS3231_SQW_PIN`) | см. `../module_ds3231/AGENTS.md` |
| Статусный светодиод | `CONNECTION_LED=2` | см. `../core_led` (TRS §3.1.7) |

## Веб-интерфейс

Маршруты: `POST /clock-mech/save`, `/clock-mech/reset`, `/clock-mech/count`; GET:
`/clock-mech/info`; GET: `/clock-mech/step|dir|en|sled|sens|n|reset|set-xx00|set-12xx|count|status|ver`.
Страница `clock-mech.html` (пункт меню — `web/_menu.html`).

## Конфигурация

`/config_clock-mech.json` — `enable_status`, `timeSource`, `stepsPerRevolution`,
`pollInterval`, `errorLimitSteps`, `sensorLedEnabled`.

## Терминальные команды

| Команда | Назначение | Доступность |
|---------|-----------|-------------|
| `c-*` | отладка управления (шаг/направление/датчики), `c-save` — сохранение конфига | только `DEVICE_CLOCKMECH` |

## Слоистая структура

Из `../../LAYERS.md`: `device_clock-mech` — механические часы: ШД, хоминг, установка стрелок;
сейчас define'ы, enum'ы, `strClockMechConfig` + крупный автомат; выделить `_types.h`,
`_engine.cpp`.

## Зависимости модулей

Указаны в `.ini` (`src_filter`) и должны быть склонированы для сборки:
- `module_udp` — `../module_udp/AGENTS.md`
- `module_otaclient` — `../module_otaclient/AGENTS.md`
- `module_ds3231` — `../module_ds3231/AGENTS.md`

## Критерии приёмки

- AC-13: `device_clock-mech`/`device_mech-ring`: калибровка по датчикам, установка/отслеживание времени; сохранение только по явному действию. **Каноническое описание** (для `device_mech-ring` — ссылка): калибровка и установка/отслеживание времени на механических часах; конфиг персистится только по явному действию (web/терминал).

## Тестирование

- HIL-стенд (уровень 5): реальный шаговый двигатель и оптодатчики — калибровка и обработка
  `errorLimitSteps` (см. `../../TESTING.md`).

## Ссылки

- Ядро и конвенции: `../../TRS.md`
- Слоистая структура: `../../LAYERS.md`
- Общие утилиты: `../../TRS.md` §3.1.12 (`common/`)
- Сборка: `../../BUILD.md`
- Реестр компонентов: `../../INVENTORY.md`

> Если устройство читается вне дерева ядра (standalone), корневые документы доступны в
> репозитории ядра avr-fota.
