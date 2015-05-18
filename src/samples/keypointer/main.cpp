#include "KeyPointApp.hpp"

int main(int argc, char *argv[])
{
    auto theApp = std::make_shared<kinski::KeyPointApp>();    
    return theApp->run();
}
