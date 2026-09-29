# NeoVerse: AI City Survival System

A console-based C++17 simulation of an AI-managed city. Engineers log in with a
clearance level, sensor data is recorded and analysed, city events flow through a
FIFO queue and an emergency LIFO stack, four polymorphic subsystems react to those
events, and the whole system state is persisted to disk and reloaded on the next run.

Built for a Year 2 C++ programming assignment covering STL containers, STL
algorithms, object-oriented design and file handling.

---

## 1. How to build and run

### Option A — Visual Studio (Windows)

1. Open `neoverse cpp.slnx` (or `neoverse cpp.vcxproj`) in Visual Studio 2022.
2. The project is already configured for **ISO C++17** (`LanguageStandard = stdcpp17`).
3. Press **F5** / **Ctrl+F5** to build and run.

If you build instead of running from the IDE, note that NeoVerse reads and writes
its `data/` folder **relative to the current working directory**. Set the debugger
working directory to `$(ProjectDir)` (the folder holding `main.cpp`) so it finds
`data/`.

### Option B — Makefile (Windows MSYS2/MinGW, Linux, macOS, WSL)

```sh
make            # builds ./neoverse
make run        # builds, then launches the simulation
make clean      # removes neoverse and the .o files
make rebuild    # clean + build
```

### Option C — one-line g++ command (no build system needed)

```sh
g++ -std=c++17 -Wall -Wextra -O2 -o neoverse *.cpp
./neoverse
```

`*.cpp` expands to exactly the nine translation units below, so this is equivalent
to the Makefile:

```
main.cpp  Analytics.cpp  CityComponent.cpp  CityData.cpp  Engineer.cpp
Event.cpp  EventManager.cpp  FileManager.cpp  Utils.cpp
```

### Running it

Start the program from the folder that contains `data/`. It will create `data/`
automatically if it is missing, and it will seed a fresh city on first run.

**Verified toolchain:** g++ 15.2.0 (MSYS2 UCRT64) and MSVC (Visual Studio 2022).
Compiles with **zero warnings** under `-std=c++17 -Wall -Wextra -Wpedantic -Wshadow`.

---

## 2. Sample login credentials

Three engineers are seeded automatically on first run (`main.cpp`, `seedDefaults`),
and are stored in `data/engineers.dat`:

| Engineer ID | Username | Password     | Clearance | What they can do |
|-------------|----------|--------------|-----------|------------------|
| `ENG001`    | `alice`  | `alice123`   | High      | Everything, including load-from-disk, remove-outdated-data, activate/deactivate subsystems and registering new engineers |
| `ENG002`    | `bob`    | `bob123`     | Medium    | Everything except the High-only actions above |
| `ENG003`    | `carol`  | `carol123`   | Low       | Read-only: dashboard, view/sort/min-max sensor data, view logs, inspect pending events, search, polymorphism demo, reports, benchmarks, design notes |

You have **3 attempts** (`max_login_attempts` in `data/config.txt`); after three
failed logins the system locks and shuts down.

Use `alice` / `alice123` to see the full feature set.

New engineers can be added at runtime from menu option **14** (High clearance
only). The next ID is generated automatically, e.g. `ENG004`.

---

## 3. Main menu

```
 1) City dashboard                    (all)
 2) Sensor data: view/sort/min-max    (all; add = Medium+)
 3) City logs                         (all; add = Medium+)
 4) View pending events               (all)
 5) Submit event                      (Medium+)
 6) Process next event                (Medium+)
 7) Run simulation                    (Medium+)
 8) Search event by ID                (all)
 9) Polymorphism demo                 (all)
10) Reports & analytics               (all)
11) Save / load / export CSV          (Medium+; load = High)
12) Remove outdated data              (High)
13) Activate/deactivate subsystem     (High)
14) Register engineer                 (High)
15) Big-O benchmarks                  (all)
16) Design notes (containers/Big-O)   (all)
 0) Exit
```

A quick tour: **7** (simulate 40 ticks) → **4** (see the queue and stack) →
**10** (full report) → **15** (live Big-O benchmarks) → **16** (design notes) → **0**.

State is auto-saved on exit for Medium and High clearance engineers.

---

## 4. Containers used, and why

