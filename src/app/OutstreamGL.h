//
//  OutstreamGL.h
//  gl
//
//  Created by Fabian on 5/13/13.
//
//

#ifndef __gl__OutstreamGL__
#define __gl__OutstreamGL__

#include "gl/gl.h"
#include "gl/Font.h"

namespace kinski{ namespace gl{
    
    /* see
     * http://savingyoutime.wordpress.com/2009/04/21/using-c-stl-streambufostream-to-create-time-stamped-logging-class/
     * for references
     */
    
    class OutstreamGL : public std::ostream
    {
    public:
        
        explicit OutstreamGL(uint32_t max_lines = 10);
        explicit OutstreamGL(const gl::Font &the_font, uint32_t max_lines = 10);
        virtual ~OutstreamGL();
        
        const std::list<std::string>& lines() const {return m_lines;};
        uint32_t max_lines() const {return m_max_lines;}
        void set_max_lines(uint32_t ml){m_max_lines = ml;}
        
        const gl::Font& font() const {return m_font;};
        gl::Font& font() {return m_font;};
        void set_font(gl::Font &the_font){m_font = the_font;};
        
        const gl::Color& color() const {return m_color;}
        void set_color(const gl::Color &c){m_color = c;}
        
        void add_line(const std::string &line);
        void draw();
    
    private:
        
        gl::Font m_font;
        gl::Color m_color;
        uint32_t m_max_lines;
        std::list<std::string> m_lines;
    };
}}//namespace

#endif /* defined(__gl__OutstreamGL__) */
