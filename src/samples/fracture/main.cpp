#include "FractureApp.h"
#include "core/networking.hpp"

int main(int argc, char *argv[])
{
    auto theApp = std::make_shared<kinski::FractureApp>();
    LOG_INFO << "local ip: " << kinski::net::local_ip();
    theApp->set_name(kinski::get_filename_part(argv[0]));
    return theApp->run();
}
