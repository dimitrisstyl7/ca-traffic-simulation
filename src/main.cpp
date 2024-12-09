/*
 * Copyright (C) 2019 Maitreya Venkataswamy - All Rights Reserved
 */

#include <iostream>

#include "mpi/mpi.h"
#include "Inputs.h"
#include "ProcessData.h"
#include "Simulation.h"

/**
 * Main point of execution of the program
 * @param argc number of command line arguments
 * @param argv command line arguments
 * @return 0 if successful, nonzero otherwise
 */
int main(int argc, char **argv) {
#ifndef DEBUG
    srand(time(nullptr));
#endif

    int rank, size;
    auto inputs = Inputs();

    // Initialize the MPI environment and obtain the rank and size of the process
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (rank == 0) {
        // Print the banner only once
        std::cout << "================================================" << std::endl;
        std::cout << "||    CELLULAR AUTOMATA TRAFFIC SIMULATION    ||" << std::endl;
        std::cout << "================================================" << std::endl;
    }

    // Create an Inputs object to contain the simulation parameters
    if (inputs.loadFromFile() != 0) {
        return 1;
    }

    // Create a Simulation object for the current simulation
    auto *simulation_ptr = new Simulation(inputs, ProcessData(rank, size));

    // Run the Simulation
    simulation_ptr->run_simulation();

    // Delete the Simulation object
    delete simulation_ptr;

    // Finalize the MPI environment
    MPI_Finalize();

    // Return with no errors
    return 0;
}
