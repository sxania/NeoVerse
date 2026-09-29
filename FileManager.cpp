// FileManager.cpp
#include "FileManager.h"
#include "Utils.h"
#include <fstream>
#include <iomanip>
#include <stdexcept>

namespace FileManager {

// ------------------------------- engineers.dat ------------------------------
bool saveEngineers(const AuthManager& auth, const std::string& path) {
    std::ofstream out(path);
    if (!out) return false;
    for (const Engineer& e : auth.all()) out << e.toRecord() << "\n";
    return static_cast<bool>(out);
}

bool loadEngineers(AuthManager& auth, const std::string& path) {
    std::ifstream in(path);
    if (!in) return false;
    std::vector<Engineer> list;
    std::string line;
    while (std::getline(in, line)) {
        line = util::trim(line);
        if (line.empty()) continue;
        try {
            list.push_back(Engineer::fromRecord(line));
        } catch (const std::exception&) {
            // skip malformed lines instead of crashing
        }
    }
    auth.setAll(std::move(list));
    return true;
}

// --------------------------------- events.dat -------------------------------
static void writeEventLine(std::ofstream& out, char status, const Event& e) {
    out << status << "|" << e.getId() << "|" << static_cast<int>(e.getType()) << "|" << e.getSeverity()
        << "|" << e.getArrivalTick() << "|" << e.getResolvedTick() << "|" << e.getDescription() << "\n";
}

bool saveEvents(const EventManager& events, const std::string& path) {
    std::ofstream out(path);
    if (!out) return false;
    out << "TICK|" << events.currentTick() << "\n";
    for (const Event& e : events.queueSnapshot()) writeEventLine(out, 'Q', e);
    std::vector<EmergencyEvent> stack = events.stackSnapshot();   // top -> bottom
    for (auto it = stack.rbegin(); it != stack.rend(); ++it)      // write bottom -> top so reload keeps order
        writeEventLine(out, 'E', *it);
    for (const Event& e : events.processedEvents()) writeEventLine(out, 'P', e);
    return static_cast<bool>(out);
}

bool loadEvents(EventManager& events, const std::string& path) {
    std::ifstream in(path);
    if (!in) return false;
    int tick = 0;
    std::vector<Event> queued, done;
    std::vector<EmergencyEvent> stackBottomFirst;
    std::string line;
    while (std::getline(in, line)) {
        line = util::trim(line);
        if (line.empty()) continue;
        std::vector<std::string> p = util::split(line, '|', 7);
        try {
            if (p[0] == "TICK" && p.size() >= 2) { tick = std::stoi(p[1]); continue; }
            if (p.size() < 7) continue;
            Event e(std::stoi(p[1]), eventTypeFromInt(std::stoi(p[2])), std::stoi(p[3]),
                    std::stoi(p[4]), p[6]);
            int resolved = std::stoi(p[5]);
            if (resolved >= 0) e.markResolved(resolved);
            if (p[0] == "Q") queued.push_back(e);
            else if (p[0] == "E") stackBottomFirst.push_back(EmergencyEvent(e));
            else if (p[0] == "P") done.push_back(e);
        } catch (const std::exception&) {
            // skip malformed line
        }
    }
    events.restoreState(tick, queued, stackBottomFirst, done);
    return true;
}

// -------------------------------- city_logs.dat -----------------------------
bool saveCityLogs(const CityData& city, const std::string& path) {
    std::ofstream out(path);
    if (!out) return false;
    for (const LogEntry& e : city.getLogs()) out << e.id << "|" << e.timestamp << "|" << e.message << "\n";
    return static_cast<bool>(out);
}

bool loadCityLogs(CityData& city, const std::string& path) {
    std::ifstream in(path);
    if (!in) return false;
    std::string line;
    while (std::getline(in, line)) {
        line = util::trim(line);
        if (line.empty()) continue;
        std::vector<std::string> p = util::split(line, '|', 3);
        if (p.size() < 3) continue;
        try {
            city.addLogRecord({std::stoi(p[0]), p[1], p[2]});
        } catch (const std::exception&) {
        }
    }
    return true;
}

// ---------------------------------- sensors.dat -----------------------------
bool saveSensors(const CityData& city, const std::string& path) {
    std::ofstream out(path);
    if (!out) return false;
    for (const SensorReading& r : city.getReadings())
        out << r.id << "|" << r.day << "|" << r.population << "|" << std::fixed << std::setprecision(1)
            << r.energyUsage << "|" << r.trafficDensity << "|" << r.alertLevel << "\n";
    return static_cast<bool>(out);
}

bool loadSensors(CityData& city, const std::string& path) {
    std::ifstream in(path);
    if (!in) return false;
    std::string line;
    while (std::getline(in, line)) {
        line = util::trim(line);
        if (line.empty()) continue;
        std::vector<std::string> p = util::split(line, '|');
        if (p.size() < 6) continue;
        try {
            city.addReadingRecord({std::stoi(p[0]), std::stoi(p[1]), std::stoi(p[2]), std::stod(p[3]),
                                   std::stoi(p[4]), std::stoi(p[5])});
        } catch (const std::exception&) {
        }
    }
    return true;
}

// -------------------------------- components.dat ----------------------------
bool saveComponents(const CityController& controller, const std::string& path) {
    std::ofstream out(path);
    if (!out) return false;
    for (const std::string& line : controller.toRecords()) out << line << "\n";
    return static_cast<bool>(out);
}

bool loadComponents(CityController& controller, const std::string& path) {
    std::ifstream in(path);
    if (!in) return false;
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(in, line)) {
        line = util::trim(line);
        if (!line.empty()) lines.push_back(line);
    }
    controller.loadRecords(lines);
    return true;
}