| Container | Where | Why this one |
|-----------|-------|--------------|
| `std::vector<Engineer>` | `AuthManager::engineers` (`Engineer.h`) | Accounts are read almost every login and must support O(1) random access and cheap iteration. The set is small, so the O(n) `find_if` scan is cheaper in practice than maintaining a sorted index. |
| `std::vector<SensorReading>` | `CityData::readings` (`CityData.h`) | Daily sensor readings need sorting and min/max scanning constantly. Contiguous memory gives O(1) random access and is far faster to traverse than a linked list. See §5. |
| `std::list<LogEntry>` | `CityData::logs` (`CityData.h`) | The historical city log is append-only and unbounded, and is trimmed from the **front**. A list gives O(1) `push_back` and O(1) `pop_front` with no reallocation and no element shifting. |
| `std::queue<Event>` | `EventManager::incoming` (`EventManager.h`) | Normal events must be served in arrival order (FIFO) — fairness. Also O(1) push/pop. |
| `std::stack<EmergencyEvent>` | `EventManager::emergencyStack` (`EventManager.h`) | Emergencies override the queue, and the **most recent** emergency is the most relevant, so LIFO is the correct discipline. Also O(1) push/pop. |
| `std::map<EventType,int>` | `Analytics::printReport` (`Analytics.cpp`) | Ordered key→count map for tallying emergency types. Ordered output, O(log n) insert. |
| `std::vector<std::unique_ptr<CityComponent>>` | `CityController::components` (`CityComponent.h`) | Polymorphic ownership of heterogeneous subsystems behind base-class pointers, with automatic lifetime management (no raw `new`/`delete`, no leaks). |
| `std::vector<Event>` | `EventManager::processed` and the snapshot functions | Resolved history must be sorted, searched and scanned repeatedly, so it is kept in a random-access container. Also the target for prioritisation, since `queue`/`stack` cannot be sorted. |

---

## 5. Vector vs linked list — the written explanation

This is the same text the program prints at menu option **16**, reproduced here.

> `vector<Engineer>` — login: `find_if` O(n); binary search O(log n) needs sorted
> data (O(n log n) once).
>
> `vector<SensorReading>` — contiguous memory → O(1) random access, cache
> friendly, `push_back` O(1) amortised.
>
> `list<LogEntry>` — unbounded growth: `push_back` O(1), never
> reallocates/copies, `pop_front` O(1).
>
> `queue<Event>` — FIFO: fairness, events are served in arrival order. push/pop O(1).
>
> `stack<EmergencyEvent>` — LIFO: the newest emergency is the most
> urgent/relevant. push/pop O(1).
>
> `sort` O(n log n) | `find` O(n) | `min_element`/`max_element` O(n) |
> `count_if` O(n) | `stable_sort` O(n log n)
>
> Insertion: vector back O(1)\*, front O(n); list front/back O(1).
> Traversal: both O(n), vector faster.
>
> \* amortised — a doubling reallocation occasionally costs O(n).

**Why the vector suits fast access.** A `std::vector` stores its elements in one
contiguous block of memory, so element *i* sits at a fixed offset from the start of
the block and can be reached in constant time. That is what "random access" means,
and it is why `readings[i]` is O(1). Contiguity also means the CPU prefetcher and
the CPU cache can stream the whole array in one go, so a full traversal of a vector
is dramatically faster in wall-clock time than the same traversal of a list, even
though both are formally O(n). A list, by contrast, scatters its nodes across the
heap: every step is a pointer chase to an unpredictable memory address, which is
cache-hostile. Appending is O(1) amortised because the vector grows geometrically
rather than one element at a time.

**Why the linked list suits unbounded logs.** The city log grows without limit and
is never queried by position — it is only appended to at the back and, when
retention is enforced, discarded from the front. A list is a better fit for exactly
that access pattern. `push_back` allocates one node and relinks a single pointer
(O(1), no reallocation and no copying of existing elements), and `pop_front`
unlinks the first node in O(1). By contrast, removing the front element of a vector
requires every remaining element to be shifted one position down, which is O(n) per
removal. Because nodes are allocated individually as needed, the list also never
needs to grow a large block and reallocate it, so long-running logs do not suffer the
copy spikes a vector experiences when it doubles. The trade-off is that a list
cannot be indexed at all: "show the newest 10 logs" has to walk the list from the
front, which is O(n) — the code does exactly this with `std::advance` and says so in
a comment.

