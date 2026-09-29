// EventManager.h - Section 3: queue<Event> (FIFO) + stack<EmergencyEvent> (LIFO)
#pragma once
#include <queue>
#include <stack>
#include <vector>
#include "CityComponent.h"
#include "Event.h"

class EventManager {
private:
    std::queue<Event>          incoming;         // normal events, first come first served
    std::stack<EmergencyEvent> emergencyStack;   // emergency overrides, newest handled first
    std::vector<Event>         processed;        // history of resolved events
    int clockTick = 0;                           // simulation clock (1 tick = 1 second)
    int nextEventId = 1;
    int emergencyThreshold = 4;

    // Single definition of "everything still pending": the emergency stack (top first)
    // followed by the FIFO queue. queue/stack expose no iterators, so both are copied
    // out first - O(n). Shared by prioritisedPending/filterPending/allEvents so the
    // three callers cannot drift apart.
    std::vector<Event> collectPending() const;

public:
    void setEmergencyThreshold(int t) { emergencyThreshold = t; }

    // Creates an event; severity >= threshold goes on the emergency stack, otherwise the queue.
    int submitEvent(EventType type, int severity, const std::string& description);   // O(1)

    // Advances the clock one tick and resolves ONE event: the emergency stack always has
    // priority (LIFO); if it is empty the front of the queue is served (FIFO).
    bool processNext(CityController& city, std::string& report);                      // O(1) + dispatch

    // ---- inspection (queues/stacks cannot be iterated, so we copy them: O(n)) ----
    std::vector<Event>          queueSnapshot() const;        // front -> back
    std::vector<EmergencyEvent> stackSnapshot() const;        // top -> bottom
    std::vector<Event>          prioritisedPending() const;   // std::stable_sort by severity
    std::vector<Event>          filterPending(EventType type) const;   // std::copy_if
    std::vector<Event>          allEvents() const;            // processed + pending

    const std::vector<Event>& processedEvents() const { return processed; }
    std::size_t queueSize() const { return incoming.size(); }
    std::size_t stackSize() const { return emergencyStack.size(); }
    int currentTick() const { return clockTick; }

    // Used by FileManager when loading a saved state.
    void restoreState(int tick, const std::vector<Event>& queued,
                      const std::vector<EmergencyEvent>& stackBottomFirst,
                      const std::vector<Event>& done);
};
