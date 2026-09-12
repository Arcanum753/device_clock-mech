#include "core_web/FSWebServerLib.h"

#include "core_json/core_json.h"

#include "device_clock-mech.h"
#include "common/common.h"
#include "device_clock-mech_version.h"
#include "core_sys/eertos.h"

#include "core_terminal/core_terminal.h"
#include "core_terminal/ErriezSerialTerminal.h"
#include "core_led/core_led.h"

#include "common/TimeLib.h"

CLASS_DEVICE_CLOCKMECH device_clock_mech;
void CLASS_DEVICE_CLOCKMECH::setFs(fs::LittleFSFS* fs)  {   _fs = fs;   }

// ============================================================
// begin()
// ============================================================
void CLASS_DEVICE_CLOCKMECH::begin() {
    //общий сброс
    DEBUGCLOCKMECH("%s\r\n", __FUNCTION__);
    GoToTaskAfterStep = Idle_task;
    _mechControlSteps = 0;
    _timeMechMin = 0;
    _timeMechHour = 0;
    _timeMinReal = 0;
    _timeHourReal = 0;
    _Mech_Status = STATUS_IDLE;
    _minPrev = 0;
    ledClearState(LED_PRIO_DEV); // сброс моргания ошибки устройства
    GetSens();
    
    defaultConfig(); //конфиги
    if (loadConfig() == false) { saveConfig(); }

    TerminalRegisterModule(clockMechTerminalRegister); // терминал
    MechInitGPIOs(); // инит GPIO

    if (_config.enable_status == MODE_WORK) {SetTask(MechSet1200_Setup); } // если норм режим то работаем
    SetTask(PollTimeTask);
}

void CLASS_DEVICE_CLOCKMECH::begin(ModContext& ctx) {
#if defined(ESP32)
    _fs = ctx.fs;
#endif
    begin();
}

// ============================================================
// web_Init()
// ============================================================
void CLASS_DEVICE_CLOCKMECH::web_Init() {
    DEBUGCLOCKMECH("%s\r\n", __FUNCTION__);

    ESPHTTPServer.on("/clock-mech/save", HTTP_POST, [this](AsyncWebServerRequest *request) {
        if (!ESPHTTPServer.checkAuth(request)) { return request->requestAuthentication(); }
        this->handleSave(request);
    });

    ESPHTTPServer.on("/clock-mech/info", HTTP_GET, [this](AsyncWebServerRequest *request) {
        if (!ESPHTTPServer.checkAuth(request)) { return request->requestAuthentication(); }
        this->handleInfo(request);
    });

    ESPHTTPServer.on("/clock-mech/reset", HTTP_POST, [this](AsyncWebServerRequest *request) {
        if (!ESPHTTPServer.checkAuth(request)) { return request->requestAuthentication(); }
        this->handleReset(request);
    });

    ESPHTTPServer.on("/clock-mech/count", HTTP_POST, [this](AsyncWebServerRequest *request) {
        if (!ESPHTTPServer.checkAuth(request)) { return request->requestAuthentication(); }
        this->handleCount(request);
    });

    
    ESPHTTPServer.on("/clock-mech/step",   HTTP_GET, [](AsyncWebServerRequest *request) { if (ESPHTTPServer.checkAuth(request)) cmdStepWeb(request); else request->requestAuthentication(); });
    ESPHTTPServer.on("/clock-mech/dir",    HTTP_GET, [](AsyncWebServerRequest *request) { if (ESPHTTPServer.checkAuth(request)) cmdDirWeb(request);  else request->requestAuthentication(); });
    ESPHTTPServer.on("/clock-mech/en",     HTTP_GET, [](AsyncWebServerRequest *request) { if (ESPHTTPServer.checkAuth(request)) cmdEnWeb(request);   else request->requestAuthentication(); });
    ESPHTTPServer.on("/clock-mech/sled",   HTTP_GET, [](AsyncWebServerRequest *request) { if (ESPHTTPServer.checkAuth(request)) cmdSledWeb(request); else request->requestAuthentication(); });
    ESPHTTPServer.on("/clock-mech/sens",   HTTP_GET, [](AsyncWebServerRequest *request) { if (ESPHTTPServer.checkAuth(request)) cmdSensWeb(request); else request->requestAuthentication(); });
    ESPHTTPServer.on("/clock-mech/n",      HTTP_GET, [](AsyncWebServerRequest *request) { if (ESPHTTPServer.checkAuth(request)) cmdNWeb(request);    else request->requestAuthentication(); });
    ESPHTTPServer.on("/clock-mech/reset",  HTTP_GET, [](AsyncWebServerRequest *request) { if (ESPHTTPServer.checkAuth(request)) cmdResetWeb(request);  else request->requestAuthentication(); });
    ESPHTTPServer.on("/clock-mech/set-xx00", HTTP_GET, [](AsyncWebServerRequest *request) { if (ESPHTTPServer.checkAuth(request)) cmdSetxx00Web(request); else request->requestAuthentication(); });
    ESPHTTPServer.on("/clock-mech/set-12xx", HTTP_GET, [](AsyncWebServerRequest *request) { if (ESPHTTPServer.checkAuth(request)) cmdSet12xxWeb(request); else request->requestAuthentication(); });
    ESPHTTPServer.on("/clock-mech/count",  HTTP_GET, [](AsyncWebServerRequest *request) { if (ESPHTTPServer.checkAuth(request)) cmdCountWeb(request); else request->requestAuthentication(); });
    ESPHTTPServer.on("/clock-mech/status", HTTP_GET, [](AsyncWebServerRequest *request) { if (ESPHTTPServer.checkAuth(request)) cmdStatusWeb(request); else request->requestAuthentication(); });

    ESPHTTPServer.on("/clock-mech/ver", HTTP_GET, [this](AsyncWebServerRequest *request) { this->html_ver_get(request);});
}

