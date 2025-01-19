/*
 * Copyright (C) 2019 Maitreya Venkataswamy - All Rights Reserved
 */

#include <algorithm>
#include <chrono>
#include <cmath>
#include <list>
#include <mpi/mpi.h>

#include "Simulation.h"
#include "ProcessData.h"
#include "Road.h"
#include "Serialization.h"
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
 * @param process_data Contains the rank and size of the MPI process, represented
 *                     by an instance of the `ProcessData` class. This is used to
 *                     manage distributed simulation across multiple processes.
 * @param road // TODO: add description
 * @return 0 if successful, nonzero otherwise
 */
int Simulation::run_simulation(const ProcessData &process_data, const Road &road) {
    // Obtain the start time
    const std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

    // Declare a vector for vehicles to be removed each step
    std::vector<int> vehicles_to_remove;

    // Lists that contains the tags for sending and receiving a vehicle between processes
    std::list<int> send_tags, recv_tags;

    // `last_recv_tag_id` contains the last send tag id from process 0
    int last_recv_tag_id;

    // `process_0_finished` indicates if `this->time >= this->inputs.max_time` and the process 0 doesn't have any remaining vehicle
    int process_0_finished = 0;

    // `last_process_finished` indicates if the last process finished execution of run_simulation
    int last_process_finished = 0;

    // `bcast_completion_last_recv_tag` indicates if the `last_recv_tag_id` was broadcast
    int bcast_completion_last_recv_tag = 0;

    if (process_data.getSize() > 1) {
        // Determine the last receive tag id (upper limit)
        last_recv_tag_id = this->inputs.max_time;

        // Initialize the tags lists with the maximum possible number of vehicles that can be spawned
        for (int i = 1; i <= last_recv_tag_id; i++) {
            if (process_data.getRank() == 0) send_tags.push_back(i); // initialize send_tags for process 0
            else recv_tags.push_back(i); // initialize recv_tags for the other process
        }
    }

    do {
        if (process_data.getSize() > 1) {
            // If the last process finished execution, inform the other processes to exit run_simulation
            const int last_process = process_data.getSize()-1;
            if (process_data.getRank() == last_process && recv_tags.front() == last_recv_tag_id) last_process_finished = 1;
            MPI_Bcast(&last_process_finished, 1, MPI_INT, last_process, MPI_COMM_WORLD);
        }

        /*#ifdef DEBUG
                std::cout << "road configuration at time " << time << ":" << std::endl;
                this->road_ptr->printRoad();
                std::cout << "performing lane switches..." << std::endl;
        #endif*/

        // Execute receive vehicle logic only if current process rank is greater than 0 (ignore first process)
        if (process_data.getRank() > 0) {
            int new_vehicle;
            const int sender = process_data.getRank() - 1; // the process which may send a new vehicle
            const int recv_tag = recv_tags.front();
            VehicleData vehicle_data;

            // Check if previous process (sender) sent new vehicle
            MPI_Iprobe(sender, recv_tag, MPI_COMM_WORLD, &new_vehicle, MPI_STATUS_IGNORE);

            if (new_vehicle) {
                MPI_Recv(&vehicle_data, 1, Serialization::getInstance().getMPIVehicleDataType(),
                    sender, recv_tag, MPI_COMM_WORLD, MPI_STATUSES_IGNORE);

                send_tags.push_back(recv_tag);
                recv_tags.pop_front();

                // Deserialize vehicle_data
                Vehicle *vehicle = Serialization::deserialize(vehicle_data);
                Lane *lane = road.getLane(vehicle->getLaneNumber());
                vehicle->setLaneNumber(lane);
                vehicle->setId(this->next_id++);
                lane->addVehicle(0, vehicle, false);
                vehicles.push_back(vehicle);

                std::cout << "process " << process_data.getRank() << ": MPI_Recv from process " << process_data.
                        getRank() - 1 << " with tag " << recv_tag << "\n"; // TODO: remove
            }
        }

        // TODO: remove
        std::cout << "\nprocess " << process_data.getRank() << ": recv_tags.size()=" << recv_tags.size() <<
                ", vehicles.size()=" << vehicles.size() << std::endl;

        const int gapTags[] = {this->inputs.max_time+1, this->inputs.max_time+2};
        updateGaps(process_data, gapTags);

        // Perform the lane switch step for all vehicles
        for (const auto &vehicle: this->vehicles) {
            vehicle->performLaneSwitch(this->road_ptr);
        }

        /*#ifdef DEBUG
                this->road_ptr->printRoad();
                std::cout << "performing lane movements..." << std::endl;
        #endif*/

        updateGaps(process_data, gapTags);

        // Perform the independent lane updates
        for (int n = 0; n < static_cast<int>(this->vehicles.size()); n++) {
            if (const int time_on_road = this->vehicles[n]->performLaneMove(process_data, send_tags, last_recv_tag_id);
                time_on_road != 0) {
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
            delete this->vehicles[vehicles_to_remove[i]];
            this->vehicles.erase(this->vehicles.begin() + vehicles_to_remove[i]);
        }
        vehicles_to_remove.clear();

        // Spawn new vehicles
        if (process_data.getRank() == 0 && this->time < this->inputs.max_time) this->road_ptr->attemptSpawn(this->inputs, &this->vehicles, &this->next_id);

        // Broadcast the `process_0_finished` only if processes are more than 2, and the `last_recv_tag_id` was not yet broadcast.
        if (process_data.getSize() > 1 && !bcast_completion_last_recv_tag) {
            MPI_Bcast(&process_0_finished, 1, MPI_INT, 0, MPI_COMM_WORLD);
        }

        // Broadcast the `last_recv_tag_id` only if processes are more than 2, `process_0_finished == 1`, and the `last_recv_tag_id` was not yet broadcast.
        if (process_data.getSize() > 1 && process_0_finished && !bcast_completion_last_recv_tag) {
            MPI_Bcast(&last_recv_tag_id, 1, MPI_INT, 0, MPI_COMM_WORLD);
            bcast_completion_last_recv_tag = 1;
            std::cout << "process " << process_data.getRank() << ": LAST RECV TAG ID " << last_recv_tag_id <<
                    ", current recv tag " << recv_tags.front() << "\n"; // TODO: remove
        }

        if (process_data.getRank() == 0) process_0_finished = this->time >= this->inputs.max_time && vehicles.empty();

    } while (process_data.getSize() > 1 ? !last_process_finished : !process_0_finished);

    // Only last process should print the results
    if (process_data.getRank() == process_data.getSize() - 1) {
        // Print the total run time and average iterations per second and seconds per iteration
        const std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
        const auto time_elapsed = static_cast<double>(std::chrono::duration_cast<std::chrono::microseconds>(end - begin)
                                      .count()) / 1000000.0;
        std::cout << "--- Simulation Performance ---" << std::endl;
        std::cout << "total computation time: " << time_elapsed << " [s]" << std::endl;
        std::cout << "average time per iteration: " << time_elapsed / inputs.max_time << " [s]" << std::endl;
        std::cout << "average iterating frequency: " << inputs.max_time / time_elapsed << " [iter/s]" << std::endl;

        /*#ifdef DEBUG
                // Print final road configuration
                std::cout << "final road configuration" << std::endl;
                this->road_ptr->printRoad();
        #endif*/

        // Print the average Vehicle time on the Road
        std::cout << "--- Simulation Results ---" << std::endl;
        std::cout << "time on road: avg=" << this->travel_time->getAverage() << ", std="
                << pow(this->travel_time->getVariance(), 0.5) << ", N=" << this->travel_time->getNumSamples()
                << std::endl;
    }

    // TODO: Remove
    std::cout << "\n\n-------------------------- PROCESS " << process_data.getRank() << " RETURNING\n\n";

    // Return with no errors
    return 0;
}

// TODO: Add docstrings
Road *Simulation::getRoad() const {
    return this->road_ptr;
}

void Simulation::updateGaps(const ProcessData &process_data, const int gapTags[]) {
    for (const auto &vehicle: this->vehicles) {
        vehicle->updateGaps(this->road_ptr, process_data, gapTags);
        /*#ifdef DEBUG
                    vehicle->printGaps();
        #endif*/
    }
}