/*
 * Copyright (C) 2019 Maitreya Venkataswamy - All Rights Reserved
 */

#include "Road.h"
#include "Inputs.h"
#include "ProcessData.h"

/**
 * Constructor for the Road
 * @param inputs instance of the Inputs class with simulation inputs
 * @param process_data Contains the rank and size of the MPI process, represented
 *                     by an instance of the `ProcessData` class. This is used to
 *                     manage distributed simulation across multiple processes.
 */
Road::Road(const Inputs &inputs, const ProcessData &process_data) {
#ifdef DEBUG
    std::cout << "creating new road with " << inputs.num_lanes << " lanes..." << std::endl;
#endif
    // Create the Lane objects for the Road
    for (int i = 0; i < inputs.num_lanes; i++) {
        this->lanes.push_back(new Lane(inputs, i, process_data));
    }
#ifdef DEBUG
    std::cout << "done creating road" << std::endl;
#endif

    this->interarrival_time_cdf = new CDF();
    if (const int status = this->interarrival_time_cdf->read_cdf("interarrival-cdf.dat"); status != 0) {
        throw std::exception();
    }
}

/**
 * Destructor of the Road
 */
Road::~Road() {
    // Delete the Lane objects of the Road
    for (const auto &lane: this->lanes) {
        delete lane;
    }
}

/**
 * Getter for the Lanes of the road
 * @return
 */
std::vector<Lane *> Road::getLanes() {
    return this->lanes;
}

/**
 * Attempts to spawn Vehicles on each Lane of the Road
 * @param inputs instance of the Inputs class with the simulation Inputs
 * @param vehicles pointer to the array of Vehicles that exist
 * @param next_id_ptr pointer to the id of the next spawned Vehicle
 * @return 0 if successful, nonzero otherwise
 */
int Road::attemptSpawn(const Inputs &inputs, std::vector<Vehicle *> *vehicles, int *next_id_ptr) const {
    for (const auto lane: this->lanes) {
        lane->attemptSpawn(inputs, vehicles, next_id_ptr, this->interarrival_time_cdf);
    }

    // Return with no errors
    return 0;
}

/**
 * Debug function to print all the Lanes of the Road for visualizing the sites in the Road
 */
#ifdef DEBUG
void Road::printRoad() const {
    for (int i = static_cast<int>(this->lanes.size() - 1); i >= 0; i--) {
        this->lanes[i]->printLane();
    }
}
#endif
