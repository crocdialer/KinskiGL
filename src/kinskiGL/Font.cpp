//
//  Font.cpp
//  kinskiGL
//
//  Created by Fabian on 3/9/13.
//
//

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"
#include "Texture.h"
#include "Mesh.h"
#include "Font.h"

namespace kinski { namespace gl {
    
    void copyMat(const MiniMat &src_mat, MiniMat &dst_mat)
    {
        uint32_t bytes_per_pixel = 1;
        
        assert(src_mat.roi.x2 <= src_mat.cols && src_mat.roi.y2 <= src_mat.rows);
        assert(dst_mat.roi.x2 <= dst_mat.cols && dst_mat.roi.y2 <= dst_mat.rows);
        assert(src_mat.roi.width() == src_mat.roi.width() && src_mat.roi.height() == src_mat.roi.height());
        
        uint32_t src_row_offset = src_mat.cols - src_mat.roi.width();
        uint32_t dst_row_offset = dst_mat.cols - dst_mat.roi.width();
        
        const uint8_t* src_area_start = src_mat.data + (src_mat.roi.y1 * src_mat.cols + src_mat.roi.x1) * bytes_per_pixel;
        uint8_t* dst_area_start = dst_mat.data + (dst_mat.roi.y1 * dst_mat.cols + dst_mat.roi.x1) * bytes_per_pixel;
        
        for (uint32_t r = 0; r < src_mat.roi.height(); r++)
        {
            const uint8_t* src_row_start = src_area_start + r * (src_mat.roi.width() + src_row_offset) * bytes_per_pixel;
            uint8_t* dst_row_start = dst_area_start + r * (dst_mat.roi.width() + dst_row_offset) * bytes_per_pixel;
            for (uint32_t c = 0; c < src_mat.roi.width(); c++)
            {
                dst_row_start[c * bytes_per_pixel] = src_row_start[c * bytes_per_pixel];
            }
        }
    }
    
    // Copyright (c) 2008-2009 Bjoern Hoehrmann <bjoern@hoehrmann.de>
    // See http://bjoern.hoehrmann.de/utf-8/decoder/dfa/ for details.
    
#define UTF8_ACCEPT 0
#define UTF8_REJECT 1
    
    uint32_t inline decode(uint32_t* state, uint32_t* codep, uint32_t byte)
    {
        static const uint8_t utf8d[] = {
            0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 00..1f
            0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 20..3f
            0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 40..5f
            0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 60..7f
            1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9, // 80..9f
            7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7, // a0..bf
            8,8,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, // c0..df
            0xa,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x4,0x3,0x3, // e0..ef
            0xb,0x6,0x6,0x6,0x5,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8, // f0..ff
            0x0,0x1,0x2,0x3,0x5,0x8,0x7,0x1,0x1,0x1,0x4,0x6,0x1,0x1,0x1,0x1, // s0..s0
            1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,0,1,0,1,1,1,1,1,1, // s1..s2
            1,2,1,1,1,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1, // s3..s4
            1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,3,1,3,1,1,1,1,1,1, // s5..s6
            1,3,1,1,1,1,1,3,1,3,1,1,1,1,1,1,1,3,1,1,1,1,1,1,1,1,1,1,1,1,1,1, // s7..s8
        };
        
        uint32_t type = utf8d[byte];
        *codep = (*state != UTF8_ACCEPT) ?
        (byte & 0x3fu) | (*codep << 6) :
        (0xff >> type) & (byte);
        *state = utf8d[256 + *state*16 + type];
        return *state;
    }
    
    struct Font::Obj
    {
        stbtt_bakedchar char_data[1024];
        uint32_t font_height;
        uint32_t line_height;
        uint32_t bitmap_width, bitmap_height;
        uint8_t* data;
        Texture texture;
        
        Obj():bitmap_width(1024),bitmap_height(1024)
        {
            data = new uint8_t[bitmap_width * bitmap_height];
            font_height = 64;
            line_height = 70;
        }
        ~Obj()
        {
            delete[] data;
        }
    };
    
    
    Font::Font():m_obj(new Obj())
    {
        
    }
    
    Texture Font::glyph_texture() const
    {
        return m_obj->texture;
    }
    
    uint32_t Font::getFontSize() const
    {
        return m_obj->font_height;
    }
    
    uint32_t Font::getLineHeight() const
    {
        return m_obj->line_height;
    }
    
