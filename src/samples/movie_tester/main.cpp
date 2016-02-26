#include "MovieTest.h"
#include "core/networking.hpp"

int main(int argc, char *argv[])
{
    auto theApp = std::make_shared<kinski::MovieTest>();
    LOG_INFO << "local ip: " << kinski::net::local_ip();
    return theApp->run();
}
