#ifndef _DEVICE_CLOCK_MECH_TYPES_h
#define _DEVICE_CLOCK_MECH_TYPES_h

// ============================================================
// device_clock-mech_types.h — типы, структуры и define'ы устройства
// «механические часы».
// Реализация: device_clock-mech.cpp (шаблон) и
// device_clock-mech_engine.cpp (автомат шагового двигателя).
// ============================================================

#include <Arduino.h>
#include <stdint.h>

#define CLOCKMECH_DIR       33
#define CLOCKMECH_STEP      12
#define CLOCKMECH_EN        14
#define CLOCKMECH_SENS_LED  27
#define CLOCKMECH_SENS_MIN  26
#define CLOCKMECH_SENS_HOUR 25

#define CLOCKMECH_MIN_STEPS_GAP 5

#define CLOCKMECH_CounterClockWise  HIGH
#define CLOCKMECH_ClockWise         LOW

#define CONFIG_FILE_CLOCKMECH    "/config_clock-mech.json"

typedef enum {
    STATUS_IDLE    = 0,
    STATUS_SET1200,
    STATUS_SETXX00,
    STATUS_SET12XX,
    STATUS_SETHOUR,
    STATUS_SETMIN,
    STATUS_POLL,
    STATUS_COUNTING,
    ERROR_NO_MECH,
    ERROR_NO_MIN,
    ERROR_NO_HOUR,
} mech_status_e;

typedef enum {
    DIR_clockwise = 0,
    DIR_COUNTERclockwise  = 1
} mech_direction_e;

typedef enum {
    MODE_DEBUG = 0,
    MODE_WORK  = 1
} mech_mode_e;

#define     HOURINCIRCLE				12
#define     MININHOUR					60
#define     MINMAX						59
#define     HOURCONTROLDEF              5	
// Штатная пауза между фазами шага, мс (STEP HIGH -> LOW)
#define     CLOCKMECH_STEP_DELAY_MS         2
// Пауза фазы шага, если ядро занято (сеть/веб/чтение FS), мс
#define     CLOCKMECH_STEP_BUSY_MS          500
// Интервал между соседними шагами, с которого считаем ядро занятым, мс
#define     CLOCKMECH_STEP_GAP_BUSY_MS      15
// Интервал больше этого — считаем новым запуском механизма (не пауза), мс
#define     CLOCKMECH_STEP_GAP_RESET_MS     1000
typedef void (*DPDR)(void);

typedef struct {
    uint8_t  enable_status;
    String   timeSource;
    uint16_t stepsPerRevolution;
    uint16_t pollInterval;
    uint16_t errorLimitSteps;
    bool     sensorLedEnabled;
} strClockMechConfig;

#endif // _DEVICE_CLOCK_MECH_TYPES_h
