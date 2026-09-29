// CityData.h - Section 2: City Data Management (dynamic containers)
#pragma once
#include <list>
#include <string>
#include <vector>

// One daily sensor reading.
struct SensorReading {
    int    id;
    int    day;
    int    population;       // citizens
    double energyUsage;      // MWh
    int    trafficDensity;   // 0-100 %
    int    alertLevel;       // 0 (calm) .. 5 (severe)
};

// One historical city log line.
struct LogEntry {
    int         id;
    std::string timestamp;
    std::string message;
};

void printReadingTable(const std::vector<SensorReading>& readings);

class CityData {
private:
    std::vector<SensorReading> readings;   // daily sensor data: fast random access
    std::list<LogEntry>        logs;       // historical logs: unlimited growth
    int nextReadingId = 1;
    int nextLogId     = 1;

public:
    // ---- sensor readings (vector) ----
    void   addReading(int day, int population, double energy, int traffic, int alert);  // O(1) amortised
    void   addReadingRecord(const SensorReading& r);          // used when loading files
    std::size_t removeReadingsBefore(int day);                // O(n) erase-remove idiom
    void   displayReadings() const;                           // O(n)
    const std::vector<SensorReading>& getReadings() const { return readings; }
    int    latestDay() const;                                // O(n) max_element; 0 when empty

    // ---- historical logs (linked list) ----
    void   addLog(const std::string& message);                // O(1) push_back
    void   addLogRecord(const LogEntry& e);                   // used when loading files
    std::size_t trimLogs(std::size_t keepNewest);             // O(k) pop_front, k = removed
    void   displayLogs(std::size_t lastN = 0) const;          // O(n) (0 = show all)
    const std::list<LogEntry>& getLogs() const { return logs; }

    void   clear();
};
