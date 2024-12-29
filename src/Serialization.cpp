//
// Created by dimit on 12/12/24.
//

#include "Vehicle.h"
#include "Serialization.h"

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
