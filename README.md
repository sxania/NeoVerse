# NeoVerse: AI City Survival System
**Programming 622 Assignment - Console-based C++ simulation (C++17)**

A menu-driven simulation of a 2035 smart city. Engineers log in, monitor sensor data, and handle a constant stream of city events using STL containers, STL algorithms and an object-oriented subsystem hierarchy. State is persisted to files.

---

## 1. How to run

**Requirements:** any C++17 compiler (g++ 9+, clang 10+, MSVC 2019+).

**Linux / macOS / WSL / MinGW**
```
make            # builds ./neoverse
./neoverse      # run from the project root so the data/ folder is found
```
or without make:
```
g++ -std=c++17 -O2 -o neoverse src/*.cpp
```

**Visual Studio / Code::Blocks / CLion:** create a C++17 console project, add every file in `src/`, and set the *working directory* to the project root (the folder that contains `data/`). If `data/` is missing, the program creates it and seeds default engineers.

## 2. Sample login credentials

| Username | Password   | Engineer ID | Clearance |
|----------|------------|-------------|-----------|
| `alice`  | `alice123` | ENG001      | High      |
| `bob`    | `bob123`   | ENG002      | Medium    |
| `carol`  | `carol123` | ENG003      | Low       |

Three failed attempts lock the system (configurable in `config.txt`). Passwords are stored encrypted in `engineers.dat` (XOR with a key, hex-encoded). A production system would use a salted hash such as bcrypt or Argon2.

**Clearance levels**

| Level  | Can do |
|--------|--------|
| Low    | View dashboard, sensor data, logs, pending events, reports, run searches, benchmarks |
| Medium | Everything in Low + add sensor readings/logs, submit events, process events, run simulation, save, export CSV |
| High   | Everything in Medium + load from disk, remove outdated data, activate/deactivate subsystems, register engineers |

## 3. Project layout and mapping to the assignment

```
NeoVerse/
  Makefile
  README.md
  src/
    main.cpp            menu, login screen, simulation loop, benchmarks
    Engineer.h/.cpp     Section 1  - Engineer class, AuthManager (vector, find_if, lower_bound)
    CityData.h/.cpp     Section 2  - vector<SensorReading> + list<LogEntry>
    Event.h/.cpp        Section 3  - Event, EmergencyEvent
    EventManager.h/.cpp Section 3  - queue<Event>, stack<EmergencyEvent>, priority/filter
    CityComponent.h/.cpp Section 4 - CityComponent + 4 subsystems, CityController
    Analytics.h/.cpp    Sections 5 & 6 - STL algorithms, reports
    FileManager.h/.cpp  Section 7  - file I/O, CSV export
    Config.h, Utils.h/.cpp  settings and helpers
  data/                 sample data files used for testing (see section 7)
```

| Section | Where in the menu |
|---------|-------------------|
| 1 Authentication | Login screen; option 14 registers engineers |
| 2 City data | Options 2 (sensors), 3 (logs), 12 (remove outdated data) |
| 3 Events | Options 4 (view queue/stack, prioritise, filter), 5, 6, 7 (simulation), 8 (search) |
| 4 OOP | Options 1 (dashboard), 9 (polymorphism demo), 13 (activate/deactivate) |
| 5 Algorithms | Options 2 (sort, min/max, count critical), 8 (find) |
| 6 Reports | Option 10 |
| 7 Files | Option 11 (save / load / export CSV); also autosave on exit |
| Big-O | Option 15 (live benchmarks), option 16 (summary) |

## 4. Containers used and why

| Container | Used for | Why |
|-----------|----------|-----|
| `vector<Engineer>` | Engineer accounts | Small, mostly-read collection; contiguous storage; works with `find_if`, `sort`, `lower_bound` |
| `vector<SensorReading>` | Daily sensor data | **Fast access:** elements sit in one contiguous block, so `readings[i]` is O(1) and traversal is cache-friendly. Readings are appended once per day and read many times, exactly what a vector does best. |
| `list<LogEntry>` | Historical city logs | **Unbounded growth:** a linked list allocates one node per entry, so growth never triggers a reallocation-and-copy of the whole history, and no large contiguous block is needed. Old logs are removed from the front in O(1) each (`pop_front`), where a vector would have to shift every remaining element. Existing nodes never move, so iterators and pointers to entries stay valid. *(Honest note: for pure appends a vector is usually faster in practice, as the benchmark in option 15 shows. The list wins for front removal, front insertion and stable iterators, which is what a rolling log needs.)* |
| `queue<Event>` | Incoming events | **FIFO:** normal events must be processed in the order they arrive, so the oldest event is always served first (fairness). |
| `stack<EmergencyEvent>` | Emergency overrides | **LIFO:** when several emergencies pile up, the most recent one reflects the current state of the city and is resolved first. |
| `vector<Event>` | Processed-event history | Append-only history that is scanned by `count_if`, `find`, `accumulate`. |
| `map<EventType,int>` | Emergency counts in the report | Ordered key -> count; `max_element` finds the most common type. |
| `vector<unique_ptr<CityComponent>>` | City subsystems | Polymorphic container: base-class pointers to Power/Transport/Health/Security systems, freed automatically. |

**Queue vs stack (Section 3):** a queue is right when order of arrival defines fairness; a stack is right when the newest item supersedes older ones. Normal events (severity 1-3) go to the queue. Events at or above the emergency threshold (severity 4-5) go on the stack and **always pre-empt** the queue: `processNext()` empties the stack first, then serves the queue.

