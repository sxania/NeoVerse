// main.cpp - NeoVerse: AI City Survival System (console application)
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <list>
#include <numeric>
#include <optional>
#include <random>
#include <string>
#include <vector>

#include "Analytics.h"
#include "CityComponent.h"
#include "CityData.h"
#include "Config.h"
#include "Engineer.h"
#include "Event.h"
#include "EventManager.h"
#include "FileManager.h"
#include "Utils.h"

namespace {

const std::string DATA_DIR       = "data/";
const std::string ENGINEERS_FILE = DATA_DIR + "engineers.dat";
const std::string EVENTS_FILE    = DATA_DIR + "events.dat";
const std::string LOGS_FILE      = DATA_DIR + "city_logs.dat";
const std::string SENSORS_FILE   = DATA_DIR + "sensors.dat";
const std::string COMPONENTS_FILE = DATA_DIR + "components.dat";
const std::string CONFIG_FILE    = DATA_DIR + "config.txt";

struct InputClosed {};   // thrown when stdin ends (e.g. piped input finishes)

struct App {
    AuthManager     auth;
    CityData        city;
    EventManager    events;
    CityController  controller;
    SystemConfig    config;
    std::mt19937    rng{2035};
};

// ------------------------------- input helpers ------------------------------
std::string readLine(const std::string& label) {
    std::cout << label << std::flush;
    std::string s;
    if (!std::getline(std::cin, s)) throw InputClosed();
    return util::trim(s);
}

int readInt(const std::string& label, int lo, int hi) {
    while (true) {
        std::string s = readLine(label);
        try {
            std::size_t pos = 0;
            int v = std::stoi(s, &pos);
            if (pos == s.size() && v >= lo && v <= hi) return v;
        } catch (...) {
        }
        std::cout << "  Please enter a whole number between " << lo << " and " << hi << ".\n";
    }
}

double readDouble(const std::string& label, double lo, double hi) {
    while (true) {
        std::string s = readLine(label);
        try {
            std::size_t pos = 0;
            double v = std::stod(s, &pos);
            if (pos == s.size() && v >= lo && v <= hi) return v;
        } catch (...) {
        }
        std::cout << "  Please enter a number between " << lo << " and " << hi << ".\n";
    }
}

EventType chooseEventType() {
    std::cout << "  1) Traffic Accident  2) Power Failure  3) Network Overload  4) Weather Alert\n";
    return eventTypeFromInt(readInt("  Event type (1-4): ", 1, 4) - 1);
}

bool allowed(const Engineer& user, Clearance needed) {
    if (static_cast<int>(user.getClearance()) >= static_cast<int>(needed)) return true;
    std::cout << "  ACCESS DENIED: this action needs " << clearanceToString(needed) << " clearance (you have "
              << clearanceToString(user.getClearance()) << ").\n";
    return false;
}

void heading(const std::string& t) { std::cout << "\n--- " << t << " ---\n"; }

// ------------------------------ persistence glue ----------------------------
void report(const std::string& what, bool ok) {
    std::cout << "  " << (ok ? "[ok]   " : "[FAIL] ") << what << "\n";
}

// std::filesystem::create_directories throws filesystem_error if the path cannot be
// created (e.g. "data" already exists as a regular file, or the folder is read-only).
// Catching it here turns an unhandled-exception abort into a clear message.
bool ensureDataDir() {
    try {
        std::filesystem::create_directories(DATA_DIR);
        return true;
    } catch (const std::filesystem::filesystem_error& e) {
        std::cout << "\n[FATAL] Cannot create the data directory '" << DATA_DIR << "': " << e.what() << "\n"
                  << "        NeoVerse stores all state in " << DATA_DIR
                  << ". Start it from a folder you have write permission to.\n";
        return false;
    }
}

void saveAll(App& app) {
    if (!ensureDataDir()) return;
    report("engineers.dat", FileManager::saveEngineers(app.auth, ENGINEERS_FILE));
    report("events.dat",    FileManager::saveEvents(app.events, EVENTS_FILE));
    report("city_logs.dat", FileManager::saveCityLogs(app.city, LOGS_FILE));
    report("sensors.dat",   FileManager::saveSensors(app.city, SENSORS_FILE));
    report("components.dat", FileManager::saveComponents(app.controller, COMPONENTS_FILE));
    report("config.txt",    FileManager::saveConfig(app.config, CONFIG_FILE));
}

void loadAll(App& app) {
    report("engineers.dat", FileManager::loadEngineers(app.auth, ENGINEERS_FILE));
    report("events.dat",    FileManager::loadEvents(app.events, EVENTS_FILE));
    app.city.clear();
    report("city_logs.dat", FileManager::loadCityLogs(app.city, LOGS_FILE));
    report("sensors.dat",   FileManager::loadSensors(app.city, SENSORS_FILE));
    report("components.dat", FileManager::loadComponents(app.controller, COMPONENTS_FILE));
    report("config.txt",    FileManager::loadConfig(app.config, CONFIG_FILE));
    app.events.setEmergencyThreshold(app.config.emergencyThreshold);
}

void seedDefaults(App& app) {
    app.auth.addEngineer(Engineer::createWithPlainPassword("ENG001", "alice", "alice123", Clearance::High));
    app.auth.addEngineer(Engineer::createWithPlainPassword("ENG002", "bob",   "bob123",   Clearance::Medium));
    app.auth.addEngineer(Engineer::createWithPlainPassword("ENG003", "carol", "carol123", Clearance::Low));
}

// Returns false if the data/ directory is unusable, in which case the program must not continue.
bool bootstrap(App& app) {
    if (!ensureDataDir()) return false;
    if (!FileManager::loadConfig(app.config, CONFIG_FILE)) FileManager::saveConfig(app.config, CONFIG_FILE);
    app.rng.seed(static_cast<unsigned>(app.config.simulationSeed));
    app.events.setEmergencyThreshold(app.config.emergencyThreshold);

    bool haveEngineers = FileManager::loadEngineers(app.auth, ENGINEERS_FILE) && app.auth.size() > 0;
    if (!haveEngineers) {
        seedDefaults(app);
        FileManager::saveEngineers(app.auth, ENGINEERS_FILE);
    }
    FileManager::loadEvents(app.events, EVENTS_FILE);
    FileManager::loadCityLogs(app.city, LOGS_FILE);
    FileManager::loadSensors(app.city, SENSORS_FILE);
    FileManager::loadComponents(app.controller, COMPONENTS_FILE);

    if (app.city.getReadings().empty()) {
        app.city.addReading(1, 1200000, 950.5, 45, 1);
        app.city.addReading(2, 1201500, 1010.2, 60, 2);
        app.city.addReading(3, 1203200, 1180.9, 85, 4);
    }
    if (app.city.getLogs().empty()) app.city.addLog("NeoVerse system initialised");
    return true;
}

// --------------------------------- login ------------------------------------
std::optional<Engineer> loginScreen(App& app) {
    std::cout << "   NEOVERSE LABS - AI CITY SURVIVAL SYSTEM  (2035)\n";
    for (int attempt = 1; attempt <= app.config.maxLoginAttempts; ++attempt) {
        std::string user = readLine("Username: ");
        std::string pass = readLine("Password: ");
        std::optional<Engineer> found = app.auth.loginLinear(user, pass);   // std::find_if inside
        if (found) {
            std::cout << "\nAccess granted. Welcome, " << found->getUsername() << " ("
                      << found->getID() << ", clearance " << clearanceToString(found->getClearance()) << ").\n";
            app.city.addLog("Login: " + found->getUsername());
            return found;
        }
        std::cout << "  Invalid credentials (" << (app.config.maxLoginAttempts - attempt) << " attempt(s) left).\n";
    }
    std::cout << "\nToo many failed attempts. System locked.\n";
    return std::nullopt;
}

// ----------------------------- sensor operations ----------------------------
void addRandomReading(App& app) {
    int day = app.city.latestDay() + 1;
    int population = 1200000 + static_cast<int>(app.rng() % 60000);
    double energy = 800.0 + static_cast<double>(app.rng() % 4000) / 10.0;
    int traffic = 20 + static_cast<int>(app.rng() % 80);
    int alert = (app.rng() % 10 < 2) ? 4 + static_cast<int>(app.rng() % 2) : static_cast<int>(app.rng() % 4);
    app.city.addReading(day, population, energy, traffic, alert);
}

void sensorMenu(App& app, const Engineer& user) {
    heading("Sensor data");
    std::cout << "  1) Show all   2) Sort   3) Highest/lowest   4) Add reading\n";
    int c = readInt("  Choice: ", 1, 4);
    const std::vector<SensorReading>& data = app.city.getReadings();
    if (c == 1) {
        app.city.displayReadings();
    } else if (c == 2 || c == 3) {
        std::cout << "  Field: 1) Energy  2) Traffic  3) Population  4) Alert level\n";
        analytics::SortKey key = static_cast<analytics::SortKey>(readInt("  Field: ", 1, 4) - 1);
        if (c == 2) {
            bool desc = readInt("  1) Ascending  2) Descending: ", 1, 2) == 2;
            printReadingTable(analytics::sortReadings(data, key, desc));
        } else {
            SensorReading lo{}, hi{};
            if (!analytics::findExtremes(data, key, lo, hi)) {
                std::cout << "  No data.\n";
            } else {
                std::cout << "  Lowest " << analytics::sortKeyName(key) << ":\n";
                printReadingTable({lo});
                std::cout << "  Highest " << analytics::sortKeyName(key) << ":\n";
                printReadingTable({hi});
            }
        }
        std::cout << "  Critical readings (alert >= " << app.config.criticalAlertLevel << "): "
                  << analytics::countCriticalReadings(data, app.config.criticalAlertLevel) << "\n";
    } else if (allowed(user, Clearance::Medium)) {
        int day = readInt("  Day number: ", 1, 100000);
        int pop = readInt("  Population: ", 0, 100000000);
        double energy = readDouble("  Energy usage (MWh): ", 0, 1e7);
        int traffic = readInt("  Traffic density (0-100): ", 0, 100);
        int alert = readInt("  Alert level (0-5): ", 0, 5);
        app.city.addReading(day, pop, energy, traffic, alert);
        app.city.addLog("Sensor reading added for day " + std::to_string(day) + " by " + user.getUsername());
        std::cout << "  Reading stored.\n";
    }
}

void logMenu(App& app, const Engineer& user) {
    heading("City logs (linked list)");
    std::cout << "  1) Show newest N   2) Show all   3) Add log entry\n";
    int c = readInt("  Choice: ", 1, 3);
    if (c == 1) app.city.displayLogs(static_cast<std::size_t>(readInt("  How many: ", 1, 100000)));
    else if (c == 2) app.city.displayLogs();
    else if (allowed(user, Clearance::Medium)) {
        std::string msg = readLine("  Message: ");
        if (!msg.empty()) { app.city.addLog(msg); std::cout << "  Log stored.\n"; }
    }
}

// ------------------------------- event handling -----------------------------
void showPending(App& app) {
    heading("Pending events");
    std::cout << "  1) Queue and stack   2) Prioritised by severity   3) Filter by type\n";
    int c = readInt("  Choice: ", 1, 3);
    if (c == 1) {
        std::cout << "  Emergency stack (top = handled next, LIFO): " << app.events.stackSize() << "\n";
        for (const EmergencyEvent& e : app.events.stackSnapshot()) std::cout << "    " << e.summary() << "\n";
        std::cout << "  Event queue (front = handled next, FIFO): " << app.events.queueSize() << "\n";
        for (const Event& e : app.events.queueSnapshot()) std::cout << "    " << e.summary() << "\n";
    } else if (c == 2) {
        for (const Event& e : app.events.prioritisedPending()) std::cout << "    " << e.summary() << "\n";
    } else {
        EventType t = chooseEventType();
        std::vector<Event> hits = app.events.filterPending(t);
        std::cout << "  " << hits.size() << " pending " << eventTypeToString(t) << " event(s)\n";
        for (const Event& e : hits) std::cout << "    " << e.summary() << "\n";
    }
}

void submitEvent(App& app, const Engineer& user) {
    heading("Submit event");
    EventType t = chooseEventType();
    int sev = readInt("  Severity (1-5): ", 1, 5);
    std::string desc = readLine("  Description: ");
    if (desc.empty()) desc = eventTypeToString(t) + " reported";
    int id = app.events.submitEvent(t, sev, desc);
    std::cout << "  Event #" << id << (sev >= app.config.emergencyThreshold
                                          ? " escalated to the EMERGENCY STACK (LIFO).\n"
                                          : " added to the FIFO queue.\n");
    app.city.addLog("Event #" + std::to_string(id) + " submitted by " + user.getUsername());
}

void processOne(App& app) {
    std::string rep;
    if (app.events.processNext(app.controller, rep)) {
        std::cout << "  " << rep << "\n";
        app.city.addLog(rep);
    } else {
        std::cout << "  Nothing to process - queue and stack are empty.\n";
    }
}

void runSimulation(App& app, int ticks) {
    static const char* descs[EVENT_TYPE_COUNT][3] = {
        {"Two-vehicle collision on the ring road", "Autonomous bus stalled at junction", "Multi-car pile-up in tunnel"},
        {"Substation fault in the north grid", "Transformer overload downtown", "Solar farm disconnected"},
        {"Citizen-ID servers saturated", "Data-centre link congested", "Suspicious traffic spike on core network"},
        {"Flash-flood warning issued", "Heatwave advisory", "Storm front approaching the coast"}};
    static const int severityTable[10] = {1, 1, 2, 2, 2, 3, 3, 4, 4, 5};   // ~30% are emergencies

    bool verbose = ticks <= 25;
    std::size_t before = app.events.processedEvents().size();
    for (int i = 0; i < ticks; ++i) {
        int arrivals = static_cast<int>(app.rng() % 3);   // 0-2 new events per simulated second
        for (int k = 0; k < arrivals; ++k) {
            int t = static_cast<int>(app.rng() % EVENT_TYPE_COUNT);
            int sev = severityTable[app.rng() % 10];
            app.events.submitEvent(eventTypeFromInt(t), sev, descs[t][app.rng() % 3]);
        }
        if (i % 5 == 0) addRandomReading(app);
        std::string rep;
        if (app.events.processNext(app.controller, rep)) {
            app.city.addLog(rep);
            if (verbose) std::cout << "  " << rep << "\n";
        }
    }
    std::cout << "  Simulation finished: " << ticks << " tick(s), "
              << (app.events.processedEvents().size() - before) << " event(s) processed, "
              << app.events.queueSize() << " queued, " << app.events.stackSize() << " on emergency stack.\n";
}

void searchEvent(App& app) {
    heading("Search event by ID (std::find)");
    int id = readInt("  Event ID: ", 1, 100000000);
    std::vector<Event> all = app.events.allEvents();
    std::optional<Event> hit = analytics::findEvent(all, id);
    if (hit) std::cout << "  Found: " << hit->summary() << "\n";
    else std::cout << "  No event with ID " << id << " (searched " << all.size() << " events).\n";
}

void polymorphismDemo(App& app) {
    heading("Polymorphism demo: one event broadcast to every CityComponent*");
    EventType t = chooseEventType();
    int sev = readInt("  Severity (1-5): ", 1, 5);
    Event probe(0, t, sev, app.events.currentTick(), "demo broadcast");
    std::cout << "  (demo only - the event is not queued)\n";
    for (const std::string& line : app.controller.broadcast(probe)) std::cout << "    " << line << "\n";
}

// ------------------------------ maintenance ---------------------------------
void toggleComponent(App& app) {
    heading("Activate / deactivate subsystem");
    std::vector<std::string> status = app.controller.statusReport();
    for (std::size_t i = 0; i < status.size(); ++i) std::cout << "  " << (i + 1) << ") " << status[i] << "\n";
    std::size_t idx = static_cast<std::size_t>(readInt("  Subsystem: ", 1, static_cast<int>(status.size()))) - 1;
    CityComponent* c = app.controller.componentAt(idx);
    if (c->isActive()) c->deactivate(); else c->activate();
    std::cout << "  " << c->getStatus() << "\n";
    app.city.addLog(c->getName() + (c->isActive() ? " activated" : " deactivated"));
}

void removeOutdated(App& app) {
    heading("Remove outdated data");
    int keepDays = readInt("  Keep readings from the last N days (config default " +
                           std::to_string(app.config.sensorRetentionDays) + "): ", 1, 100000);
    int cutoff = app.city.latestDay() - keepDays + 1;
    std::size_t r = app.city.removeReadingsBefore(cutoff);
    std::cout << "  Removed " << r << " outdated sensor reading(s) (vector erase-remove, O(n)).\n";
    int keepLogs = readInt("  Keep newest N logs (config default " + std::to_string(app.config.logRetentionCount) +
                           "): ", 1, 10000000);
    std::size_t l = app.city.trimLogs(static_cast<std::size_t>(keepLogs));
    std::cout << "  Removed " << l << " old log(s) (list pop_front, O(1) each).\n";
}

void registerEngineer(App& app) {
    heading("Register new engineer");
    std::string user = readLine("  Username: ");
    if (user.empty() || user.find_first_of("| ") != std::string::npos) {
        std::cout << "  Username may not be empty or contain spaces or '|'.\n";
        return;
    }
    std::string pass = readLine("  Password: ");
    if (pass.size() < 4) { std::cout << "  Password must be at least 4 characters.\n"; return; }
    Clearance level = clearanceFromInt(readInt("  Clearance 1) Low  2) Medium  3) High: ", 1, 3) - 1);
    Engineer e = Engineer::createWithPlainPassword(app.auth.nextEngineerID(), user, pass, level);
    if (app.auth.addEngineer(e)) {
        std::cout << "  Registered " << e.describe() << "\n";
        app.city.addLog("Engineer registered: " + e.getUsername());
    } else {
        std::cout << "  Username already exists.\n";
    }
}

void persistenceMenu(App& app, const Engineer& user) {
    heading("File handling");
    std::cout << "  1) Save all   2) Load all   3) Export CSV\n";
    int c = readInt("  Choice: ", 1, 3);
    if (c == 1 && allowed(user, Clearance::Medium)) saveAll(app);
    else if (c == 2 && allowed(user, Clearance::High)) { loadAll(app); std::cout << "  State reloaded from disk.\n"; }
    else if (c == 3 && allowed(user, Clearance::Medium)) {
        report("data/city_logs_export.csv", FileManager::exportLogsCSV(app.city, DATA_DIR + "city_logs_export.csv"));
        report("data/events_export.csv",    FileManager::exportEventsCSV(app.events, DATA_DIR + "events_export.csv"));
        report("data/sensors_export.csv",   FileManager::exportSensorsCSV(app.city, DATA_DIR + "sensors_export.csv"));
    }
}

// -------------------------------- benchmarks --------------------------------
template <typename F>
long long timeMicros(F&& f) {
    auto s = std::chrono::steady_clock::now();
    f();
    return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - s).count();
}

