// EventManager.cpp
#include "EventManager.h"
#include "Utils.h"
#include <algorithm>
#include <iterator>

int EventManager::submitEvent(EventType type, int severity, const std::string& description) {
    severity = std::clamp(severity, 1, 5);
    Event e(nextEventId++, type, severity, clockTick, util::sanitize(description));
    if (severity >= emergencyThreshold) emergencyStack.push(EmergencyEvent(e));
    else incoming.push(e);
    return e.getId();
}

bool EventManager::processNext(CityController& city, std::string& report) {
    if (emergencyStack.empty() && incoming.empty()) return false;
    ++clockTick;

    if (!emergencyStack.empty()) {
        EmergencyEvent e = emergencyStack.top();   // LIFO: most recent emergency first
        emergencyStack.pop();
        e.markResolved(clockTick);
        report = "[t=" + std::to_string(clockTick) + "] EMERGENCY OVERRIDE #" + std::to_string(e.getId()) +
                 " " + eventTypeToString(e.getType()) + " (sev " + std::to_string(e.getSeverity()) +
                 ") -> " + city.dispatch(e) + " | team: " + e.getResponseTeam();
        processed.push_back(e);   // stored as a plain Event (base part only)
    } else {
        Event e = incoming.front();                // FIFO: oldest event first
        incoming.pop();
        e.markResolved(clockTick);
        report = "[t=" + std::to_string(clockTick) + "] Queue #" + std::to_string(e.getId()) + " " +
                 eventTypeToString(e.getType()) + " (sev " + std::to_string(e.getSeverity()) + ") -> " +
                 city.dispatch(e);
        processed.push_back(e);
    }
    return true;
}

std::vector<Event> EventManager::queueSnapshot() const {
    std::vector<Event> out;
    std::queue<Event> copy = incoming;   // a copy, so the real queue is untouched
    while (!copy.empty()) {
        out.push_back(copy.front());
        copy.pop();
    }
    return out;
}

std::vector<EmergencyEvent> EventManager::stackSnapshot() const {
    std::vector<EmergencyEvent> out;
    std::stack<EmergencyEvent> copy = emergencyStack;
    while (!copy.empty()) {
        out.push_back(copy.top());
        copy.pop();
    }
    return out;
}

std::vector<Event> EventManager::collectPending() const {
    std::vector<Event> all;
    for (const EmergencyEvent& e : stackSnapshot()) all.push_back(e);   // emergencies first
    for (const Event& e : queueSnapshot()) all.push_back(e);
    return all;
}

std::vector<Event> EventManager::prioritisedPending() const {
    std::vector<Event> all = collectPending();
    // stable_sort keeps arrival order among events of equal severity. O(n log n)
    std::stable_sort(all.begin(), all.end(),
                     [](const Event& a, const Event& b) { return a.getSeverity() > b.getSeverity(); });
    return all;
}

std::vector<Event> EventManager::filterPending(EventType type) const {
    const std::vector<Event> all = collectPending();
    std::vector<Event> out;
    std::copy_if(all.begin(), all.end(), std::back_inserter(out),
                 [type](const Event& e) { return e.getType() == type; });
    return out;
}

std::vector<Event> EventManager::allEvents() const {
    std::vector<Event> all = processed;
    const std::vector<Event> pending = collectPending();
    all.insert(all.end(), pending.begin(), pending.end());
    return all;
}

void EventManager::restoreState(int tick, const std::vector<Event>& queued,
                                const std::vector<EmergencyEvent>& stackBottomFirst,
                                const std::vector<Event>& done) {
    incoming = std::queue<Event>();
    emergencyStack = std::stack<EmergencyEvent>();
    processed = done;
    clockTick = tick;
    int maxId = 0;
    for (const Event& e : queued) { incoming.push(e); maxId = std::max(maxId, e.getId()); }
    for (const EmergencyEvent& e : stackBottomFirst) { emergencyStack.push(e); maxId = std::max(maxId, e.getId()); }
    for (const Event& e : done) maxId = std::max(maxId, e.getId());
    nextEventId = maxId + 1;
}
