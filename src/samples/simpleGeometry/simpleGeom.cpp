#include "kinskiApp/GLFW_App.h"
#include "kinskiApp/TextureIO.h"

#include "kinskiCV/CVThread.h"

#include "kinskiGL/SerializerGL.h"
#include "kinskiGL/Scene.h"
#include "kinskiGL/Camera.h"
#include "kinskiGL/Mesh.h"
#include "kinskiGL/Fbo.h"

#include "AssimpConnector.h"

using namespace std;
using namespace kinski;
using namespace glm;

    template <typename T>
    class MovingAverage
    {
    public:
        explicit MovingAverage(uint32_t sz = 5):m_size(sz){}
        inline void push(const T &theValue)
        {
            m_values.push_back(theValue);
            if(m_values.size() > m_size) m_values.pop_front();
        }
        void foo()
        {
//            glm::vec2 average;
//            std::list<glm::vec2>::const_iterator it;
//            for (it = m_mouse_positions.begin() ; it != m_mouse_positions.end(); ++it)
//                average += *it;
//            average /= m_mouse_positions.size();
//            
//            float sum_nominator = 0.0f, sum_denominator = 0.0f;
//            for (it = m_mouse_positions.begin() ; it != m_mouse_positions.end(); ++it)
//            {
//                sum_nominator += (it->x - average.x) * (it->y - average.y);
//                sum_denominator += (it->x - average.x) * (it->x - average.x);
//            }
//            float a1 = sum_nominator / sum_denominator;
            //float a0 =
        }
        
        const T filter()
        {
            T ret;
            typename std::list<T>::const_iterator it = m_values.begin();
            for ( ; it != m_values.end(); ++it)
                ret += *it;
            ret /= (float)m_values.size();
            m_values.clear();
            return ret;
        }

    private:
        std::list<T> m_values;
        uint32_t m_size;
    };

class SimpleGeometryApp : public GLFW_App
{
private:
    
    gl::Fbo m_frameBuffer;
    gl::Texture m_textures[4];
    
    gl::Material::Ptr m_material[4], m_pointMaterial;
    gl::Geometry::Ptr m_geometry;
    gl::Geometry::Ptr m_straightPlane;
    
    gl::MeshPtr m_mesh, m_selected_mesh;
    gl::PerspectiveCamera::Ptr m_Camera;
    gl::Scene m_scene;
    
    RangedProperty<float>::Ptr m_distance;
    RangedProperty<float>::Ptr m_textureMix;
    Property_<string>::Ptr m_modelPath;
    RangedProperty<float>::Ptr m_animationTime;
    
    Property_<bool>::Ptr m_wireFrame;
    Property_<bool>::Ptr m_drawNormals;
    Property_<glm::vec3>::Ptr m_lightDir;
    
    Property_<glm::vec4>::Ptr m_color;
    Property_<glm::mat3>::Ptr m_rotation;
    RangedProperty<float>::Ptr m_rotationSpeed;
    
    // opencv interface
    CVThread::Ptr m_cvThread;
    
    // mouse rotation control
    vec2 m_clickPos, m_dragPos, m_inertia;
    float m_rotation_damping;
    bool  m_mouse_down;
    mat3 m_lastTransform;
    MovingAverage<glm::vec2> m_avg_filter;

public:
    
