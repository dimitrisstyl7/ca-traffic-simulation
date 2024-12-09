/*
 * Copyright (C) 2019 Maitreya Venkataswamy - All Rights Reserved
 */

#ifndef CA_TRAFFIC_SIMULATION_LANE_H
#define CA_TRAFFIC_SIMULATION_LANE_H

#include <vector>
#include <deque>

#include "Inputs.h"
#include "CDF.h"
#include "ProcessData.h"

// Forward Declarations
class Vehicle;

/**
 * Class for a lane in the road of the simulation. Each lane contains the "sites" for the vehicles and allows access
 * to all the information about the vehicles on the road through its methods.
 */
class Lane {
    std::vector<std::deque<Vehicle *> > sites;
    int lane_num;
    int steps_to_spawn;

public:
    Lane(const Inputs &inputs, int lane_num, const ProcessData &process_data);

    [[nodiscard]] int getSize() const;

    [[nodiscard]] int getLaneNumber() const;

    [[nodiscard]] bool hasVehicleInSite(int site) const;

    int addVehicle(int site, Vehicle *vehicle_ptr);

    int removeVehicle(int site);

    int attemptSpawn(const Inputs &inputs, std::vector<Vehicle *> *vehicles, int *next_id_ptr,
                     const CDF *interarrival_time_cdf);
#ifdef DEBUG
    void printLane() const;
#endif
};


#endif //CA_TRAFFIC_SIMULATION_LANE_H
