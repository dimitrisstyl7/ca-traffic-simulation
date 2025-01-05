//
// Created by dimit on 12/12/24.
//

#include "Vehicle.h"
#include "Serialization.h"

Serialization *Serialization::instance = nullptr;

// TODO: Add docstrings
void Serialization::createMPIVehicleDataType() {
    constexpr MPI_Datatype types[] = {
        MPI_INT, MPI_INT, MPI_INT, MPI_INT, MPI_INT, MPI_INT, MPI_INT,
        MPI_INT, MPI_INT, MPI_INT, MPI_DOUBLE, MPI_DOUBLE, MPI_INT
    };
    const int block_lengths[] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
    constexpr MPI_Aint offsets[] = {
        offsetof(VehicleData, lane_num),
        offsetof(VehicleData, position),
        offsetof(VehicleData, speed),
        offsetof(VehicleData, max_speed),
        offsetof(VehicleData, gap_forward),
        offsetof(VehicleData, gap_other_forward),
        offsetof(VehicleData, gap_other_backward),
        offsetof(VehicleData, look_forward),
        offsetof(VehicleData, look_other_forward),
        offsetof(VehicleData, look_other_backward),
        offsetof(VehicleData, prob_slow_down),
        offsetof(VehicleData, prob_change),
        offsetof(VehicleData, time_on_road)
    };
    MPI_Type_create_struct(13, block_lengths, offsets, types, &MPIVehicleDataType);
    MPI_Type_commit(&MPIVehicleDataType);
}

// TODO: Add docstrings
void Serialization::freeMPIVehicleDataType() {
    MPI_Type_free(&MPIVehicleDataType);
}

MPI_Datatype Serialization::getMPIVehicleDataType() const {
    return MPIVehicleDataType;
}

// TODO: Add docstrings
VehicleData Serialization::serialize(const Vehicle &vehicle) {
    return VehicleData{
        vehicle.getLaneNumber(),
        vehicle.getPosition(),
        vehicle.getSpeed(),
        vehicle.getMaxSpeed(),
        vehicle.getGapForward(),
        vehicle.getGapOtherForward(),
        vehicle.getGapOtherBackward(),
        vehicle.getLookForward(),
        vehicle.getLookOtherForward(),
        vehicle.getLookOtherBackward(),
        vehicle.getProbSlowDown(),
        vehicle.getProbChange(),
        vehicle.getTimeOnRoad()
    };
}

// TODO: Add docstrings
Vehicle *Serialization::deserialize(const VehicleData &vehicle_data) {
    const auto vehicle = new Vehicle();
    return vehicle->setLaneNumber(vehicle_data.lane_num)
            ->setPosition(vehicle_data.position)
            ->setSpeed(vehicle_data.speed)
            ->setMaxSpeed(vehicle_data.max_speed)
            ->setGapForward(vehicle_data.gap_forward)
            ->setGapOtherForward(vehicle_data.gap_other_forward)
            ->setGapOtherBackward(vehicle_data.gap_other_backward)
            ->setLookOtherForward(vehicle_data.look_other_forward)
            ->setLookOtherBackward(vehicle_data.look_other_backward)
            ->setProbSlowDown(vehicle_data.prob_slow_down)
            ->setProbChange(vehicle_data.prob_change)
            ->setTimeOnRoad(vehicle_data.time_on_road);
}