    void setup()
    {
        m_rotation_damping = .9;
        
        /******************** add search paths ************************/
        kinski::addSearchPath("~/Desktop/");
        kinski::addSearchPath("~/Pictures/");
        
        /*********** init our application properties ******************/
        
        m_distance = RangedProperty<float>::create("view distance", 25, 0, 5000);
        registerProperty(m_distance);
        
        m_textureMix = RangedProperty<float>::create("texture mix ratio", 0.2, 0, 1);
        registerProperty(m_textureMix);
        
        m_modelPath = Property_<string>::create("Model path", "duck.dae");
        registerProperty(m_modelPath);

        m_animationTime = RangedProperty<float>::create("Animation time", 0, 0, 1);
        registerProperty(m_animationTime);
        
        m_wireFrame = Property_<bool>::create("Wireframe", false);
        registerProperty(m_wireFrame);
        
        m_drawNormals = Property_<bool>::create("Normals", false);
        registerProperty(m_drawNormals);
        
        m_lightDir = Property_<vec3>::create("Light dir", vec3(1));
        registerProperty(m_lightDir);
        
        m_color = Property_<glm::vec4>::create("Material color", glm::vec4(1 ,1 ,0, 0.6));
        registerProperty(m_color);
        
        m_rotation = Property_<glm::mat3>::create("Geometry Rotation", glm::mat3());
        registerProperty(m_rotation);
        
        m_rotationSpeed = RangedProperty<float>::create("Rotation Speed", 0, -100, 100);
        registerProperty(m_rotationSpeed);
        
        // add properties
        addPropertyListToTweakBar(getPropertyList());
        
        setBarColor(vec4(0, 0 ,0 , .5));
        setBarSize(ivec2(250, 500));

        // enable observer mechanism
        observeProperties();
        
        /********************** construct a simple scene ***********************/
        
        gl::Fbo::Format fboFormat;
        //TODO: mulitsampling fails
        //fboFormat.setSamples(4);
        m_frameBuffer = gl::Fbo(getWidth(), getHeight(), fboFormat);
        
        // create a simplex noise texture
        {
            int w = 512, h = 512;
            float data[w * h];
            
            for (int i = 0; i < h; i++)
                for (int j = 0; j < w; j++)
                {
                    data[i * h + j] = (glm::simplex( vec3(0.0125f * vec2(i, j), 0.025)) + 1) / 2.f;
                }
            
            m_textures[1].update(data, GL_RED, w, h, true);
        }
        
        m_material[0] = gl::Material::Ptr(new gl::Material);
        m_material[1] = gl::Material::Ptr(new gl::Material);
        m_material[1]->setDiffuse(glm::vec4(0, 1, 0, 1));
        try
        {
            m_textures[0] = gl::createTextureFromFile("Earth2.jpg");
            m_material[0]->addTexture(m_textures[0]);
            m_material[0]->addTexture(m_textures[1]);
            m_material[0]->setShader(gl::createShaderFromFile("shader_normalMap.vert",
                                                              "shader_normalMap.frag"));
        }catch(Exception &e)
        {
            LOG_ERROR<<e.what();
        }

        m_pointMaterial = gl::Material::Ptr(new gl::Material);
        //m_pointMaterial->addTexture(gl::TextureIO::loadTexture("smoketex.png"));
        m_pointMaterial->setPointSize(30.f);
        m_pointMaterial->setBlending();
        //m_pointMaterial->setDepthWrite(false);
        
        m_Camera = gl::PerspectiveCamera::Ptr(new gl::PerspectiveCamera);
        m_Camera->setClippingPlanes(1, 5000);
        
        // test box shape
        //gl::Geometry::Ptr myBox(gl::createBox(vec3(50, 100, 50)));
        gl::Geometry::Ptr myBox(gl::createSphere(100, 36));
        
        gl::Mesh::Ptr myBoxMesh(new gl::Mesh(myBox, m_material[0]));
        myBoxMesh->setPosition(vec3(0, -100, 0));
        m_scene.addObject(myBoxMesh);
        
        // load state from config file
        try
        {
            Serializer::loadComponentState(shared_from_this(), "config.json", PropertyIO_GL());
        }catch(Exception &e)
        {
            LOG_WARNING << e.what();
        }
    }
    