// ============================================================
// Веб-обработчики
// ============================================================
void CLASS_DEVICE_CLOCKMECH::handleInfo(AsyncWebServerRequest *request) {
    DEBUGCLOCKMECH("%s\r\n", __FUNCTION__);
    JsonDocument doc;
    doc["enable_status"]        = _config.enable_status;
    doc["timeSource"]           = _config.timeSource;
    doc["stepsPerRevolution"]   = _config.stepsPerRevolution;
    doc["pollInterval"]         = _config.pollInterval;
    doc["errorLimitSteps"]      = _config.errorLimitSteps;
    doc["sensorLedEnabled"]     = _config.sensorLedEnabled;
    doc["status"]               = _Mech_Status;
    doc["mechMin"]              = _timeMechMin;
    doc["mechHour"]             = _timeMechHour;
    doc["mechControlSteps"]     = _mechControlSteps;

    time_t t = getCurrentTime();
    if (t > 0) {
        char buf[9];
        snprintf(buf, sizeof(buf), "%02d:%02d:%02d", hour(t), minute(t), second(t));
        doc["currentTime"] = buf;
    } else { doc["currentTime"] = "N/A"; }
    String json;
    serializeJson(doc, json);
    request->send(200, "application/json", json);
}

void CLASS_DEVICE_CLOCKMECH::handleSave(AsyncWebServerRequest *request) {
    DEBUGCLOCKMECH("%s\r\n", __FUNCTION__);

    if (request->args() > 0) {
        for (uint8_t i = 0; i < request->args(); i++) {
            String name = request->argName(i);
            String val  = request->arg(i);

            if (name == "enable_status")            { _config.enable_status = (uint8_t)val.toInt(); }
            else if (name == "timeSource")          { _config.timeSource = val; }
            else if (name == "stepsPerRevolution")  { _config.stepsPerRevolution = (uint16_t)val.toInt(); }
            else if (name == "pollInterval")        { _config.pollInterval = (uint16_t)val.toInt(); if (_config.pollInterval < 1) _config.pollInterval = 1; }
            else if (name == "errorLimitSteps")     { _config.errorLimitSteps = (uint16_t)val.toInt(); }
            else if (name == "sensorLedEnabled")    { _config.sensorLedEnabled = (val == "true"); }
        }
        saveConfig();
        request->send(200, "text/plain", "OK");
    }
}

void CLASS_DEVICE_CLOCKMECH::handleReset(AsyncWebServerRequest *request) {
    DEBUGCLOCKMECH("%s\r\n", __FUNCTION__);
    if (_config.enable_status == MODE_WORK) { request->send(403, "application/json", "{\"error\":\"Blocked: WORK mode\"}"); return; }
    SetTask(MechSet1200_Setup);
    request->send(200, "text/plain", "OK");
}

void CLASS_DEVICE_CLOCKMECH::handleCount(AsyncWebServerRequest *request) {
    DEBUGCLOCKMECH("%s\r\n", __FUNCTION__);
    if (_config.enable_status == MODE_WORK) { request->send(403, "application/json", "{\"error\":\"Blocked: WORK mode\"}"); return; }
    SetTask(MechCountStepsSetup);
    request->send(200, "text/plain", "OK");
}

// ============================================================
// Конфиг
// ============================================================
void CLASS_DEVICE_CLOCKMECH::defaultConfig() {
    _config.enable_status      = MODE_DEBUG;
    _config.timeSource         = "ds3231";
    _config.stepsPerRevolution = 0;
    _config.pollInterval       = 5;
    _config.errorLimitSteps    = 500;
    _config.sensorLedEnabled   = true;
}