**Measured on the development machine** (menu option 15, `runBenchmarks`):

| Benchmark | Result |
|-----------|--------|
| Append 200,000 items | vector **1,952 µs** vs list **22,622 µs** |
| Traverse 200,000 items | vector **184 µs** vs list **5,711 µs** |
| Insert 20,000 items at the **front** | vector **34,741 µs** (O(n) each) vs list **2,632 µs** (O(1) each) |
| Login lookup, 20,000 engineers, 300 worst-case lookups | linear `find_if` **64,486 µs** vs binary `lower_bound` **299 µs** |

These are illustrative, not a formal proof — but they reproduce the expected
asymptotic behaviour and support the container choices above.

---

## 6. Big-O analysis

### Insertion and traversal

| Operation | `std::vector` | `std::list` | Note |
|-----------|---------------|-------------|------|
| Insert at back | O(1) amortised | O(1) | vector occasionally reallocates and copies on doubling |
| Insert at front | O(n) | O(1) | vector must shift every element; list relinks one pointer |
| Remove from front | O(n) | O(1) | this is why the log uses a list |
| Traverse all | O(n) | O(n) | vector much faster in practice (contiguous, cache friendly) |
| Random access by index | O(1) | O(n) | list must walk from the head; no `operator[]` exists |

### Login search

| Strategy | Complexity | Used where | Justification |
|----------|-----------|-----------|---------------|
| `std::find_if` (linear) | O(n) | The real login path (`loginScreen` → `AuthManager::loginLinear`) | Works on unsorted data with no preprocessing. The engineer list is small (typically 3–20), so a linear scan is faster in practice than the overhead of maintaining a sorted index. |
| `std::lower_bound` (binary) | O(log n) per lookup, but O(n log n) **once** to sort | `AuthManager::loginBinary`, exercised by menu option 15 | Only worth it when n is large and the list is reused for many lookups. The vector is sorted lazily and the `sortedByUsername` flag is invalidated whenever an engineer is added, so the sort is paid at most once per change. |

The benchmark at menu option 15 creates 20,000 engineers, sorts them once, then
measures 300 worst-case lookups (searching for the last username) under each
strategy, so the comparison isolates lookup cost from sort cost.

### STL algorithms used

| Algorithm | Complexity | Where | Why it was chosen |
|-----------|-----------|-------|-------------------|
| `std::sort` | O(n log n) | `analytics::sortReadings` (`Analytics.cpp`) | Sorts the whole set when the user asks for a ranked table. O(n log n) beats the O(n²) of a hand-written insertion sort at any realistic n. It takes the vector **by value** so the stored data keeps its original order. |
| `std::stable_sort` | O(n log n) | `EventManager::prioritisedPending` (`EventManager.cpp`) | Prioritises pending events by descending severity. `stable_sort` is used instead of `sort` because equal-severity events must stay in arrival order — a dispatcher should not reorder a same-priority backlog. |
| `std::find` | O(n) | `analytics::findEvent` (`Analytics.cpp`) | Searches the event history for one specific ID. Linear is optimal: the target is unordered, so no better algorithm exists. Uses a dedicated `Event::operator==` that compares IDs only. |
| `std::find_if` | O(n) | `AuthManager::loginLinear`, `CityController::dispatch` | Predicate search — match a username, or find the subsystem responsible for an event type. Same O(n) bound, no sorting required. |
| `std::lower_bound` | O(log n) | `AuthManager::loginBinary` | See the login table above. Requires pre-sorted data. |
| `std::min_element` | O(n) | `analytics::findExtremes` | Finds the lowest reading in one pass. Strictly better than sorting to read `front()` — O(n) instead of O(n log n) when only the extreme is needed. |
| `std::max_element` | O(n) | `analytics::findExtremes`, `CityController` state, `Analytics::printReport` | Same reasoning as `min_element`. Also used on the `std::map` in the report to find the most common emergency type without a second pass. |
| `std::count_if` | O(n) | `analytics::countCriticalReadings`, `analytics::countCriticalEvents` | Counts readings/events meeting a threshold. One pass, no allocation, no intermediate container. |
| `std::any_of` | O(n) | `AuthManager::addEngineer`, `nextEngineerID` | Duplicate username/ID check. Early-exits on first match, so the common case is much faster than O(n). |
| `std::remove_if` + `erase` | O(n) | `CityData::removeReadingsBefore` | Erase-remove idiom: shifts survivors forward in a single pass, then chops the tail. Avoids the O(n²) of erasing element-by-element. |
| `std::copy_if` | O(n) | `EventManager::filterPending` | Filters pending events by type into a new vector, because `queue`/`stack` cannot be iterated or sorted directly. |
| `std::accumulate` | O(n) | `Analytics::printReport` | Computes average response time and average energy/traffic in one pass. |

