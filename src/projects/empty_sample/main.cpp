#include "EmptySample.h"
#include "core/networking.hpp"
#include "core/file_functions.hpp"

int main(int argc, char *argv[])
{
    auto theApp = std::make_shared<kinski::EmptySample>();
    LOG_INFO<<"local ip: " << kinski::net::local_ip();
    theApp->set_name(kinski::get_filename_part(argv[0]));
    return theApp->run();
}