    void update(const float timeDelta)
    {
        m_avg_filter.push(glm::vec2(0));
        m_inertia *= m_rotation_damping;
        
        if(!m_mouse_down && glm::length2(m_inertia) > 0.0025)
        {
            *m_rotation = mat3_cast(glm::quat(*m_rotation) *
                                    glm::quat(vec3(glm::radians(-m_inertia.y),
                                                   glm::radians(-m_inertia.x), 0)));
        }
        else if(!m_mouse_down || displayTweakBar())
        {
            *m_rotation = mat3( glm::rotate(mat4(m_rotation->value()),
                                            *m_rotationSpeed * timeDelta,
                                            vec3(0, 1, .5)));
        }

        
        if(m_mesh)
        {
            m_mesh->material()->setWireframe(m_wireFrame->value());
            m_mesh->material()->uniform("u_lightDir", m_lightDir->value());
            m_mesh->material()->uniform("u_textureMix", m_textureMix->value());
            m_mesh->material()->setDiffuse(m_color->value());
            m_mesh->material()->setBlending(m_color->value().a < 1.0f);

            if(m_mesh->geometry()->hasBones())
            {
                m_mesh->geometry()->updateAnimation(getApplicationTime() / 5.0f);
//              m_mesh->getGeometry()->updateAnimation(m_animationTime->val() *
//                                                   m_mesh->getGeometry()->animation()->duration);
            }
        }
        
        m_material[0]->uniform("u_time",getApplicationTime());
    }
    
    void draw()
    {
//        m_frameBuffer.bindFramebuffer();
//        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//        glViewport(0, 0, m_frameBuffer.getWidth(), m_frameBuffer.getHeight());
        
        gl::drawTexture(m_textures[0], windowSize());

        gl::loadMatrix(gl::PROJECTION_MATRIX, m_Camera->getProjectionMatrix());
        gl::loadMatrix(gl::MODEL_VIEW_MATRIX, m_Camera->getViewMatrix());
        gl::drawGrid(500, 500, 100, 100);
        
        m_scene.render(m_Camera);
        
        if(m_selected_mesh)
        {
            gl::loadMatrix(gl::MODEL_VIEW_MATRIX, m_Camera->getViewMatrix() * m_selected_mesh->transform());
            gl::drawAxes(m_selected_mesh);
            gl::drawBoundingBox(m_selected_mesh);
            if(*m_drawNormals) gl::drawNormals(m_selected_mesh);
            
//            gl::drawPoints(m_mesh2->geometry()->vertexBuffer().id(),
//                           m_mesh2->geometry()->vertices().size(),
//                           m_pointMaterial);
            
            if(m_selected_mesh->geometry()->hasBones())
            {
                vector<vec3> points;
                buildSkeleton(m_selected_mesh->geometry()->rootBone(), points);
                gl::drawPoints(points);
                gl::drawLines(points, vec4(1, 0, 0, 1));
            }
            
        }
        
//        m_frameBuffer.unbindFramebuffer();
//        glViewport(0, 0, getWidth(), getHeight());
//        gl::drawTexture(m_frameBuffer.getTexture(), windowSize() );
        
        //gl::drawTexture(m_frameBuffer.getDepthTexture(), windowSize() / 2.0f, windowSize() / 2.0f);
    }
    
    void buildSkeleton(std::shared_ptr<gl::Bone> currentBone, vector<vec3> &points)
    {
        list<shared_ptr<gl::Bone> >::iterator it = currentBone->children.begin();
        for (; it != currentBone->children.end(); ++it)
        {
            mat4 globalTransform = currentBone->worldtransform;
            mat4 childGlobalTransform = (*it)->worldtransform;
            points.push_back(globalTransform[3].xyz());
            points.push_back(childGlobalTransform[3].xyz());
            
            buildSkeleton(*it, points);
        }
    }
    
    void mousePress(const MouseEvent &e)
    {
        m_clickPos = vec2(e.getX(), e.getY());
        m_lastTransform = *m_rotation;
        m_mouse_down = true;
        
        if(gl::Object3DPtr picked_obj = m_scene.pick(gl::calculateRay(m_Camera, e.getX(), e.getY()),
                                                     true))
        {
            LOG_TRACE<<"picked id: "<< picked_obj->getID();
            
            if( gl::MeshPtr m = dynamic_pointer_cast<gl::Mesh>(picked_obj)){
                
                if(m_selected_mesh != m)
                {
                    if(m_selected_mesh){ m_selected_mesh->material() = m_material[0]; }
                    
                    m_selected_mesh = m;
                    m_material[0] = m_selected_mesh->material();
                    m_material[1]->shader() = m_material[0]->shader();
                    m_selected_mesh->material() = m_material[1];
                }
            }
        }
        else{
            if(e.isRight() && m_selected_mesh){
                m_selected_mesh->material() = m_material[0];
                m_selected_mesh.reset();
            }
        }
    }
    