    void Font::load(const std::string &thePath, size_t theSize)
    {
        //TODO: check extension
        try
        {
            m_obj->font_height = theSize;
            m_obj->line_height = theSize * 1.1;
            std::vector<uint8_t> font_file = kinski::readBinaryFile(thePath);
            stbtt_BakeFontBitmap(&font_file[0], stbtt_GetFontOffsetForIndex(&font_file[0], 0),
                                 m_obj->font_height, m_obj->data, m_obj->bitmap_width,
                                 m_obj->bitmap_height, 32, 768, m_obj->char_data);

            // create RGBA data
            size_t num_bytes = m_obj->bitmap_width * m_obj->bitmap_height * 4;
            uint8_t rgba_data[num_bytes];
            uint8_t *dst_ptr = m_obj->data, *rgba_ptr = rgba_data, *rgba_end = rgba_data + num_bytes;
            for (; rgba_ptr < rgba_end; rgba_ptr += 4, dst_ptr++)
            {
                rgba_ptr[0] = 255;
                rgba_ptr[1] = 255;
                rgba_ptr[2] = 255;
                rgba_ptr[3] = *dst_ptr;
            }
            
            m_obj->texture.update(rgba_data, GL_UNSIGNED_BYTE, GL_RGBA, m_obj->bitmap_width,
                                  m_obj->bitmap_height, true);
            m_obj->texture.set_mipmapping(true);
            
        } catch (const Exception &e)
        {
            LOG_ERROR<<e.what();
        } catch (const std::exception &e)
        {
            LOG_ERROR<<"Unknown error loading font '"<<thePath<<"': "<<e.what();
        }
    }
    
    Texture Font::create_texture(const std::string &theText, const glm::vec4 &theColor) const
    {
        Texture ret;
        float x = 0, y = 0;
        uint32_t max_x = 0, max_y = 0;
        stbtt_aligned_quad q;
        typedef std::list< std::pair<Area<uint32_t>, Area<uint32_t> > > Area_Pairs;
        Area_Pairs area_pairs;
        
        std::string::const_iterator it = theText.begin();
        for (; it != theText.end(); ++it)
        {
            uint32_t codepoint = 0;
            uint32_t state = UTF8_ACCEPT;
            
            while (decode(&state, &codepoint, (uint8_t)*it)){ ++it; }
            
            //new line
            if(codepoint == 10)
            {
                x = 0;
                y += m_obj->font_height * 1.1;
            }
            
            stbtt_GetBakedQuad(m_obj->char_data, m_obj->bitmap_width, m_obj->bitmap_height,
                               codepoint-32, &x, &y, &q, 1);
            
            int w = q.x1 - q.x0;
            int h = q.y1 - q.y0;
            
            if(max_x < q.x1) max_x = q.x1;
            if(max_y < q.y1 + m_obj->font_height) max_y = q.y1 + m_obj->font_height;
            
            Area<uint32_t> src (static_cast<uint32_t>(q.s0 * m_obj->bitmap_width),
                                static_cast<uint32_t>(q.t0 * m_obj->bitmap_height),
                                static_cast<uint32_t>(q.s0 * m_obj->bitmap_width + w),
                                static_cast<uint32_t>(q.t0 * m_obj->bitmap_height + h));
            Area<uint32_t> dst (static_cast<uint32_t>(q.x0),
                                static_cast<uint32_t>(m_obj->font_height + q.y0),
                                static_cast<uint32_t>(q.x0 + w),
                                static_cast<uint32_t>(m_obj->font_height + q.y0 + h));

            area_pairs.push_back(std::make_pair(src, dst));
        }
        uint8_t dst_data[max_x * max_y];
        std::fill(dst_data, dst_data + max_x * max_y, 0);
        
        MiniMat src_mat(m_obj->data, m_obj->bitmap_height, m_obj->bitmap_width);
        MiniMat dst_mat(dst_data, max_y, max_x);
        
        Area_Pairs::iterator area_it = area_pairs.begin();
        for (; area_it != area_pairs.end(); ++area_it)
        {
            src_mat.roi = area_it->first;
            dst_mat.roi = area_it->second;
            copyMat(src_mat, dst_mat);
        }
        
        // create RGBA data
        uint8_t r = 255 * theColor.r, g = 255 * theColor.g, b = 255 * theColor.b;
        uint8_t rgba_data[max_x * max_y * 4];
        uint8_t *dst_ptr = dst_data, *rgba_ptr = rgba_data, *rgba_end = rgba_data + max_x * max_y * 4;
        for (; rgba_ptr < rgba_end; rgba_ptr += 4, dst_ptr++)
        {
            rgba_ptr[0] = r;
            rgba_ptr[1] = g;
            rgba_ptr[2] = b;
            rgba_ptr[3] = *dst_ptr;
        }
        
        ret.update(rgba_data, GL_UNSIGNED_BYTE, GL_RGBA, max_x, max_y, true);
        return ret;
    }
    
