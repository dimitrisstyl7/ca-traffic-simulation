/*
 * Copyright (C) 2019 Maitreya Venkataswamy - All Rights Reserved
 */

#include <cstdlib>
#include <iomanip>
#include <list>
#include <mpi/mpi.h>

#include "Vehicle.h"
#include "Lane.h"
#include "Road.h"
#include "Serialization.h"

int inform_receiver_to_run_simulation = 1;

/**
 * Constructor for the Vehicle
 * @param lane_ptr pointer to the Lane in which the Vehicle starts in
 * @param id unique ID number of the Vehicle
 * @param initial_position initial site number of the Vehicle in the Lane
 * @param inputs instance of the Inputs class with the simulation inputs
 */
Vehicle::Vehicle(Lane *lane_ptr, const int id, const int initial_position, const Inputs &inputs) {
    // Set the ID number of the Vehicle
    this->id = id;

    // Set the initial position of the Vehicle
    this->position = initial_position;

    // Set the Lane pointer to the pointer to the Lane that contains the Vehicle
    this->lane_ptr = lane_ptr;

    // Set the maximum speed of the Vehicle
    this->max_speed = inputs.max_speed;

    // Set the initial speed of the Vehicle to the maximum speed
    this->speed = this->max_speed;

    // Set the look forward distance of the Vehicle
    this->look_forward = this->speed + 1;

    // Set the other lane look forward distance of the Vehicle
    this->look_other_forward = this->look_forward;

    // Set the other lane look backward distance of the Vehicle
    this->look_other_backward = inputs.look_other_backward;

    // Set the slow down probability of the Vehicle
    this->prob_slow_down = inputs.prob_slow_down;

    // Set the lane change probability of the Vehicle
    this->prob_change = inputs.prob_change;

    // Initialize the time spend on the Road
    this->time_on_road = 0;
}

/**
 * Update the perceived gaps between the Vehicle and the surrounding Vehicles in the Road
 * @param road_ptr pointer to the Road that the Vehicle is in
 * @return 0 if successful, nonzero otherwise

 */
int Vehicle::updateGaps(Road *road_ptr) {
    // Locate the preceding Vehicle and update the forward gap
    this->gap_forward = this->lane_ptr->getSize() - 1;
    for (int i = this->position + 1; i < this->lane_ptr->getSize(); i++) {
        if (this->lane_ptr->hasVehicleInSite(i)) {
            this->gap_forward = i - this->position - 1;
            break;
        }
    }

    // Update vehicle look forward distances
    this->look_forward = this->speed + 1;
    this->look_other_forward = this->look_forward;

    // Determine the other lane of interest
    Lane *other_lane_ptr;
    if (this->lane_ptr->getLaneNumber() == 0) {
        other_lane_ptr = road_ptr->getLanes()[1];
    } else {
        other_lane_ptr = road_ptr->getLanes()[0];
    }

    // Update the forward gap in the other lane
    this->gap_other_forward = this->lane_ptr->getSize() - 1;
    for (int i = this->position; i < this->lane_ptr->getSize(); i++) {
        if (other_lane_ptr->hasVehicleInSite(i)) {
            this->gap_other_forward = i - this->position - 1;
            break;
        }
    }

    // Update the backward gap in the other lane
    this->gap_other_backward = this->lane_ptr->getSize() - 1;
    for (int i = this->position; i >= 0; i--) {
        if (other_lane_ptr->hasVehicleInSite(i)) {
            this->gap_other_backward = this->position - i - 1;
            break;
        }
    }

    // Return with zero errors
    return 0;
}

/**
 * Moved the Vehicle to the other Lane in the Road
 * @param road_ptr pointer to the Road in which the Vehicle is on
 * @return 0 if successful, nonzero otherwise
 */
int Vehicle::performLaneSwitch(Road *road_ptr) {
    // Evaluate if the Vehicle will change lanes and then perform the lane change
    if (this->gap_forward < this->look_forward &&
        this->gap_other_forward > this->look_other_forward &&
        this->gap_other_backward > this->look_other_backward &&
        static_cast<double>(rand()) / static_cast<double>(RAND_MAX) <= this->prob_change) {
        // Determine the lane that the Vehicle is switching to
        Lane *other_lane_ptr;
        if (this->lane_ptr->getLaneNumber() == 0) {
            other_lane_ptr = road_ptr->getLanes()[1];
        } else {
            other_lane_ptr = road_ptr->getLanes()[0];
        }

#ifdef DEBUG
        std::cout << "vehicle " << this->id << " switched lane " << this->lane_ptr->getLaneNumber() << " -> "
                << other_lane_ptr->getLaneNumber() << std::endl;
#endif

        // Copy the Vehicle pointer to the other Lane
        other_lane_ptr->addVehicle(this->position, this, true);

        // Remove the Vehicle pointer from the current Lane
        this->lane_ptr->removeVehicle(this->position);

        // Set the pointer to the Lane in the Vehicle to the new lane
        this->lane_ptr = other_lane_ptr;
    }

    // Return with zero errors
    return 0;
}

