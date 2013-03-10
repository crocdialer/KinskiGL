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
#include "Font.h"

namespace kinski { namespace gl {
    
    void copyArea(const uint8_t* src, uint8_t* dst,
                  const Area<uint32_t> &src_area, const Area<uint32_t> &dst_area)
    {
        uint32_t bytes_per_pixel = 1;
        uint32_t data_rows = 1024, data_cols = 1024;
        
        assert(src_area.x + src_area.width < data_cols && src_area.y + src_area.height < data_rows);
        assert(dst_area.x + dst_area.width < data_cols && dst_area.y + dst_area.height < data_rows);
        assert(src_area.width == dst_area.width && src_area.height == dst_area.height);
        
        uint32_t row_offset = data_cols - src_area.width;
        const uint8_t* src_area_start = src + (src_area.y * data_cols + src_area.x) * bytes_per_pixel;
        uint8_t* dst_area_start = dst + (dst_area.y * data_cols + dst_area.x) * bytes_per_pixel;
        
        for (int r = 0; r < src_area.height; r++)
        {
            const uint8_t* src_row_start = src_area_start + r * (src_area.width + row_offset) * bytes_per_pixel;
            uint8_t* dst_row_start = dst_area_start + r * (dst_area.width + row_offset) * bytes_per_pixel;
            for (int c = 0; c < src_area.width; c++)
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
    
    Texture Font::texture() const
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
    
    Texture Font::render_text(const std::string &theText)
    {
        Texture ret;
        float x = 0, y = 0;
        stbtt_aligned_quad q;
        uint8_t dst_data[m_obj->bitmap_width * m_obj->bitmap_height];
        std::fill(dst_data, dst_data + m_obj->bitmap_width * m_obj->bitmap_height, 0);
        
        std::string::const_iterator it = theText.begin();
        for (; it != theText.end(); ++it)
        {
            stbtt_GetBakedQuad(m_obj->char_data, m_obj->bitmap_width, m_obj->bitmap_height,
                               *it-32, &x, &y, &q, 1);
            int w = q.x1 - q.x0;
            int h = q.y1 - q.y0;
            
            Area<uint32_t> src = {  .x = q.s0 * (m_obj->bitmap_width - 1),
                                    .y = q.t0 * (m_obj->bitmap_height - 1),
                                    .width = w, .height = h };
            
            Area<uint32_t> dst = { .x = q.x0, .y = m_obj->font_height + q.y0, .width = w, .height = h };
            
            copyArea(m_obj->data, dst_data, src, dst);
        }
        ret.update(dst_data, GL_UNSIGNED_BYTE, GL_RED, m_obj->bitmap_width, m_obj->bitmap_height,
                   true);
        
//        /* now convert from stbtt_aligned_quad to source/dest SDL_Rects */
//        
//        /* width and height are simple */
//        
//        /* t0,s0 and t1,s1 are texture-space coordinates, that is floats from
//         * 0.0-1.0. we have to scale them back to the pixel space used in the
//         * glyph data bitmap. its a simple as multiplying by the glyph bitmap
//         * dimensions */

        return ret;
    }
    
}}// namespace