void runBenchmarks() {
    heading("Big-O benchmarks");
    // (a) login search: linear vs binary on many accounts
    const int N = 20000;
    AuthManager big;
    for (int i = 0; i < N; ++i) {
        char buf[16];
        std::snprintf(buf, sizeof buf, "user%05d", i);
        big.addEngineer(Engineer::createWithPlainPassword("E" + std::to_string(i), buf, "pw", Clearance::Low));
    }
    big.sortByUsername();   // sort once up front, then compare pure lookup cost
    const int LOOKUPS = 300;
    bool sink = false;
    long long tLin = timeMicros([&] { for (int i = 0; i < LOOKUPS; ++i) sink ^= big.loginLinear("user19999", "pw").has_value(); });
    long long tBin = timeMicros([&] { for (int i = 0; i < LOOKUPS; ++i) sink ^= big.loginBinary("user19999", "pw").has_value(); });
    std::cout << "  Login search in " << N << " engineers, " << LOOKUPS << " worst-case lookups:\n"
              << "    linear  (find_if,     O(n))     : " << tLin << " us\n"
              << "    binary  (lower_bound, O(log n)) : " << tBin << " us\n";

    // (b) vector vs list
    const int M = 200000;
    std::vector<int> v;
    std::list<int> l;
    long long vIns = timeMicros([&] { for (int i = 0; i < M; ++i) v.push_back(i); });
    long long lIns = timeMicros([&] { for (int i = 0; i < M; ++i) l.push_back(i); });
    long long vSum = 0, lSum = 0;
    long long vTrav = timeMicros([&] { vSum = std::accumulate(v.begin(), v.end(), 0LL); });
    long long lTrav = timeMicros([&] { lSum = std::accumulate(l.begin(), l.end(), 0LL); });
    std::cout << "  Append " << M << " items:   vector " << vIns << " us | list " << lIns << " us\n"
              << "  Traverse " << M << " items: vector " << vTrav << " us | list " << lTrav << " us  (checksums "
              << (vSum == lSum ? "match" : "DIFFER") << ")\n";

    const int F = 20000;
    std::vector<int> vf;
    std::list<int> lf;
    long long vFront = timeMicros([&] { for (int i = 0; i < F; ++i) vf.insert(vf.begin(), i); });
    long long lFront = timeMicros([&] { for (int i = 0; i < F; ++i) lf.push_front(i); });
    std::cout << "  Insert " << F << " items at the FRONT: vector " << vFront << " us (O(n) each) | list " << lFront
              << " us (O(1) each)\n";
    if (sink) std::cout << "";   // stops the optimiser removing the lookups
}

