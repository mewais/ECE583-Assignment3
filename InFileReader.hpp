#ifndef INFILE_READER_HPP
#define INFILE_READER_HPP

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "Partitioner.hpp"

namespace INFILE
{
    void readInFile(std::string& FileName, bool Verbose);
}

#endif // INFILE_READER_HPP
