#include "kinskiGL/App.h"
#include "kinskiGL/Texture.h"
#include "kinskiGL/Geometry.h"
#include "kinskiGL/Shader.h"
#include "Data.h"

#include "TextureIO.h"

#include <boost/variant.hpp>

using namespace std;
using namespace kinski;
using namespace glm;

class SimpleGeometryApp : public App
{
private:
    
    gl::Shader m_shader;
    
    gl::Geometry m_geometry;
    
    class InsertUniformVisitor : public boost::static_visitor<>
    {
    private:
        gl::Shader &m_shader;
        const std::string &m_uniform;
        
    public:
        
        InsertUniformVisitor(gl::Shader &theShader, const std::string &theUniform)
        :m_shader(theShader), m_uniform(theUniform){};
        
        template <typename T>
        void operator()( T &value ) const
        {
            m_shader.uniform(m_uniform, value);
        }
    };
    
    void drawGeometry(const gl::Geometry &theGeom, gl::Shader &theShader)
    {
        typedef boost::variant<GLint, GLfloat, GLdouble, glm::vec2,
            glm::vec3, glm::vec4, glm::mat3, glm::mat4> UniformValue;
        
        typedef map<string, UniformValue > UniformMap;
        
        gl::scoped_bind<gl::Shader> scopebind(theShader);
        UniformMap uniforms;
        const int four = 4;
        double trouble = 3.1415;
        
        uniforms["u_blaa"] = four;
        uniforms["u_trouble"] = trouble;
        
        for (UniformMap::iterator it = uniforms.begin(); it != uniforms.end(); it++)
        {
            boost::apply_visitor(InsertUniformVisitor(theShader, it->first), it->second);
        }
        
    };
    
    
public:
    
    void setup()
    {
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_TEXTURE_2D);
        glClearColor(0, 0, 0, 1);
        
        try 
        {
            m_shader.loadFromData(g_vertShaderSrc, g_fragShaderSrc);
        }catch (std::exception &e) 
        {
            fprintf(stderr, "%s\n",e.what());
        }
        
        m_geometry = gl::Plane(100, 100);
        
    }
    
    void tearDown()
    {
        printf("ciao simple geometry\n");
    }
    
    void update(const float timeDelta)
    {
    
    }
    
    void draw()
    {
        
    }
};

int main(int argc, char *argv[])
{
    App::Ptr theApp(new SimpleGeometryApp);
    
    return theApp->run();
}
