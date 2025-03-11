#include "version.hpp"
#include "g13_main.hpp"
#include "g13_manager.hpp"
#include <getopt.h>

using namespace G13;

int main(const int argc, char* argv[]) {
    // Ensure an instance of G13_Manager is started
    G13_Manager::Instance();

    G13_Manager::start_logging();
    G13_Manager::SetLogLevel("INFO");
    G13_OUT("g13d v" << VERSION_STRING << " " << __DATE__ << " " << __TIME__);

    // TODO: move out argument parsing
    const option long_opts[] = {
            {"logo", required_argument, nullptr, 'l'},
            {"config", required_argument, nullptr, 'c'},
            {"pipe_in", required_argument, nullptr, 'i'},
            {"pipe_out", required_argument, nullptr, 'o'},
            {"umask", required_argument, nullptr, 'u'},
            {"log_level", required_argument, nullptr, 'd'},
            // {"log_file", required_argument, nullptr, 'f'},
            {"help", no_argument, nullptr, 'h'},
            {nullptr, no_argument, nullptr, 0}
        };
    while (true) {
        const auto short_opts = "l:c:i:o:u:d:h";
        const auto opt = getopt_long(argc, argv, short_opts, long_opts, nullptr);

        if (-1 == opt) {
            break;
        }

        switch (opt) {
        case 'l':
            G13_Manager::setStringConfigValue("logo", std::string(optarg));
            G13_Manager::setLogoFilename(std::string(optarg));
            break;

        case 'c':
            G13_Manager::setStringConfigValue("config", std::string(optarg));
            break;

        case 'i':
            G13_Manager::setStringConfigValue("pipe_in", std::string(optarg));
            break;

        case 'o':
            G13_Manager::setStringConfigValue("pipe_out", std::string(optarg));
            break;

        case 'u':
            G13_Manager::setStringConfigValue("umask", std::string(optarg));
            break;

        case 'd':
            G13_Manager::setStringConfigValue("log_level", std::string(optarg));
            G13_Manager::SetLogLevel(G13_Manager::getStringConfigValue("log_level"));
            break;

        case 'h': // -h or --help
        case '?': // Unrecognized option
        default:
            printHelp();
            break;
        }
    }
    return G13_Manager::Run();
}

void printHelp() {
    constexpr auto indent = 24;
    std::cout << "Allowed options" << std::endl;
    std::cout << std::left << std::setw(indent) << "  --help" << "produce help message" << std::endl;
    std::cout << std::left << std::setw(indent) << "  --logo <file>" << "set logo from file" << std::endl;
    std::cout << std::left << std::setw(indent) << "  --config <file>" << "load config commands from file" << std::endl;
    std::cout << std::left << std::setw(indent) << "  --pipe_in <name>" << "specify name for input pipe" << std::endl;
    std::cout << std::left << std::setw(indent) << "  --pipe_out <name>" << "specify name for output pipe" << std::endl;
    std::cout << std::left << std::setw(indent) << "  --umask <octal>" << "specify umask for pipes creation" <<
        std::endl;
    std::cout << std::left << std::setw(indent) << "  --log_level <level>" << "logging level" << std::endl;
    exit(1);
}
