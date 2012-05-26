#include "kinskiGL/App.h"
#include "kinskiGL/Texture.h"
#include "kinskiGL/Shader.h"

using namespace std;
using namespace kinski;

class PoopApp : public App 
{
private:
    
    glm::mat4 m_projectionMatrix;
    gl::Texture m_texture;
    gl::Shader m_shader;
    
public:
    
    void setup()
    {
        m_projectionMatrix = glm::ortho(0, 1, 0, 1, 0, 1);
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