## 5. Big-O decisions

| Operation | Structure / algorithm | Complexity | Justification |
|-----------|----------------------|------------|---------------|
| Login (default) | `std::find_if` on unsorted vector | O(n) | Simple, no ordering maintenance, ideal for a small engineer list |
| Login (alternative) | `std::lower_bound` on vector sorted by username | O(log n) per lookup; O(n log n) once to sort | Wins for large user bases with few changes. Every new engineer invalidates the sort, so for a handful of accounts linear search is just as good. Option 15 shows 20,000 accounts: linear ~20 ms vs binary ~0.06 ms for 300 lookups |
| Add sensor reading | `vector::push_back` | O(1) amortised | |
| Remove outdated readings | `remove_if` + `erase` (erase-remove idiom) | O(n) | One pass; erasing element by element would be O(n^2) |
| Traverse readings / logs | iterators | O(n) both | Vector is faster in practice (contiguous memory) |
| Add log | `list::push_back` | O(1) | |
| Trim old logs | `list::pop_front` x k | O(k) | |
| Show newest N logs | `std::advance` on list | O(n) | No random access on a list |
| Submit event | `queue::push` / `stack::push` | O(1) | |
| Process event | `front/top` + `pop` | O(1) | |
| Prioritise pending events | `std::stable_sort` by severity | O(n log n) | Stable, so equal-severity events keep arrival order |
| Filter pending events | `std::copy_if` | O(n) | |
| Sort sensor data | `std::sort` | O(n log n) | Sorts a copy, stored data keeps its order |
| Highest / lowest value | `std::max_element` / `std::min_element` | O(n) each | One pass, cheaper than sorting (O(n log n)) just to read the ends |
| Count critical alerts | `std::count_if` | O(n) | |
| Search for an event | `std::find` | O(n) | History is in arrival order, not sorted by ID, so a linear scan is the right tool |
| Report totals / averages | `std::accumulate`, `std::max_element` | O(n) | |

Queues and stacks cannot be iterated, so viewing them copies the container first (O(n)). Processing never needs that copy.

## 6. Object-oriented design (Section 4)

```
CityComponent (abstract)  - private: componentID, name, active
  |- PowerSystem          - powerLevel  : double   - supplyPower()
  |- TransportSystem      - trafficFlow : int      - manageTraffic()
  |- HealthSystem         - hospitalCnt : int      - provideCare()
  |- SecuritySystem       - threatLevel : int      - monitorCity()
```
* **Encapsulation:** all data members are private; access goes through getters/methods.
* **Inheritance:** four subsystems derive from `CityComponent`.
* **Polymorphism:** `processEvent()` is a pure virtual function each subsystem overrides. `CityController` holds `unique_ptr<CityComponent>` and calls `processEvent()` through the base pointer. Menu option 9 broadcasts one event to all four subsystems and each reacts differently.
* **Constructors and destructors:** the base destructor is `virtual` so deleting through a base pointer is safe. Destructors print a `[shutdown]` line on exit so you can see them run.

Event routing: Traffic Accident -> Transport, Power Failure -> Power, Network Overload -> Security, Weather Alert -> Health.

## 7. Files and formats (Section 7)

| File | Format |
|------|--------|
| `engineers.dat` | `ENG001\|username\|encryptedPassword\|clearance(0-2)` |
| `events.dat` | First line `TICK\|n`; then `Q\|E\|P` + `\|id\|type\|severity\|arrival\|resolved\|description` (Q = queue, E = emergency stack listed bottom to top, P = processed) |
| `city_logs.dat` | `id\|timestamp\|message` |
| `sensors.dat` | `id\|day\|population\|energy\|traffic\|alert` |
| `components.dat` | `componentID\|active\|stateValue` (subsystem state) |
| `config.txt` | `key=value` settings: login attempts, emergency threshold, critical alert level, retention values, random seed |
| `*_export.csv` | CSV exports of logs, events and sensor readings (option 11 -> 3) |

Malformed lines in data files are skipped rather than crashing the program. The included `data/` folder holds a ready-made test state (29 processed events, plus 2 emergencies and 3 normal events still pending) so you can immediately try menu option 4 and option 6 to watch LIFO vs FIFO order.

## 8. Simulation rules

* 1 tick = 1 simulated second. Each tick 0-2 new events arrive (random, seeded by `simulation_seed` so runs are repeatable) and **one** event is processed, so a backlog can build up and response times vary.
* Response time = resolved tick - arrival tick.
* About 30% of random events are emergencies (severity 4-5).
* A new sensor reading (one "day") is added every 5 ticks.
* If a subsystem is OFFLINE its events are reported as needing manual handling.

## 9. Suggested test walk-through

1. Log in as `alice`. Option 4 -> 1 shows the pending stack and queue from the sample data.
2. Option 6 three times: the newest emergency is resolved first (LIFO), then the older one, then the queue in arrival order (FIFO).
3. Option 5: submit a severity-5 event and watch it jump ahead of the queue.
4. Option 7 (30 ticks), then option 10 for the report.
5. Option 2 -> sort / highest-lowest; option 8 to search an event ID.
6. Option 9 to see polymorphism; option 13 to take a subsystem offline and re-run option 9.
7. Option 11 -> 1 (save), restart, and confirm everything (including subsystem state) is restored.
8. Log in as `carol` and try options 5 or 12 to see access control.
9. Option 15 to see the Big-O benchmarks.
