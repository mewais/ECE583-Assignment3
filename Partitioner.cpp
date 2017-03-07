#include "Partitioner.hpp"

namespace PARTITIONER
{
    uint32_t NumThreads;
    LAYOUT::LayoutWidget* MainWindow;

    std::vector<std::vector<uint32_t>> ConnectedBlocks;

    std::vector<pthread_t> WorkerThreads;
    pthread_mutex_t GUIMutex;
    pthread_mutex_t BestMutex;
    pthread_mutex_t IOMutex;
    std::vector<Partitions> ThreadStartPoints;
    std::vector<Locations> ThreadLocations;
    Partitions BestSolution;
    uint32_t BestSolutionCost = UINT32_MAX;
    uint32_t MaxPartitionSize;
    std::vector<Coordinates> GUIThreadStartPoints;

    void setNumberOfThreads(uint32_t Num, bool Verbose)
    {
        NumThreads = Num;
        // Number of threads has to be a power of 2
        // A power of two is of this form 0001000, we shift and check if
        // this is the form we have indeed.
        bool Count = false;
        for (uint32_t i = 0; i < 32; i++)
        {
            if (Num & 1)
            {
                // If we already found a 1, this is not a power of 2.
                if (Count)
                {
                    std::cout << "EPartitioner: Number of threads has to be a power of 2\n";
                    exit(0);
                }
                else
                {
                    Count = true;
                }
            }
            Num = Num >> 1;
        }

        if (Verbose)
            std::cout << "\tEPartitioner: Initializing " << NumThreads << " worker threads\n";

        // Initializing stuff.
        WorkerThreads.resize(NumThreads);
        ThreadStartPoints.resize(NumThreads);
        ThreadLocations.resize(NumThreads);
        for (uint32_t i = 0; i < NumThreads; i++)
        {
            ThreadLocations[i].resize(ConnectedBlocks.size());
        }
        pthread_mutex_init(&GUIMutex, NULL);
        pthread_mutex_init(&IOMutex, NULL);
        pthread_mutex_init(&BestMutex, NULL);
    }

    void setNumberOfBlocks(uint32_t Num, bool Verbose)
    {
        ConnectedBlocks.resize(Num);
        // The maximum number of blocks per partition is exactly the half, or
        // ceil the half if the number of blocks is odd.
        MaxPartitionSize = Num/2;
        if (MaxPartitionSize*2 != Num)          // Number of blocks was odd
            MaxPartitionSize++;
        if (Verbose)
            std::cout << "\tEPartitioner: Initializing " << Num << " blocks\n";
    }

    void connectBlocks(uint32_t First, uint32_t Second, bool Verbose)
    {
        ConnectedBlocks[First].push_back(Second);
        ConnectedBlocks[Second].push_back(First);

        if(Verbose)
            std::cout << "\tEPartitioner: Connecteing Blocks " << First << " and " << Second << ".\n";
    }

    void attachGUI(LAYOUT::LayoutWidget *Window)
    {
        MainWindow = Window;
        for (uint32_t i = 0; i < NumThreads; i++)
            GUIThreadStartPoints.push_back(MainWindow->getOriginalStartPoint());
    }

    // This is stupid, but GUIThreadStartPoints is passed through some functions
    // and neads to be initialized even without a GUI.
    void detachGUI()
    {
        GUIThreadStartPoints.resize(NumThreads);
    }

    // My multithreading works as follows: It visits ONLY the first depths of
    // the tree until it reaches a number of nodes equal to the number of
    // threads. Every thread the can go through the tree in the usual way.
    // EXAMPLE: if you use 4 threads, you need to visit two depth levels in
    // the tree, then you will have 4 nodes and each thread can start from
    // one of those nodes.
    void initPartitioning(bool Verbose)
    {
        for (uint32_t i = 0; i < NumThreads; i++)
        {
            // This first element is always in the first partition
            // NOTE: this is not shown in the GUI.
            std::get<0>(ThreadStartPoints[i]).push_back(0);
            ThreadLocations[i][0] = 1;

            // Since It's not good to use recursion here, or maybe it is but
            // it's not really worth it. And since the number of threads is
            // usually low, we can do a simple trick:
            // Use a number of bits equal to the log 2 of number of threads,
            // each thread will take a different combination of those bits
            // and assign stuff to either partition depending on the bits.
            // Example: a thread getting the bit sequence 1010 will asiign
            // the first element to Right partition, the second to left,
            // and so on.
            uint32_t CopyI = i;
            for (uint32_t j = 0; j < std::log2(NumThreads); j++)
            {
                // Now assign the start point of each thread based on the bits
                if(CopyI & 1)
                {
                    std::get<1>(ThreadStartPoints[i]).push_back(j+1);
                    ThreadLocations[i][j+1] = 2;
                    if (MainWindow)
                    {
                        GUIThreadStartPoints[i] = MainWindow->paintTreeBranch(GUIThreadStartPoints[i], j+1, true, Verbose);
                        MainWindow->update();
                    }
                }
                else
                {
                    std::get<0>(ThreadStartPoints[i]).push_back(j+1);
                    ThreadLocations[i][j+1] = 1;
                    if (MainWindow)
                    {
                        GUIThreadStartPoints[i] = MainWindow->paintTreeBranch(GUIThreadStartPoints[i], j+1, false, Verbose);
                        MainWindow->update();
                    }
                }
                CopyI = CopyI >> 1;
            }
            if (Verbose)
            {
                std::cout << "\tEPartitioner: Thread " << i << " will start with:\n";
                std::cout << "\t\tPartition1: ";
                for (uint32_t j = 0; j < std::get<0>(ThreadStartPoints[i]).size(); j++)
                {
                    std::cout << std::get<0>(ThreadStartPoints[i])[j] << " ";
                }
                std::cout << "\n\t\tPartition2: ";
                for (uint32_t j = 0; j < std::get<1>(ThreadStartPoints[i]).size(); j++)
                {
                    std::cout << std::get<1>(ThreadStartPoints[i])[j] << " ";
                }
                std::cout << "\n";
            }
        }
    }

