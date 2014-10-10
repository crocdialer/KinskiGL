//
//  Font.h
//  gl
//
//  Created by Fabian on 3/9/13.
//
//

#ifndef __gl__Font__
#define __gl__Font__

#include "KinskiGL.h"

namespace kinski { namespace gl {
    
    class KINSKI_API Font
    {
    public:
        Font();
        
        void load(const std::string &thePath, size_t theSize, size_t line_height = 0);
        Texture glyph_texture() const;
        Texture create_texture(const std::string &theText, const glm::vec4 &theColor = glm::vec4(1)) const;
        gl::MeshPtr create_mesh(const std::string &theText, const glm::vec4 &theColor = glm::vec4(1)) const;
        
        uint32_t getFontSize() const;
        uint32_t getLineHeight() const;
        
    private:
        // forward declared Implementation object
        struct Obj;
        typedef std::shared_ptr<Obj> ObjPtr;
        ObjPtr m_obj;
        
    public:
        //! Emulates shared_ptr-like behavior
        operator bool() const { return m_obj.get() != nullptr; }
        void reset() { m_obj.reset(); }
    };
    
}}// namespace

#endif /* defined(__gl__Font__) */
