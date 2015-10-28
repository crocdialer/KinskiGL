//
//  OpenNIConnector.h
//  gl
//
//  Created by Fabian on 5/5/13.
//
//

#ifndef __gl__OpenNIConnector__
#define __gl__OpenNIConnector__

#include <thread>
#include <mutex>
#include <condition_variable>
#include "gl/gl.h"
#include "core/Component.h"

// forward declare OpenNI stuff to avoid header inclusion
namespace xn
{
    class DepthMetaData;
    class SceneMetaData;
}

namespace kinski{ namespace gl{
    
    class OpenNIConnector : public kinski::Component
    {
    public:
        
        struct User
        {
            uint32_t id;
            glm::vec3 position;
            User(uint32_t theID, const glm::vec3 &thePos):id(theID), position(thePos){}
        };
        
        typedef std::shared_ptr<OpenNIConnector> Ptr;
        typedef std::vector<User> UserList;
        
        OpenNIConnector();
        ~OpenNIConnector();
        void updateProperty(const Property::ConstPtr &theProperty);
        
        void init();
        void start();
        void stop();
        
        inline bool has_new_frame() const {return m_new_frame;}
        
        //! thread runs here, do not fiddle around
        void run();
        
        UserList get_user_positions() const;
        gl::Texture get_depth_texture();
        
        void update_depth_buffer(gl::Buffer the_vertex_buf) const;
        
        const std::vector<gl::Color>& user_colors() const {return m_user_colors;};
        std::vector<gl::Color>& user_colors() {return m_user_colors;};
        
    private:
        
        void update_depth_texture(const xn::DepthMetaData& dmd, const xn::SceneMetaData& smd);
        
        mutable bool m_new_frame;
        std::vector<gl::Color> m_user_colors;
        
        struct Obj;
        typedef std::shared_ptr<Obj> ObjPtr;
        ObjPtr m_obj;
        
        //! Emulates shared_ptr-like behavior
        operator bool() const { return m_obj.get() != nullptr; }
        void reset() { m_obj.reset(); }
        
        const uint16_t* m_depth_data = nullptr;
        UserList m_user_list;
        gl::Texture m_depth_texture, m_depth_texture_raw;
        
        bool m_running;
        std::thread m_thread;
        mutable std::mutex m_mutex;
        std::condition_variable m_conditionVar;
        
        Property_<bool>::Ptr m_live_input;
        Property_<std::string>::Ptr m_config_path;
        Property_<std::string>::Ptr m_oni_path;
    };
    
    class OpenNIException: public Exception
    {
    public:
        OpenNIException(const std::string &theStr):
        Exception("OpenNI Exception: " + theStr){}
    };
    
}}//namespace

#endif /* defined(__gl__OpenNIConnector__) */
