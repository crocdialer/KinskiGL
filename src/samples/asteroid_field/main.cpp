#include "AsteroidField.h"
#include "core/networking.h"

int main(int argc, char *argv[])
{
    auto theApp = std::make_shared<kinski::AsteroidField>();
    LOG_INFO<<"local ip: " << kinski::net::local_ip();
    theApp->set_name(kinski::get_filename_part(argv[0]));
    return theApp->run();
}
