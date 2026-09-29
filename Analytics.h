// Analytics.h - Section 5 (STL algorithms) and Section 6 (system reports)
#pragma once
#include <optional>
#include <string>
#include <vector>
#include "CityComponent.h"
#include "CityData.h"
#include "Config.h"
#include "Event.h"
#include "EventManager.h"

namespace analytics {

enum class SortKey { Energy, Traffic, Population, Alert };
std::string sortKeyName(SortKey k);

// std::sort        - O(n log n). Takes the vector BY VALUE so the stored data keeps its order.
std::vector<SensorReading> sortReadings(std::vector<SensorReading> data, SortKey key, bool descending);

// std::min_element / std::max_element - O(n) each, single pass, no sorting required.
bool findExtremes(const std::vector<SensorReading>& data, SortKey key,
                  SensorReading& lowest, SensorReading& highest);

// std::count_if    - O(n)
long countCriticalReadings(const std::vector<SensorReading>& data, int criticalLevel);
long countCriticalEvents(const std::vector<Event>& events, int emergencyThreshold);

// std::find        - O(n) (uses Event::operator==, which compares IDs)
std::optional<Event> findEvent(const std::vector<Event>& events, int id);

// Section 6: full analytics report (uses iterators, std::map, <algorithm>, <numeric>)
void printReport(const EventManager& events, const CityData& city,
                 const CityController& controller, const SystemConfig& config);

}  // namespace analytics
