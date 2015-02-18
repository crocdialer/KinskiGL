//
//  Renderer.h
//  gl
//
//  Created by Fabian on 4/21/13.
//
//

#ifndef __gl__Renderer__
#define __gl__Renderer__

#include "KinskiGL.h"
#include "Mesh.h"

namespace kinski{ namespace gl{
    
    struct KINSKI_API RenderBin
    {
        struct item
        {
            //! the items mesh
            gl::Mesh *mesh;
            //! the items transform in eye-coords
            glm::mat4 transform;
        };
        
        struct light
        {
            //! a lightsource
            gl::Light *light;
            //! the light´s transform in eye-coords
            glm::mat4 transform;
        };
        
        struct sort_items_increasing
        {
            inline bool operator()(const item &lhs, const item &rhs)
            {return lhs.transform[3].z > rhs.transform[3].z;}
        };
        
        struct sort_items_decreasing
        {
            inline bool operator()(const item &lhs, const item &rhs)
            {return lhs.transform[3].z < rhs.transform[3].z;}
        };
        
        RenderBin(const CameraPtr &cam): camera(cam){};
        CameraPtr camera;
        std::list<item> items;
        std::list<light> lights;
    };
    
    class KINSKI_API Renderer
    {
    public:
        
        typedef std::shared_ptr<Renderer> Ptr;
        typedef std::shared_ptr<const Renderer> ConstPtr;
        
        enum UniformBufferIndex {LIGHT_UNIFORM_BUFFER = 0};
        enum UniformBlockBinding {MATERIAL_BLOCK = 0, LIGHT_BLOCK = 1};
        
        Renderer();
        virtual ~Renderer(){};
        
        void render(const RenderBinPtr &theBin);
        
        void set_light_uniforms(MaterialPtr &the_mat, const std::list<RenderBin::light> &light_list);
        void update_uniform_buffers(const std::list<RenderBin::light> &light_list);
        
    private:
        void draw_sorted_by_material(const CameraPtr &cam, const std::list<RenderBin::item> &item_list,
                                     const std::list<RenderBin::light> &light_list);
        
        gl::Buffer m_uniform_buffer[1];
        
    };
    
}}// namespace

#endif /* defined(__gl__Renderer__) */