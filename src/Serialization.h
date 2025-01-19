//
// Created by dimit on 12/12/24.
//

#ifndef SERIALIZATION_H
#define SERIALIZATION_H

#include <mpi/mpi.h>
#include "Vehicle.h"

// TODO: Add docstrings
struct VehicleData {
    int lane_num{};
    int position{};
    int speed{};
    int max_speed{};
    int gap_forward{};
    int gap_other_forward{};
    int gap_other_backward{};
    int look_forward{};
    int look_other_forward{};
    int look_other_backward{};
    double prob_slow_down{};
    double prob_change{};
    int time_on_road{};
};

// TODO: Add docstrings
class Serialization {
    // Private static instance variable
    static Serialization *instance;

    MPI_Datatype MPIVehicleDataType;

    // Private constructor to prevent external instantiation
    Serialization() = default;

    // Private destructor to prevent external deletion
    ~Serialization() = default;

public:
    static Serialization &getInstance() {
        // If the instance doesn't exist, create it
        if (!instance) {
            instance = new Serialization();
        }
        return *instance;
    }

    // Delete the copy constructor
    Serialization(const Serialization &) = delete;

    // Delete the assignment operator
    Serialization &operator=(const Serialization &) = delete;

    void createMPIVehicleDataType();

    void freeMPIVehicleDataType();

    [[nodiscard]] MPI_Datatype getMPIVehicleDataType() const;

    [[nodiscard]] static VehicleData serialize(const Vehicle &vehicle);

    [[nodiscard]] static Vehicle *deserialize(const VehicleData &vehicle_data);
};

#endif //SERIALIZATION_H