---

## 7. Event processing: why a queue and why a stack

**`std::queue<Event>` for normal events — FIFO, because fairness matters.**
City events are reported by many independent sensors and citizens. Nothing about a
traffic accident makes it more deserving of attention than a weather alert that
arrived one second later, so the only defensible service discipline is
**first-come, first-served**. A queue guarantees that the order events are handled
in matches the order they arrived, which means no event can be starved by a
continuous flood of later arrivals. `queue` also exposes exactly the operations
needed — `push` at the back, `pop`/`front` at the front — and hides the rest, so
there is no way to accidentally remove an event from the middle.

**`std::stack<EmergencyEvent>` for emergencies — LIFO, because recency means
urgency.** An emergency is by definition an interruption of the normal schedule. In
a real incident the newest emergency is almost always the most relevant, because it
reflects the *current* state of the city: an earlier power failure may already have
been contained, whereas a fresh one means the situation is getting worse. A LIFO
stack therefore lets the dispatcher react to the situation as it is *now* rather
than to a backlog of stale crises, and it enforces the override semantics for free —
`processNext` drains the emergency stack completely before it will touch the FIFO
queue. Crucially, `stack` makes accidental "cherry-picking" of an old emergency
impossible, which is the behaviour a safety-critical system wants.

**`EmergencyEvent` is a subclass of `Event`**, so the two containers stay
interchangeable: an event is constructed once and pushed into whichever structure
its severity warrants. Severity ≥ `emergency_threshold` (default 4, from
`data/config.txt`) goes on the emergency stack; everything else goes on the queue.

**Why prioritisation needs a copy.** `std::queue` and `std::stack` are *container
adapters*: they expose no iterators and no random access, so they cannot be sorted
in place. `EventManager::prioritisedPending` therefore copies the stack and the
queue into a `std::vector<Event>` (via `collectPending`, which reads the stack
top-first so emergencies appear ahead of queued events), then applies
`std::stable_sort` by descending severity. The originals are left untouched — the
copy is a read-only view for display, and the live queue/stack still drive actual
processing order. The cost is O(n) to copy plus O(n log n) to sort, which is
irrelevant at simulation scale and is the standard idiom for this problem.

---

## 8. File formats

All state lives in `data/`, created automatically on first run. Every file is
plain text, `|`-delimited, with one record per line. Malformed lines are skipped
rather than aborting the load, and a missing file is reported as `[FAIL]` rather
than crashing.

