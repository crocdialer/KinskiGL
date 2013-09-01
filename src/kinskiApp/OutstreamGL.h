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
        void draw();
    
    private:
        
        explicit OutstreamGL(const gl::Font &the_font);
        
        gl::Font m_font;
        std::list<std::string> m_lines;
    };
    
    // This is the streambuffer; its function is to store formatted data and send
    // it to a character output when solicited (sync/overflow methods) . You do not
    // instantiate it by yourself on your application; it will be automatically used
    // by an actual output stream (like the TimestampLoggerStream class defined ahead)
    class StreamBufferGL : public std::streambuf
    {
    public:
        StreamBufferGL(OutstreamGL *ostreamGL, size_t buff_sz = 2048):
        m_outstreamGL(ostreamGL),
        m_buffer(buff_sz + 1)
        {
            //set putbase pointer and endput pointer
            char *base = &m_buffer[0];
            setp(base, base + buff_sz);
        }
    protected:
        
        // flush the characters in the buffer
        int flushBuffer ()
        {
            int num = pptr() - pbase();
            
            std::string out_string(pbase(), pptr());
            m_outstreamGL->lines().push_front(out_string);
            
//            if (write(1, &m_buffer[0], num) != num) {
//                return EOF;
//            }
            pbump(-num); // reset put pointer accordingly
            return num;
        }
        virtual int overflow ( int c = EOF )
        {
            if (c != EOF)
            {
                *pptr() = c;    // insert character into the buffer
                pbump(1);
            }
            if (flushBuffer() == EOF)
                return EOF;
            return c;
        }
        virtual int sync()
        {
            if (flushBuffer() == EOF)
                return -1;    // ERROR
            return 0;
        }
    private:
        OutstreamGL* m_outstreamGL;
        std::vector<char> m_buffer;
    };
}}//namespace

#endif /* defined(__kinskiGL__OutstreamGL__) */
