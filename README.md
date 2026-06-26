# LocoLink 

A multi-threaded C++ railway routing and reservation engine designed to bypass traditional database bottlenecks using an in-memory graph, a custom LRU cache, and a CQRS concurrency model.

## System Overview

LocoLink is engineered for speed and concurrent transaction safety. 

**Data Flow Pipeline:**
* **Initialization:** Disk (MySQL) → Fast C++ RAM Maps.
* **Read Operations (Queries):** Client → Query Queue → Search Thread → LRU Cache / Dijkstra → Future/Promise Tunnel → Client.
* **Write Operations (Commands):** Client → Command Queue → Booking Thread → RAM Deduction (Atomic) → Asynchronous DB Insert.

---

## Core Architecture

### 1. The Data Layer
Decouples disk I/O from algorithmic processing to ensure zero database latency during live searches.
* **Graph Structure:** The network is modeled strictly as consecutive directed edges. A train making 5 stops is represented as 4 independent `TrainSegment` structs, enabling segment-level seat fragmentation tracking.
* **Initialization Sweep:** `loadGraphData()` executes a single O(N) startup sweep. Data is fetched via `mysqlx::SqlResult` and cast natively into standard library containers (`std::unordered_map`, `std::vector`).
* **Write Safety:** Database insertions utilize PreparedStatement placeholder binding to strictly map variables and prevent SQL injection.

### 2. The Algorithmic Brain
Computes optimal routes through a time-dependent graph while enforcing physical business logic constraints.
* **Time-Dependent Dijkstra:** Uses a Min-Priority Queue (`std::greater`) to mathematically guarantee the earliest absolute arrival time in O(E log V) complexity.
* **Dynamic Domain Pruning:** Segments are instantly pruned during edge relaxation if departure time is physically impossible or if the capacity block (`seatLedger[segmentId] <= 0`) is met in O(1) time.
* **Cost-Function Heuristic:** A 60-minute "Transfer Penalty" is dynamically injected into edge costs when switching train numbers, prioritizing human-friendly, zero-layover routes over pure mathematical minimums.
* **Atomic Transactions:** Bookings are split into a Verification Phase and an Execution Phase. If one segment fails, the transaction rolls back before execution, ensuring the RAM ledger remains pristine.

### 3. The Custom LRU Cache Layer
Bypasses O(E log V) pathfinding for high-frequency queries by serving data in O(1) time complexity.
* **Lock-Free Structure:** Deployed exclusively within the isolated Search Thread to completely eliminate Mutex contention.
* **O(1) Eviction & Updates:** Fuses a doubly linked list (`std::list`) with a hash map (`std::unordered_map`). Upon a cache hit, `std::splice` physically unlinks and re-inserts the node at the front in a single pointer operation, avoiding memory reallocation.
* **Lazy Validation Ready:** Caches the raw vector of segment ID strings rather than parsed tickets, mathematically allowing lazy validation against the RAM ledger.

### 4. Concurrency & CQRS
Ensures high-volume transactional booking bursts do not degrade read/search latency for concurrent users.
* **CQRS Segregation:** Operations are pushed into thread-safe `std::queue<SearchTask>` and `std::queue<BookingRequest>`.
* **Zero-Spin Synchronization:** Worker loops utilize `std::condition_variable` tied to a `std::unique_lock`, suspending execution gracefully when queues are empty to eliminate idle CPU spin.
* **Lazy Cache Invalidation:** Solves the stale cache problem efficiently. Upon a cache hit, the manager performs an O(1) ledger lookup. If a cached segment has ≤0 seats, the cache is instantly marked invalid and Dijkstra dynamically routes around the new bottleneck.
* **Asynchronous Returns:** Thread boundary communication is handled via `std::promise` and `std::future`. Promises are wrapped in `std::shared_ptr` to leverage automatic reference-counting, ensuring deterministic memory deallocation.

## Database Setup

LocoLink uses MySQL with the modern X DevAPI (Port 33060). Before running the project, you must set up the database.

1. Log into your local MySQL server.
2. Create the database: `CREATE DATABASE locolink_db;`
3. Ensure the following tables exist with the correct schema:
   - `Stations`: `(station_id, name)`
   - `TrainSegments`: `(segmentId, trainNumber, sourceId, destId, departureTime, arrivalTime, availableSeats)`
   - `Bookings`: `(booking_id, train_id, seats_booked, booking_timestamp)`
4. Update the connection string in `src/DatabaseManager.cpp` with your local MySQL credentials.

## Build Instructions

You will need `cmake`, a C++17-compatible compiler, and the `mysql-connector-c++` library installed (via Homebrew on macOS).

```bash
mkdir build
cd build
cmake ..
make
./LocoLink