    void startPartitioning(bool Verbose)
    {
        // Start creating worker threads.
        for (uint32_t i = 0; i < NumThreads; i++)
        {
            std::tuple<uint32_t, Partitions, Coordinates, uint32_t, bool> *Args;
            Args = new std::tuple<uint32_t, Partitions, Coordinates, uint32_t, bool>;
            *Args = std::make_tuple(i, ThreadStartPoints[i], GUIThreadStartPoints[i], std::log2(NumThreads), Verbose);
            pthread_create(&WorkerThreads[i], NULL, &recursivePartitioning, (void*)Args);
        }
        for (uint32_t i = 0; i < NumThreads; i++)
        {
            pthread_join(WorkerThreads[i], NULL);
        }

        // Print the result here
        std::cout << "EPartitioner: Best Solution is:\n";
        std::cout << "\tPartition1: ";
        for (uint32_t j = 0; j < std::get<0>(BestSolution).size(); j++)
        {
            std::cout << std::get<0>(BestSolution)[j] << " ";
        }
        std::cout << "\n\tPartition2: ";
        for (uint32_t j = 0; j < std::get<1>(BestSolution).size(); j++)
        {
            std::cout << std::get<1>(BestSolution)[j] << " ";
        }
        std::cout << "\n";
        std::cout << "EPartitioner: Best Solution Cost is: " << BestSolutionCost << "\n";
    }

    // This should have been the same as the function below, but instead of
    // converting the void args to parameters everytime it's called, I only
    // use it as a one time interface when calling threads.
    void *recursivePartitioning(void *Args)
    {
        std::tuple<uint32_t, Partitions, Coordinates, uint32_t, bool> RealArgs;
        RealArgs = *(std::tuple<uint32_t, Partitions, Coordinates, uint32_t, bool>*)Args;
        recursivePartitioning(std::get<0>(RealArgs), std::get<1>(RealArgs), std::get<2>(RealArgs), std::get<3>(RealArgs), std::get<4>(RealArgs));
        return NULL;
    }

