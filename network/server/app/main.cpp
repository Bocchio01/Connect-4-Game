#include <iostream>
#include <csignal>
#include <cstdlib>
#include <string>
#include <thread>

#include "protocol/protocol.hpp"
#include "server/game_server.hpp"

// ================================================================
// Global server pointer for signal handler
// ================================================================
GameServer *g_server = nullptr;

/**
 * Signal handler for graceful shutdown
 */
void signalHandler(int signal)
{
    std::cout << "\n\nReceived signal " << signal << " (";

    switch (signal)
    {
    case SIGINT:
        std::cout << "SIGINT";
        break;
    case SIGTERM:
        std::cout << "SIGTERM";
        break;
    default:
        std::cout << "UNKNOWN";
        break;
    }

    std::cout << ")" << std::endl;
    std::cout << "Shutting down gracefully..." << std::endl;

    if (g_server)
    {
        g_server->stop();
    }

    std::exit(0);
}

/**
 * Print usage information
 */
void printUsage(const char *program_name)
{
    std::cout << "Usage: " << program_name << " [PORT]" << std::endl;
    std::cout << "  -h, --help    Show this message\n";
    std::cout << "  --background  Run in background (daemon/service)\n";
    std::cout << std::endl;
}

/**
 * @brief Start the game server logic
 */
int runServer(uint16_t port)
{
    try
    {
        GameServer server(port);
        g_server = &server;

        std::signal(SIGINT, signalHandler);
        std::signal(SIGTERM, signalHandler);
#ifndef _WIN32
        std::signal(SIGPIPE, SIG_IGN);
#endif

        std::cout << "Server started on port " << port << std::endl;
        std::cout << "Press Ctrl+C to stop (if running in foreground)\n";
        server.start(); // blocking call
    }
    catch (const std::exception &e)
    {
        std::cerr << "Server error: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "Server terminated." << std::endl;
    return 0;
}

// ================================================================
//  LINUX: Daemonization logic
// ================================================================
#ifndef _WIN32
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

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
#include <windows.h>

SERVICE_STATUS_HANDLE g_ServiceStatusHandle = nullptr;
SERVICE_STATUS g_ServiceStatus{}; // all fields zeroed
uint16_t g_servicePort = Protocol::DEFAULT_PORT;

void ReportServiceStatus(DWORD currentState, DWORD win32ExitCode, DWORD waitHint)
{
    g_ServiceStatus.dwCurrentState = currentState;
    g_ServiceStatus.dwWin32ExitCode = win32ExitCode;
    g_ServiceStatus.dwWaitHint = waitHint;
    g_ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    g_ServiceStatus.dwServiceSpecificExitCode = 0;

    g_ServiceStatus.dwControlsAccepted =
        (currentState == SERVICE_START_PENDING) ? 0 : SERVICE_ACCEPT_STOP;

    SetServiceStatus(g_ServiceStatusHandle, &g_ServiceStatus);
}

void WINAPI ServiceCtrlHandler(DWORD ctrl)
{
    if (ctrl == SERVICE_CONTROL_STOP)
    {
        ReportServiceStatus(SERVICE_STOP_PENDING, NO_ERROR, 0);
        if (g_server)
            g_server->stop();
        ReportServiceStatus(SERVICE_STOPPED, NO_ERROR, 0);
    }
}

void WINAPI ServiceMain(DWORD argc, LPWSTR *argv)
{
    (void)argc;
    (void)argv;

    g_ServiceStatusHandle = RegisterServiceCtrlHandler(const_cast<LPCSTR>("ConnectxService"), ServiceCtrlHandler);
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
        {nullptr, nullptr}};
    StartServiceCtrlDispatcherW(serviceTable);
}
#endif

/**
 * Main entry point
 */
int main(int argc, char *argv[])
{
    uint16_t port = Protocol::DEFAULT_PORT;
    bool runInBackground = false;

    // Parse arguments
    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help")
        {
            printUsage(argv[0]);
            return 0;
        }
        else if (arg == "--daemon" || arg == "--background")
            runInBackground = true;
        else
        {
            try
            {
                int portNum = std::stoi(arg);
                if (portNum < 1 || portNum > 65535)
                    throw std::out_of_range("port");
                port = static_cast<uint16_t>(portNum);
            }
            catch (...)
            {
                std::cerr << "Invalid port: " << arg << std::endl;
                printUsage(argv[0]);
                return 1;
            }
        }
    }

    if (runInBackground)
    {
        std::cout << "Starting server in background mode..." << std::endl;
#ifdef _WIN32
        runAsWindowsService(port);
        return 0;
#else
        daemonize();
        return runServer(port);
#endif
    }

    // Default: foreground
    return runServer(port);
}