bool CLASS_DEVICE_CLOCKMECH::loadConfig() {
    DEBUGCLOCKMECH("%s\r\n", __FUNCTION__);
    JsonDocument doc;
    if (core_json.jsonFileLoadDoc(CONFIG_FILE_CLOCKMECH, doc) == false) { return false; }

    _config.enable_status       = doc["enable_status"].as<uint8_t>();
    _config.timeSource          = doc["timeSource"].as<String>();
    _config.stepsPerRevolution  = doc["stepsPerRevolution"].as<uint16_t>();
    _config.pollInterval        = doc["pollInterval"].as<uint16_t>();
    _config.errorLimitSteps     = doc["errorLimitSteps"].as<uint16_t>();
    _config.sensorLedEnabled    = doc["sensorLedEnabled"].as<bool>();

    if (_config.timeSource != "ds3231" && _config.timeSource != "ntp") { _config.timeSource = "ds3231"; }
    if (_config.pollInterval < 1)   { _config.pollInterval = 5; }
    if (_config.stepsPerRevolution < 1) { _config.stepsPerRevolution = 400; }

    return true;
}

bool CLASS_DEVICE_CLOCKMECH::saveConfig() {
    DEBUGCLOCKMECH("%s\r\n", __FUNCTION__);
    JsonDocument doc;
    core_json.jsonFileLoadDoc(CONFIG_FILE_CLOCKMECH, doc);
    doc["enable_status"]        = _config.enable_status;
    doc["timeSource"]           = _config.timeSource;
    doc["stepsPerRevolution"]   = _config.stepsPerRevolution;
    doc["pollInterval"]         = _config.pollInterval;
    doc["errorLimitSteps"]      = _config.errorLimitSteps;
    doc["sensorLedEnabled"]     = _config.sensorLedEnabled;
    return core_json.jsonFileSaveDoc(CONFIG_FILE_CLOCKMECH, doc);
}

// ============================================================
// Версионные методы
// ============================================================
String CLASS_DEVICE_CLOCKMECH::getVersionStr() { return String(DEVICE_CLOCK_MECH_VERSION);  }
String CLASS_DEVICE_CLOCKMECH::getGeneratedTime() { return String(DEVICE_CLOCK_MECH_GENERATED_TIME);    }
String CLASS_DEVICE_CLOCKMECH::getCommitDateStr() { return String(DEVICE_CLOCK_MECH_COMMIT_DATE_STR);   }

void CLASS_DEVICE_CLOCKMECH::html_ver_get(AsyncWebServerRequest *request) {
    DEBUGCLOCKMECH("%s\r\n", __FUNCTION__);
    String values = "";
    values += "clockmechversion|" + getVersionStr()    + "|div\n";
    values += "clockmechgentime|" + getGeneratedTime() + "|div\n";
    values += "clockmechgendate|" + getCommitDateStr() + "|div\n";
    request->send(200, "text/plain", values);
}

// ============================================================
// Регистрация терминальных команд
// ============================================================
void clockMechTerminalRegister() {
    term.addCommand("c-step",   CLASS_DEVICE_CLOCKMECH::cmdStep);
    term.addCommand("c-dir",    CLASS_DEVICE_CLOCKMECH::cmdDir);
    term.addCommand("c-enc",     CLASS_DEVICE_CLOCKMECH::cmdEn);
    term.addCommand("c-sled",   CLASS_DEVICE_CLOCKMECH::cmdSled);
    term.addCommand("c-sens",   CLASS_DEVICE_CLOCKMECH::cmdSens);
    term.addCommand("c-n",      CLASS_DEVICE_CLOCKMECH::cmdN);
    term.addCommand("c-12",     CLASS_DEVICE_CLOCKMECH::cmdSet1200);
    term.addCommand("c-cnt",    CLASS_DEVICE_CLOCKMECH::cmdCount);
    term.addCommand("c-mode",   CLASS_DEVICE_CLOCKMECH::cmdMode);
    term.addCommand("c-pol",    CLASS_DEVICE_CLOCKMECH::cmdPoll);
    term.addCommand("c-stat",   CLASS_DEVICE_CLOCKMECH::cmdStatus);
    term.addCommand("c-set",   CLASS_DEVICE_CLOCKMECH::cmdSetArrows);
    term.addCommand("c-save",   CLASS_DEVICE_CLOCKMECH::cmdSave);
    term.addCommand("c-m00",   CLASS_DEVICE_CLOCKMECH::cmdSetxx00);
    term.addCommand("c-h12",   CLASS_DEVICE_CLOCKMECH::cmdSet12xx);
}
