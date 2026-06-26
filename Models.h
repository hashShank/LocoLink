#pragma once
#include <string>
#include <vector>

// Represents a node in our graph
struct Station {
    std::string id;        // e.g., "DEL"
    std::string name;      // e.g., "New Delhi"
};

// Represents a directed edge in our time-dependent graph
struct TrainSegment {
    std::string segmentId;   // e.g., "12004-NDLS-MTJ"
    std::string trainNumber; // e.g., "12004"
    std::string sourceId;
    std::string destId;
    int departureTime;       // e.g., 0800
    int arrivalTime;
    int availableSeats;      // The in-memory ledger you will decrement
};

// Represents the payload pushed into your Booking Queue (CQRS Pattern)
struct BookingRequest {
    std::string bookingId; // Unique UUID
    std::string trainId;
    int seatsRequested;
    std::string bookingTimestamp; // Captures C++ time right when RAM deducts the seat
};

// Represents a collapsed portion of the journey on a single train
struct TripLeg {
    std::string trainNumber;
    std::string boardStation;
    std::string getOffStation;
    int boardTime;
    int getOffTime;
};