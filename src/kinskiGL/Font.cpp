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
    
    template<typename T>
    class Area
    {
    public:
        T x1, y1, x2, y2;
        
        //Area():x1(0), y1(0), x2(0), y2(0){};
        
        // does not seem to work in std::map
        bool operator<(const Area<T> &other) const
        {
            if ( x1 != other.x1 ) return x1 < other.x1;
            if ( y1 != other.y1 ) return y1 < other.y1;
            if ( x2 != other.x2 ) return x2 < other.x2;
            if ( y2 != other.y2 ) return y2 < other.y2;
            
            return false;
        }
        
        uint32_t width() const { return x2 - x1; };
        uint32_t height() const { return y2 - y1; };
    };
    
    struct MiniMat
    {
        uint8_t* data;
        uint32_t rows, cols;
        Area<uint32_t> roi;
        MiniMat(uint8_t* theData, uint32_t theRows, uint32_t theCols,
                const Area<uint32_t> &theRoi = Area<uint32_t>()):
        data(theData), rows(theRows), cols(theCols), roi(theRoi){};
    };
    
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
        
        for (int r = 0; r < src_mat.roi.height(); r++)
        {
            const uint8_t* src_row_start = src_area_start + r * (src_mat.roi.width() + src_row_offset) * bytes_per_pixel;
            uint8_t* dst_row_start = dst_area_start + r * (dst_mat.roi.width() + dst_row_offset) * bytes_per_pixel;
            for (int c = 0; c < src_mat.roi.width(); c++)
            {
                dst_row_start[c * bytes_per_pixel] = src_row_start[c * bytes_per_pixel];
            }
        }
    }
    
    struct Font::Obj
    {
        stbtt_bakedchar char_data[96];
        uint32_t font_height;
        uint32_t bitmap_width, bitmap_height;
        uint8_t* data;
        Texture texture;
        
        Obj():bitmap_width(1024),bitmap_height(1024)
        {
            data = new uint8_t[bitmap_width * bitmap_height];
            font_height = 64;
        }
        ~Obj()
        {
            delete[] data;
        }
    };
    
    
    Font::Font():m_obj(new Obj)
    {
        
    }
    
    Texture Font::glyph_texture() const
    {
        return m_obj->texture;
    }
    
    void Font::load(const std::string &thePath)
    {
        //TODO: check extension
        try
        {
            std::vector<uint8_t> font_file = kinski::readBinaryFile(thePath);
            
            stbtt_BakeFontBitmap(&font_file[0], stbtt_GetFontOffsetForIndex(&font_file[0], 0),
                                 m_obj->font_height, m_obj->data, m_obj->bitmap_width,
                                 m_obj->bitmap_height, 32, 96, m_obj->char_data);
            
            m_obj->texture.update(m_obj->data, GL_UNSIGNED_BYTE, GL_RED, m_obj->bitmap_width,
                                  m_obj->bitmap_height, true);
        } catch (const Exception &e)
        {
            LOG_ERROR<<e.what();
        } catch (...)
        {
            LOG_ERROR<<"Unknown Error loading Font: '"<<thePath<<"'";
        }
    }
    
    Texture Font::render_text(const std::string &theText) const
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
            //new line
            if(*it == 10)
            {
                x = 0;
                y += m_obj->font_height * 1.1;
            }
            
            stbtt_GetBakedQuad(m_obj->char_data, m_obj->bitmap_width, m_obj->bitmap_height,
                               *it-32, &x, &y, &q, 1);
            
            int w = q.x1 - q.x0;
            int h = q.y1 - q.y0;
            
            if(max_x < q.x1) max_x = q.x1;
            if(max_y < q.y1 + m_obj->font_height) max_y = q.y1 + m_obj->font_height;
            
            Area<uint32_t> src = {  .x1 = q.s0 * (m_obj->bitmap_width),
                                    .y1 = q.t0 * (m_obj->bitmap_height),
                                    .x2 = q.s0 * (m_obj->bitmap_width) + w,
                                    .y2 = q.t0 * (m_obj->bitmap_height) + h };
            Area<uint32_t> dst = {  .x1 = q.x0,
                                    .y1 = m_obj->font_height + q.y0,
                                    .x2 = q.x0 + w,
                                    .y2 = m_obj->font_height + q.y0 + h };

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
        uint8_t rgba_data[max_x * max_y * 4];
        uint8_t *dst_ptr = dst_data, *rgba_ptr = rgba_data, *rgba_end = rgba_data + max_x * max_y * 4;
        for (; rgba_ptr < rgba_end; rgba_ptr += 4, dst_ptr++)
        {
            rgba_ptr[0] = 255;
            rgba_ptr[1] = 255;
            rgba_ptr[2] = 255;
            rgba_ptr[3] = *dst_ptr;
        }
        
        ret.update(rgba_data, GL_UNSIGNED_BYTE, GL_RGBA, max_x, max_y, true);
        return ret;
    }
    
    gl::MeshPtr Font::draw_text(const std::string &theText) const
    {
        GeometryPtr geom(new gl::Geometry);
        geom->setPrimitiveType(GL_TRIANGLES);
        gl::MaterialPtr mat(new gl::Material);
        mat->addTexture(glyph_texture());
        MeshPtr ret (new gl::Mesh(geom, mat));
        
        std::vector<glm::vec3>& vertices = geom->vertices();
        std::vector<glm::vec2>& tex_coords = geom->texCoords();
        
        float x = 0, y = 0;
        stbtt_aligned_quad q;
        uint8_t dst_data[m_obj->bitmap_width * m_obj->bitmap_height];
        std::fill(dst_data, dst_data + m_obj->bitmap_width * m_obj->bitmap_height, 0);
        
        std::string::const_iterator it = theText.begin();
        for (; it != theText.end(); ++it)
        {
            //new line
            if(*it == 10)
            {
                x = 0;
                y += m_obj->font_height * 1.1;
            }
            
            stbtt_GetBakedQuad(m_obj->char_data, m_obj->bitmap_width, m_obj->bitmap_height,
                               *it-32, &x, &y, &q, 1);
            int w = q.x1 - q.x0;
            int h = q.y1 - q.y0;

            
            
            //vertices.push_back(<#const value_type &__x#>)
            
//            Area<uint32_t> src = {  .x = q.s0 * (m_obj->bitmap_width),
//                .y = q.t0 * (m_obj->bitmap_height),
//                .width = w, .height = h };
//            Area<uint32_t> dst = { .x = q.x0, .y = m_obj->font_height + q.y0, .width = w, .height = h };
//            copyArea(m_obj->data, dst_data, src, dst);
        }
       
        
        return ret;
    }
    
}}// namespace