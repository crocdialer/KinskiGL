#include "Aura_App.h"

using namespace kinski;

int main(int argc, char *argv[])
{
    App::Ptr theApp(new Aura_App);
    theApp->set_name("Aura");
    theApp->setWindowSize(1024, 1024);
    //theApp->setFullSceen();
    return theApp->run();
}
