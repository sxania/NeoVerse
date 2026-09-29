// CityData.cpp
#include "CityData.h"
#include "Utils.h"
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <iterator>

void printReadingTable(const std::vector<SensorReading>& readings) {
    if (readings.empty()) {
        std::cout << "  (no sensor readings stored)\n";
        return;
    }
    std::cout << std::left << std::setw(6) << "ID" << std::setw(6) << "Day" << std::setw(13)
              << "Population" << std::setw(14) << "Energy(MWh)" << std::setw(12) << "Traffic(%)"
              << "Alert\n";
    std::cout << std::string(57, '-') << "\n";
    for (std::vector<SensorReading>::const_iterator it = readings.begin(); it != readings.end(); ++it) {
        std::cout << std::left << std::setw(6) << it->id << std::setw(6) << it->day << std::setw(13)
                  << it->population << std::setw(14) << std::fixed << std::setprecision(1)
                  << it->energyUsage << std::setw(12) << it->trafficDensity << it->alertLevel << "\n";
    }
}

// ------------------------------ sensor readings -----------------------------
void CityData::addReading(int day, int population, double energy, int traffic, int alert) {
    readings.push_back({nextReadingId++, day, population, energy,
                        std::clamp(traffic, 0, 100), std::clamp(alert, 0, 5)});
}

void CityData::addReadingRecord(const SensorReading& r) {
    readings.push_back(r);
    nextReadingId = std::max(nextReadingId, r.id + 1);
}

std::size_t CityData::removeReadingsBefore(int day) {
    // erase-remove idiom: remove_if shifts the survivors forward in one O(n) pass,
    // erase then chops off the tail. Much cheaper than erasing element-by-element (O(n^2)).
    auto newEnd = std::remove_if(readings.begin(), readings.end(),
                                 [day](const SensorReading& r) { return r.day < day; });
    std::size_t removed = static_cast<std::size_t>(std::distance(newEnd, readings.end()));
    readings.erase(newEnd, readings.end());
    return removed;
}

void CityData::displayReadings() const { printReadingTable(readings); }

// Highest day number stored - NOT simply readings.back(), because readings may be
// added out of chronological order. "Remove outdated data" uses this as its reference
// point, so it has to be the real maximum. std::max_element is a single O(n) pass.
int CityData::latestDay() const {
    if (readings.empty()) return 0;
    return std::max_element(readings.begin(), readings.end(),
                            [](const SensorReading& a, const SensorReading& b) { return a.day < b.day; })
        ->day;
}

// -------------------------------- city logs ---------------------------------
void CityData::addLog(const std::string& message) {
    logs.push_back({nextLogId++, util::timestamp(), util::sanitize(message)});
}

void CityData::addLogRecord(const LogEntry& e) {
    logs.push_back(e);
    nextLogId = std::max(nextLogId, e.id + 1);
}

std::size_t CityData::trimLogs(std::size_t keepNewest) {
    std::size_t removed = 0;
    while (logs.size() > keepNewest) {   // oldest entries live at the front
        logs.pop_front();                // O(1) each - no shifting like a vector would need
        ++removed;
    }
    return removed;
}

void CityData::displayLogs(std::size_t lastN) const {
    if (logs.empty()) {
        std::cout << "  (no logs stored)\n";
        return;
    }
    std::list<LogEntry>::const_iterator it = logs.begin();
    if (lastN > 0 && lastN < logs.size())
        std::advance(it, static_cast<long>(logs.size() - lastN));   // O(n) walk - lists have no random access
    for (; it != logs.end(); ++it)
        std::cout << "  #" << it->id << " [" << it->timestamp << "] " << it->message << "\n";
}

void CityData::clear() {
    readings.clear();
    logs.clear();
    nextReadingId = nextLogId = 1;
}
