///
/// @file programOptions.cpp
/// 
/// @Copyright 2019 The MathWorks, Inc.

#include "ProgramOptions.hpp"
#include "cxxopts.hpp"


CommandLineArgs ProcessCommandLine(int argc, char**argv)
{
    CommandLineArgs args;

    cxxopts::Options options("clang-xform", "Clang tool for large-scale C++ code refactoring");

    options.add_options("Group")
        ("h, help", "produce help message")
        ("j, num-threads", "number of threads", cxxopts::value<int>())
        ("p, compile-commands", "compile commands", cxxopts::value<std::string>())
        ("o, output", "output file", cxxopts::value<std::string>())
        ("a, apply", "apply replacements", cxxopts::value<std::string>())
        ("f, input-files", "input files", cxxopts::value<std::vector<std::string> >())
        ("m, matchers", "matchers to apply", cxxopts::value<std::vector<std::string> >())
        ("c, config", "config file", cxxopts::value<std::string>())
        ("d, display", "display registered matchers", cxxopts::value<bool>())
        ("q, quiet", "silent output", cxxopts::value<bool>())
        ("l, log", "log file", cxxopts::value<std::string>());

    options.parse_positional({"input-files"});

    auto result = options.parse(argc, argv);

    if (result.count("num-threads")) {
        args.numThreads = result["num-threads"].as<int>();
    }

    if (result.count("compile-commands")) {
        args.compileCommands = result["compile-commands"].as<std::string>();
    }

    if (result.count("output")) {
        args.outputFile = result["output"].as<std::string>();
    }

    if (result.count("apply")) {
        args.replaceFile = result["apply"].as<std::string>();
    }

    if (result.count("input-files")) {
        args.inputFiles = result["input-files"].as<std::vector<std::string> >();
    }

    if (result.count("matchers")) {
        args.matchers = result["matchers"].as<std::vector<std::string> >();
    }

    if (result.count("config")) {
        args.configFile = result["config"].as<std::string>();
    }

    if (result.count("display")) {
        args.display = result["display"].as<bool>();
    }

    if (result.count("quiet")) {
        args.quiet = result["quiet"].as<bool>();
    }

    if (result.count("log")) {
        args.logFile = result["log"].as<std::string>();
    }

    if (result.count("help"))
    {
        std::cout << options.help({"Group"}) << std::endl;
        exit(0);
    }
    
    return args;
}
