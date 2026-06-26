#include "RouteEngine.h"
#include <iostream>
#include <climits>
#include <algorithm>

int RouteEngine::hhmmToMinutes(int hhmm) const {
    int hours = hhmm / 100;
    int mins = hhmm % 100;
    return (hours * 60) + mins;
}

int RouteEngine::minutesToHhmm(int minutes) const {
    int hours = minutes / 60;
    int mins = minutes % 60;
    return (hours * 100) + mins;
}

void RouteEngine::initialize(const std::unordered_map<std::string, Station>& stations, 
                             const std::vector<TrainSegment>& segments) {
    graph.clear();
    seatLedger.clear();

    // 1. Initialize Adjacency list nodes
    for (const auto& pair : stations) {
        graph[pair.first] = std::vector<TrainSegment>();
    }

    // 2. Map edges and fill the fast RAM seat ledger
    for (const auto& seg : segments) {
        graph[seg.sourceId].push_back(seg);
        seatLedger[seg.segmentId] = seg.availableSeats;
        segmentLookup[seg.segmentId] = seg;
    }
}

std::vector<std::string> RouteEngine::searchRoute(const std::string& sourceId, 
                                                 const std::string& destId, 
                                                 int startTimeHhmm) {
    int startMins = hhmmToMinutes(startTimeHhmm);

    // Min-Priority Queue for Dijkstra: stores pairs of {Earliest Arrival Time, Station ID}
    std::priority_queue<std::pair<int, std::string>, 
                        std::vector<std::pair<int, std::string>>, 
                        std::greater<std::pair<int, std::string>>> pq;

    // Tracking tracking metrics
    std::unordered_map<std::string, int> minArrivalTime;
    std::unordered_map<std::string, TrainSegment> parentEdge;

    for (const auto& pair : graph) {
        minArrivalTime[pair.first] = INT_MAX;
    }

    // Initialize root
    minArrivalTime[sourceId] = startMins;
    pq.push({startMins, sourceId});

    while (!pq.empty()) {
        auto [currentTime, currentStation] = pq.top();
        pq.pop();

        // If we found an older, slower path from a delayed evaluation, skip it
        if (currentTime > minArrivalTime[currentStation]) continue;
        if (currentStation == destId) break;

        // Traverse edges out of this station
        for (const auto& edge : graph[currentStation]) {
            int edgeDepMins = hhmmToMinutes(edge.departureTime);
            int edgeArrMins = hhmmToMinutes(edge.arrivalTime);

            if (edgeDepMins < currentTime) continue;
            if (seatLedger[edge.segmentId] <= 0) continue;

            // --- THE TRANSFER PENALTY FIX ---
            int inconveniencePenalty = 0;
            // If this isn't the first station, AND the train number is changing, it's a transfer
            if (currentStation != sourceId && edge.trainNumber != parentEdge[currentStation].trainNumber) {
                inconveniencePenalty = 60; // Adds 60 minutes of artificial 'cost' to switching trains
            }

            // We relax the edge based on the penalized cost, but track the REAL arrival time
            if (edgeArrMins + inconveniencePenalty < minArrivalTime[edge.destId]) {
                minArrivalTime[edge.destId] = edgeArrMins + inconveniencePenalty; // Track by Cost
                parentEdge[edge.destId] = edge;
                pq.push({edgeArrMins, edge.destId}); // Push real time to the queue for temporal checks
            }
        }
    }

    // Reconstruct the path backwards from destination to source
    std::vector<std::string> path;
    if (minArrivalTime[destId] == INT_MAX) {
        return path; // Return empty vector if no valid path exists
    }

    std::string current = destId;
    while (current != sourceId) {
        TrainSegment edge = parentEdge[current];
        path.push_back(edge.segmentId);
        current = edge.sourceId;
    }
    
    // Reverse to get correct chronological order (source -> destination)
    std::reverse(path.begin(), path.end());
    return path;
}

bool RouteEngine::bookRoute(const std::vector<std::string>& segmentIds, int seatsRequested) {
    // Phase 1: Verification (All-or-Nothing check)
    for (const auto& id : segmentIds) {
        if (seatLedger[id] < seatsRequested) {
            return false; // Transaction fails if even a single segment is starved
        }
    }

    // Phase 2: Execution (Deduct seats)
    for (const auto& id : segmentIds) {
        seatLedger[id] -= seatsRequested;
    }

    return true;
}

void RouteEngine::printLedgerState() const {
    std::cout << "\n--- CURRENT RAM SEAT LEDGER ---" << std::endl;
    for (const auto& pair : seatLedger) {
        std::cout << "Segment [" << pair.first << "] -> Available Seats: " << pair.second << std::endl;
    }
    std::cout << "--------------------------------\n" << std::endl;
}

TrainSegment RouteEngine::getSegmentById(const std::string& segmentId) const {
    // .at() acts as a strict, instant O(1) hash map lookup
    return segmentLookup.at(segmentId);
}

std::vector<TripLeg> RouteEngine::generateTicket(const std::vector<std::string>& path) {
    std::vector<TripLeg> ticket;
    if (path.empty()) return ticket;

    // We will look up the full segment details from our graph
    // (In a real system, you might store segments in a hash map for O(1) lookup. 
    // Here we assume we can fetch the segment details by ID).
    
    // Let's simulate collapsing the route
    TripLeg currentLeg;
    bool isFirstSegment = true;

    for (const std::string& segId : path) {
        // Find the segment details in our graph (Assuming a helper function 'getSegmentById')
        // For demonstration, let's say 'seg' is the retrieved TrainSegment
        TrainSegment seg = getSegmentById(segId); 

        if (isFirstSegment) {
            currentLeg.trainNumber = seg.trainNumber;
            currentLeg.boardStation = seg.sourceId;
            currentLeg.boardTime = seg.departureTime;
            currentLeg.getOffStation = seg.destId;
            currentLeg.getOffTime = seg.arrivalTime;
            isFirstSegment = false;
        } 
        else if (seg.trainNumber == currentLeg.trainNumber) {
            // User is staying on the same train. Just extend the get-off point!
            currentLeg.getOffStation = seg.destId;
            currentLeg.getOffTime = seg.arrivalTime;
        } 
        else {
            // Train changed! Push the completed leg to the ticket, and start a new leg.
            ticket.push_back(currentLeg);
            
            currentLeg.trainNumber = seg.trainNumber;
            currentLeg.boardStation = seg.sourceId;
            currentLeg.boardTime = seg.departureTime;
            currentLeg.getOffStation = seg.destId;
            currentLeg.getOffTime = seg.arrivalTime;
        }
    }
    // Push the final leg
    ticket.push_back(currentLeg);
    
    return ticket;
}