    gl::MeshPtr Font::create_mesh(const std::string &theText, const glm::vec4 &theColor) const
    {
        GeometryPtr geom = Geometry::create();
        geom->setPrimitiveType(GL_TRIANGLES);
        gl::MaterialPtr mat(new gl::Material);
        mat->setDiffuse(theColor);
        mat->addTexture(glyph_texture());
        mat->setBlending(true);
        mat->setTwoSided(true);
        MeshPtr ret = gl::Mesh::create(geom, mat);
        ret->entries().clear();
        
        std::vector<glm::vec3>& vertices = geom->vertices();
        std::vector<glm::vec2>& tex_coords = geom->texCoords();
        std::vector<glm::vec4>& colors = geom->colors();
        
        float x = 0, y = 0;
        uint32_t max_y = 0;
        stbtt_aligned_quad q;
        std::list<stbtt_aligned_quad> quads;
   
        std::string::const_iterator it = theText.begin();
        for (; it != theText.end(); ++it)
        {
            uint32_t codepoint = 0;
            uint32_t state = UTF8_ACCEPT;
            
            while (decode(&state, &codepoint, (uint8_t)*it)){ ++it; }
            
            //new line
            if(codepoint == 10)
            {
                x = 0;
                y += m_obj->line_height;
            }
            stbtt_GetBakedQuad(m_obj->char_data, m_obj->bitmap_width, m_obj->bitmap_height,
                               codepoint - 32, &x, &y, &q, 1);
            
            if(max_y < q.y1 + m_obj->font_height){ max_y = q.y1 + m_obj->font_height;}
            quads.push_back(q);
        }
        
        // reserve memory
        vertices.reserve(quads.size() * 4);
        tex_coords.reserve(quads.size() * 4);
        colors.reserve(quads.size() * 4);
        
        std::list<stbtt_aligned_quad>::const_iterator quad_it = quads.begin();
        for (; quad_it != quads.end(); ++quad_it)
        {
            const stbtt_aligned_quad &quad = *quad_it;
            
            int w = quad.x1 - quad.x0;
            int h = quad.y1 - quad.y0;

            Area<float> tex_Area (quad.s0, 1 - quad.t0, quad.s1, 1 - quad.t1);
            Area<uint32_t> vert_Area (static_cast<uint32_t>(quad.x0),
                                      static_cast<uint32_t>(max_y - (m_obj->font_height + quad.y0)),
                                      static_cast<uint32_t>(quad.x0 + w),
                                      static_cast<uint32_t>(max_y - (m_obj->font_height + quad.y0 + h)));
            
            // CREATE QUAD
            // create vertices
            vertices.push_back(glm::vec3(vert_Area.x1, vert_Area.y2, 0));
            vertices.push_back(glm::vec3(vert_Area.x2, vert_Area.y2, 0));
            vertices.push_back(glm::vec3(vert_Area.x2, vert_Area.y1, 0));
            vertices.push_back(glm::vec3(vert_Area.x1, vert_Area.y1, 0));
            
            // create texcoords
            tex_coords.push_back(glm::vec2(tex_Area.x1, tex_Area.y2));
            tex_coords.push_back(glm::vec2(tex_Area.x2, tex_Area.y2));
            tex_coords.push_back(glm::vec2(tex_Area.x2, tex_Area.y1));
            tex_coords.push_back(glm::vec2(tex_Area.x1, tex_Area.y1));
            
            // create colors
            for (int i = 0; i < 4; i++)
            {
                colors.push_back(glm::vec4(1));
            }
        }
        for (uint32_t i = 0; i < vertices.size(); i += 4)
        {
            geom->appendFace(i, i + 1, i + 2);
            geom->appendFace(i, i + 2, i + 3);
        }
        geom->computeVertexNormals();
        geom->computeBoundingBox();
        return ret;
    }
    
}}// namespace