### `data/engineers.dat` — one engineer per line
```
ENG001|alice|2f09063500434156|2
ENG002|bob|2c0a0d675741|1
ENG003|carol|2d041d3909434156|0
```
| Field | Meaning |
|-------|---------|
| 1 | Engineer ID (`ENG001` format) |
| 2 | Username |
| 3 | Encrypted password (see [Known limitations](#9-known-limitations)) |
| 4 | Clearance as an integer: `0` = Low, `1` = Medium, `2` = High |

### `data/events.dat` — first line is the clock, then one line per event
```
TICK|98
Q|99|1|2|97|-1|Transformer overload downtown
E|42|0|5|60|-1|Multi-car pile-up in tunnel
P|1|0|1|0|1|hi
```
The first line is `TICK|<currentTick>`. Every subsequent line is
`<status>|<id>|<type>|<severity>|<arrivalTick>|<resolvedTick>|<description>`:

| Field | Meaning |
|-------|---------|
| status | `Q` = in the FIFO queue, `E` = on the emergency stack, `P` = processed |
| type | `0` = Traffic Accident, `1` = Power Failure, `2` = Network Overload, `3` = Weather Alert |
| severity | `1`–`5` |
| resolvedTick | `-1` while unresolved, otherwise the tick it was resolved |

`E` lines are written **bottom → top** of the stack so that reloading pushes them
back in the same order and the LIFO order survives a restart.

### `data/city_logs.dat` — one log line per entry
```
1|2026-09-19 15:11:31|NeoVerse system initialised
```
Format: `id|timestamp|message`. Messages are sanitised on write, so `|` and
newlines inside a message are replaced and can never break the format.

### `data/config.txt` — `key=value`, `#` starts a comment
```
# NeoVerse configuration (key=value)
max_login_attempts=3
emergency_threshold=4
critical_alert_level=4
sensor_retention_days=30
log_retention_count=500
simulation_seed=2035
```
| Key | Default | Effect |
|-----|---------|--------|
| `max_login_attempts` | 3 | Failed logins before the system locks |
| `emergency_threshold` | 4 | Severity at/above which an event becomes an emergency |
| `critical_alert_level` | 4 | Sensor alert level at/above which a reading is "critical" |
| `sensor_retention_days` | 30 | Default window for "remove outdated data" |
| `log_retention_count` | 500 | Default number of newest logs to keep when trimming |
| `simulation_seed` | 2035 | Seeds the RNG so simulations are repeatable |

`simulation_seed` is why the built-in simulation produces identical results on every
run — useful when demonstrating determinism in the report.

### `data/sensors.dat` — one reading per line
```
1|1|1200000|950.5|45|1
```
Format: `id|day|population|energyUsageMWh|trafficDensityPct|alertLevel`
(alert level `0` = calm … `5` = severe).

### `data/components.dat` — one subsystem per line
```
1|1|57.000000
2|1|8.000000
3|1|20.000000
4|0|9.000000
```
Format: `id|active|value`, where `active` is `1`/`0` and `value` is the subsystem's
state — power level (%), traffic flow (%), hospitals available, threat level (0–10).

### CSV exports

Menu option 11 → 3 writes three Excel-friendly files (headers included, fields
quoted and embedded quotes doubled):

| File | Contents |
|------|----------|
| `data/city_logs_export.csv` | `id,timestamp,message` |
| `data/events_export.csv` | `id,type,severity,arrival_tick,resolved_tick,response_time,description` |
| `data/sensors_export.csv` | `id,day,population,energy_mwh,traffic_percent,alert_level` |

---

## 9. Known limitations

These are deliberate, documented trade-offs rather than bugs, but they are worth
stating plainly.

### The password "encryption" is not real hashing

Passwords are obfuscated with a **repeating-key XOR cipher**, hex-encoded
(`Engineer::encrypt`):

```cpp
static const std::string key = "NeoVerse2035";
// c = plain[i] ^ key[i % key.size()]
```

**This is reversible, not a hash.** The key is hardcoded in the source, so anyone
who can read `Engineer.cpp` or `data/engineers.dat` can recover every plaintext
password in seconds. XOR with a repeating key is trivially broken with known-plaintext
analysis, the key is far too short, and there is no salt — so identical passwords
produce identical ciphertexts. `verifyPassword` also compares the re-encrypted
strings, which is not constant-time and is theoretically vulnerable to timing
analysis. Note that `ENG001` really does encode `alice123` as
`2f09063500434156`.

This is acceptable for a teaching simulation, where the brief asks only to show
that passwords are not stored in plain text, and the source comment says so. A
production system must use a slow, salted, adaptive hash such as **bcrypt**,
**scrypt** or **Argon2**, and compare with a constant-time function.

### Emergency events are sliced when stored in history

`EventManager::processNext` pushes a resolved emergency into the `processed` vector,
which is a `std::vector<Event>`:

```cpp
processed.push_back(e);   // e is an EmergencyEvent -> only the Event base is kept
```

This is a deliberate slice: the derived `responseTeam` member is **not retained**.
Every report and the CSV export therefore show emergency events as plain events,
with no record of which team handled them. It is safe — the slice happens by value
on a polymorphic type, so there is no undefined behaviour, and it is commented in
the source — but it is a loss of information. Storing
`std::vector<std::unique_ptr<Event>>` would preserve the dynamic type if that
mattered.

### Response time is measured in clock ticks, not seconds

`processNext` only increments `clockTick` when there is an event to resolve. An
idle second does not advance the clock. Consequently "average response time" in the
report is the average number of *events processed* between an event arriving and
being resolved, not elapsed wall-clock or simulated time. This keeps the metric
stable regardless of how long the operator spends at the menu, but it is not a
measure of real responsiveness.

### "Most common emergency" counts processed events only

The emergency-type breakdown in the report is tallied from the `processed` history
only. Emergencies that are still sitting on the stack or in the queue are **not**
included, so the breakdown can differ from the live "Critical events" count shown
further down the same report. Counting pending events as well would be a one-line
change to the loop in `Analytics::printReport`.

### Other, smaller notes

- `AuthManager::loginBinary` is not on the live login path. Login uses the O(n)
  `find_if`; the binary version exists to make the Big-O comparison in §6
  measurable and is exercised by menu option 15.
- Event IDs are not reused after a load — `restoreState` sets the next ID to one
  past the highest ID found in any file — but they are not persisted separately, so
  the ID counter is derived from the events themselves.
- The simulation is a closed model driven by a seeded `std::mt19937`; it does not
  read real sensor feeds.
- Memory safety: the project uses `std::unique_ptr` and RAII throughout, with no
  raw `new`/`delete`. It runs clean under `-fsanitize=undefined`. AddressSanitizer
  could not be linked in the development environment (no ASan runtime available for
  the MinGW toolchain), so leak-freedom is argued from the ownership model rather
  than measured.

---

## 10. Project layout

```
neoverse cpp/
├── Makefile                  # g++ build (make / make run / make clean / make rebuild)
├── README.md                 # this file
├── .gitignore
├── neoverse cpp.slnx         # Visual Studio solution
├── neoverse cpp.vcxproj      # Visual Studio project (stdcpp17, WarningLevel3)
├── main.cpp                  # menus, input handling, login, persistence glue
│
├── Engineer.h / .cpp         # Section 1  Engineer, Clearance, AuthManager, XOR cipher
├── CityData.h / .cpp         # Section 2  vector<SensorReading>, list<LogEntry>
├── Event.h / .cpp            # Section 3  Event, EmergencyEvent, EventType
├── EventManager.h / .cpp     # Section 3  queue<Event>, stack<EmergencyEvent>
├── CityComponent.h / .cpp    # Section 4  CityComponent + 4 derived, CityController
├── Analytics.h / .cpp        # Sections 5 & 6  STL algorithms and reports
├── FileManager.h / .cpp      # Section 7  persistence and CSV export
├── Config.h                  # SystemConfig defaults
├── Utils.h / .cpp            # timestamp, trim, sanitise, CSV escape, split
│
└── data/                     # created automatically; safe to delete to reset
    ├── engineers.dat
    ├── events.dat
    ├── city_logs.dat
    ├── sensors.dat
    ├── components.dat
    ├── config.txt
    └── *_export.csv          # written by menu option 11
```

---

## 11. Submission checklist

- [x] All nine `.cpp` and nine `.h` files present
- [x] `data/` test files included
- [x] `README.md` (this file)
- [x] `Makefile` and a one-line `g++` command
- [x] Clean build with `-std=c++17 -Wall -Wextra` (zero warnings)
- [ ] PDF report uploaded to Moodle
- [ ] Cover page (name, student number, module, lecturer, date)
- [ ] Times New Roman 12 pt, 1.5 line spacing
- [ ] Harvard referencing
- [ ] Signed declaration of originality

> **Before zipping:** delete the stale root-level data files (`engineers.dat`,
> `events.dat`, `sensors.dat`, `components.dat`, `config.txt`, `city_logs.dat`,
> `city_logs_export.csv`, `events_export.csv`, `sensors_export.csv`) and the build
> output folders `x64/` and `neoverse cpp/`. The program only ever reads `data/`;
> the root-level copies are leftovers from an earlier version and contradict
> `data/`, which makes persistence look broken. See `.gitignore` and
> `STALE_FILES_TO_DELETE.md`.
>
> **To reset the simulation to a clean state:** delete the whole `data/` folder and
> start the program again — it recreates and reseeds everything.
