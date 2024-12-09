/*
 * Copyright (C) 2019 Maitreya Venkataswamy - All Rights Reserved
 */

#ifndef CA_TRAFFIC_SIMULATION_STATISTIC_H
#define CA_TRAFFIC_SIMULATION_STATISTIC_H

#include <vector>

/**
 * Class for the statistics of a property of the simulation, like Vehicle travel time on the road. Has methods for
 * adding samples to the statistic, or getting mean and variance
 */
class Statistic {
    std::vector<double> values;

public:
    Statistic() = default;

    ~Statistic() = default;

    void addValue(double value);

    [[nodiscard]] double getAverage() const;

    [[nodiscard]] double getVariance() const;

    [[nodiscard]] int getNumSamples() const;
};


#endif //CA_TRAFFIC_SIMULATION_STATISTIC_H
