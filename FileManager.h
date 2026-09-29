// FileManager.h - Section 7: File Handling & Persistence
#pragma once
#include <string>
#include "CityData.h"
#include "Config.h"
#include "CityComponent.h"
#include "Engineer.h"
#include "EventManager.h"

// All functions return true on success, false if the file could not be read/written.
namespace FileManager {

// engineers.dat   : ENG001|username|encryptedPassword|clearance
bool saveEngineers(const AuthManager& auth, const std::string& path);
bool loadEngineers(AuthManager& auth, const std::string& path);

// events.dat      : first line TICK|n, then  Q|E|P |id|type|severity|arrival|resolved|description
//                   (Q = queued, E = emergency stack (bottom -> top), P = processed)
bool saveEvents(const EventManager& events, const std::string& path);
bool loadEvents(EventManager& events, const std::string& path);

// city_logs.dat   : id|timestamp|message
bool saveCityLogs(const CityData& city, const std::string& path);
bool loadCityLogs(CityData& city, const std::string& path);

// sensors.dat     : id|day|population|energy|traffic|alert
bool saveSensors(const CityData& city, const std::string& path);
bool loadSensors(CityData& city, const std::string& path);

// components.dat  : id|active|value   (subsystem state: power level, traffic flow, ...)
bool saveComponents(const CityController& controller, const std::string& path);
bool loadComponents(CityController& controller, const std::string& path);

// config.txt      : key=value (lines starting with # are comments)
bool saveConfig(const SystemConfig& cfg, const std::string& path);
bool loadConfig(SystemConfig& cfg, const std::string& path);

// CSV exports (open in Excel)
bool exportLogsCSV(const CityData& city, const std::string& path);
bool exportEventsCSV(const EventManager& events, const std::string& path);
bool exportSensorsCSV(const CityData& city, const std::string& path);

}  // namespace FileManager
