/*
 * Copyright (C) 2019 Maitreya Venkataswamy - All Rights Reserved
 */

#include <cmath>

#include "Statistic.h"

/**
 * Adds a sample to the statistic
 * @param value value of the sample
 */
void Statistic::addValue(const double value) {
    // Add the value to the vector of values
    this->values.push_back(value);
}

/**
 * Gets the average of all the samples in the Statistic
 * @return average of the samples in the Statistic
 */
double Statistic::getAverage() const {
    // Initialize a sum for the average
    double sum = 0.0;

    // Add each value in the vector of values to the sum
    for (double const value: this->values) {
        sum += value;
    }

    // Divide the sum by the number of points and return the average
    return sum / static_cast<double>(this->values.size());
}

/**
 * Gets the variance of all the samples in the Statistic
 * @return variance of the samples in the Statistic
 */
double Statistic::getVariance() const {
    // Obtain average
    double const avg = this->getAverage();

    // Initialize a sum for the variance
    double sum = 0.0;

    // Add each value in the vector of values to the sum
    for (const double value: this->values) {
        sum += pow(value - avg, 2);
    }

    // Divide the sum by the number of points minus 1 and return the variance
    return sum / (static_cast<double>(this->values.size()) - 1.0);
}

/**
 * Gets the number of samples that have been added to the Statistic
 * @return number of samples in the Statistic
 */
int Statistic::getNumSamples() const {
    return static_cast<int>(this->values.size());
}
