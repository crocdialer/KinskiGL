// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

//  Font.cpp
//
//  Created by Fabian on 3/9/13.

#include "core/file_functions.hpp"
#include "Texture.hpp"
#include "Mesh.hpp"
#include "Font.hpp"

#define STB_RECT_PACK_IMPLEMENTATION
#include "stb_rect_pack.inl"
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.inl"

// no support in libstdc++ as of gcc 4.9
//#include <codecvt>

// -> fallback to boost/locale
#include <boost/locale/encoding_utf.hpp>

std::wstring utf8_to_wstring(const std::string& str)
{
    // the UTF-8 / UTF-16 standard conversion facet
//    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> utf16conv;
//    return utf16conv.from_bytes(str);
    
    return boost::locale::conv::utf_to_utf<wchar_t>(str.c_str(), str.c_str() + str.size());
}

//std::string wstring_to_utf8(const std::wstring& str)
//{
//    return utf_to_utf<char>(str.c_str(), str.c_str() + str.size());
//}

#if defined(KINSKI_RASPI)
    #define BITMAP_WIDTH 1024
#else 
    #define BITMAP_WIDTH 2048
#endif

namespace kinski { namespace gl {
    
    struct string_mesh_container
    {
        std::string text;
        MeshPtr mesh;
        uint64_t counter;
        string_mesh_container():counter(0){};
        string_mesh_container(const std::string &t, const MeshPtr &m):text(t), mesh(m), counter(0){}
        bool operator<(const string_mesh_container &other) const {return counter < other.counter;}
    };
    
    void copy_image(const ImagePtr &src_mat, ImagePtr &dst_mat)
    {
        uint32_t bytes_per_pixel = 1;
        
        assert(src_mat->roi.x2 <= src_mat->cols && src_mat->roi.y2 <= src_mat->rows);
        assert(dst_mat->roi.x2 <= dst_mat->cols && dst_mat->roi.y2 <= dst_mat->rows);
        assert(src_mat->roi.width() == src_mat->roi.width() && src_mat->roi.height() == src_mat->roi.height());
        
        uint32_t src_row_offset = src_mat->cols - src_mat->roi.width();
        uint32_t dst_row_offset = dst_mat->cols - dst_mat->roi.width();
        
        const uint8_t* src_area_start = src_mat->data + (src_mat->roi.y1 * src_mat->cols + src_mat->roi.x1) * bytes_per_pixel;
        uint8_t* dst_area_start = dst_mat->data + (dst_mat->roi.y1 * dst_mat->cols + dst_mat->roi.x1) * bytes_per_pixel;
        
        for (uint32_t r = 0; r < src_mat->roi.height(); r++)
        {
            const uint8_t* src_row_start = src_area_start + r * (src_mat->roi.width() + src_row_offset) * bytes_per_pixel;
            uint8_t* dst_row_start = dst_area_start + r * (dst_mat->roi.width() + dst_row_offset) * bytes_per_pixel;
            for (uint32_t c = 0; c < src_mat->roi.width(); c++)
            {
                dst_row_start[c * bytes_per_pixel] = src_row_start[c * bytes_per_pixel];
            }
        }
    }
    
    struct Font::Obj
    {
        std::string path;
//        stbtt_bakedchar char_data[1024];
        stbtt_packedchar char_data[1024];
        uint32_t font_height;
        uint32_t line_height;
        uint32_t bitmap_width, bitmap_height;
        uint8_t* data;
        Texture texture;
        
        // how many string meshes are buffered at max
        size_t max_mesh_buffer_size;
        std::unordered_map<std::string, string_mesh_container> string_mesh_map;
        
        Obj():bitmap_width(BITMAP_WIDTH), bitmap_height(BITMAP_WIDTH), max_mesh_buffer_size(500)
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
    
    const std::string Font::path() const
    {
        return m_obj->path;
    }
    
    Texture Font::glyph_texture() const
    {
        return m_obj->texture;
    }
    
    uint32_t Font::font_size() const
    {
        return m_obj->font_height;
    }
    
    uint32_t Font::line_height() const
    {
        return m_obj->line_height;
    }
    
