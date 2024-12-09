//
// Created by dimit on 12/8/24.
//

#ifndef PROCESSDATA_H
#define PROCESSDATA_H


/**
 * @class ProcessData
 * @brief Represents the rank and size of a process (MPI).
 *
 * This class is designed for use in MPI-based parallel or distributed
 * computing environments. The `rank` represents the unique identifier
 * for a process, and `size` represents the total number of processes
 * in the communicator.
 *
 * @param rank The rank of the process within the communicator. Each process
 *             in an MPI communicator is assigned a unique rank, starting from 0.
 * @param size The total number of processes in the MPI communicator. This defines
 *             the size of the group of processes that can communicate with each other.
 */
class ProcessData {
    int rank; ///< The rank of the process.
    int size; ///< The size of the process group.

public:
    /**
     * @brief Constructs a ProcessData object with a given rank and size.
     * @param rank The rank of the process within the communicator.
     * @param size The total number of processes in the communicator.
     */
    ProcessData(const int rank, const int size): rank(rank), size(size) {
    }

    /**
     * @brief Retrieves the rank of the process.
     * @return The rank of the process.
     */
    [[nodiscard]] int getRank() const { return rank; }

    /**
     * @brief Retrieves the size of the process group.
     * @return The size of the process group.
     */
    [[nodiscard]] int getSize() const { return size; }
};

#endif //PROCESSDATA_H
