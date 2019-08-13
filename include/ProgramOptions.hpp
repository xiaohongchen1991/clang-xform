///
/// @file ProgramOptions.hpp
/// 
/// @Copyright 2018 The MathWorks, Inc.

#include <string>
#include <vector>
#include <limits>

#ifndef PROGRAM_OPTIONS_HPP
#define PROGRAM_OPTIONS_HPP

// Command line arguments.
struct CommandLineArgs
{
    CommandLineArgs() = default;
    
    // list of files to parse (empty means all files)
    std::vector<std::string> inputFiles;
    // number of threads
    int numThreads = std::numeric_limits<int>::max();
    // compile commands
    std::string compileCommands;
    // output file
    std::string outputFile;
    // replacement file
    std::string replaceFile;
    // matchers to apply
    std::vector<std::string> matchers;
    // config file
    std::string configFile;
    // display registered matchers
    bool display = false;
    // silent output
    bool quiet = false;
    // log file
    std::string logFile;
};

// Parse the command line arguments.
CommandLineArgs ProcessCommandLine(int argc, char**argv);


#endif // PROGRAM_OPTIONS_HPP
