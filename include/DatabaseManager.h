#pragma once
#include "Models.h"
#include <vector>
#include <unordered_map>
#include <memory>
#include <mysqlx/xdevapi.h>

class DatabaseManager {
private:
    std::unique_ptr<mysqlx::Session> session;

public:
    DatabaseManager();
    ~DatabaseManager() = default;

    // The massive startup fetch: Pulls all stations and segments into C++ RAM
    void loadGraphData(std::unordered_map<std::string, Station>& stations, 
                       std::vector<TrainSegment>& segments);

    // Called asynchronously by the DB Logger Thread to save confirmed tickets
    void logBookingToDB(const BookingRequest& request);
};