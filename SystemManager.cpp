#include "SystemManager.h"
#include <iostream>

SystemManager::SystemManager() : isRunning(false), routeCache(1000) {}

SystemManager::~SystemManager() {
    stopSystem();
}

void SystemManager::startSystem() {
    std::cout << "Booting LocoLink System Manager..." << std::endl;
    
    // 1. The Startup Fetch: Disk -> RAM
    std::unordered_map<std::string, Station> stations;
    std::vector<TrainSegment> segments;
    dbManager.loadGraphData(stations, segments);
    routeEngine.initialize(stations, segments);

    // 2. Ignite the threads
    isRunning = true;
    searchThread = std::thread(&SystemManager::searchWorkerLoop, this);
    bookingThread = std::thread(&SystemManager::bookingWorkerLoop, this);
    
    std::cout << "System Online. Threads active." << std::endl;
}

void SystemManager::stopSystem() {
    if (!isRunning) return;
    isRunning = false;
    
    // Wake up sleeping threads so they can exit gracefully
    searchCV.notify_all();
    bookingCV.notify_all();
    
    if (searchThread.joinable()) searchThread.join();
    if (bookingThread.joinable()) bookingThread.join();
}

std::future<std::vector<TripLeg>> SystemManager::submitSearch(const std::string& src, const std::string& dest, int time) {
    auto prom = std::make_shared<std::promise<std::vector<TripLeg>>>();
    std::future<std::vector<TripLeg>> fut = prom->get_future();
    {
        std::lock_guard<std::mutex> lock(searchMutex);
        searchQueue.push({src, dest, time, prom});
    }
    searchCV.notify_one(); // Wake up the search thread
    return fut;
}

void SystemManager::submitBooking(const BookingRequest& request) {
    {
        std::lock_guard<std::mutex> lock(bookingMutex);
        bookingQueue.push(request);
    }
    bookingCV.notify_one(); // Wake up the booking thread
}

// ==========================================
// BACKGROUND THREAD LOGIC
// ==========================================

void SystemManager::searchWorkerLoop() {
    while (isRunning) {
        SearchTask task;
        {
            std::unique_lock<std::mutex> lock(searchMutex);
            searchCV.wait(lock, [this] { return !searchQueue.empty() || !isRunning; });
            
            if (!isRunning && searchQueue.empty()) break;
            
            task = searchQueue.front();
            searchQueue.pop();
        }

        // --- THE TRAFFIC COP (Cache Logic) ---
        std::string queryKey = task.sourceId + "-" + task.destId + "-" + std::to_string(task.startTime);
        auto cachedPath = routeCache.get(queryKey);

        bool cacheValid = false;

        if (cachedPath.has_value()) {
            cacheValid = true;
            // THE VERIFICATION LOOP: Check RAM ledger to prevent ghost tickets
            for (const std::string& segId : cachedPath.value()) {
                if (routeEngine.getAvailableSeats(segId) <= 0) {
                    cacheValid = false; // Cache is stale!
                    break;
                }
            }
        }

        std::vector<TripLeg> finalTicket;

        if (cacheValid) {
            // CACHE HIT (Valid)
            finalTicket = routeEngine.generateTicket(cachedPath.value());
        } else {
            // CACHE MISS (Or Invalidated) - Run Dijkstra
            std::vector<std::string> newPath = routeEngine.searchRoute(task.sourceId, task.destId, task.startTime);
            if (!newPath.empty()) {
                routeCache.put(queryKey, newPath); // Store the raw path in cache
                finalTicket = routeEngine.generateTicket(newPath);
            }
        }

        // Shoot the ticket back to the main thread via the promise tunnel
        task.resultPromise->set_value(finalTicket);
    }
}

void SystemManager::bookingWorkerLoop() {
    while (isRunning) {
        BookingRequest req;
        {
            std::unique_lock<std::mutex> lock(bookingMutex);
            bookingCV.wait(lock, [this] { return !bookingQueue.empty() || !isRunning; });
            
            if (!isRunning && bookingQueue.empty()) break;
            
            req = bookingQueue.front();
            bookingQueue.pop();
        }

        // 1. In CQRS, the booking thread holds the authority to mutate the RAM graph
        // (In a real system, we'd pass the specific segment IDs. For now, assume we parsed them)
        // bool success = routeEngine.bookRoute(segmentIds, req.seatsRequested);
        
        // 2. If RAM deduction succeeds, asynchronously log to Disk to keep DB updated
        dbManager.logBookingToDB(req);
    }
}