/**
 * Moves the Vehicle to the next site in the current Lane during the time-step based on the speed of the Vehicle
 * @param process_data Contains the rank and size of the MPI process, represented
 *                     by an instance of the `ProcessData` class. This is used to
 *                     manage distributed simulation across multiple processes.
 * @param send_tags // TODO: fill
 * @param last_recv_tag_id // TODO: fill
 * @return 0 if successful, nonzero otherwise
 */
int Vehicle::performLaneMove(const ProcessData &process_data, std::list<int> &send_tags, int &last_recv_tag_id) {
    // Increment the time on road counter
    this->time_on_road++;

    // Update Vehicle speed based on vehicle speed update rules
    if (this->speed != this->max_speed) {
        this->speed++;
        /*#ifdef DEBUG
                std::cout << "vehicle " << this->id << " increased speed " << this->speed - 1 << " -> " << this->speed
                        << std::endl;
        #endif*/
    }

    this->speed = std::min(this->speed, this->gap_forward);
    /*#ifdef DEBUG
        if (this->speed == 0) {
            std::cout << "vehicle " << this->id << " stopped behind preceding vehicle" << std::endl;
        }
    #endif*/

    if (this->speed > 0) {
        if (static_cast<double>(rand()) / static_cast<double>(RAND_MAX) <= this->prob_slow_down) {
            this->speed--;
            /*#ifdef DEBUG
                        std::cout << "vehicle " << this->id << " decreased speed " << this->speed + 1 << " -> " << this->speed
                                << std::endl;
            #endif*/
        }
    }

    if (this->speed > 0) {
        // Compute the new position of the vehicle
        const int new_position = (this->position + this->speed) % this->lane_ptr->getSize();

        // If the vehicle reached the end of the road, remove the Vehicle from the Lane and return the time on road
        if (this->position > new_position) {
            /*#ifdef DEBUG
                        std::cout << "vehicle " << this->id << " spent " << this->time_on_road << " steps on the road" << std::endl;
            #endif*/

            // Send vehicle to next process only if number of processes are more than 2 and current process is not the last one
            if (process_data.getSize() > 1 && process_data.getRank() < process_data.getSize() - 1) {
                const VehicleData vehicle_data = Serialization::serialize(*this);
                const int receiver = process_data.getRank() + 1;

                // Inform the next process that it should start running its own simulation
                if (inform_receiver_to_run_simulation) {
                    MPI_Send(&inform_receiver_to_run_simulation, 1, MPI_INT, receiver, 0, MPI_COMM_WORLD);
                    inform_receiver_to_run_simulation = 0;
                }

                const int tag = send_tags.front();

                // Update the `last_recv_tag_id` only if current rank is 0 (first process)
                if (process_data.getRank() == 0) last_recv_tag_id = tag;

                MPI_Send(&vehicle_data, sizeof(vehicle_data), MPI_BYTE, receiver, tag, MPI_COMM_WORLD);
                send_tags.pop_front();

                // std::cout << "process " << process_data.getRank() << ": MPI_Send to process " << process_data.
                //         getRank() + 1 << " with send tag " << tag << "\n"; // TODO: remove
            }

            // Remove vehicle from the Road
            this->lane_ptr->removeVehicle(this->position);

            // Return the time on the Road
            return this->time_on_road;
        }

        /*#ifdef DEBUG
                std::cout << "vehicle " << this->id << " moved " << this->position << " -> " << new_position << std::endl;
        #endif*/

        // Update Vehicle position in the Lane object sites
        this->lane_ptr->addVehicle(new_position, this, true);

        // Remove vehicle from the old site
        this->lane_ptr->removeVehicle(this->position);

        // Update the Vehicle position value
        this->position = new_position;
    }

    // Return with no errors
    return 0;
}

/**
 * Getter method for the ID number of the Vehicle
 * @return
 */
