// CityComponent.cpp
#include "CityComponent.h"
#include "Utils.h"
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <sstream>

// ------------------------------- CityComponent ------------------------------
CityComponent::CityComponent(int id, std::string name_)
    : componentID(id), name(std::move(name_)), active(true) {}

CityComponent::~CityComponent() {
    std::cout << "  [shutdown] " << name << " (component " << componentID << ") released\n";
}

void CityComponent::activate() { active = true; }
void CityComponent::deactivate() { active = false; }

std::string CityComponent::getStatus() const {
    return name + " [" + (active ? "ACTIVE" : "OFFLINE") + "]";
}

static std::string offlineMessage(const CityComponent& c) {
    return c.getName() + " is OFFLINE - event must be handled manually";
}

// -------------------------------- PowerSystem -------------------------------
PowerSystem::PowerSystem(int id, double level) : CityComponent(id, "Power System"), powerLevel(level) {}
PowerSystem::~PowerSystem() {}

void PowerSystem::supplyPower(double amount) { powerLevel = std::min(100.0, powerLevel + amount); }

std::string PowerSystem::getStatus() const {
    std::ostringstream o;
    o << CityComponent::getStatus() << " power level " << std::fixed << std::setprecision(1) << powerLevel << "%";
    return o.str();
}

std::string PowerSystem::processEvent(const Event& e) {
    if (!isActive()) return offlineMessage(*this);
    std::ostringstream o;
    o << std::fixed << std::setprecision(1);
    if (e.getType() == EventType::PowerFailure) {
        powerLevel = std::max(0.0, powerLevel - 4.0 * e.getSeverity());   // outage drains the grid
        supplyPower(3.0 * e.getSeverity());                               // backup generators respond
        o << "Power System: backup grid engaged, level now " << powerLevel << "%";
    } else {
        o << "Power System: no grid impact, load balanced (level " << powerLevel << "%)";
    }
    return o.str();
}

// ------------------------------ TransportSystem -----------------------------
TransportSystem::TransportSystem(int id, int flow) : CityComponent(id, "Transport System"), trafficFlow(flow) {}
TransportSystem::~TransportSystem() {}

void TransportSystem::manageTraffic() { trafficFlow = std::min(100, trafficFlow + 8); }

std::string TransportSystem::getStatus() const {
    return CityComponent::getStatus() + " traffic flow " + std::to_string(trafficFlow) + "%";
}

std::string TransportSystem::processEvent(const Event& e) {
    if (!isActive()) return offlineMessage(*this);
    if (e.getType() == EventType::TrafficAccident) {
        trafficFlow = std::max(0, trafficFlow - 10 * e.getSeverity());   // road blocked
        manageTraffic();                                                 // reroute vehicles
        return "Transport System: routes diverted, traffic flow now " + std::to_string(trafficFlow) + "%";
    }
    return "Transport System: no road impact, flow steady at " + std::to_string(trafficFlow) + "%";
}

// ------------------------------- HealthSystem -------------------------------
HealthSystem::HealthSystem(int id, int hospitals) : CityComponent(id, "Health System"), hospitalCnt(hospitals) {}
HealthSystem::~HealthSystem() {}

void HealthSystem::provideCare() { hospitalCnt = std::min(20, hospitalCnt + 1); }   // a hospital frees up

std::string HealthSystem::getStatus() const {
    return CityComponent::getStatus() + " hospitals available " + std::to_string(hospitalCnt);
}

std::string HealthSystem::processEvent(const Event& e) {
    if (!isActive()) return offlineMessage(*this);
    if (e.getType() == EventType::WeatherAlert || e.getType() == EventType::TrafficAccident) {
        if (e.getSeverity() >= 4) hospitalCnt = std::max(0, hospitalCnt - 1);   // hospital reaches capacity
        else provideCare();
        return "Health System: ambulances dispatched, hospitals available " + std::to_string(hospitalCnt);
    }
    return "Health System: on standby (hospitals available " + std::to_string(hospitalCnt) + ")";
}

// ------------------------------ SecuritySystem ------------------------------
SecuritySystem::SecuritySystem(int id, int threat) : CityComponent(id, "Security System"), threatLevel(threat) {}
SecuritySystem::~SecuritySystem() {}

void SecuritySystem::monitorCity() { threatLevel = std::max(0, threatLevel - 1); }   // scan lowers threat

std::string SecuritySystem::getStatus() const {
    return CityComponent::getStatus() + " threat level " + std::to_string(threatLevel) + "/10";
}

std::string SecuritySystem::processEvent(const Event& e) {
    if (!isActive()) return offlineMessage(*this);
    if (e.getType() == EventType::NetworkOverload) {
        threatLevel = std::min(10, threatLevel + e.getSeverity());   // overload may be an attack
        monitorCity();
        return "Security System: traffic filtered and network scanned, threat level " + std::to_string(threatLevel) + "/10";
    }
    monitorCity();
    return "Security System: routine monitoring, threat level " + std::to_string(threatLevel) + "/10";
}

// ------------------------------ CityController ------------------------------
CityController::CityController() {
    components.push_back(std::make_unique<PowerSystem>(1));
    components.push_back(std::make_unique<TransportSystem>(2));
    components.push_back(std::make_unique<HealthSystem>(3));
    components.push_back(std::make_unique<SecuritySystem>(4));
}

std::string CityController::dispatch(const Event& e) {
    auto it = std::find_if(components.begin(), components.end(),
                           [&](const std::unique_ptr<CityComponent>& c) {
                               return c->primaryEventType() == e.getType();
                           });
    if (it == components.end()) return "No subsystem registered for this event";
    return (*it)->processEvent(e);   // virtual call - resolved at run time
}

std::vector<std::string> CityController::broadcast(const Event& e) {
    std::vector<std::string> out;
    for (auto& c : components) out.push_back(c->processEvent(e));   // same call, different behaviour
    return out;
}

std::vector<std::string> CityController::statusReport() const {
    std::vector<std::string> out;
    for (const auto& c : components) out.push_back(c->getStatus());
    return out;
}

CityComponent* CityController::componentAt(std::size_t index) {
    return index < components.size() ? components[index].get() : nullptr;
}

std::vector<std::string> CityController::toRecords() const {
    std::vector<std::string> out;
    for (const auto& comp : components)
        out.push_back(std::to_string(comp->getID()) + "|" + (comp->isActive() ? "1" : "0") + "|" +
                      std::to_string(comp->getStateValue()));
    return out;
}

void CityController::loadRecords(const std::vector<std::string>& lines) {
    for (const std::string& line : lines) {
        std::vector<std::string> p = util::split(line, '|');
        if (p.size() < 3) continue;
        try {
            int id = std::stoi(p[0]);
            auto it = std::find_if(components.begin(), components.end(),
                                   [id](const std::unique_ptr<CityComponent>& c) { return c->getID() == id; });
            if (it == components.end()) continue;
            if (p[1] == "1") (*it)->activate(); else (*it)->deactivate();
            (*it)->setStateValue(std::stod(p[2]));
        } catch (const std::exception&) {
        }
    }
}
