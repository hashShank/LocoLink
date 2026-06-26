#include "SystemManager.h"
#include <iostream>
#include <string>
#include <chrono>
#include <random>

void printTicket(const std::vector<TripLeg>& ticket) {
    if (ticket.empty()) {
        std::cout << "\n[!] No route found or trains are fully booked.\n";
        return;
    }

    std::cout << "\n================ TICKET ================\n";
    for (size_t i = 0; i < ticket.size(); ++i) {
        const auto& leg = ticket[i];
        std::cout << "Leg " << (i + 1) << ": Train " << leg.trainNumber << "\n"
                  << "  Board: " << leg.boardStation << " at " << leg.boardTime << "\n"
                  << "  Alight: " << leg.getOffStation << " at " << leg.getOffTime << "\n";
    }
    std::cout << "========================================\n\n";
}

void runLoadTest(SystemManager& manager) {
    std::cout << "\n--- INITIATING HIGH-FREQUENCY LOAD TEST ---\n";
    const int ITERATIONS = 5000;
    
    // Test 1: Cold Cache (Forcing Dijkstra to run 5000 times)
    auto startCold = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < ITERATIONS; ++i) {
        // Appending 'i' to the time forces a unique search every loop so the cache always misses
        manager.submitSearch("NDLS", "BOM", i % 1400).get(); 
    }
    auto endCold = std::chrono::high_resolution_clock::now();
    auto coldDuration = std::chrono::duration_cast<std::chrono::milliseconds>(endCold - startCold).count();
    
    // Test 2: Hot Cache (Hitting the LRU Cache 5000 times)
    auto startHot = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < ITERATIONS; ++i) {
        // Querying the exact same time hits the O(1) Cache
        manager.submitSearch("NDLS", "BOM", 1630).get(); 
    }
    auto endHot = std::chrono::high_resolution_clock::now();
    auto hotDuration = std::chrono::duration_cast<std::chrono::milliseconds>(endHot - startHot).count();

    std::cout << "Results for " << ITERATIONS << " Route Searches:\n";
    std::cout << "Dijkstra Engine (Cold): " << coldDuration << " ms total (" << (coldDuration * 1000.0 / ITERATIONS) << " microseconds/req)\n";
    std::cout << "LRU Cache Layer (Hot):  " << hotDuration << " ms total (" << (hotDuration * 1000.0 / ITERATIONS) << " microseconds/req)\n";
    std::cout << "Performance Multiplier: Cache is " << (double)coldDuration / (hotDuration == 0 ? 1 : hotDuration) << "x faster.\n\n";
}

int main() {
    SystemManager manager;
    
    // Boots up the Database connection and the Worker Threads
    manager.startSystem();

    while (true) {
        std::cout << "\n--- LocoLink CLI ---\n";
        std::cout << "1. Search Route\n";
        std::cout << "2. Exit\n";
        std::cout << "3. Run Performance Load Test\n";
        std::cout << "Choice: ";
        
        int choice;
        std::cin >> choice;

        if (choice == 3) {
            runLoadTest(manager);
        }

        if (choice == 2) break;

        if (choice == 1) {
            std::string src, dest;
            int time;
            
            std::cout << "Enter Source Station (e.g., NDLS): ";
            std::cin >> src;
            std::cout << "Enter Dest Station (e.g., BOM): ";
            std::cin >> dest;
            std::cout << "Enter Start Time (HHMM, e.g., 0800): ";
            std::cin >> time;

            std::cout << "\nSearching...\n";
            
            // 1. Send the search to the background Queue
            std::future<std::vector<TripLeg>> futureTicket = manager.submitSearch(src, dest, time);
            
            // 2. The Main Thread goes to sleep here, waiting for the bell!
            std::vector<TripLeg> finalTicket = futureTicket.get(); 
            
            // 3. Worker thread rang the bell, we woke up, and now we print it!
            printTicket(finalTicket);
        }
    }

    // Safely shuts down threads before closing
    manager.stopSystem();
    return 0;
}