int Vehicle::getId() const {
    return this->id;
}

/**
 * Getter method for the total time the Vehicle has spent on the Road
 * @param inputs
 * @return
 */
double Vehicle::getTravelTime(const Inputs &inputs) const {
    return inputs.step_size * this->time_on_road;
}

/**
 * Debug method for printing the gap information of the Vehicle
 */
#ifdef DEBUG
void Vehicle::printGaps() const {
    std::cout << "vehicle " << std::setw(2) << this->id << " gaps, >:" << this->gap_forward << " ^>:"
            << this->gap_other_forward << " ^<:" << this->gap_other_backward << std::endl;
}
#endif

// TODO: Add docstrings
void Vehicle::setId(const int id) {
    this->id = id;
}

// TODO: Add docstrings
int Vehicle::getLaneNumber() const {
    return this->lane_ptr->getLaneNumber();
}

// TODO: Add docstrings
void Vehicle::setLaneNumber(Lane *lane_ptr) {
    this->lane_ptr = lane_ptr;
}


// TODO: Add docstrings
int Vehicle::getPosition() const {
    return this->position;
}

// TODO: Add docstrings
int Vehicle::getSpeed() const {
    return this->speed;
}

// TODO: Add docstrings
int Vehicle::getMaxSpeed() const {
    return this->max_speed;
}

// TODO: Add docstrings
int Vehicle::getGapForward() const {
    return this->gap_forward;
}

// TODO: Add docstrings
int Vehicle::getGapOtherForward() const {
    return this->gap_other_forward;
}

// TODO: Add docstrings
int Vehicle::getGapOtherBackward() const {
    return this->gap_other_backward;
}

// TODO: Add docstrings
int Vehicle::getLookForward() const {
    return look_forward;
}

// TODO: Add docstrings
int Vehicle::getLookOtherForward() const {
    return this->look_other_forward;
}

// TODO: Add docstrings
int Vehicle::getLookOtherBackward() const {
    return this->look_other_backward;
}

// TODO: Add docstrings
double Vehicle::getProbSlowDown() const {
    return this->prob_slow_down;
}

// TODO: Add docstrings
double Vehicle::getProbChange() const {
    return this->prob_change;
}

// TODO: Add docstrings
int Vehicle::getTimeOnRoad() const {
    return this->time_on_road;
}

// TODO: Add docstrings
Vehicle *Vehicle::setLaneNumber(const int lane_number) {
    this->lane_ptr = new Lane(lane_number);
    return this;
}

// TODO: Add docstrings
Vehicle *Vehicle::setPosition(const int position) {
    this->position = position;
    return this;
}

// TODO: Add docstrings
Vehicle *Vehicle::setSpeed(const int speed) {
    this->speed = speed;
    return this;
}

// TODO: Add docstrings
Vehicle *Vehicle::setMaxSpeed(const int max_speed) {
    this->max_speed = max_speed;
    return this;
}

// TODO: Add docstrings
Vehicle *Vehicle::setGapForward(const int gap_forward) {
    this->gap_forward = gap_forward;
    return this;
}

// TODO: Add docstrings
Vehicle *Vehicle::setGapOtherForward(const int gap_other_forward) {
    this->gap_other_forward = gap_other_forward;
    return this;
}

// TODO: Add docstrings
Vehicle *Vehicle::setGapOtherBackward(const int gap_other_backward) {
    this->gap_other_backward = gap_other_backward;
    return this;
}

// TODO: Add docstrings
Vehicle *Vehicle::setLookForward(const int look_forward) {
    this->look_forward = look_forward;
    return this;
}

// TODO: Add docstrings
Vehicle *Vehicle::setLookOtherForward(const int look_other_forward) {
    this->look_other_forward = look_other_forward;
    return this;
}

// TODO: Add docstrings
Vehicle *Vehicle::setLookOtherBackward(const int look_other_backward) {
    this->look_other_backward = look_other_backward;
    return this;
}

// TODO: Add docstrings
Vehicle *Vehicle::setProbSlowDown(const double prob_slow_down) {
    this->prob_slow_down = prob_slow_down;
    return this;
}

// TODO: Add docstrings
Vehicle *Vehicle::setProbChange(const double prob_change) {
    this->prob_change = prob_change;
    return this;
}

// TODO: Add docstrings
Vehicle *Vehicle::setTimeOnRoad(const int time_on_road) {
    this->time_on_road = time_on_road;
    return this;
}
