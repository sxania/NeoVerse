// Event.h - Section 3: Event Processing System (event types)
#pragma once
#include <string>

enum class EventType { TrafficAccident = 0, PowerFailure = 1, NetworkOverload = 2, WeatherAlert = 3 };

constexpr int EVENT_TYPE_COUNT = 4;

std::string eventTypeToString(EventType t);
EventType   eventTypeFromInt(int value);

// ---------------------------------------------------------------------------
// Event: something that happens in the city. Severity runs 1 (minor) .. 5 (catastrophic).
// Times are simulation "ticks" (1 tick = 1 simulated second).
// ---------------------------------------------------------------------------
class Event {
private:
    int         id;
    EventType   type;
    int         severity;
    int         arrivalTick;
    int         resolvedTick;   // -1 while unresolved
    std::string description;

public:
    Event();
    explicit Event(int id);   // "search key" event - used with std::find
    Event(int id, EventType type, int severity, int arrivalTick, std::string description);
    virtual ~Event() = default;

    int getId() const { return id; }
    EventType getType() const { return type; }
    int getSeverity() const { return severity; }
    int getArrivalTick() const { return arrivalTick; }
    int getResolvedTick() const { return resolvedTick; }
    const std::string& getDescription() const { return description; }

    void markResolved(int tick) { resolvedTick = tick; }
    bool isResolved() const { return resolvedTick >= 0; }
    int  responseTime() const { return isResolved() ? resolvedTick - arrivalTick : 0; }

    // Two events are "equal" when their IDs match (lets std::find work on Event).
    bool operator==(const Event& other) const { return id == other.id; }

    virtual std::string summary() const;
};

// ---------------------------------------------------------------------------
// EmergencyEvent: a high-severity event that overrides the normal queue.
// ---------------------------------------------------------------------------
class EmergencyEvent : public Event {
private:
    std::string responseTeam;

public:
    EmergencyEvent();
    explicit EmergencyEvent(const Event& base);   // promote a normal event to an emergency

    const std::string& getResponseTeam() const { return responseTeam; }
    std::string summary() const override;
};