void printDesignNotes() {
    heading("Container choices and Big-O summary");
    std::cout <<
        "  vector<Engineer>       login: find_if O(n); binary search O(log n) needs sorted data (O(n log n) once).\n"
        "  vector<SensorReading>  contiguous memory -> O(1) random access, cache friendly, push_back O(1) amortised.\n"
        "  list<LogEntry>         unbounded growth: push_back O(1), never reallocates/copies, pop_front O(1).\n"
        "  queue<Event>           FIFO: fairness - events are served in arrival order. push/pop O(1).\n"
        "  stack<EmergencyEvent>  LIFO: the newest emergency is the most urgent/relevant. push/pop O(1).\n"
        "  sort O(n log n) | find O(n) | min_element/max_element O(n) | count_if O(n) | stable_sort O(n log n)\n"
        "  Insertion: vector back O(1)*, front O(n); list front/back O(1).  Traversal: both O(n), vector faster.\n";
}

// ---------------------------------- menu ------------------------------------
void printMenu(const Engineer& u) {
    std::cout << "\nMAIN MENU  [" << u.getUsername() << " / " << clearanceToString(u.getClearance()) << "]\n"
              << "  1) City dashboard                 (all)\n"
              << "  2) Sensor data: view/sort/min-max (all; add = Medium+)\n"
              << "  3) City logs                      (all; add = Medium+)\n"
              << "  4) View pending events            (all)\n"
              << "  5) Submit event                   (Medium+)\n"
              << "  6) Process next event             (Medium+)\n"
              << "  7) Run simulation                 (Medium+)\n"
              << "  8) Search event by ID             (all)\n"
              << "  9) Polymorphism demo              (all)\n"
              << " 10) Reports & analytics            (all)\n"
              << " 11) Save / load / export CSV       (Medium+; load = High)\n"
              << " 12) Remove outdated data           (High)\n"
              << " 13) Activate/deactivate subsystem  (High)\n"
              << " 14) Register engineer              (High)\n"
              << " 15) Big-O benchmarks               (all)\n"
              << " 16) Design notes (containers/Big-O)(all)\n"
              << "  0) Exit\n";
}

