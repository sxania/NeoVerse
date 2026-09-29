// Event.cpp
#include "Event.h"
#include <sstream>

std::string eventTypeToString(EventType t) {
    switch (t) {
        case EventType::TrafficAccident: return "Traffic Accident";
        case EventType::PowerFailure:    return "Power Failure";
        case EventType::NetworkOverload: return "Network Overload";
        case EventType::WeatherAlert:    return "Weather Alert";
    }
    return "Unknown";
}

EventType eventTypeFromInt(int value) {
    if (value < 0 || value >= EVENT_TYPE_COUNT) value = 0;
    return static_cast<EventType>(value);
}

// ----------------------------------- Event ----------------------------------
Event::Event()
    : id(0), type(EventType::TrafficAccident), severity(1), arrivalTick(0), resolvedTick(-1), description("") {}

Event::Event(int id_)
    : id(id_), type(EventType::TrafficAccident), severity(1), arrivalTick(0), resolvedTick(-1), description("") {}

Event::Event(int id_, EventType type_, int severity_, int arrivalTick_, std::string description_)
    : id(id_), type(type_), severity(severity_), arrivalTick(arrivalTick_), resolvedTick(-1),
      description(std::move(description_)) {}

std::string Event::summary() const {
    std::ostringstream o;
    o << "#" << id << " [" << eventTypeToString(type) << "] severity " << severity
      << ", arrived t=" << arrivalTick;
    if (isResolved()) o << ", resolved t=" << resolvedTick << " (response " << responseTime() << "s)";
    o << " - " << description;
    return o.str();
}

// ------------------------------- EmergencyEvent -----------------------------
static std::string teamFor(EventType t) {
    switch (t) {
        case EventType::TrafficAccident: return "Rapid Response Unit";
        case EventType::PowerFailure:    return "Grid Repair Squad";
        case EventType::NetworkOverload: return "Cyber Defence Team";
        case EventType::WeatherAlert:    return "Disaster Relief Crew";
    }
    return "General Response";
}

EmergencyEvent::EmergencyEvent() : Event(), responseTeam("General Response") {}

EmergencyEvent::EmergencyEvent(const Event& base) : Event(base), responseTeam(teamFor(base.getType())) {}

std::string EmergencyEvent::summary() const {
    return "EMERGENCY " + Event::summary() + " | team: " + responseTeam;
}