    void mouseDrag(const MouseEvent &e)
    {
        vec2 mouseDiff = vec2(e.getX(), e.getY()) - m_clickPos;
        if(e.isLeft() && (e.isAltDown() || !displayTweakBar()))
        {
            *m_rotation = mat3_cast(glm::quat(m_lastTransform) *
                                    glm::quat(vec3(glm::radians(-mouseDiff.y),
                                                   glm::radians(-mouseDiff.x), 0)));
        }
        m_avg_filter.push(vec2(e.getX(), e.getY()) - m_dragPos);
        m_dragPos = vec2(e.getX(), e.getY());
    }
    
    void mouseRelease(const MouseEvent &e)
    {
        m_mouse_down = false;
        if(!displayTweakBar())
            m_inertia = m_avg_filter.filter();
    }
    
    void mouseWheel(const MouseEvent &e)
    {
        *m_distance -= e.getWheelIncrement();
    }
    
    void keyPress(const KeyEvent &e)
    {
        GLFW_App::keyPress(e);
        
        switch (e.getChar())
        {
        case KeyEvent::KEY_s:
            Serializer::saveComponentState(shared_from_this(), "config.json", PropertyIO_GL());
            break;
            
        case KeyEvent::KEY_r:
            try
            {
                Serializer::loadComponentState(shared_from_this(), "config.json", PropertyIO_GL());
            }catch(Exception &e)
            {
                LOG_WARNING << e.what();
            }
            break;
                
        default:
            break;
        }
    }

    void resize(int w, int h)
    {
        m_Camera->setAspectRatio(getAspectRatio());
        gl::Fbo::Format fboFormat;
        //TODO: mulitsampling fails
        //fboFormat.setSamples(4);
        m_frameBuffer = gl::Fbo(w, h, fboFormat);
    }
    
    // Property observer callback
    void updateProperty(const Property::ConstPtr &theProperty)
    {
        // one of our porperties was changed
        if(theProperty == m_color)
        {
            if(m_selected_mesh) m_selected_mesh->material()->setDiffuse(*m_color);
        }
        else if(theProperty == m_lightDir || theProperty == m_textureMix)
        {
            m_material[0]->uniform("u_lightDir", *m_lightDir);
            m_material[0]->uniform("u_textureMix", *m_textureMix);
        }
        else if(theProperty == m_distance || theProperty == m_rotation)
        {
            vec3 look_at;
            if(m_selected_mesh)
                look_at = gl::OBB(m_selected_mesh->boundingBox(), m_selected_mesh->transform()).center;
            
            mat4 tmp = glm::mat4(m_rotation->value());
            tmp[3] = vec4(look_at + m_rotation->value()[2] * m_distance->value(), 1.0f);
            m_Camera->transform() = tmp;
        }
        else if(theProperty == m_modelPath)
        {
            try
            {
                gl::MeshPtr m = gl::AssimpConnector::loadModel(*m_modelPath);
                m_scene.removeObject(m_mesh);
                m_mesh = m;
                m_mesh->material()->setShinyness(0.9);
                m_scene.addObject(m_mesh);
            } catch (Exception &e)
            {
                LOG_ERROR<< e.what();
                m_modelPath->removeObserver(shared_from_this());
                *m_modelPath = "- not found -";
                m_modelPath->addObserver(shared_from_this());
            }
        }
    }
    
    void tearDown()
    {
        LOG_PRINT<<"ciao simple geometry";
    }
};

int main(int argc, char *argv[])
{
    App::Ptr theApp(new SimpleGeometryApp);
    theApp->setWindowSize(1024, 600);
    
    return theApp->run();
}
