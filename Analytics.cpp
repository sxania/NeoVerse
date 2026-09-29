// Analytics.cpp
#include "Analytics.h"
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <map>
#include <numeric>

namespace analytics {

static double keyOf(const SensorReading& r, SortKey k) {
    switch (k) {
        case SortKey::Energy:     return r.energyUsage;
        case SortKey::Traffic:    return r.trafficDensity;
        case SortKey::Population: return r.population;
        case SortKey::Alert:      return r.alertLevel;
    }
    return 0;
}

std::string sortKeyName(SortKey k) {
    switch (k) {
        case SortKey::Energy:     return "energy usage";
        case SortKey::Traffic:    return "traffic density";
        case SortKey::Population: return "population";
        case SortKey::Alert:      return "alert level";
    }
    return "";
}

std::vector<SensorReading> sortReadings(std::vector<SensorReading> data, SortKey key, bool descending) {
    std::sort(data.begin(), data.end(), [key, descending](const SensorReading& a, const SensorReading& b) {
        return descending ? keyOf(a, key) > keyOf(b, key) : keyOf(a, key) < keyOf(b, key);
    });
    return data;
}

bool findExtremes(const std::vector<SensorReading>& data, SortKey key,
                  SensorReading& lowest, SensorReading& highest) {
    if (data.empty()) return false;
    auto cmp = [key](const SensorReading& a, const SensorReading& b) { return keyOf(a, key) < keyOf(b, key); };
    lowest  = *std::min_element(data.begin(), data.end(), cmp);
    highest = *std::max_element(data.begin(), data.end(), cmp);
    return true;
}

long countCriticalReadings(const std::vector<SensorReading>& data, int criticalLevel) {
    return std::count_if(data.begin(), data.end(),
                         [criticalLevel](const SensorReading& r) { return r.alertLevel >= criticalLevel; });
}

long countCriticalEvents(const std::vector<Event>& events, int emergencyThreshold) {
    return std::count_if(events.begin(), events.end(),
                         [emergencyThreshold](const Event& e) { return e.getSeverity() >= emergencyThreshold; });
}

std::optional<Event> findEvent(const std::vector<Event>& events, int id) {
    auto it = std::find(events.begin(), events.end(), Event(id));
    if (it == events.end()) return std::nullopt;
    return *it;
}

// ------------------------------ Section 6 report ----------------------------
void printReport(const EventManager& events, const CityData& city,
                 const CityController& controller, const SystemConfig& config) {
    const std::vector<Event>& done = events.processedEvents();

    std::cout << "\n";

    // 1. Total events processed
    std::cout << "Total events processed : " << done.size() << "\n";

    // 2. Most common emergency type - count with a std::map, find the winner with max_element
    std::map<EventType, int> emergencyCounts;
    for (std::vector<Event>::const_iterator it = done.begin(); it != done.end(); ++it)
        if (it->getSeverity() >= config.emergencyThreshold) ++emergencyCounts[it->getType()];

    std::cout << "Most common emergency  : ";
    if (emergencyCounts.empty()) {
        std::cout << "none recorded yet\n";
    } else {
        auto best = std::max_element(emergencyCounts.begin(), emergencyCounts.end(),
                                     [](const std::pair<const EventType, int>& a,
                                        const std::pair<const EventType, int>& b) { return a.second < b.second; });
        std::cout << eventTypeToString(best->first) << " (" << best->second << " occurrences)\n";
        std::cout << "Emergency breakdown    :";
        for (std::map<EventType, int>::const_iterator it = emergencyCounts.begin(); it != emergencyCounts.end(); ++it)
            std::cout << "  " << eventTypeToString(it->first) << "=" << it->second;
        std::cout << "\n";
    }

    // 3. Average (and worst) response time - std::accumulate / std::max_element
    double totalResponse = std::accumulate(done.begin(), done.end(), 0.0,
                                           [](double acc, const Event& e) { return acc + e.responseTime(); });
    double avgResponse = done.empty() ? 0.0 : totalResponse / static_cast<double>(done.size());
    std::cout << "Average response time  : " << std::fixed << std::setprecision(2) << avgResponse << " s\n";
    if (!done.empty()) {
        auto slowest = std::max_element(done.begin(), done.end(),
                                        [](const Event& a, const Event& b) { return a.responseTime() < b.responseTime(); });
        std::cout << "Slowest response       : " << slowest->responseTime() << " s (event #" << slowest->getId() << ")\n";
    }

    // 4. System load summary
    const std::vector<SensorReading>& rd = city.getReadings();
    std::cout << "\n--- System load summary ---\n";
    std::cout << "Simulation clock       : t=" << events.currentTick() << "\n";
    std::cout << "Pending in FIFO queue  : " << events.queueSize() << "\n";
    std::cout << "Pending on emergency stack: " << events.stackSize() << "\n";
    std::cout << "Sensor readings stored : " << rd.size() << " | city logs stored: " << city.getLogs().size() << "\n";
    if (!rd.empty()) {
        double avgEnergy  = std::accumulate(rd.begin(), rd.end(), 0.0,
                                            [](double a, const SensorReading& r) { return a + r.energyUsage; }) / rd.size();
        double avgTraffic = std::accumulate(rd.begin(), rd.end(), 0.0,
                                            [](double a, const SensorReading& r) { return a + r.trafficDensity; }) / rd.size();
        std::cout << "Average energy usage   : " << std::setprecision(1) << avgEnergy << " MWh\n";
        std::cout << "Average traffic density: " << avgTraffic << " %\n";
        std::cout << "Critical sensor alerts : " << countCriticalReadings(rd, config.criticalAlertLevel) << "\n";
    }
    std::cout << "Critical events (sev >= " << config.emergencyThreshold << "): "
              << countCriticalEvents(events.allEvents(), config.emergencyThreshold) << "\n";
    std::cout << "\n--- Subsystem status ---\n";
    for (const std::string& line : controller.statusReport()) std::cout << "  " << line << "\n";
    std::cout << "\n";
}

}  // namespace analytics