    void Font::load(const std::string &thePath, size_t theSize, size_t line_height)
    {
        //TODO: check extension
        try
        {
            auto p = search_file(thePath);
            std::vector<uint8_t> font_file = kinski::read_binary_file(p);
            m_obj->path = p;
            m_obj->string_mesh_map.clear();
            m_obj->font_height = theSize;
            m_obj->line_height = line_height > 0 ? line_height : theSize;
            
            //TODO: rect pack here
            stbtt_pack_context spc;
            stbtt_PackBegin(&spc, m_obj->data, m_obj->bitmap_width,
                            m_obj->bitmap_height, 0, 1, nullptr);
            
//            stbtt_PackSetOversampling(&spc, 4, 4);//            -- for improved quality on small fonts
            int num_chars = 768;
            stbtt_PackFontRange(&spc, &font_file[0], 0,
                                m_obj->font_height, 32, num_chars, m_obj->char_data);
            
            stbtt_PackEnd(&spc);
            
//            stbtt_BakeFontBitmap(&font_file[0], stbtt_GetFontOffsetForIndex(&font_file[0], 0),
//                                 m_obj->font_height, m_obj->data, m_obj->bitmap_width,
//                                 m_obj->bitmap_height, 32, 768, m_obj->char_data);

            // create RGBA data
            size_t num_bytes = m_obj->bitmap_width * m_obj->bitmap_height * 4;
            uint8_t *rgba_data = new uint8_t[num_bytes];
            uint8_t *dst_ptr = m_obj->data, *rgba_ptr = rgba_data, *rgba_end = rgba_data + num_bytes;
            for (; rgba_ptr < rgba_end; rgba_ptr += 4, dst_ptr++)
            {
                rgba_ptr[0] = 255;
                rgba_ptr[1] = 255;
                rgba_ptr[2] = 255;
                rgba_ptr[3] = *dst_ptr;
            }
            
            // create a new texture object for our glyphs
            m_obj->texture = gl::Texture(rgba_data, GL_RGBA, m_obj->bitmap_width,
                                         m_obj->bitmap_height);
            m_obj->texture.setFlipped();
            m_obj->texture.set_mipmapping(true);
            delete [](rgba_data);
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
        
        // workaround for weirdness in stb_truetype (blank 1st characters on line)
        const float start_x = 0.f;
        
        float x = start_x, y = 0.f;
        uint32_t max_x = 0, max_y = 0;
        stbtt_aligned_quad q;
        typedef std::list< std::pair<Area<uint32_t>, Area<uint32_t> > > Area_Pairs;
        Area_Pairs area_pairs;
        
        auto wstr = utf8_to_wstring(theText);
        
        for (auto it = wstr.begin(); it != wstr.end(); ++it)
        {
            const auto &codepoint = *it;
            
            //new line
            if(codepoint == '\n')
            {
                x = start_x;
                y += m_obj->line_height;
                continue;
            }
            
//            stbtt_GetBakedQuad(m_obj->char_data, m_obj->bitmap_width, m_obj->bitmap_height,
//                               codepoint - 32, &x, &y, &q, 1);
            
            stbtt_GetPackedQuad(m_obj->char_data, m_obj->bitmap_width, m_obj->bitmap_height,
                                codepoint - 32, &x, &y, &q, 1);
            
            int w = q.x1 - q.x0;
            int h = q.y1 - q.y0;
            
            if(max_x < q.x1) max_x = q.x1;
            if(max_y < q.y1 + m_obj->font_height) max_y = q.y1 + m_obj->font_height;
            
            Area<uint32_t> src (static_cast<uint32_t>(q.s0 * m_obj->bitmap_width),
                                static_cast<uint32_t>(q.t0 * m_obj->bitmap_height),
                                static_cast<uint32_t>(q.s0 * m_obj->bitmap_width + w),
                                static_cast<uint32_t>(q.t0 * m_obj->bitmap_height + h));
            Area<uint32_t> dst (static_cast<uint32_t>(q.x0 - start_x),
                                static_cast<uint32_t>(m_obj->font_height + q.y0),
                                static_cast<uint32_t>(q.x0 + w - start_x),
                                static_cast<uint32_t>(m_obj->font_height + q.y0 + h));

            area_pairs.push_back(std::make_pair(src, dst));
        }
        uint8_t dst_data[max_x * max_y];
        std::fill(dst_data, dst_data + max_x * max_y, 0);
        
        auto src_mat = Image::create(m_obj->data, m_obj->bitmap_height, m_obj->bitmap_width);
        auto dst_mat = Image::create(dst_data, max_y, max_x);
        
        Area_Pairs::iterator area_it = area_pairs.begin();
        for (; area_it != area_pairs.end(); ++area_it)
        {
            src_mat->roi = area_it->first;
            dst_mat->roi = area_it->second;
            copy_image(src_mat, dst_mat);
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
        // look for an existing mesh
        auto mesh_iter = m_obj->string_mesh_map.find(theText);
        
        if(m_obj->string_mesh_map.find(theText) != m_obj->string_mesh_map.end())
        {
            mesh_iter->second.counter++;
            mesh_iter->second.mesh->setTransform(mat4());
            return mesh_iter->second.mesh;
        }
        
        // create a new mesh object
        GeometryPtr geom = Geometry::create();
        geom->setPrimitiveType(GL_TRIANGLES);
        gl::MaterialPtr mat = gl::Material::create();
        mat->setDiffuse(theColor);
        mat->addTexture(glyph_texture());
        mat->setBlending(true);
        mat->setTwoSided(true);
        MeshPtr ret = gl::Mesh::create(geom, mat);
        ret->entries().clear();
        
        std::vector<glm::vec3>& vertices = geom->vertices();
        std::vector<glm::vec2>& tex_coords = geom->texCoords();
        std::vector<glm::vec4>& colors = geom->colors();
        
        // workaround for weirdness in stb_truetype (blank 1st characters on line)
        const float start_x = 0.6f;
        
        float x = start_x, y = 0.f;
        uint32_t max_y = 0;
        stbtt_aligned_quad q;
        std::list<stbtt_aligned_quad> quads;
        
        auto wstr = utf8_to_wstring(theText);
        
        for (auto it = wstr.begin(); it != wstr.end(); ++it)
        {
            const auto &codepoint = *it;
            
            //new line
            if(codepoint == '\n')
            {
                x = start_x;
                y += m_obj->line_height;
                continue;
            }
            stbtt_GetPackedQuad(m_obj->char_data, m_obj->bitmap_width, m_obj->bitmap_height,
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
            Area<uint32_t> vert_Area (static_cast<uint32_t>(quad.x0 - start_x),
                                      static_cast<uint32_t>(max_y - (m_obj->font_height + quad.y0)),
                                      static_cast<uint32_t>(quad.x0 + w - start_x),
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
            for (int i = 0; i < 4; i++){ colors.push_back(glm::vec4(1)); }
        }
        for (uint32_t i = 0; i < vertices.size(); i += 4)
        {
            geom->appendFace(i, i + 1, i + 2);
            geom->appendFace(i, i + 2, i + 3);
        }
        geom->computeVertexNormals();
        geom->computeBoundingBox();
        
        // free the less frequent used half of our buffered string-meshes
        if(m_obj->string_mesh_map.size() >= m_obj->max_mesh_buffer_size)
        {
            LOG_TRACE<<"font-mesh buffersize: " << m_obj->max_mesh_buffer_size << " -> clearing ...";
            std::list<string_mesh_container> tmp_list;

            for (auto &item : m_obj->string_mesh_map){tmp_list.push_back(item.second);}
            tmp_list.sort();
            
            std::list<string_mesh_container>::reverse_iterator list_it = tmp_list.rbegin();
            m_obj->string_mesh_map.clear();
            
            for (uint32_t i = 0; i < tmp_list.size() / 2; i++, ++list_it)
            {
                list_it->counter--;
                m_obj->string_mesh_map[list_it->text] = *list_it;
            }
        }
        // insert the newly created mesh
        m_obj->string_mesh_map[theText] = string_mesh_container(theText, ret);
        
        return ret;
    }
    
    gl::Object3DPtr Font::create_text_obj(const std::string &the_text, float the_linewidth,
                                          Align the_align) const
    {
        gl::Object3DPtr ret = gl::Object3D::create();
        
        // create text meshes (1 per line)
        auto lines = split(the_text, '\n', false);
        
        // common material
//        gl::MaterialPtr mat = gl::Material::create();
//        mat->setDepthTest(false);
//        mat->setDepthWrite(false);
        
        vec2 line_offset;
        
        for(uint32_t i = 0; i < lines.size(); i++)
        {
            string l = lines[i];
            
            auto line_mesh = create_mesh(l)->copy();
            
            // center line_mesh
            auto line_aabb = line_mesh->boundingBox();
            
            //split line, if necessary
            while(line_aabb.width() > the_linewidth)
            {
                // split up line into words (seperated by ' ')
                auto words = split(l);
                
                if(words.size() < 2){ break; }
                //            line_mesh->material()->setDiffuse(gl::COLOR_DARK_RED);
                
                std::string last_word = words.back();
                words.pop_back();
                
                // rebuild string
                l.clear();
                
                for (auto &w : words){ l.append(w + " "); }
                
                // cut last ' ' char
                if(!l.empty()){ l = l.substr(0, l.size() - 1); }
                
                if(i + 1 < lines.size())
                {
                    lines[i + 1] = last_word + " " + lines[i + 1];
                }
                else{ lines.push_back(last_word); }
                
                // recreate cut mesh
                line_mesh = create_mesh(l)->copy();
                
                // new aabb
                line_aabb = line_mesh->boundingBox();
            }

            switch (the_align)
            {
                case Align::LEFT:
                    line_offset.x = 0.f;
                    break;
                case Align::CENTER:
                    line_offset.x = (the_linewidth - line_aabb.width()) / 2.f;
                    break;
                case Align::RIGHT:
                    line_offset.x = (the_linewidth - line_aabb.width());
                    break;
            }
            
            line_mesh->setPosition(vec3(line_offset.x, line_offset.y - line_aabb.height(), 0.f));
            
//            line_mesh->material() = mat;
            
            // advance offset
            line_offset.y -= line_height();
            
            if(!l.empty())
            { ret->add_child(line_mesh); }
        }
        
        return ret;
    }
    
}}// namespace