    // Now there are two ways to implement this thing, the first is simple
    // partitioning which will pass the "Current Solution" down to the last
    // call, this might be huge, I don't want copies of this everytime a new
    // instance of this function is called.
    // A better solution would be to use only one of those, push to it with
    // each new call, and pop from it upon each return. This is the solution
    // we use.
    // TODO: but I'm not really going to do it. since I pass TID here, maybe
    // I could do this without passing the CurrentSolution and the GUICurrentSolution.
    void recursivePartitioning(uint32_t TID, Partitions& CurrentSolution, Coordinates& GUICurrentSolution, uint32_t Level, bool Verbose)
    {
        // Begin by calculating our cost, we will use it later.
        uint32_t Cost = calculateCost(CurrentSolution, ThreadLocations[TID]);

        // If we exceed the maximum size for each partition, do not continue
        if (std::get<0>(CurrentSolution).size() > MaxPartitionSize ||
            std::get<1>(CurrentSolution).size() > MaxPartitionSize)
        {
            if (Verbose)
            {
                pthread_mutex_lock(&IOMutex);
                std::cout << "EPartitioner: Pruned tree at size " << std::get<0>(CurrentSolution).size() << ", " << std::get<1>(CurrentSolution).size() << "\n";
                pthread_mutex_unlock(&IOMutex);
            }
            return;
        }

        // If we reach a terminal node, exit.
        // NOTE: this check has to happen after the size check, otherwise some
        // unbalanced solutions that exist at leaves might be accepted.
        // DO NOT MOVE.
        if(Level+1 == ConnectedBlocks.size())     // We have nothing more to partition
        {
            if (Verbose)
            {
                pthread_mutex_lock(&IOMutex);
                std::cout << "\tEPartitioner: Reached Terminal Node.\n";
                pthread_mutex_unlock(&IOMutex);
            }

            // This is a hack I've seen somewhere.
            // Those two if statements might seem redundant at the first glance,
            // but they're not, the outer one guarantees that you will never
            // acquire the mutex (which is costly) unless you really have a
            // potential benefit.
            if (Cost < BestSolutionCost)
            {
                // Now update the best solution for the other guys
                pthread_mutex_lock(&BestMutex);
                if (Cost < BestSolutionCost)
                {
                    BestSolutionCost = Cost;
                    BestSolution = CurrentSolution;
                }
                pthread_mutex_unlock(&BestMutex);
            }
            return;
        }

        // Also if the current cost is equal to the minimum cost we've seen, just
        // forget about this.
        if (Cost == BestSolutionCost)
        {
            if (Verbose)
            {
                pthread_mutex_lock(&IOMutex);
                std::cout << "EPartitioner: Pruned tree at cost " << Cost << "\n";
                pthread_mutex_unlock(&IOMutex);
            }
            return;
        }

        // We need to copy this because unlike the CurrentSolution, stuff cannot
        // be pushed and popped from here.
        Coordinates CopyGUICurrentSolution;

        // Try pushing this to the fisrt partition, and continue based on this.
        // Also update the GUI.
        std::get<0>(CurrentSolution).push_back(Level+1);
        ThreadLocations[TID][Level+1] = 1;
        if(Verbose)
        {
            pthread_mutex_lock(&IOMutex);
            std::cout << "\tEPartitioner: Thread " << TID << " now has:\n";
            std::cout << "\t\tPartition1: ";
            for (uint32_t j = 0; j < std::get<0>(CurrentSolution).size(); j++)
            {
                std::cout << std::get<0>(CurrentSolution)[j] << " ";
            }
            std::cout << "\n\t\tPartition2: ";
            for (uint32_t j = 0; j < std::get<1>(CurrentSolution).size(); j++)
            {
                std::cout << std::get<1>(CurrentSolution)[j] << " ";
            }
            std::cout << "\n";
            pthread_mutex_unlock(&IOMutex);
        }
        if(MainWindow)
        {
            pthread_mutex_lock(&GUIMutex);
            CopyGUICurrentSolution = GUICurrentSolution;
            GUICurrentSolution = MainWindow->paintTreeBranch(GUICurrentSolution, Level+1, false, false);
            MainWindow->update();
            pthread_mutex_unlock(&GUIMutex);
        }
        recursivePartitioning(TID, CurrentSolution, GUICurrentSolution, Level+1, Verbose);
        // Now pop whatever you pushed in, we need to try a brand new branch.
        std::get<0>(CurrentSolution).pop_back();

        // Now do the same for the second partition.
        std::get<1>(CurrentSolution).push_back(Level+1);
        ThreadLocations[TID][Level+1] = 2;
        if(Verbose)
        {
            pthread_mutex_lock(&IOMutex);
            std::cout << "\tEPartitioner: Thread " << TID << " now has:\n";
            std::cout << "\t\tPartition1: ";
            for (uint32_t j = 0; j < std::get<0>(CurrentSolution).size(); j++)
            {
                std::cout << std::get<0>(CurrentSolution)[j] << " ";
            }
            std::cout << "\n\t\tPartition2: ";
            for (uint32_t j = 0; j < std::get<1>(CurrentSolution).size(); j++)
            {
                std::cout << std::get<1>(CurrentSolution)[j] << " ";
            }
            std::cout << "\n";
            pthread_mutex_unlock(&IOMutex);
        }
        if(MainWindow)
        {
            pthread_mutex_lock(&GUIMutex);
            GUICurrentSolution = MainWindow->paintTreeBranch(CopyGUICurrentSolution, Level+1, true, false);
            MainWindow->update();
            pthread_mutex_unlock(&GUIMutex);
        }
        recursivePartitioning(TID, CurrentSolution, GUICurrentSolution, Level+1, Verbose);
        std::get<1>(CurrentSolution).pop_back();
        ThreadLocations[TID][Level+1] = 0;
    }

    uint32_t calculateCost(Partitions& Targets, Locations& TargetLocations)
    {
        uint32_t Cost = 0;
        // We should count the number of connections between partitions.
        // Pick one of the sides, start counting!
        for(uint32_t i = 0; i < std::get<0>(Targets).size(); i++)
        {
            // For every element in here, loop over its connections
            for (uint32_t j = 0; j < ConnectedBlocks[std::get<0>(Targets)[i]].size(); j++)
            {
                // If the connection crosses the partitions boundary, add it
                // if (std::find(std::get<1>(Targets).begin(), std::get<1>(Targets).end(),
                //     ConnectedBlocks[std::get<0>(Targets)[i]][j]) != std::get<1>(Targets).end())
                // {
                //     Cost++;
                // }

                // This is an alternative, it saves me the search commented above.
                // This is why I have been using this data structure all along.
                // It saved me a precious 0.4 seconds when running cm138a
                if (TargetLocations[ConnectedBlocks[std::get<0>(Targets)[i]][j]] == 2)
                {
                    Cost++;
                }
            }
        }
        return Cost;
    }

    uint32_t getNumberOfBlocks()
    {
        return ConnectedBlocks.size();
    }
}
