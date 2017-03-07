#include "InFileReader.hpp"

namespace INFILE
{
    void readInFile(std::string& FileName, bool Verbose)
    {
        // Open file
        std::ifstream InFile(FileName);

        if(!InFile)
        {
            std::cout << "EPartitioner: Invalid File Name.\n";
            exit(0);
        }

        // Start by setting the number of blocks, grid size.
        uint32_t NumBlocks, NumNets;
        InFile >> NumBlocks >> NumNets;
        PARTITIONER::setNumberOfBlocks(NumBlocks, Verbose);

        // Now we should initialize the connections.
        if (Verbose)
            std::cout << "\tEPartitioner: Will read " << NumNets << " nets.\n";

        uint32_t NumConnections, First, Second;
        for (uint32_t i = 0; i < NumNets; i++)
        {
            InFile >> NumConnections;
            InFile >> First;
            for (uint32_t j = 0; j < NumConnections-1; j++)
            {
                InFile >> Second;
                PARTITIONER::connectBlocks(First, Second, Verbose);
                First = Second;
            }
        }
    }
}
