#include <iostream>
#include <exception>
#include <spdlog/spdlog.h>

#include "cli/interface.hpp"

int main(int argc, char *argv[])
{
    spdlog::set_level(spdlog::level::err);

    try
    {
        CLIInterface cli;
        return cli.run(argc, argv);
    }
    catch (const std::exception &e)
    {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
    catch (...)
    {
        std::cerr << "Unknown fatal error" << std::endl;
        return 1;
    }
}
