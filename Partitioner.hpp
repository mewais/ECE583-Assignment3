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
    // This data structure is what holds the "set" of things assigned
    // to each partition.
    typedef std::pair<std::vector<uint32_t>,std::vector<uint32_t>> Partitions;
    typedef std::pair<uint32_t, uint32_t> Coordinates;

    extern uint32_t NumThreads;
    extern LAYOUT::LayoutWidget* MainWindow;

    // every entry in this vector contains a vector of the node IDs
    // it's connected to.
    // This data structure is not a 2D array. The vector lengths are
    // variable.
    extern std::vector<std::vector<uint32_t>> ConnectedBlocks;

    extern std::vector<pthread_t> WorkerThreads;
    extern pthread_mutex_t GUIMutex;
    extern pthread_mutex_t BestMutex;
    extern pthread_mutex_t IOMutex;

    // This data structure holds the "starting points" of all threads
    extern std::vector<Partitions> ThreadStartPoints;
    extern Partitions BestSolution;
    extern uint32_t BestSolutionCost;
    extern uint32_t MaxPartitionSide;
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
    uint32_t calculateCost(Partitions& Targets);

    uint32_t getNumberOfBlocks();
}

#endif // PARTITIONER_HPP
