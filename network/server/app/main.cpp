#include <csignal>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include "protocol/protocol.hpp"
#include "server/game_server.hpp"

#ifdef _WIN32
#include <windows.h>
SERVICE_STATUS_HANDLE g_ServiceStatusHandle = nullptr;
SERVICE_STATUS g_ServiceStatus{}; // all fields zeroed
uint16_t g_servicePort = Protocol::DEFAULT_PORT;
void ReportServiceStatus(DWORD currentState, DWORD win32ExitCode, DWORD waitHint);
void WINAPI ServiceCtrlHandler(DWORD ctrl);
void WINAPI ServiceMain(DWORD argc, LPWSTR *argv);
void runAsWindowsService(uint16_t port);
#endif

#ifndef _WIN32
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
void daemonize();
#endif

GameServer *game_server = nullptr;

void setupLogger(spdlog::level::level_enum console_log_level,
                 spdlog::level::level_enum file_log_level = spdlog::level::debug);
void parseArguments(int argc, char *argv[], uint16_t &port, bool &run_as_daemon);
void printUsage(const char *program_name);
int runServer(uint16_t port);
void signalHandler(int signal);

// ================================================================
//  MAIN ENTRY POINT
// ================================================================
int main(int argc, char *argv[])
{
    bool run_as_daemon = false;
    uint16_t port = Protocol::DEFAULT_PORT;

    setupLogger(spdlog::level::info);
    parseArguments(argc, argv, port, run_as_daemon);

    if (run_as_daemon)
    {
        spdlog::debug("Starting server in daemon/service mode...");
#ifdef _WIN32
        runAsWindowsService(port);
        std::exit(EXIT_SUCCESS);
#else
        daemonize();
        return runServer(port);
#endif
    }

    spdlog::debug("Starting server in foreground mode...");
    return runServer(port);
}

/**
 * Signal handler for graceful shutdown
 */
void signalHandler(int signal)
{
    spdlog::debug("Received signal {}. Shutting down server...", signal);
    if (game_server)
        game_server->stop();

    std::exit(EXIT_SUCCESS);
}

/**
 * Print usage information
 */
void printUsage(const char *program_name)
{
    std::cout << "Usage: " << program_name << std::endl;
    std::cout << "  -h, --help    Show this message\n";
    std::cout << "  -p, --port    Specify the port number (default: " << Protocol::DEFAULT_PORT << ")\n";
    std::cout << "  -d, --daemon  Run in background (daemon/service)\n";
    std::cout << std::endl;
}

void setupLogger(spdlog::level::level_enum console_log_level, spdlog::level::level_enum file_log_level)
{
    try
    {
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_level(console_log_level);
        console_sink->set_pattern("[%^%l%$] %v");

        auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("server.log", true);
        file_sink->set_level(file_log_level);
        file_sink->set_pattern("[%Y-%m-%d %H:%M:%S] [%l] %v");

        std::vector<spdlog::sink_ptr> sinks{console_sink, file_sink};
        auto logger = std::make_shared<spdlog::logger>("multi_logger", sinks.begin(), sinks.end());
        logger->set_level(spdlog::level::trace);

        spdlog::set_default_logger(logger);
        spdlog::flush_on(spdlog::level::info);

        spdlog::debug("Logger initialized successfully");
    }
    catch (const spdlog::spdlog_ex &ex)
    {
        std::cerr << "Logger initialization failed: " << ex.what() << std::endl;
    }
}

void parseArguments(int argc, char *argv[], uint16_t &port, bool &run_as_daemon)
{
    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];

        // Help argument
        if (arg == "-h" || arg == "--help")
        {
            printUsage(argv[0]);
            std::exit(EXIT_SUCCESS);
        }

        // Port argument
        else if (arg == "-p" || arg == "--port")
        {
            if (i + 1 < argc)
            {
                std::string portStr = argv[++i];
                try
                {
                    int portNum = std::stoi(portStr);
                    if (portNum < 1 || portNum > 65535)
                        throw std::out_of_range("port");
                    port = static_cast<uint16_t>(portNum);
                }
                catch (...)
                {
                    spdlog::error("Invalid port number: {}", portStr);
                    printUsage(argv[0]);
                    std::exit(EXIT_FAILURE);
                }
            }
            else
            {
                spdlog::error("Port number not specified after {}", arg);
                printUsage(argv[0]);
                std::exit(EXIT_FAILURE);
            }
        }

        // Daemon/Service argument
        else if (arg == "-d" || arg == "--daemon")
        {
            run_as_daemon = true;
        }

        // Unknown argument
        else
        {
            spdlog::error("Unknown argument: {}", arg);
            printUsage(argv[0]);
            std::exit(EXIT_FAILURE);
        }
    }
}

