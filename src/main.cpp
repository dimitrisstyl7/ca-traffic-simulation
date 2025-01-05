/*
 * Copyright (C) 2019 Maitreya Venkataswamy - All Rights Reserved
 */

#include <iostream>

#include "mpi/mpi.h"
#include "Inputs.h"
#include "ProcessData.h"
#include "Simulation.h"
#include "Serialization.h"

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

    // If number of process are more than 2, create MPIVehicleDataType
    if (size > 1) Serialization::getInstance().createMPIVehicleDataType();

    // Create an Inputs object to contain the simulation parameters
    if (inputs.loadFromFile() != 0) {
        return 1;
    }

    const auto process_data = ProcessData(rank, size);

    // Create a Simulation object for the current simulation
    auto *simulation_ptr = new Simulation(inputs, process_data);

    // Wait previous process to send the first vehicle
    int run_simulation;
    if (const int sender = process_data.getRank() - 1; sender >= 0) {
        std::cout << "process " << sender + 1 << ": waiting for run simulation\n\n"; // TODO: Remove
        MPI_Recv(&run_simulation, 1, MPI_INT, sender, 0, MPI_COMM_WORLD, MPI_STATUSES_IGNORE);
    }

    // TODO: Add description
    const Road *road = simulation_ptr->getRoad();

    // Run the Simulation
    std::cout << "process " << rank << ": start run simulation\n\n"; // TODO: Remove
    simulation_ptr->run_simulation(process_data, *road);

    // Delete the Simulation object
    delete simulation_ptr;

    // TODO: Remove
    std::cout << "\n\n-------------------------- PROCESS " << process_data.getRank() << " BARRIER\n\n";

    MPI_Barrier(MPI_COMM_WORLD);

    // TODO: Remove
    std::cout << "\n\n-------------------------- PROCESS " << process_data.getRank() << " FINALIZE\n\n";

    // If number of process are more than 2, deallocate the resources assigned to MPIVehicleDataType
    if (size > 1) Serialization::getInstance().freeMPIVehicleDataType();

    // Finalize the MPI environment
    MPI_Finalize();

    // TODO: Remove
    std::cout << "\n\n-------------------------- PROCESS " << process_data.getRank() << " FINISHED\n\n";

    // Return with no errors
    return 0;
}
