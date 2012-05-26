#include "kinskiGL/App.h"

using namespace std;

class PoopApp : public kinski::App 
{
    
public:
    
    void setup()
    {
        
    }
    
    void update(const float delta)
    {
        
    }
    
    void draw()
    {
        // OpenGL rendering goes here...
    }
};

int main(int argc, char *argv[])
{
    PoopApp theApp;
    
    return theApp.run();
}
