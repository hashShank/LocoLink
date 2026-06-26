#pragma once
#include "Models.h"
#include "DatabaseManager.h"
#include "RouteEngine.h"
#include "RouteCache.h"

#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <memory>
#include <atomic>

// The payload for the Search Queue
struct SearchTask {
    std::string sourceId;
    std::string destId;
    int startTime;
    // The 'tunnel' to send the ticket back to the user
    std::shared_ptr<std::promise<std::vector<TripLeg>>> resultPromise; 
};

class SystemManager {
private:
    DatabaseManager dbManager;
    RouteEngine routeEngine;
    RouteCache routeCache;

    // --- Concurrency Structures ---
    std::atomic<bool> isRunning;

    // Search Pipeline (Queries)
    std::queue<SearchTask> searchQueue;
    std::mutex searchMutex;
    std::condition_variable searchCV;
    std::thread searchThread;

    // Booking Pipeline (Commands)
    std::queue<BookingRequest> bookingQueue;
    std::mutex bookingMutex;
    std::condition_variable bookingCV;
    std::thread bookingThread;

    // The internal background loops
    void searchWorkerLoop();
    void bookingWorkerLoop();

public:
    SystemManager();
    ~SystemManager();

    // Boots up the engine and background threads
    void startSystem();
    void stopSystem();

    // Pushes a search to the queue and returns a 'future' that will eventually hold the ticket
    std::future<std::vector<TripLeg>> submitSearch(const std::string& src, const std::string& dest, int time);
    
    // Pushes a booking payload to the async booking queue
    void submitBooking(const BookingRequest& request);
    
    // Quick helper exposed for the CLI later
    void printLedger() const { routeEngine.printLedgerState(); }
};