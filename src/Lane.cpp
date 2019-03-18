/*
 * Copyright (C) 2019 Maitreya Venkataswamy - All Rights Reserved
 */

#include <cstdlib>

#include "Lane.h"
#include "Vehicle.h"

Lane::Lane(unsigned int size, unsigned int lane_num) {
    // Allocate memory in for the vehicle pointers
    this->sites.reserve(size);

    // Initialize each Vehicle pointer in the Lane to a null pointer
    for (int i = 0; i < this->sites.size(); i++) {
        this->sites[i] = nullptr;
    }

    // Set the lane number for the lane
    this->lane_num = lane_num;
}

int Lane::initializeCars(double percent_full, unsigned int max_speed, std::vector<Vehicle*>* vehicles,
                         unsigned int look_forward, unsigned int look_other_forward,
                         unsigned int look_other_backward) {
    // Initialize cars in the sites of the Lane with given percentage
    for (int i = 0; i < this->sites.size(); i++) {
        if (std::rand() / RAND_MAX <= 1.0) {
            this->sites[i] = new Vehicle(this, i, max_speed, look_forward, look_other_forward, look_other_backward);
            vehicles->push_back(this->sites[i]);
        }
    }

    // Return with no errors
    return 0;
}

unsigned int Lane::getSize() {
    return this->sites.size();
}
bool Lane::hasVehicleInSite(unsigned int site) {
    if (this->sites[site]) {
        return true;
    } else {
        return false;
    }
}