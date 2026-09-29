// CityComponent.h - Section 4: Object-Oriented City Architecture
#pragma once
#include <memory>
#include <string>
#include <vector>
#include "Event.h"

// ---------------------------------------------------------------------------
// CityComponent: abstract base class. componentID/name are private (encapsulation);
// derived classes reach them through public getters. processEvent() is pure virtual
// so every subsystem MUST override it -> runtime polymorphism.
// ---------------------------------------------------------------------------
class CityComponent {
private:
    int         componentID;
    std::string name;
    bool        active;

public:
    CityComponent(int id, std::string name);
    virtual ~CityComponent();   // virtual: deleting a derived object through a base pointer is safe

    void activate();
    void deactivate();
    bool isActive() const { return active; }
    int  getID() const { return componentID; }
    const std::string& getName() const { return name; }

    virtual std::string getStatus() const;                     // base status, extended by children
    virtual std::string processEvent(const Event& e) = 0;      // each subsystem reacts differently
    virtual EventType   primaryEventType() const = 0;          // the event type this subsystem owns

    // Persistence hooks: each subsystem exposes/restores its one main state value.
    virtual double getStateValue() const = 0;
    virtual void   setStateValue(double v) = 0;
};

class PowerSystem : public CityComponent {
private:
    double powerLevel;   // % of grid capacity available
public:
    explicit PowerSystem(int id, double level = 100.0);
    ~PowerSystem() override;
    void supplyPower(double amount);
    double getPowerLevel() const { return powerLevel; }
    std::string getStatus() const override;
    std::string processEvent(const Event& e) override;
    EventType primaryEventType() const override { return EventType::PowerFailure; }
    double getStateValue() const override { return static_cast<double>(powerLevel); }
    void setStateValue(double v) override { powerLevel = static_cast<double>(v); }
};

class TransportSystem : public CityComponent {
private:
    int trafficFlow;   // % of normal traffic flow
public:
    explicit TransportSystem(int id, int flow = 90);
    ~TransportSystem() override;
    void manageTraffic();
    int getTrafficFlow() const { return trafficFlow; }
    std::string getStatus() const override;
    std::string processEvent(const Event& e) override;
    EventType primaryEventType() const override { return EventType::TrafficAccident; }
    double getStateValue() const override { return static_cast<double>(trafficFlow); }
    void setStateValue(double v) override { trafficFlow = static_cast<int>(v); }
};

class HealthSystem : public CityComponent {
private:
    int hospitalCnt;   // hospitals currently able to take new patients
public:
    explicit HealthSystem(int id, int hospitals = 12);
    ~HealthSystem() override;
    void provideCare();
    int getHospitalCnt() const { return hospitalCnt; }
    std::string getStatus() const override;
    std::string processEvent(const Event& e) override;
    EventType primaryEventType() const override { return EventType::WeatherAlert; }
    double getStateValue() const override { return static_cast<double>(hospitalCnt); }
    void setStateValue(double v) override { hospitalCnt = static_cast<int>(v); }
};

class SecuritySystem : public CityComponent {
private:
    int threatLevel;   // 0 (safe) .. 10 (critical)
public:
    explicit SecuritySystem(int id, int threat = 1);
    ~SecuritySystem() override;
    void monitorCity();
    int getThreatLevel() const { return threatLevel; }
    std::string getStatus() const override;
    std::string processEvent(const Event& e) override;
    EventType primaryEventType() const override { return EventType::NetworkOverload; }
    double getStateValue() const override { return static_cast<double>(threatLevel); }
    void setStateValue(double v) override { threatLevel = static_cast<int>(v); }
};

// ---------------------------------------------------------------------------
// CityController: owns every subsystem through base-class pointers (polymorphic
// container). It never needs to know the concrete type of each component.
// ---------------------------------------------------------------------------
class CityController {
private:
    std::vector<std::unique_ptr<CityComponent>> components;

public:
    CityController();
    std::string dispatch(const Event& e);                  // route to the responsible subsystem
    std::vector<std::string> broadcast(const Event& e);    // ask EVERY subsystem - shows polymorphism
    std::vector<std::string> statusReport() const;
    CityComponent* componentAt(std::size_t index);
    std::size_t count() const { return components.size(); }

    // "id|active|value" lines for saving / loading subsystem state
    std::vector<std::string> toRecords() const;
    void loadRecords(const std::vector<std::string>& lines);
};
