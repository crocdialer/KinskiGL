#include "MediaPlayer.hpp"
#include "core/networking.hpp"

int main(int argc, char *argv[])
{
    auto theApp = std::make_shared<kinski::MediaPlayer>(argc, argv);
    LOG_INFO << "local ip: " << kinski::net::local_ip();
    return theApp->run();
}
