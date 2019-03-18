/*
 * Copyright (C) 2019 Maitreya Venkataswamy - All Rights Reserved
 */

#ifndef CA_TRAFFIC_SIMULATION_LANE_H
#define CA_TRAFFIC_SIMULATION_LANE_H

#include <vector>

// Forward Declarations
class Vehicle;

class Lane {
private:
    unsigned int size;
    std::vector<Vehicle*> sites;
    unsigned int lane_num;
public:
    Lane(unsigned int size, unsigned int lane_num);
    int initializeCars(double percent_full, unsigned int max_speed, std::vector<Vehicle*>* vehicles);
};


#endif //CA_TRAFFIC_SIMULATION_LANE_H