void mainLoop(App& app, const Engineer& user) {
    while (true) {
        printMenu(user);
        int choice = readInt("Select: ", 0, 16);
        switch (choice) {
            case 0: return;
            case 1:
                heading("City dashboard");
                for (const std::string& s : app.controller.statusReport()) std::cout << "  " << s << "\n";
                std::cout << "  Clock t=" << app.events.currentTick() << " | queue " << app.events.queueSize()
                          << " | emergency stack " << app.events.stackSize() << "\n";
                break;
            case 2: sensorMenu(app, user); break;
            case 3: logMenu(app, user); break;
            case 4: showPending(app); break;
            case 5: if (allowed(user, Clearance::Medium)) submitEvent(app, user); break;
            case 6: if (allowed(user, Clearance::Medium)) processOne(app); break;
            case 7:
                if (allowed(user, Clearance::Medium)) {
                    heading("Simulation");
                    runSimulation(app, readInt("  Ticks (seconds) to simulate (1-1000): ", 1, 1000));
                }
                break;
            case 8: searchEvent(app); break;
            case 9: polymorphismDemo(app); break;
            case 10: analytics::printReport(app.events, app.city, app.controller, app.config); break;
            case 11: persistenceMenu(app, user); break;
            case 12: if (allowed(user, Clearance::High)) removeOutdated(app); break;
            case 13: if (allowed(user, Clearance::High)) toggleComponent(app); break;
            case 14: if (allowed(user, Clearance::High)) registerEngineer(app); break;
            case 15: runBenchmarks(); break;
            case 16: printDesignNotes(); break;
        }
    }
}

}  // namespace

int main() {
    App app;
    if (!bootstrap(app)) return 1;
    std::optional<Engineer> user;
    try {
        user = loginScreen(app);
        if (user) mainLoop(app, *user);
    } catch (const InputClosed&) {
        std::cout << "\n(input closed)\n";
    }

    // Engineers with Medium+ clearance auto-save on exit.
    if (user && static_cast<int>(user->getClearance()) >= static_cast<int>(Clearance::Medium)) {
        std::cout << "\nSaving system state...\n";
        app.city.addLog("Logout: " + user->getUsername());
        saveAll(app);
    }
    std::cout << "Shutting down NeoVerse...\n";
    return 0;   // 'app' goes out of scope here -> component destructors run
}
