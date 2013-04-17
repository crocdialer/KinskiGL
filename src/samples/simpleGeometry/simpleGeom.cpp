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
    Property_<float>::Ptr m_shinyness;

public:
    
    void setup()
    {
        ViewerApp::setup();
        set_precise_selection(true);
        
        /******************** add search paths ************************/
        kinski::addSearchPath("~/Desktop");
        kinski::addSearchPath("~/Desktop/creatures", true);
        kinski::addSearchPath("~/Pictures");
        kinski::addSearchPath("/Library/Fonts");
        list<string> files = kinski::getDirectoryEntries("~/Desktop/sample", true, "png");
        
        m_font.load("Courier New Bold.ttf", 24);
        
        /*********** init our application properties ******************/
        
        m_textureMix = RangedProperty<float>::create("texture mix ratio", 0.2, 0, 1);
        registerProperty(m_textureMix);
        
        m_modelPath = Property_<string>::create("Model path", "duck.dae");
        registerProperty(m_modelPath);

        m_animationTime = RangedProperty<float>::create("Animation time", 0, 0, 1);
        registerProperty(m_animationTime);
        
        m_color = Property_<glm::vec4>::create("Material color", glm::vec4(1 ,1 ,0, 0.6));
        registerProperty(m_color);
        
        m_shinyness = Property_<float>::create("Shinyness", 1.0);
        registerProperty(m_shinyness);
        
        create_tweakbar_from_component(shared_from_this());

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
            
            gl::Texture::Format fmt;
            fmt.setInternalFormat(GL_RED);
            m_textures[1] = gl::Texture (w, h, fmt);
            //m_textures[1].setFlipped();
            m_textures[1].update(data, GL_RED, w, h, true);
        }
        
        
        //m_textures[1] = gl::createTextureFromFile("stone.png");
        m_textures[2] = m_font.create_texture("Du musst was an die Tafel schreiben.\n"
                                              "Rechne vor der Klasse eine Aufgabe!!\n\n  1 + 3 = 5");
        //m_textures[2] = m_font.glyph_texture();

        gl::Geometry::Ptr myBox(gl::createBox(glm::vec3(50.f)));//(gl::createSphere(100, 36));
        gl::MaterialPtr mat(new gl::Material);
        materials().push_back(mat);
        
        try
        {
            m_textures[0] = gl::createTextureFromFile("Earth2.jpg", true);
            mat->addTexture(m_textures[0]);
            mat->addTexture(m_textures[1]);
            mat->setShinyness(60);
            //mat->setShader(gl::createShader(gl::SHADER_PHONG_NORMALMAP));
            mat->setShader(gl::createShaderFromFile("shader_normalMap.vert",
                                                    "shader_normalMap.frag"));
        }catch(Exception &e)
        {
            LOG_ERROR<<e.what();
        }
        gl::Mesh::Ptr myBoxMesh(new gl::Mesh(myBox, mat));
        myBoxMesh->createVertexArray();
        myBoxMesh->setPosition(vec3(0, -100, 0));
        scene().addObject(myBoxMesh);
        
        //gl::MeshPtr kafka_mesh = m_font.create_mesh(kinski::readFile("kafka_short.txt"));
        gl::MeshPtr kafka_mesh = m_font.create_mesh("Strauß, du hübscher Eiergäggelö!");
        kafka_mesh->setPosition(kafka_mesh->position() - kafka_mesh->boundingBox().center());
        //scene().addObject(kafka_mesh);
        
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
            m_mesh->material()->setDiffuse(m_color->value());
            m_mesh->material()->setBlending(m_color->value().a < 1.0f);

        }
        for (int i = 0; i < materials().size(); i++)
        {
            materials()[i]->uniform("u_time",getApplicationTime());
            materials()[i]->uniform("u_lightDir", light_direction());
            materials()[i]->setShinyness(*m_shinyness);
            materials()[i]->setAmbient(0.2 * clear_color());
        }
    }
    
    void draw()
    {
//        m_frameBuffer.bindFramebuffer();
//        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//        glViewport(0, 0, m_frameBuffer.getWidth(), m_frameBuffer.getHeight());
        
        //background
        gl::drawTexture(m_textures[0], windowSize());
        gl::drawText2D("Kuckucksuhr", m_font, glm::vec2(100, 100));
        
        gl::setMatrices(camera());
        
        if(draw_grid()){ gl::drawGrid(500, 500, 100, 100); }
        
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
            // Label
            gl::loadMatrix(gl::MODEL_VIEW_MATRIX, camera()->getViewMatrix() * m_label->transform());
            m_label->setRotation(glm::mat3(camera()->transform()));
            gl::drawMesh(m_label);
        }
//        m_frameBuffer.unbindFramebuffer();
//        glViewport(0, 0, getWidth(), getHeight());
//        gl::drawTexture(m_frameBuffer.getTexture(), windowSize() );
        //gl::drawTexture(m_frameBuffer.getDepthTexture(), windowSize() / 2.0f, windowSize() / 2.0f);
    }
    
    void buildSkeleton(gl::BonePtr currentBone, vector<vec3> &points)
    {
        list<gl::BonePtr>::iterator it = currentBone->children.begin();
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
            m_label = m_font.create_mesh("Du liest Kafka: " + kinski::as_string(selected_mesh()->getID()));
            m_label->setPosition(selected_mesh()->position()
                                 + camera()->up() * (selected_mesh()->boundingBox().height() / 2.f
                                                     + m_label->boundingBox().height())
                                 - m_label->boundingBox().center());
            m_label->setRotation(glm::mat3(camera()->transform()));
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
        else if(theProperty == m_shinyness)
        {
            m_mesh->material()->setShinyness(*m_shinyness);
        }
        else if(theProperty == m_modelPath)
        {
            try
            {
                gl::MeshPtr m = gl::AssimpConnector::loadModel(*m_modelPath);
                scene().removeObject(m_mesh);
                m_mesh = m;
                m->material()->setShinyness(*m_shinyness);
                m->material()->setSpecular(glm::vec4(1));
                scene().addObject(m_mesh);
            } catch (Exception &e){ LOG_ERROR<< e.what(); }
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
