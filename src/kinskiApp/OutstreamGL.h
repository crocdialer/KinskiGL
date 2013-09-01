//
//  OutstreamGL.h
//  kinskiGL
//
//  Created by Fabian on 5/13/13.
//
//

#ifndef __kinskiGL__OutstreamGL__
#define __kinskiGL__OutstreamGL__

#include "kinskiGL/KinskiGL.h"
#include "kinskiGL/Font.h"

namespace kinski{ namespace gl{
    
    class OutstreamGL;
    typedef std::shared_ptr<OutstreamGL> OutstreamGLPtr;
    
    /* see
     * http://savingyoutime.wordpress.com/2009/04/21/using-c-stl-streambufostream-to-create-time-stamped-logging-class/
     * for references
     */
    
    class OutstreamGL : public std::ostream
    {
    public:
        
        virtual ~OutstreamGL();
        
        static OutstreamGLPtr create(const gl::Font &the_font)
        {
            OutstreamGLPtr ret(new OutstreamGL(the_font));
            return ret;
        }
        
        std::list<std::string>& lines() {return m_lines;};
        const std::list<std::string>& lines() const {return m_lines;};
        uint32_t max_lines() const {return m_max_lines;}
        void set_max_lines(uint32_t ml){m_max_lines = ml;}
        void draw();
    
    private:
        
        explicit OutstreamGL(const gl::Font &the_font, uint32_t max_lines = 10);
        
        gl::Font m_font;
        uint32_t m_max_lines;
        std::list<std::string> m_lines;
    };
    
    // This is the streambuffer; its function is to store formatted data and send
    // it to a character output when solicited (sync/overflow methods) . You do not
    // instantiate it by yourself on your application; it will be automatically used
    // by an actual output stream (like the TimestampLoggerStream class defined ahead)
    class StreamBufferGL : public std::streambuf
    {
    public:
        StreamBufferGL(OutstreamGL *ostreamGL, size_t buff_sz = 2048);
        
    protected:
        
        // flush the characters in the buffer
        int flushBuffer ();
        virtual int overflow ( int c = EOF );
        virtual int sync();
        
    private:
        OutstreamGL* m_outstreamGL;
        std::vector<char> m_buffer;
    };
}}//namespace

#endif /* defined(__kinskiGL__OutstreamGL__) */
