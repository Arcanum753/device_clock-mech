#ifndef _DEVICE_CLOCKMECH_h
#define _DEVICE_CLOCKMECH_h

#include "main.h"

#include "mod_context.h"

#ifdef DEBUG_CLOCKMECH
#define DEBUGCLOCKMECH(...) DBG_MOD("[D_CLOCKMECH] ", __VA_ARGS__)
#else
#define DEBUGCLOCKMECH(...)
#endif

#include <LittleFS.h>
#include "device_clock-mech_types.h"
void clockMechTerminalRegister() ;

// Планируемая после шага задача (определена в _engine.cpp)
extern DPDR GoToTaskAfterStep;

class CLASS_DEVICE_CLOCKMECH {
public:
    void setFs(fs::LittleFSFS* fs);
    void begin();
    void begin(ModContext& ctx);
    void web_Init();
    time_t getCurrentTime();

    // Кассета светодиодной индикации ошибки устройства
    static void ledMacrosClockMechError();
    
    
    static void cmdStep();
    static void cmdDir();
    static void cmdEn();
    static void cmdSled();
    static void cmdSens();
    static void cmdN();
    static void cmdSet1200();
    static void cmdSetxx00();
    static void cmdSet12xx();
    static void cmdCount();
    static void cmdSave();
    static void cmdMode();
    static void cmdPoll();
    static void GetSens();
    static void PollTimeTask();
    static void cmdStatus();
    static void cmdSetArrows();
    static void cmdStepWeb(AsyncWebServerRequest *request);
    static void cmdDirWeb(AsyncWebServerRequest *request);
    static void cmdEnWeb(AsyncWebServerRequest *request);
    static void cmdSledWeb(AsyncWebServerRequest *request);
    static void cmdSensWeb(AsyncWebServerRequest *request);
    static void cmdNWeb(AsyncWebServerRequest *request);
    static void cmdResetWeb(AsyncWebServerRequest *request);
    static void cmdSetxx00Web(AsyncWebServerRequest *request);
    static void cmdSet12xxWeb(AsyncWebServerRequest *request);
    static void cmdCountWeb(AsyncWebServerRequest *request);
    static void cmdStatusWeb(AsyncWebServerRequest *request);
private:
    String getVersionStr();
    String getGeneratedTime();
    String getCommitDateStr();
    void html_ver_get(AsyncWebServerRequest *request);

    void handleInfo(AsyncWebServerRequest *request);
    void handleSave(AsyncWebServerRequest *request);
    void handleReset(AsyncWebServerRequest *request);
    void handleCount(AsyncWebServerRequest *request);

    // Конфиг
    void defaultConfig();
    bool loadConfig();
    bool saveConfig();

    // Логика устройства
    void MechTimeSet (uint8_t _inH, uint8_t _inM);
    //work
    void MechInitGPIOs();
    static void MechMoveStepDown();
    static void MechMoveStepUp();
    static void MechNCmdStep();
    
    //set xx:00
    static void MechSetxx00_Setup();
    static void MechSetxx00_Task();
    static void MechSetxx00_endOk();
    static void MechSetxx00_endFail();


    //set12:xx
    static void MechSet12xx_Setup();
    static void MechSet12xx_Task();
    static void MechSet12xx_endOk();
    static void MechSet12xx_endFail();

    //set12:00
    static void MechSet1200_Setup();
    static void MechSet1200_Task();
    static void MechSet1200_endOk();
    static void MechSet1200_endFail();

    //work
    static void MechCountStepsSetup();
    static void MechCountStepsTask();
    static void MechCountStepsOk();
    static void MechCountStepsFail();

    static void MechSetArrows();

    static void MechSetArrowHourSetup();
    static void MechSetArrowHourTask();
    static void MechSetArrowHourEndOk();
    static void MechSetArrowHourEndFail();

    // static void MechSetArrowMinback();
    
    static void MechSetArrowMinSetup();
    static void MechSetArrowMinTask();
    static void MechSetArrowMinOk();
    static void MechSetArrowMinFail();

protected:
    fs::LittleFSFS*     _fs;
    strClockMechConfig _config;
    uint16_t            _mechControlSteps;
    uint8_t             _timeHourReal;
    uint8_t             _timeMinReal;
    uint8_t             _timeMechHour;
    uint16_t            _timeMechMin;
    uint8_t             _Mech_Status;
    uint16_t            _minPrev;
    int                 _sensorLedState;
    int                 _sensorLedStateHOUR;
    int                 _sensorLedStateMIN;
};

extern CLASS_DEVICE_CLOCKMECH device_clock_mech;

#endif
