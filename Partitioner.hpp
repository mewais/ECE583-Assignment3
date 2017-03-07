#ifndef PARTITIONER_HPP
#define PARTITIONER_HPP

#include <vector>
#include <iostream>
#include <cmath>
#include <utility>
#include <tuple>
#include <pthread.h>
#include <algorithm>

#include "Layout.hpp"

namespace LAYOUT
{
    class LayoutWidget;
}

namespace PARTITIONER
{
    typedef std::pair<std::vector<uint32_t>,std::vector<uint32_t>> Partitions;
    typedef std::pair<uint32_t, uint32_t> Coordinates;
    typedef std::vector<uint32_t> Locations;

    extern uint32_t NumThreads;
    extern LAYOUT::LayoutWidget* MainWindow;

    // every entry in this vector contains a vector of the node IDs
    // it's connected to.
    // This data structure is not a 2D array. The vector lengths are
    // variable.
    extern std::vector<std::vector<uint32_t>> ConnectedBlocks;

    // Pthreads stuff
    extern std::vector<pthread_t> WorkerThreads;
    extern pthread_mutex_t GUIMutex;
    extern pthread_mutex_t BestMutex;
    extern pthread_mutex_t IOMutex;

    // This data structure holds the "starting points" of all threads, each
    // element in this vector holds the set of things assigned to each partition.
    extern std::vector<Partitions> ThreadStartPoints;
    // This data structure is just a "rewrite" of the one above, each element
    // in this vector holds an integer representing where this block lies, 0
    // means not yet assigned to partition, 1 means assigned to P1, 2 means
    // assigned to P2.
    // As mentioned above, this is just rewriting the one above, it just helps
    // me do thing faster.
    extern std::vector<Locations> ThreadLocations;

    extern Partitions BestSolution;
    extern uint32_t BestSolutionCost;
    extern uint32_t MaxPartitionSize;

    // This data structure holds the "starting points" of all threads,
    // It is similar to the one above except that this one holds X,Y
    // coordinates for the GUI.
    extern std::vector<Coordinates> GUIThreadStartPoints;

    void setNumberOfThreads(uint32_t Num, bool Verbose);
    void setNumberOfBlocks(uint32_t Num, bool Verbose);
    void attachGUI(LAYOUT::LayoutWidget *Window);
    void detachGUI();                   // Sorry
    void connectBlocks(uint32_t First, uint32_t Second, bool Verbose);

    // Initialize the actual multithreading stuff, see inside this
    // function for elaboration.
    void initPartitioning(bool Verbose);
    void startPartitioning(bool Verbose);
    void recursivePartitioning(uint32_t TID, Partitions& CurrentSolution, Coordinates& GUICurrentSolution, uint32_t Level, bool Verbose);
    void *recursivePartitioning(void *Args);
    uint32_t calculateCost(Partitions& Targets, Locations& TargetLocations);

    uint32_t getNumberOfBlocks();
}

#endif // PARTITIONER_HPP
