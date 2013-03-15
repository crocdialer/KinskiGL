#include "kinskiApp/ViewerApp.h"
#include "kinskiCV/CVThread.h"
#include "kinskiGL/Fbo.h"
#include "AssimpConnector.h"

using namespace std;
using namespace kinski;
using namespace glm;

class SimpleGeometryApp : public ViewerApp
{
private:
    
    gl::Fbo m_frameBuffer;
    gl::Texture m_textures[4];
    gl::MeshPtr m_mesh, m_label;
    gl::Font m_font;
    
    RangedProperty<float>::Ptr m_textureMix;
    Property_<string>::Ptr m_modelPath;
    RangedProperty<float>::Ptr m_animationTime;
    Property_<glm::vec4>::Ptr m_color;

public:
    
    void setup()
    {
        ViewerApp::setup();
        set_precise_selection(true);
        
        /******************** add search paths ************************/
        kinski::addSearchPath("~/Desktop");
        kinski::addSearchPath("~/Desktop/sample", true);
        kinski::addSearchPath("~/Pictures");
        kinski::addSearchPath("/Library/Fonts");
        //list<string> files = kinski::getDirectoryEntries("~/Desktop/sample", true, "png");
        
        m_font.load("Chalkduster.ttf", 64);
        
        /*********** init our application properties ******************/
        
        m_textureMix = RangedProperty<float>::create("texture mix ratio", 0.2, 0, 1);
        registerProperty(m_textureMix);
        
        m_modelPath = Property_<string>::create("Model path", "duck.dae");
        registerProperty(m_modelPath);

        m_animationTime = RangedProperty<float>::create("Animation time", 0, 0, 1);
        registerProperty(m_animationTime);
        
        m_color = Property_<glm::vec4>::create("Material color", glm::vec4(1 ,1 ,0, 0.6));
        registerProperty(m_color);
        
        // add properties
        addPropertyListToTweakBar(getPropertyList());

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
        
        try
        {
            m_textures[0] = gl::createTextureFromFile("smoketex.png");
            materials()[0]->addTexture(m_textures[0]);
            materials()[0]->addTexture(m_textures[1]);
            materials()[0]->setShader(gl::createShaderFromFile("shader_normalMap.vert",
                                                               "shader_normalMap.frag"));
        }catch(Exception &e)
        {
            LOG_ERROR<<e.what();
        }
        
        m_textures[2] = m_font.create_texture("Du bist ein gelber Kakadoo. Kakafaka schackalacka lulubaaaaa\nNe neue Zeile ...", glm::vec4(1.0f, .4f, .0f, 1.f));
        //m_textures[2] = m_font.glyph_texture();

        gl::Geometry::Ptr myBox(gl::createSphere(100, 36));
        gl::Mesh::Ptr myBoxMesh(new gl::Mesh(myBox, materials()[0]));
        myBoxMesh->setPosition(vec3(0, -100, 0));
        scene().addObject(myBoxMesh);
        
        
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
        ViewerApp::update(timeDelta);
        
        if(m_mesh)
        {
            m_mesh->material()->setWireframe(wireframe());
            m_mesh->material()->uniform("u_lightDir", light_direction());
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
        materials()[0]->uniform("u_time",getApplicationTime());
        materials()[0]->uniform("u_lightDir", light_direction());
        materials()[0]->uniform("u_textureMix", *m_textureMix);
    }
    
    void draw()
    {
//        m_frameBuffer.bindFramebuffer();
//        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//        glViewport(0, 0, m_frameBuffer.getWidth(), m_frameBuffer.getHeight());
        
        gl::drawTexture(m_textures[2], m_textures[2].getSize());

        gl::loadMatrix(gl::PROJECTION_MATRIX, camera()->getProjectionMatrix());
        gl::loadMatrix(gl::MODEL_VIEW_MATRIX, camera()->getViewMatrix());
        gl::drawGrid(500, 500, 100, 100);
        
//        gl::drawBoundingBox(font_mesh);
//        gl::drawMesh(font_mesh);
        
        scene().render(camera());
        
        if(selected_mesh())
        {
            gl::loadMatrix(gl::MODEL_VIEW_MATRIX, camera()->getViewMatrix() * selected_mesh()->transform());
            gl::drawAxes(selected_mesh());
            gl::drawBoundingBox(selected_mesh());
            if(normals()) gl::drawNormals(selected_mesh());

//            gl::drawPoints(selected_mesh()->geometry()->vertexBuffer().id(),
//                           selected_mesh()->geometry()->vertices().size());
            
            if(selected_mesh()->geometry()->hasBones())
            {
                vector<vec3> points;
                buildSkeleton(selected_mesh()->geometry()->rootBone(), points);
                gl::drawPoints(points);
                gl::drawLines(points, vec4(1, 0, 0, 1));
            }
            
            
            gl::loadMatrix(gl::MODEL_VIEW_MATRIX, camera()->getViewMatrix() * m_label->transform());
            gl::drawMesh(m_label);
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

    void resize(int w, int h)
    {
        ViewerApp::resize(w, h);
        
        gl::Fbo::Format fboFormat;
        m_frameBuffer = gl::Fbo(w, h, fboFormat);
    }
    
    void mousePress(const MouseEvent &e)
    {
        ViewerApp::mousePress(e);
        
        if(selected_mesh())
        {
            m_label = m_font.create_mesh("My Id is " + kinski::as_string(selected_mesh()->getID()));
            m_label->setPosition(selected_mesh()->position()
                                    + glm::vec3(0, selected_mesh()->boundingBox().height() / 2.f
                                                + m_label->boundingBox().height(), 0)
                                    - m_label->boundingBox().center());
        }
        
    }
    
    // Property observer callback
    void updateProperty(const Property::ConstPtr &theProperty)
    {
        ViewerApp::updateProperty(theProperty);
        
        // one of our porperties was changed
        if(theProperty == m_color)
        {
            if(selected_mesh()) selected_mesh()->material()->setDiffuse(*m_color);
        }
        else if(theProperty == m_modelPath)
        {
            try
            {
                gl::MeshPtr m = gl::AssimpConnector::loadModel(*m_modelPath);
                scene().removeObject(m_mesh);
                m_mesh = m;
                m_mesh->material()->setShinyness(0.9);
                scene().addObject(m_mesh);
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
