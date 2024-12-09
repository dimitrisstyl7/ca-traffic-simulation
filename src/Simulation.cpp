/*
 * Copyright (C) 2019 Maitreya Venkataswamy - All Rights Reserved
 */

#include <chrono>
#include <algorithm>
#include <cmath>

#include "Road.h"
#include "Simulation.h"

#include "ProcessData.h"
#include "Vehicle.h"

/**
 * Constructor for the Simulation
 * @param inputs The configuration and parameters for the simulation, encapsulated
 *               in an instance of the `Inputs` class. This includes details about
 *               the road, vehicles, and other simulation settings.
 * @param process_data Contains the rank and size of the MPI process, represented
 *                     by an instance of the `ProcessData` class. This is used to
 *                     manage distributed simulation across multiple processes.
 */
Simulation::Simulation(const Inputs &inputs, const ProcessData &process_data) {
    // Create the Road object for the simulation
    this->road_ptr = new Road(inputs, process_data);

    // Set the simulation time to zero
    this->time = 0;

    // Initialize the first Vehicle id
    this->next_id = 0;

    // Obtain the simulation inputs
    this->inputs = inputs;

    // Initialize Statistic for travel time
    this->travel_time = new Statistic();
}

/**
 * Destructor for the Simulation
 */
Simulation::~Simulation() {
    // Delete the Road object in the simulation
    delete this->road_ptr;

    // Delete all the Vehicle objects in the Simulation
    for (const auto &vehicle: this->vehicles) {
        delete vehicle;
    }
}

/**
 * Executes the simulation
 * @return 0 if successful, nonzero otherwise
 */
int Simulation::run_simulation() {
    // Obtain the start time
    const std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

    // Declare a vector for vehicles to be removed each step
    std::vector<int> vehicles_to_remove;

    while (this->time < this->inputs.max_time) {
#ifdef DEBUG
        std::cout << "road configuration at time " << time << ":" << std::endl;
        this->road_ptr->printRoad();
        std::cout << "performing lane switches..." << std::endl;
#endif

        // Perform the lane switch step for all vehicles
        for (const auto &vehicle: this->vehicles) {
            vehicle->updateGaps(this->road_ptr);
#ifdef DEBUG
            vehicle->printGaps();
#endif
        }

        for (const auto &vehicle: this->vehicles) {
            vehicle->performLaneSwitch(this->road_ptr);
        }

#ifdef DEBUG
        this->road_ptr->printRoad();
        std::cout << "performing lane movements..." << std::endl;
#endif

        // Perform the independent lane updates
        for (const auto &vehicle: this->vehicles) {
            vehicle->updateGaps(this->road_ptr);
#ifdef DEBUG
            vehicle->printGaps();
#endif
        }

        // TODO: Add parallel logic in performLaneMove
        for (int n = 0; n < static_cast<int>(this->vehicles.size()); n++) {
            if (const int time_on_road = this->vehicles[n]->performLaneMove(); time_on_road != 0) {
                vehicles_to_remove.push_back(n);
            }
        }

        // End of iteration steps
        // Increment time
        this->time++;

        // Remove finished vehicles
        std::sort(vehicles_to_remove.begin(), vehicles_to_remove.end());
        for (int i = static_cast<int>(vehicles_to_remove.size()) - 1; i >= 0; i--) {
            // Update travel time statistic if beyond warm-up period
            if (this->time > this->inputs.warmup_time) {
                this->travel_time->addValue(this->vehicles[vehicles_to_remove[i]]->getTravelTime(this->inputs));
            }

            // Delete the Vehicle
            // TODO: Add logic, when the vehicle should be removed
            delete this->vehicles[vehicles_to_remove[i]];
            this->vehicles.erase(this->vehicles.begin() + vehicles_to_remove[i]);
        }
        vehicles_to_remove.clear();

        // Spawn new Vehicles
        // TODO: Spawn should occurred only in first process
        this->road_ptr->attemptSpawn(this->inputs, &this->vehicles, &this->next_id);
    }

    // Print the total run time and average iterations per second and seconds per iteration
    const std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    const auto time_elapsed = static_cast<double>(std::chrono::duration_cast<std::chrono::microseconds>(end - begin).
                                  count()) / 1000000.0;
    std::cout << "--- Simulation Performance ---" << std::endl;
    std::cout << "total computation time: " << time_elapsed << " [s]" << std::endl;
    std::cout << "average time per iteration: " << time_elapsed / inputs.max_time << " [s]" << std::endl;
    std::cout << "average iterating frequency: " << inputs.max_time / time_elapsed << " [iter/s]" << std::endl;

#ifdef DEBUG
    // Print final road configuration
    std::cout << "final road configuration" << std::endl;
    this->road_ptr->printRoad();
#endif

    // Print the average Vehicle time on the Road
    std::cout << "--- Simulation Results ---" << std::endl;
    std::cout << "time on road: avg=" << this->travel_time->getAverage() << ", std="
            << pow(this->travel_time->getVariance(), 0.5) << ", N=" << this->travel_time->getNumSamples()
            << std::endl;

    // Return with no errors
    return 0;
}
