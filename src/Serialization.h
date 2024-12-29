//
// Created by dimit on 12/12/24.
//

#ifndef SERIALIZE_H
#define SERIALIZE_H

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
public:
    [[nodiscard]] static VehicleData serialize(const Vehicle &vehicle);

    [[nodiscard]] static Vehicle *deserialize(const VehicleData &vehicle_data);
};

#endif //SERIALIZE_H
