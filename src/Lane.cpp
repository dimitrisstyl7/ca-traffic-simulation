/*
 * Copyright (C) 2019 Maitreya Venkataswamy - All Rights Reserved
 */

#include <cstdlib>
#include <sstream>
#include <iomanip>

#include "Lane.h"
#include "Vehicle.h"
#include "Inputs.h"

/**
 * Constructor for the Lane class
 * @param inputs instance of the Inputs class with simulation inputs
 * @param lane_num the number of lane in the road, starting with zero as the first lane
 * @param process_data Contains the rank and size of the MPI process, represented
 *                     by an instance of the `ProcessData` class. This is used to
 *                     manage distributed simulation across multiple processes.
 */
Lane::Lane(const Inputs &inputs, const int lane_num, const ProcessData &process_data) {
#ifdef DEBUG
    std::cout << "creating lane " << lane_num << "...";
#endif
    // Allocate memory for the vehicle pointers list

    // Remainder when dividing the total sites among processes
    const int remainder = inputs.length % process_data.getSize();

    // Number of sites assigned to each process
    const int n_of_elements = inputs.length / process_data.getSize();

    // If this is the last process, allocate additional sites to account for the remainder
    if (process_data.getRank() == process_data.getSize() - 1) {
        this->sites.reserve(n_of_elements + remainder); // Reserve memory for the base number plus the remainder
        this->sites.resize(n_of_elements + remainder); // Resize to include the remainder
    } else {
        // For other processes, allocate memory for the base number of sites
        this->sites.reserve(n_of_elements); // Reserve memory for the base number of sites
        this->sites.resize(n_of_elements); // Resize to the base number of sites
    }

    // Set the lane number for the lane
    this->lane_num = lane_num;
#ifdef DEBUG
    std::cout << "done, lane " << lane_num << " created with length " << this->sites.size() << std::endl;
#endif

    this->steps_to_spawn = 0;
}

/**
 * Getter method for the number of sites in the Lane
 * @return number of sites in the Lane
 */
int Lane::getSize() const {
    return static_cast<int>(this->sites.size());
}

/**
 * Getter method for the Lane's number
 * @return the number of the Lane
 */
int Lane::getLaneNumber() const {
    return this->lane_num;
}

/**
 * Checks if the Lane has a Vehicle in a specific site
 * @param site the site in which to check for a Vehicle
 * @return whether or not the Lane has a Vehicle in the site
 */
bool Lane::hasVehicleInSite(const int site) const {
    return !this->sites[site].empty();
}

/**
 * Adds a Vehicle to a site in the Lane
 * @param site which site to add the Vehicle to
 * @param vehicle_ptr pointer to the Vehicle to add to the site
 * @return 0 if successful, nonzero otherwise
 */
int Lane::addVehicle(const int site, Vehicle *vehicle_ptr) {
    // Place the Vehicle in the site
    this->sites[site].push_back(vehicle_ptr);

    // Return with zero errors
    return 0;
}

/**
 * Removes a Vehicle from a site in the Lane
 * @param site which site to remove the Vehicle from
 * @return 0 if successful, nonzero otherwise
 */
int Lane::removeVehicle(const int site) {
    // Remove the Vehicle from the site
    this->sites[site].pop_front();

    // Return with zero errors
    return 0;
}

/**
 * Attempts to spawn a Vehicle that has entered the Lane at the first site. Uses a CDF to sample to determine whether
 * or not a Vehicle was spawned.
 * @param inputs instance of the Inputs class with the simulation inputs
 * @param vehicles pointer to list of Vehicles to add the spawned Vehicles to
 * @param next_id_ptr pointer to the id number of the next spawned Vehicle
 * @param interarrival_time_cdf CDF of the Vehicle interarrival times
 * @return
 */
int Lane::attemptSpawn(const Inputs &inputs, std::vector<Vehicle *> *vehicles, int *next_id_ptr,
                       const CDF *interarrival_time_cdf) {
    if (this->steps_to_spawn == 0) {
        if (!this->hasVehicleInSite(0)) {
            // Spawn Vehicle
#ifdef DEBUG
            std::cout << "creating vehicle " << (*next_id_ptr) << " in lane " << this->lane_num << " at site " << 0
                    << std::endl;
#endif
            this->sites[0].push_front(new Vehicle(this, (*next_id_ptr)++, 0, inputs));
            vehicles->push_back(this->sites[0].front());

            // Randomly choose the Vehicles initial speed to be zero bases in slow down probability
            if (static_cast<double>(std::rand()) / static_cast<double>(RAND_MAX) < inputs.prob_slow_down) {
                vehicles->back()->setSpeed(0);
            }

            // "Schedule" next Vehicle spawn
            this->steps_to_spawn = static_cast<int>(interarrival_time_cdf->query() / inputs.step_size);
        }
    } else {
        this->steps_to_spawn--;
    }

    // Return with no error
    return 0;
}

/**
 * Debug function to print the Lane to visualize the sites
 */
#ifdef DEBUG
void Lane::printLane() const {
    std::ostringstream lane_string_stream;
    for (const auto &site: this->sites) {
        if (site.empty()) {
            lane_string_stream << "[   ]";
        } else {
            lane_string_stream << "[" << std::setw(3) << site.front()->getId() << "]";
        }
    }
    std::cout << lane_string_stream.str() << std::endl;
}
#endif
