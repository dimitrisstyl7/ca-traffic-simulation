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
 * @param process_data TODO: fill
 * @param gapTags TODO: fill
 * @return 0 if successful, nonzero otherwise
 */
int Vehicle::updateGaps(Road *road_ptr, const ProcessData &process_data, const int gapTags[]) {
    const int j = process_data.getRank(); // current process rank
    const int lane_size = this->lane_ptr->getSize();
    const int this_vehicle_global_pos = lane_size * j + this->position;

    /* Array that stores the global position of the first and last vehicles for the current process lanes.
       int x1 => first vehicle's global position of current process lane 0
       int x2 => first vehicle's global position of current process lane 1
       int y1 => last vehicle's global position of current process lane 0
       int y2 => last vehicle's global position of current process lane 1

       z ∈ { x1, y1, x2, y2 }

       int global_position => lane_size * j + z

       if global_position == -1, means that no vehicle exists in lane 0.
       if global_position != -1, contains the vehicle's global position.

       int global_positions[] = { x1, x2, y1, y2 } */
    const int x1 = road_ptr->getLane(0)->findPosOfFirstVehicle(this->position);
    const int x2 = road_ptr->getLane(1)->findPosOfFirstVehicle(this->position);
    const int y1 = road_ptr->getLane(0)->findPosOfLastVehicle(this->position);
    const int y2 = road_ptr->getLane(1)->findPosOfLastVehicle(this->position);
    const int global_positions[] = {
        x1 != -1 ? lane_size * j + x1 : -1,
        x2 != -1 ? lane_size * j + x2 : -1,
        y1 != -1 ? lane_size * j + y1 : -1,
        y2 != -1 ? lane_size * j + y2 : -1
    };


    int next_vehicles_pos[] = {-1, -1}, prev_vehicles_pos[] = {-1, -1};

    if (process_data.getSize() > 1) {
        const int current_rank = process_data.getRank();
        const int next_rank = current_rank + 1;
        const int prev_rank = current_rank - 1;

        // Send the first and last vehicles for each lane, to the next and previous processes

        // Send first two positions (x1, x2) to the next process
        if (next_rank < process_data.getSize()) {
            MPI_Send(global_positions, 2, MPI_INT, next_rank, gapTags[0], MPI_COMM_WORLD);
        }

        // Send last two positions (y1, y2) to the previous process
        if (prev_rank >= 0) {
            MPI_Send(&global_positions[2], 2, MPI_INT, prev_rank, gapTags[1], MPI_COMM_WORLD);
        }

        // Receive the first and last vehicles global position for each lane, from the next and previous processes

        if (current_rank == 0) {
            int next_rank_sent_data;
            MPI_Iprobe(next_rank, gapTags[1], MPI_COMM_WORLD, &next_rank_sent_data, MPI_STATUS_IGNORE);

            if (next_rank_sent_data) {
                MPI_Recv(next_vehicles_pos, 2, MPI_INT, next_rank, gapTags[1], MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }
        } else if (current_rank == process_data.getSize()-1) {
            int prev_rank_sent_data;
            MPI_Iprobe(prev_rank, gapTags[0], MPI_COMM_WORLD, &prev_rank_sent_data, MPI_STATUS_IGNORE);

            if (prev_rank_sent_data) {
                MPI_Recv(prev_vehicles_pos, 2, MPI_INT, prev_rank, gapTags[0], MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }
        } else {
            // if 0 < current_rank < process_data.getSize()-1
            int next_rank_sent_data, prev_rank_sent_data;

            MPI_Iprobe(next_rank, gapTags[1], MPI_COMM_WORLD, &next_rank_sent_data, MPI_STATUS_IGNORE);
            MPI_Iprobe(prev_rank, gapTags[0], MPI_COMM_WORLD, &prev_rank_sent_data, MPI_STATUS_IGNORE);

            if (next_rank_sent_data) {
                MPI_Recv(next_vehicles_pos, 2, MPI_INT, next_rank, gapTags[1], MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }

            if (prev_rank_sent_data) {
                MPI_Recv(prev_vehicles_pos, 2, MPI_INT, prev_rank, gapTags[0], MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }
        }
    }

    // Locate the preceding vehicle and update the forward gap
    this->gap_forward = lane_size - 1;
    for (int i = this->position + 1; i < lane_size; i++) {
        if (this->lane_ptr->hasVehicleInSite(i)) {
            this->gap_forward = i - this->position - 1;
            break;
        }

        // If the number of the process equals 1, don't execute the code below
        if (process_data.getSize() == 1) continue;

        // If code execution reach this point, the gap will be calculated between this process and the next process.
        // Assign upper limit, in case a preceding vehicle doesn't exist (next_vehicle_position == -1).
        const int next_vehicle_pos = next_vehicles_pos[this->getLaneNumber()];
        this->gap_forward = next_vehicle_pos != -1 ? next_vehicle_pos - this_vehicle_global_pos - 1 : 2*(lane_size - 1);

        /*std::cout << "process " << process_data.getRank() << ": this->gap_forward=" << this->gap_forward << std::endl;
        // TODO: remove*/
    }

    // Update vehicle look forward distances
    this->look_forward = this->speed + 1;
    this->look_other_forward = this->look_forward;

    // Determine the other lane of interest
    const Lane *other_lane_ptr = this->lane_ptr->getLaneNumber() == 0
                                     ? road_ptr->getLanes()[1]
                                     : road_ptr->getLanes()[0];

    const int other_lane_number = other_lane_ptr->getLaneNumber(); // other lane number

    // Update the forward gap in the other lane
    this->gap_other_forward = lane_size - 1;
    for (int i = this->position; i < lane_size; i++) {
        if (other_lane_ptr->hasVehicleInSite(i)) {
            this->gap_other_forward = i - this->position - 1;
            break;
        }

        // If the number of the process equals 1 or the process is the last one, don't execute the code below
        if (process_data.getSize() == 1) continue;

        // If code execution reach this point, the gap will be calculated between this process and the next process.
        // Assign upper limit, in case a preceding vehicle doesn't exist (next_vehicle_position == -1).
        const int next_vehicle_pos = next_vehicles_pos[other_lane_number];
        this->gap_other_forward = next_vehicle_pos != -1 ? next_vehicle_pos - this_vehicle_global_pos - 1 : 2*(lane_size - 1);

        /*std::cout << "process " << process_data.getRank() << ": this->gap_other_forward=" << this->gap_other_forward << std::endl;
        // TODO: remove*/
    }

    // Update the backward gap in the other lane
    this->gap_other_backward = lane_size - 1;
    for (int i = this->position; i >= 0; i--) {
        if (other_lane_ptr->hasVehicleInSite(i)) {
            this->gap_other_backward = this->position - i - 1;
            break;
        }

        // If the number of the process equals 1, don't execute the code below
        if (process_data.getSize() == 1) continue;

        // If code execution reach this point, the gap will be calculated between this process and the next process.
        // Assign upper limit, in case a preceding vehicle doesn't exist (next_vehicle_position == -1).
        const int prev_vehicle_pos = prev_vehicles_pos[this->getLaneNumber()];
        this->gap_other_backward = prev_vehicle_pos != -1 ? this_vehicle_global_pos - prev_vehicle_pos - 1 : 2*(lane_size - 1);

        /*std::cout << "process " << process_data.getRank() << ": this->gap_other_backward=" << this->gap_other_backward << std::endl;
        // TODO: remove*/
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

    if (process_data.getRank() == 1) { // TODO: remove
        for (int i=0; i<lane_ptr->getSize(); i++)
            std::cout <<"process 1: " << "lanes[0].sites["<<i<<"].size="<<lane_ptr->sites[i].size() << std::endl;
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
                // Serialize vehicle
                const VehicleData vehicle_data = Serialization::serialize(*this);
                const int receiver = process_data.getRank() + 1;
                const int tag = send_tags.front();

                // Update the `last_recv_tag_id` only if current rank is 0 (first process)
                if (process_data.getRank() == 0) last_recv_tag_id = tag;

                MPI_Send(&vehicle_data, 1, Serialization::getInstance().getMPIVehicleDataType(), receiver, tag,
                         MPI_COMM_WORLD);
                send_tags.pop_front();
                std::cout << "process " << process_data.getRank() << ": MPI_Send to process " << process_data.
                    getRank() + 1 << " with send tag " << tag << "\n"; // TODO: remove
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

// TODO: Add docstrings
Lane* Vehicle::getLane() const {
    return this->lane_ptr;
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
