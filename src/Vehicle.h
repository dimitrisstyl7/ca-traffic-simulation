/*
 * Copyright (C) 2019 Maitreya Venkataswamy - All Rights Reserved
 */

#ifndef CA_TRAFFIC_SIMULATION_VEHICLE_H
#define CA_TRAFFIC_SIMULATION_VEHICLE_H

#include <list>

#include "Inputs.h"
#include "Road.h"

// Forward declarations
class Lane;

/**
 * Constructor for a Vehicle in the simulation. Has methods for performing movements based on the CA rules of the
 * simulation.
 */
class Vehicle {
    Lane *lane_ptr{};
    int id{};
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

public:
    Vehicle(Lane *lane_ptr, int id, int initial_position, const Inputs &inputs);

    Vehicle() = default;

    ~Vehicle() = default;

    int updateGaps(Road *road_ptr);

    int performLaneSwitch(Road *road_ptr);

    int performLaneMove(const ProcessData &process_data, std::list<int> &send_tags, int &last_recv_tag_id);

    [[nodiscard]] int getId() const;

    [[nodiscard]] double getTravelTime(const Inputs &inputs) const;

    [[nodiscard]] int getLaneNumber() const;

    [[nodiscard]] int getPosition() const;

    [[nodiscard]] int getSpeed() const;

    [[nodiscard]] int getMaxSpeed() const;

    [[nodiscard]] int getGapForward() const;

    [[nodiscard]] int getGapOtherForward() const;

    [[nodiscard]] int getGapOtherBackward() const;

    [[nodiscard]] int getLookForward() const;

    [[nodiscard]] int getLookOtherForward() const;

    [[nodiscard]] int getLookOtherBackward() const;

    [[nodiscard]] double getProbSlowDown() const;

    [[nodiscard]] double getProbChange() const;

    [[nodiscard]] int getTimeOnRoad() const;

    void setId(int id);

    [[nodiscard]] Vehicle *setLaneNumber(int lane_number);

    void setLaneNumber(Lane *lane_ptr);

    [[nodiscard]] Vehicle *setPosition(int position);

    Vehicle *setSpeed(int speed);

    [[nodiscard]] Vehicle *setMaxSpeed(int max_speed);

    [[nodiscard]] Vehicle *setGapForward(int gap_forward);

    [[nodiscard]] Vehicle *setGapOtherForward(int gap_other_forward);

    [[nodiscard]] Vehicle *setGapOtherBackward(int gap_other_backward);

    [[nodiscard]] Vehicle *setLookForward(int look_forward);

    [[nodiscard]] Vehicle *setLookOtherForward(int look_other_forward);

    [[nodiscard]] Vehicle *setLookOtherBackward(int look_other_backward);

    [[nodiscard]] Vehicle *setProbSlowDown(double prob_slow_down);

    [[nodiscard]] Vehicle *setProbChange(double prob_change);

    [[nodiscard]] Vehicle *setTimeOnRoad(int time_on_road);

#ifdef DEBUG
    void printGaps() const;
#endif
};


#endif //CA_TRAFFIC_SIMULATION_VEHICLE_H
