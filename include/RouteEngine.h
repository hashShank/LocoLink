#pragma once
#include "Models.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <queue>

class RouteEngine {
private:
    // Adjacency List: Station ID -> Vector of outgoing Train Segments
    std::unordered_map<std::string, std::vector<TrainSegment>> graph;
    // O(1) lookup map for our ticket parser
    std::unordered_map<std::string, TrainSegment> segmentLookup;

    // RAM Seat Ledger: Segment ID -> Remaining Available Seats
    // This allows O(1) checks and modifications without traversing the graph
    std::unordered_map<std::string, int> seatLedger;

    // Helper utilities for absolute minute conversion
    int hhmmToMinutes(int hhmm) const;
    int minutesToHhmm(int minutes) const;
    TrainSegment getSegmentById(const std::string& segmentId) const;

public:
    RouteEngine() = default;

    // Populates the internal RAM structures using data loaded by DatabaseManager
    void initialize(const std::unordered_map<std::string, Station>& stations, 
                    const std::vector<TrainSegment>& segments);

    // Executes Time-Dependent Dijkstra
    // Returns a ordered list of segment IDs making up the optimal journey path
    std::vector<std::string> searchRoute(const std::string& sourceId, 
                                         const std::string& destId, 
                                         int startTimeHhmm);

    // Atomically reserves seats across a block of segments (Transaction simulation)
    bool bookRoute(const std::vector<std::string>& segmentIds, int seatsRequested);

    // Debug tool to print current RAM ledger status
    void printLedgerState() const;

    // Takes the raw segment path and collapses it into flight-style layovers
    std::vector<TripLeg> generateTicket(const std::vector<std::string>& path);

    int getAvailableSeats(const std::string& segmentId) const { return seatLedger.at(segmentId); }
};