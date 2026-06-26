#include "DatabaseManager.h"
#include <iostream>

DatabaseManager::DatabaseManager() {
    try {
        // Connect using X Protocol (Port 33060).
        session = std::make_unique<mysqlx::Session>("localhost", 33060, "root", "Shashank@0230");
        session->sql("USE locolink_db").execute();
    } catch (const mysqlx::Error &err) {
        std::cerr << "DB Connection Error: " << err.what() << std::endl;
        exit(1);
    }
}

void DatabaseManager::loadGraphData(std::unordered_map<std::string, Station>& stations, 
                                    std::vector<TrainSegment>& segments) {
    try {
        // 1. Fetch all stations into the map
        mysqlx::SqlResult res = session->sql("SELECT * FROM Stations").execute();
        
        for (mysqlx::Row row : res) {
            Station s;
            s.id = (std::string)row[0];   // station_id
            s.name = (std::string)row[1]; // name
            stations[s.id] = s;
        }

        // 2. Fetch all train segments (The Edges) into the vector
        res = session->sql("SELECT * FROM TrainSegments").execute();
        
        for (mysqlx::Row row : res) {
            TrainSegment ts;
            ts.segmentId = (std::string)row[0];
            ts.trainNumber = (std::string)row[1];
            ts.sourceId = (std::string)row[2];
            ts.destId = (std::string)row[3];
            ts.departureTime = (int)row[4];
            ts.arrivalTime = (int)row[5];
            ts.availableSeats = (int)row[6];
            
            segments.push_back(ts);
        }

        std::cout << "Successfully loaded " << stations.size() << " stations and " 
                  << segments.size() << " segments into RAM." << std::endl;

    } catch (const mysqlx::Error &err) {
        std::cerr << "Fetch Error: " << err.what() << std::endl;
    }
}

void DatabaseManager::logBookingToDB(const BookingRequest& request) {
    try {
        // Modern X DevAPI method chaining for parameter binding
        session->sql("INSERT INTO Bookings (booking_id, train_id, seats_booked, booking_timestamp) VALUES (?, ?, ?, ?)")
            .bind(request.bookingId)
            .bind(request.trainId)
            .bind(request.seatsRequested)
            .bind(request.bookingTimestamp)
            .execute();
            
    } catch (const mysqlx::Error &err) {
        std::cerr << "Database Logging Error: " << err.what() << std::endl;
    }
}