// ---------------------------------- config.txt ------------------------------
bool saveConfig(const SystemConfig& cfg, const std::string& path) {
    std::ofstream out(path);
    if (!out) return false;
    out << "# NeoVerse configuration (key=value)\n"
        << "max_login_attempts=" << cfg.maxLoginAttempts << "\n"
        << "emergency_threshold=" << cfg.emergencyThreshold << "\n"
        << "critical_alert_level=" << cfg.criticalAlertLevel << "\n"
        << "sensor_retention_days=" << cfg.sensorRetentionDays << "\n"
        << "log_retention_count=" << cfg.logRetentionCount << "\n"
        << "simulation_seed=" << cfg.simulationSeed << "\n";
    return static_cast<bool>(out);
}

bool loadConfig(SystemConfig& cfg, const std::string& path) {
    std::ifstream in(path);
    if (!in) return false;
    std::string line;
    while (std::getline(in, line)) {
        line = util::trim(line);
        if (line.empty() || line[0] == '#') continue;
        std::vector<std::string> kv = util::split(line, '=', 2);
        if (kv.size() != 2) continue;
        try {
            int v = std::stoi(kv[1]);
            if (kv[0] == "max_login_attempts") cfg.maxLoginAttempts = v;
            else if (kv[0] == "emergency_threshold") cfg.emergencyThreshold = v;
            else if (kv[0] == "critical_alert_level") cfg.criticalAlertLevel = v;
            else if (kv[0] == "sensor_retention_days") cfg.sensorRetentionDays = v;
            else if (kv[0] == "log_retention_count") cfg.logRetentionCount = v;
            else if (kv[0] == "simulation_seed") cfg.simulationSeed = v;
        } catch (const std::exception&) {
        }
    }
    return true;
}

// ------------------------------------ CSV -----------------------------------
bool exportLogsCSV(const CityData& city, const std::string& path) {
    std::ofstream out(path);
    if (!out) return false;
    out << "id,timestamp,message\n";
    for (const LogEntry& e : city.getLogs())
        out << e.id << "," << util::csvEscape(e.timestamp) << "," << util::csvEscape(e.message) << "\n";
    return static_cast<bool>(out);
}

bool exportEventsCSV(const EventManager& events, const std::string& path) {
    std::ofstream out(path);
    if (!out) return false;
    out << "id,type,severity,arrival_tick,resolved_tick,response_time,description\n";
    for (const Event& e : events.allEvents())
        out << e.getId() << "," << util::csvEscape(eventTypeToString(e.getType())) << "," << e.getSeverity()
            << "," << e.getArrivalTick() << "," << e.getResolvedTick() << "," << e.responseTime() << ","
            << util::csvEscape(e.getDescription()) << "\n";
    return static_cast<bool>(out);
}

bool exportSensorsCSV(const CityData& city, const std::string& path) {
    std::ofstream out(path);
    if (!out) return false;
    out << "id,day,population,energy_mwh,traffic_percent,alert_level\n";
    for (const SensorReading& r : city.getReadings())
        out << r.id << "," << r.day << "," << r.population << "," << std::fixed << std::setprecision(1)
            << r.energyUsage << "," << r.trafficDensity << "," << r.alertLevel << "\n";
    return static_cast<bool>(out);
}

}  // namespace FileManager
