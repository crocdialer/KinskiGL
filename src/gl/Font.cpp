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
#include "stb_rect_pack.h"

//#define STBTT_RASTERIZER_VERSION 1
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

// still causing trouble with gcc 5.4
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

#define BITMAP_WIDTH(font_sz) font_sz > 50 ? 2048 : 1024

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
    
    struct FontImpl
    {
        std::string path;
        std::unique_ptr<stbtt_packedchar[]> char_data;
        uint32_t font_height;
        uint32_t line_height;
        
        Image_<uint8_t >::Ptr bitmap;
        Texture texture, sdf_texture;
        bool use_sdf;
        
        // how many string meshes are buffered at max
        size_t max_mesh_buffer_size;
        std::unordered_map<std::string, string_mesh_container> string_mesh_map;
        
        FontImpl():
        max_mesh_buffer_size(500)
        {
            font_height = 64;
            line_height = 70;
            use_sdf = false;
        }
        
        std::tuple<Image_<uint8_t>::Ptr, std::unique_ptr<stbtt_packedchar[]>>
        create_bitmap(const std::vector<uint8_t> &the_font, float the_font_size,
                      uint32_t the_bitmap_width, uint32_t the_padding)
        {
            std::unique_ptr<stbtt_packedchar[]> c_data(new stbtt_packedchar[1024]);
            auto img = Image_<uint8_t>::create(the_bitmap_width, the_bitmap_width, 1);
            
            // rect packing
            stbtt_pack_context spc;
            stbtt_PackBegin(&spc, img->m_data, img->m_width, img->m_height, 0, the_padding, nullptr);
            
            int num_chars = 768;
            stbtt_PackFontRange(&spc, const_cast<uint8_t*>(the_font.data()), 0, the_font_size, 32,
                                num_chars, c_data.get());
            stbtt_PackEnd(&spc);
            return std::make_tuple(img, std::move(c_data));
        }
        
        std::list<stbtt_aligned_quad> create_quads(const std::string  &the_text,
                                                   uint32_t *the_max_x, uint32_t *the_max_y)
        {
            // workaround for weirdness in stb_truetype (blank 1st characters on line)
            constexpr float start_x = 0.5f;
            float x = start_x;
            float y = 0.f;
            stbtt_aligned_quad q;
            std::list<stbtt_aligned_quad> quads;
            
            // converts the codepoints to 16bit
            auto wstr = utf8_to_wstring(the_text);
            
            for (auto it = wstr.begin(); it != wstr.end(); ++it)
            {
                const auto &codepoint = *it;
                
                //new line
                if(codepoint == '\n')
                {
                    x = start_x;
                    y += line_height;
                    continue;
                }
                stbtt_GetPackedQuad(char_data.get(), bitmap->m_width, bitmap->m_height,
                                    codepoint - 32, &x, &y, &q, 0);
                
                if(the_max_y && *the_max_y < q.y1 + font_height){ *the_max_y = q.y1 + font_height;}
                if(the_max_x && *the_max_x < q.x1){ *the_max_x = q.x1; }
                quads.push_back(q);
            }
            return quads;
        }
    };
    
    
    Font::Font():m_impl(new FontImpl())
    {
        
    }
    
    const std::string Font::path() const
    {
        return m_impl->path;
    }
    
    Texture Font::glyph_texture() const
    {
        return m_impl->texture;
    }
    
    Texture Font::sdf_texture() const
    {
        return m_impl->sdf_texture;
    }
    
    uint32_t Font::font_size() const
    {
        return m_impl->font_height;
    }
    
    uint32_t Font::line_height() const
    {
        return m_impl->line_height;
    }
    
    void Font::set_line_height(uint32_t the_line_height)
    {
        m_impl->line_height = the_line_height;
    }
    
    bool Font::use_sdf() const{ return m_impl->use_sdf;}
    void Font::set_use_sdf(bool b){ m_impl->use_sdf = b; }
    
    void Font::load(const std::string &thePath, size_t theSize, bool use_sdf)
    {
        try
        {
            auto p = fs::search_file(thePath);
            std::vector<uint8_t> font_file = fs::read_binary_file(p);
            m_impl->path = p;
            m_impl->string_mesh_map.clear();
            m_impl->font_height = theSize;
            m_impl->line_height = theSize;
            m_impl->use_sdf = use_sdf;
            
            auto tuple = m_impl->create_bitmap(font_file, theSize, BITMAP_WIDTH(theSize),
                                               use_sdf ? 6 : 2);
            
            m_impl->bitmap = std::get<0>(tuple);
            m_impl->char_data = std::move(std::get<1>(tuple));
            
            // signed distance field
            if(use_sdf)
            {
                auto dist_img = compute_distance_field(m_impl->bitmap, 5);
                dist_img = dist_img->blur();
                m_impl->sdf_texture = create_texture_from_image(dist_img, true);
            }
            
#if defined(KINSKI_ARM)
            GLint tex_format = GL_LUMINANCE_ALPHA;
            
            // create data
            size_t num_bytes = m_impl->bitmap->width() * m_impl->bitmap->height() * 2;
            auto luminance_alpha_data = std::unique_ptr<uint8_t>(new uint8_t[num_bytes]);
            uint8_t *src_ptr = m_impl->bitmap->data();
            *out_ptr = luminance_alpha_data.get(), *data_end = luminance_alpha_data.get() + num_bytes;
            
            for (; out_ptr < data_end; out_ptr += 2, ++src_ptr)
            {
                out_ptr[0] = 255;
                out_ptr[1] = *src_ptr;
            }
            
            // create a new texture object for our glyphs
            gl::Texture::Format fmt;
            fmt.internal_format = tex_format;
            m_impl->texture = gl::Texture(luminance_alpha_data.get(), tex_format, m_impl->bitmap->width(),
                                         m_impl->bitmap->height(), fmt);
            m_impl->texture.set_flipped();
            m_impl->texture.set_mipmapping(true);
#else
            m_impl->texture = create_texture_from_image(m_impl->bitmap, true);
            m_impl->texture.set_swizzle(GL_ONE, GL_ONE, GL_ONE, GL_RED);
#endif
        } catch (const Exception &e)
        {
            LOG_ERROR<<e.what();
        } catch (const std::exception &e)
        {
            LOG_ERROR<<"Unknown error loading font '"<<thePath<<"': "<<e.what();
        }
    }
    
    gl::AABB Font::create_aabb(const std::string &theText) const
    {
        gl::AABB ret;
        uint32_t max_x = 0, max_y = 0;
        auto quads = m_impl->create_quads(theText, &max_x, &max_y);
        
        for(const auto &quad : quads)
        {
            ret += gl::AABB(gl::vec3(quad.x0, quad.y0, 0),
                            gl::vec3(quad.x1, quad.y1, 0));
        }
        return ret;
    }

    ImagePtr Font::create_image(const std::string &theText, const vec4 &theColor) const
    {
        uint32_t max_x = 0, max_y = 0;
        typedef std::list<std::pair<Area_<uint32_t>, Area_<uint32_t>>> Area_Pairs;
        Area_Pairs area_pairs;
        auto quads = m_impl->create_quads(theText, &max_x, &max_y);
        
        for(auto &q : quads)
        {
            Area_<uint32_t> src(static_cast<uint32_t>(q.s0 * m_impl->bitmap->m_width),
                                static_cast<uint32_t>(q.t0 * m_impl->bitmap->m_height),
                                static_cast<uint32_t>(q.s1 * m_impl->bitmap->m_width),
                                static_cast<uint32_t>(q.t1 * m_impl->bitmap->m_height));
            Area_<uint32_t> dst(static_cast<uint32_t>(q.x0),
                                static_cast<uint32_t>(m_impl->font_height + q.y0),
                                static_cast<uint32_t>(q.x1),
                                static_cast<uint32_t>(m_impl->font_height + q.y1));

            area_pairs.push_back(std::make_pair(src, dst));
        }
        auto dst_img = Image_<uint8_t >::create(max_x, max_y, 1);

        for(const auto &a : area_pairs)
        {
            m_impl->bitmap->m_roi = a.first;
            dst_img->m_roi = a.second;
            copy_image<uint8_t>(m_impl->bitmap, dst_img);
        }
        return dst_img;
    }

    Texture Font::create_texture(const std::string &theText, const glm::vec4 &theColor) const
    {
        Texture ret;
        auto img = create_image(theText, theColor);
#if defined(KINSKI_ARM)
        GLint tex_format = GL_LUMINANCE_ALPHA;
        
        // create data
        size_t num_bytes = img->width() * img->height() * 2;
        auto luminance_alpha_data = std::unique_ptr<uint8_t[]>(new uint8_t[num_bytes]);
        uint8_t *src_ptr = img->data();
        uint8_t *out_ptr = luminance_alpha_data.get(), *data_end = luminance_alpha_data.get() + num_bytes;
        
        for (; out_ptr < data_end; out_ptr += 2, ++src_ptr)
        {
            out_ptr[0] = 255;
            out_ptr[1] = *src_ptr;
        }
        
        // create a new texture object
        gl::Texture::Format fmt;
        fmt.internal_format = tex_format;
        ret = gl::Texture(luminance_alpha_data.get(), tex_format, img->width(), img->height(), fmt);
        ret.set_flipped();
#else
        ret = create_texture_from_image(img, false);
        ret.set_swizzle(GL_ONE, GL_ONE, GL_ONE, GL_RED);
#endif
        return ret;
    }
    
    gl::MeshPtr Font::create_mesh(const std::string &theText, const glm::vec4 &theColor) const
    {
        // look for an existing mesh
        auto mesh_iter = m_impl->string_mesh_map.find(theText);
        
        if(mesh_iter != m_impl->string_mesh_map.end())
        {
            mesh_iter->second.counter++;
            mesh_iter->second.mesh->set_transform(mat4());
            return mesh_iter->second.mesh;
        }
        
        // create a new mesh object
        GeometryPtr geom = Geometry::create();
        geom->set_primitive_type(GL_TRIANGLES);
        gl::MaterialPtr mat = gl::Material::create();
        mat->set_diffuse(theColor);
        mat->set_blending(true);
        
        if(m_impl->use_sdf)
        {
            mat->set_shader(gl::create_shader(ShaderType::SDF_FONT));
            mat->add_texture(m_impl->sdf_texture, Texture::Usage::COLOR);
            mat->uniform("u_buffer", 0.725f);
            mat->uniform("u_gamma", 0.05f);
        }
        else{ mat->add_texture(m_impl->texture, Texture::Usage::COLOR); }
        
        MeshPtr ret = gl::Mesh::create(geom, mat);
        ret->entries().clear();
        
        std::vector<glm::vec3>& vertices = geom->vertices();
        std::vector<glm::vec2>& tex_coords = geom->tex_coords();
        std::vector<glm::vec4>& colors = geom->colors();
        
        uint32_t max_y = 0;
        auto quads = m_impl->create_quads(theText, nullptr, &max_y);
        
        // reserve memory
        vertices.reserve(quads.size() * 4);
        tex_coords.reserve(quads.size() * 4);
        colors.reserve(quads.size() * 4);
        
        for (const auto &quad : quads)
        {
            int w = quad.x1 - quad.x0;
            int h = quad.y1 - quad.y0;

            Area_<float> tex_Area (quad.s0, 1 - quad.t0, quad.s1, 1 - quad.t1);
            Area_<uint32_t> vert_Area (static_cast<uint32_t>(quad.x0),
                                       static_cast<uint32_t>(max_y - (m_impl->font_height + quad.y0)),
                                       static_cast<uint32_t>(quad.x0 + w),
                                       static_cast<uint32_t>(max_y - (m_impl->font_height + quad.y0 + h)));
            
            // CREATE QUAD
            // create vertices
            vertices.push_back(glm::vec3(vert_Area.x0, vert_Area.y1, 0));
            vertices.push_back(glm::vec3(vert_Area.x1, vert_Area.y1, 0));
            vertices.push_back(glm::vec3(vert_Area.x1, vert_Area.y0, 0));
            vertices.push_back(glm::vec3(vert_Area.x0, vert_Area.y0, 0));
            
            // create texcoords
            tex_coords.push_back(glm::vec2(tex_Area.x0, tex_Area.y1));
            tex_coords.push_back(glm::vec2(tex_Area.x1, tex_Area.y1));
            tex_coords.push_back(glm::vec2(tex_Area.x1, tex_Area.y0));
            tex_coords.push_back(glm::vec2(tex_Area.x0, tex_Area.y0));
            
            // create colors
            for (int i = 0; i < 4; i++){ colors.push_back(glm::vec4(1)); }
        }
        for (uint32_t i = 0; i < vertices.size(); i += 4)
        {
            geom->append_face(i, i + 1, i + 2);
            geom->append_face(i, i + 2, i + 3);
        }
        geom->compute_face_normals();
        geom->compute_aabb();

        // free the less frequent used half of our buffered string-meshes
        if(m_impl->string_mesh_map.size() >= m_impl->max_mesh_buffer_size)
        {
            LOG_TRACE<<"font-mesh buffersize: " << m_impl->max_mesh_buffer_size << " -> clearing ...";
            std::list<string_mesh_container> tmp_list;

            for (auto &item : m_impl->string_mesh_map){tmp_list.push_back(item.second);}
            tmp_list.sort();
            
            std::list<string_mesh_container>::reverse_iterator list_it = tmp_list.rbegin();
            m_impl->string_mesh_map.clear();
            
            for (uint32_t i = 0; i < tmp_list.size() / 2; i++, ++list_it)
            {
                list_it->counter--;
                m_impl->string_mesh_map[list_it->text] = *list_it;
            }
        }
        // insert the newly created mesh
        m_impl->string_mesh_map[theText] = string_mesh_container(theText, ret);
        return ret;
    }

    gl::Object3DPtr Font::create_text_object(std::list<std::string> the_lines,
                                             Align the_align,
                                             uint32_t the_linewidth,
                                             uint32_t the_lineheight) const
    {
        if(!the_lineheight){ the_lineheight = line_height(); }
        gl::Object3DPtr root = gl::Object3D::create();
        auto parent = root;
        
        vec2 line_offset;
        bool reformat = false;
        
        for(auto it = the_lines.begin(); it != the_lines.end(); ++it)
        {
            if(!reformat){ parent = root; }
            string& l = *it;
            
            // center line_mesh
            auto line_aabb = create_aabb(l);
            
            reformat = false;
            auto insert_it = it;
            insert_it++;
            
            //split line, if necessary
            while(the_linewidth && (line_aabb.width() > the_linewidth))
            {
                size_t indx = l.find_last_of(' ');
                if(indx == string::npos){ break; }
                
                std::string last_word = l.substr(indx + 1);
                l = l.substr(0, indx);
                
                if(!reformat)
                {
                    parent = gl::Object3D::create();
                    root->add_child(parent);
                    reformat = true;
                    insert_it = the_lines.insert(insert_it, last_word);
                }
                else if(insert_it != the_lines.end())
                {
                    *insert_it = last_word + " " + *insert_it;
                }
                else{ the_lines.push_back(last_word); }
                
                // new aabb
                line_aabb = create_aabb(l);
            }
            
            switch(the_align)
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
            auto line_mesh = create_mesh(l)->copy();
            line_mesh->set_position(vec3(line_offset.x, line_offset.y - line_aabb.height(), 0.f));
            
            // advance offset
            line_offset.y -= line_height();
            
            // add line
            parent->add_child(line_mesh);
        }
        return root;
    }
    
    gl::Object3DPtr Font::create_text_object(const std::string &the_text,
                                             Align the_align,
                                             uint32_t the_linewidth,
                                             uint32_t the_lineheight) const
    {
        // create text meshes (1 per line)
        auto lines = split(the_text, '\n', false);
        return create_text_object(std::list<std::string>(lines.begin(), lines.end()),
                                  the_align, the_linewidth, the_lineheight);
    }
}}// namespace
