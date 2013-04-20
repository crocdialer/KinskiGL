#include "frustum_tester.hpp"

using namespace std;
using namespace glm;

namespace kinski
{
    namespace gl{
        void draw_plane(const Plane &p)
        {
            //static gl::MeshPtr m = createPlane(20, 20);
            
        }
        
        gl::MeshPtr create_frustum_mesh(const CameraPtr &cam)
        {
            glm::mat4 inverse_projection = glm::inverse(cam->getProjectionMatrix());
            
            gl::GeometryPtr geom (new gl::Geometry);
            geom->setPrimitiveType(GL_LINE_STRIP);
            static glm::vec3 vertices[8] = {vec3(-1, -1, 1), vec3(1, -1, 1), vec3(1, 1, 1), vec3(-1, 1, 1),
                                     vec3(-1, -1, -1), vec3(1, -1, -1), vec3(1, 1, -1), vec3(-1, 1, -1)};
            static GLuint indices[] = {0, 1, 2, 3, 0, 4, 5, 6, 7, 4, 0, 3, 7, 6, 2, 1, 5};
            int num_indices = sizeof(indices) / sizeof(GLuint);
            
            for (int i = 0; i < 8; i++)
            {
                vec4 proj_v = inverse_projection * vec4(vertices[i], 1.f);
                geom->vertices().push_back(vec3(proj_v) / proj_v.w);
            }
            for (int i = 0; i < num_indices; i++)
            {
                geom->indices().push_back(indices[i]);
            }
            gl::MaterialPtr mat = gl::Material::create();
            gl::MeshPtr m = gl::Mesh::create(geom, mat);
            return m;
        }
    }
    void Frustum_Tester::setup()
    {
        ViewerApp::setup();
        
        /******************** add search paths ************************/
        kinski::addSearchPath("/Library/Fonts");
        m_font.load("Courier New Bold.ttf", 24);
        
        /*********** init our application properties ******************/
        m_frustum_rotation = Property_<glm::mat3>::create("Frustum rotation", mat3());
        registerProperty(m_frustum_rotation);
        m_perspective = Property_<bool>::create("Perspective / Ortho", true);
        registerProperty(m_perspective);
        m_near = RangedProperty<float>::create("Near", 10.f, 0, 100);
        registerProperty(m_near);
        m_far = RangedProperty<float>::create("Far", 60.f, 0, 500);
        registerProperty(m_far);
        m_fov = RangedProperty<float>::create("Fov", 45.f, 0, 120);
        registerProperty(m_fov);
        
        create_tweakbar_from_component(shared_from_this());
        
        // enable observer mechanism
        observeProperties();
    
        // load state from config file
        try
        {
            Serializer::loadComponentState(shared_from_this(), "config.json", PropertyIO_GL());
        }catch(Exception &e)
        {
            LOG_WARNING << e.what();
        }
        
        /********************** construct a simple scene ***********************/
        gl::GeometryPtr points(new gl::Geometry);
        points->setPrimitiveType(GL_POINTS);
        points->vertices().reserve(30000);
        points->colors().reserve(30000);
        for (int i = 0; i < 30000; i++)
        {
            points->vertices().push_back(glm::linearRand(glm::vec3(-100), glm::vec3(100)));
            points->colors().push_back(glm::vec4(1.f));
        }
        
        gl::MaterialPtr mat = gl::Material::create();
        m_point_mesh = gl::Mesh::create(points, mat);
        
        if(*m_perspective)
            m_test_cam = gl::CameraPtr(new gl::PerspectiveCamera(16.f/9.f, *m_fov, *m_near, *m_far));
        else
            m_test_cam = gl::CameraPtr(new gl::OrthographicCamera(-25, 25, -20, 20, *m_near, *m_far));
        
        m_test_cam->setPosition(vec3(0, 0, 50));
        
        m_frustum_mesh = create_frustum_mesh(m_test_cam);
    }
    
    void Frustum_Tester::update(float timeDelta)
    {
        ViewerApp::update(timeDelta);
        
        m_test_cam->setRotation(glm::quat_cast(m_frustum_rotation->value()));
        gl::Frustum f = m_test_cam->frustum();
        for(int i = 0; i < m_point_mesh->geometry()->vertices().size(); i++)
        {
            if(f.intersect(m_point_mesh->geometry()->vertices()[i]))
            {
                m_point_mesh->geometry()->colors()[i] = vec4(1, .7, 0 ,1);
            }
            else
            {
                m_point_mesh->geometry()->colors()[i] = vec4(.3);
            }
        }
        
        m_point_mesh->geometry()->createGLBuffers();
    }
    
    void Frustum_Tester::draw()
    {
        gl::setMatrices(camera());
        if(draw_grid()) gl::drawGrid(50, 50);
        
        gl::drawMesh(m_point_mesh);
        
        gl::loadMatrix(gl::MODEL_VIEW_MATRIX, camera()->getViewMatrix() * m_test_cam->transform());
        gl::drawMesh(m_frustum_mesh);
        
        // draw fps string
        gl::drawText2D(kinski::as_string(framesPerSec()), m_font,
                       vec4(vec3(1) - clear_color().xyz(), 1.f),
                       glm::vec2(windowSize().x - 110, 10));
    }
    
    // Property observer callback
    void Frustum_Tester::updateProperty(const Property::ConstPtr &theProperty)
    {
        ViewerApp::updateProperty(theProperty);
        
        if(theProperty == m_near || theProperty == m_far || theProperty == m_fov ||
           theProperty == m_perspective)
        {
            if(*m_perspective)
                m_test_cam = gl::CameraPtr(new gl::PerspectiveCamera(16.f/9.f, *m_fov, *m_near, *m_far));
            else
                m_test_cam = gl::CameraPtr(new gl::OrthographicCamera(-25, 25, -20, 20, *m_near, *m_far));
            
            m_frustum_mesh = create_frustum_mesh(m_test_cam);
            m_test_cam->setPosition(vec3(0, 0, 50));
        }
    }
    
    void Frustum_Tester::tearDown()
    {
        LOG_PRINT<<"ciao frustum tester";
    }
}

int main(int argc, char *argv[])
{
    kinski::App::Ptr theApp(new kinski::Frustum_Tester);
    return theApp->run();
}