/**
 * @brief Start the game server logic
 */
int runServer(uint16_t port)
{
    try
    {
        GameServer server(port);
        game_server = &server;

        std::signal(SIGINT, signalHandler);
        std::signal(SIGTERM, signalHandler);
#ifndef _WIN32
        std::signal(SIGPIPE, SIG_IGN);
#endif

        server.start();
    }
    catch (const std::exception &e)
    {
        spdlog::error("Server error: {}", e.what());
        return 1;
    }

    return 0;
}

// ================================================================
//  LINUX: Daemonization logic
// ================================================================
#ifndef _WIN32
void daemonize()
{
    pid_t pid = fork();
    if (pid < 0)
        std::exit(EXIT_FAILURE);
    if (pid > 0)
        std::exit(EXIT_SUCCESS);

    if (setsid() < 0)
        std::exit(EXIT_FAILURE);

    signal(SIGCHLD, SIG_IGN);
    signal(SIGHUP, SIG_IGN);

    pid = fork();
    if (pid < 0)
        std::exit(EXIT_FAILURE);
    if (pid > 0)
        std::exit(EXIT_SUCCESS);

    umask(0);
    chdir("/");

    // Redirect std streams to /dev/null
    int fd = open("/dev/null", O_RDWR, 0);
    if (fd != -1)
    {
        dup2(fd, STDIN_FILENO);
        dup2(fd, STDOUT_FILENO);
        dup2(fd, STDERR_FILENO);
        if (fd > 2)
            close(fd);
    }
}
#endif

// ================================================================
//  WINDOWS: Service Implementation
// ================================================================
#ifdef _WIN32
void ReportServiceStatus(DWORD currentState, DWORD win32ExitCode, DWORD waitHint)
{
    g_ServiceStatus.dwCurrentState = currentState;
    g_ServiceStatus.dwWin32ExitCode = win32ExitCode;
    g_ServiceStatus.dwWaitHint = waitHint;
    g_ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    g_ServiceStatus.dwServiceSpecificExitCode = 0;

    g_ServiceStatus.dwControlsAccepted = (currentState == SERVICE_START_PENDING) ? 0 : SERVICE_ACCEPT_STOP;

    SetServiceStatus(g_ServiceStatusHandle, &g_ServiceStatus);
}

void WINAPI ServiceCtrlHandler(DWORD ctrl)
{
    if (ctrl == SERVICE_CONTROL_STOP)
    {
        ReportServiceStatus(SERVICE_STOP_PENDING, NO_ERROR, 0);
        if (game_server)
            game_server->stop();
        ReportServiceStatus(SERVICE_STOPPED, NO_ERROR, 0);
    }
}

void WINAPI ServiceMain(DWORD argc, LPWSTR *argv)
{
    (void)argc;
    (void)argv;

    g_ServiceStatusHandle = RegisterServiceCtrlHandlerW(L"ConnectxService", ServiceCtrlHandler);
    if (!g_ServiceStatusHandle)
        return;

    ReportServiceStatus(SERVICE_START_PENDING, NO_ERROR, 3000);

    // Launch your server thread
    std::thread([]
                {
        runServer(g_servicePort);
        ReportServiceStatus(SERVICE_STOPPED, NO_ERROR, 0); })
        .detach();

    ReportServiceStatus(SERVICE_RUNNING, NO_ERROR, 0);

    while (g_ServiceStatus.dwCurrentState == SERVICE_RUNNING)
        Sleep(1000);
}

void runAsWindowsService(uint16_t port)
{
    g_servicePort = port;

    SERVICE_TABLE_ENTRYW serviceTable[] = {
        {const_cast<LPWSTR>(L"ConnectxService"), (LPSERVICE_MAIN_FUNCTIONW)ServiceMain},
        {nullptr, nullptr},
    };
    StartServiceCtrlDispatcherW(serviceTable);
}
#endif
