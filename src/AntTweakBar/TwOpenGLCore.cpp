//  ---------------------------------------------------------------------------
//
//  @file       TwOpenGLCore.cpp
//  @author     Philippe Decaudin - http://www.antisphere.com
//  @license    This file is part of the AntTweakBar library.
//              For conditions of distribution and use, see License.txt
//
//  note:       Work In Progress
//
//  ---------------------------------------------------------------------------

#define ANT_OGL_HEADER_INCLUDED ////
#include "TwPrecomp.h"

#define GL3_PROTOTYPES 1 ////

#if defined(ANT_OSX)
  #include <OpenGL/gl3.h>
#else
  #include <GL/gl.h>
#endif

#include "LoadOGLCore.h"
#include "TwOpenGLCore.h"
#include "TwMgr.h"



using namespace std;

extern const char *g_ErrCantLoadOGL;
extern const char *g_ErrCantUnloadOGL;

//  ---------------------------------------------------------------------------

#ifdef _DEBUG
    static void CheckGLCoreError(const char *file, int line, const char *func)
    {
        int err=0;
        char msg[256];
        while( (err=_glGetError())!=0 )
        {
            sprintf(msg, "%s(%d) : [%s] GL_CORE_ERROR=0x%x\n", file, line, func, err);
            #ifdef ANT_WINDOWS
                OutputDebugString(msg);
            #endif
            fprintf(stderr, "%s", msg);
        }
    }
#   ifdef __FUNCTION__
#       define CHECK_GL_ERROR CheckGLCoreError(__FILE__, __LINE__, __FUNCTION__)
#   else
#       define CHECK_GL_ERROR CheckGLCoreError(__FILE__, __LINE__, "")
#   endif
#else
#   define CHECK_GL_ERROR ((void)(0))
#endif

//  ---------------------------------------------------------------------------

static GLuint BindFont(const CTexFont *_Font)
{
    GLuint TexID = 0;

    int w = _Font->m_TexWidth;
    int h = _Font->m_TexHeight;
    color32 *font32 = new color32[w*h];
    color32 *p = font32;
    for( int i=0; i<w*h; ++i, ++p )
        *p = 0x00ffffff | (((color32)(_Font->m_TexBytes[i]))<<24);

    _glGenTextures(1, &TexID); CHECK_GL_ERROR;
    _glBindTexture(GL_TEXTURE_2D, TexID); CHECK_GL_ERROR;
    _glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_FALSE); CHECK_GL_ERROR;
    _glPixelStorei(GL_UNPACK_LSB_FIRST, GL_FALSE); CHECK_GL_ERROR;
    _glPixelStorei(GL_UNPACK_ROW_LENGTH, 0); CHECK_GL_ERROR;
    _glPixelStorei(GL_UNPACK_SKIP_ROWS, 0); CHECK_GL_ERROR;
    _glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0); CHECK_GL_ERROR;
    _glPixelStorei(GL_UNPACK_ALIGNMENT, 1); CHECK_GL_ERROR;
    _glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _Font->m_TexWidth, _Font->m_TexHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, font32); CHECK_GL_ERROR;
    _glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); CHECK_GL_ERROR;
    _glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); CHECK_GL_ERROR;
    _glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); CHECK_GL_ERROR;
    _glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); CHECK_GL_ERROR;
    _glBindTexture(GL_TEXTURE_2D, 0); CHECK_GL_ERROR;

    delete[] font32;

    return TexID;
}

static void UnbindFont(GLuint _FontTexID)
{
    if( _FontTexID>0 )
        _glDeleteTextures(1, &_FontTexID);
}

//  ---------------------------------------------------------------------------

static GLuint CompileShader(GLuint shader)
{
	_glCompileShader(shader); CHECK_GL_ERROR;

	GLint status;
	_glGetShaderiv (shader, GL_COMPILE_STATUS, &status); CHECK_GL_ERROR;
	if (status == GL_FALSE)
	{
		GLint infoLogLength;
		_glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength); CHECK_GL_ERROR;

		GLchar strInfoLog[256];
		_glGetShaderInfoLog(shader, sizeof(strInfoLog), NULL, strInfoLog); CHECK_GL_ERROR;
#ifdef ANT_WINDOWS
		OutputDebugString("Compile failure: ");
		OutputDebugString(strInfoLog);
		OutputDebugString("\n");
#endif
		fprintf(stderr, "Compile failure: %s\n", strInfoLog);
		shader = 0;
	}

	return shader;
}

static GLuint CreateProgram(GLuint vertShader, GLuint fragShader)
{
	GLuint program = _glCreateProgram(); CHECK_GL_ERROR;
	_glAttachShader(program, vertShader); CHECK_GL_ERROR;
	_glAttachShader(program, fragShader); CHECK_GL_ERROR;
	return program;
}

static GLuint LinkProgram(GLuint program)
{
    _glLinkProgram(program); CHECK_GL_ERROR;

    GLint status;
    _glGetProgramiv (program, GL_LINK_STATUS, &status); CHECK_GL_ERROR;
    if (status == GL_FALSE)
    {
        GLint infoLogLength;
        _glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength); CHECK_GL_ERROR;

        GLchar strInfoLog[256];
        _glGetProgramInfoLog(program, sizeof(strInfoLog), NULL, strInfoLog); CHECK_GL_ERROR;
#ifdef ANT_WINDOWS
		OutputDebugString("Linker failure: ");
		OutputDebugString(strInfoLog);
		OutputDebugString("\n");
#endif
        fprintf(stderr, "Linker failure: %s\n", strInfoLog);
        program = 0;
    }

    return program;
}

int CTwGraphOpenGLCore::Init()
{
    m_Drawing = false;
    m_FontTexID = 0;
    m_FontTex = NULL;

    if( LoadOpenGLCore()==0 )
    {
        g_TwMgr->SetLastError(g_ErrCantLoadOGL);
        return 0;
    }
    CHECK_GL_ERROR;

    // Create shaders
    const GLchar *lineRectVS[] = {
        "#version 150 core\n"
        "uniform vec2 offset = vec2(0.0f, 0.0f);"
        "in vec2 vertex;"
        "in vec4 color;"
        "out vec4 frag_color;"
        "void main() { gl_Position = vec4(vertex + offset, 0, 1); frag_color = color; }"
    };

    m_LineRectVS = _glCreateShader(GL_VERTEX_SHADER); CHECK_GL_ERROR;
    _glShaderSource(m_LineRectVS, 1, lineRectVS, NULL); CHECK_GL_ERROR;
	m_LineRectVS = CompileShader(m_LineRectVS);

    const GLchar *lineRectFS[] = {
        "#version 150 core\n"
        "in vec4 frag_color;"
        "out vec4 color;"
        "void main() { color = frag_color; }"
    };

    m_LineRectFS = _glCreateShader(GL_FRAGMENT_SHADER); CHECK_GL_ERROR;
    _glShaderSource(m_LineRectFS, 1, lineRectFS, NULL); CHECK_GL_ERROR;
	m_LineRectFS = CompileShader(m_LineRectFS);

	m_LineRectProgram = CreateProgram(m_LineRectVS, m_LineRectFS);
	_glBindAttribLocation(m_LineRectProgram, 0, "vertex"); CHECK_GL_ERROR;
	_glBindAttribLocation(m_LineRectProgram, 1, "color"); CHECK_GL_ERROR;
    m_LineRectProgram = LinkProgram(m_LineRectProgram);

    // Create shaders
    const GLchar *textVS[] = {
        "#version 150 core\n"
        "uniform vec2 offset = vec2(0.0f, 0.0f);"
        "uniform vec4 constant_color = vec4(0.0f, 0.0f, 0.0f, 0.0f);"
        "in vec2 vertex;"
        "in vec2 uv;"
        "in vec4 color;"
        "out vec4 frag_color;"
        "out vec2 frag_uv;"
        "void main() "
        "{ "
        "   gl_Position = vec4(vertex + offset, 0, 1);"
        "   frag_uv = uv;"
        "   if( constant_color.a != 0.0f)"
        "   {"
        "     frag_color = constant_color;"
        "   } else {"
        "     frag_color = color;"
        "   }"
        "}"
    };

    m_TextVS = _glCreateShader(GL_VERTEX_SHADER); CHECK_GL_ERROR;
    _glShaderSource(m_TextVS, 1, textVS, NULL); CHECK_GL_ERROR;
	m_TextVS = CompileShader(m_TextVS);

    const GLchar *textFS[] = {
        "#version 150 core\n"
        "uniform sampler2D diffuseTex;"
        "in vec4 frag_color;"
        "in vec2 frag_uv;"
        "out vec4 color;"
        "void main() { vec4 diffuse = texture(diffuseTex, frag_uv); color = frag_color * diffuse; }"
    };

    m_TextFS = _glCreateShader(GL_FRAGMENT_SHADER); CHECK_GL_ERROR;
    _glShaderSource(m_TextFS, 1, textFS, NULL); CHECK_GL_ERROR;
	m_TextFS = CompileShader(m_TextFS);

	m_TextProgram = CreateProgram(m_TextVS, m_TextFS);
	_glBindAttribLocation(m_TextProgram, 0, "vertex"); CHECK_GL_ERROR;
	_glBindAttribLocation(m_TextProgram, 1, "uv"); CHECK_GL_ERROR;
	_glBindAttribLocation(m_TextProgram, 2, "color"); CHECK_GL_ERROR;
    m_TextProgram = LinkProgram(m_TextProgram);

    // Create line/rect vertex buffer
    _glGenVertexArrays(1, &m_LineRectVArray); CHECK_GL_ERROR;
    _glBindVertexArray(m_LineRectVArray); CHECK_GL_ERROR;
    _glGenBuffers(1, &m_LineRectBuffer); CHECK_GL_ERROR;
    _glBindBuffer(GL_ARRAY_BUFFER, m_LineRectBuffer); CHECK_GL_ERROR;

    _glVertexAttribPointer(0, 2, GL_FLOAT, GL_TRUE, 6 * sizeof(float), NULL); CHECK_GL_ERROR;
    _glEnableVertexAttribArray(0); CHECK_GL_ERROR;

    _glVertexAttribPointer(1, 4, GL_FLOAT, GL_TRUE, 6 * sizeof(float), (const GLvoid*)(2 * sizeof(float))); CHECK_GL_ERROR;
    _glEnableVertexAttribArray(1); CHECK_GL_ERROR;
    _glBindVertexArray(0); CHECK_GL_ERROR;

    return 1;
}

//  ---------------------------------------------------------------------------

int CTwGraphOpenGLCore::Shut()
{
    assert(m_Drawing==false);

    UnbindFont(m_FontTexID);

    _glDeleteShader(m_LineRectVS); CHECK_GL_ERROR;
    _glDeleteShader(m_LineRectFS); CHECK_GL_ERROR;
    _glDeleteShader(m_TextVS); CHECK_GL_ERROR;
    _glDeleteShader(m_TextFS); CHECK_GL_ERROR;
    _glDeleteProgram(m_LineRectProgram); CHECK_GL_ERROR;
    _glDeleteProgram(m_TextProgram); CHECK_GL_ERROR;
    _glDeleteBuffers(1, &m_LineRectBuffer); CHECK_GL_ERROR;

    _glDeleteVertexArrays(1, &m_LineRectVArray); CHECK_GL_ERROR;

    int Res = 1;
    if( UnloadOpenGLCore()==0 )
    {
        g_TwMgr->SetLastError(g_ErrCantUnloadOGL);
        Res = 0;
    }

    return Res;
}

//  ---------------------------------------------------------------------------

void CTwGraphOpenGLCore::BeginDraw(int _WndWidth, int _WndHeight)
{
    assert(m_Drawing==false && _WndWidth>0 && _WndHeight>0);
    m_Drawing = true;
    m_WndWidth = _WndWidth;
    m_WndHeight = _WndHeight;
    m_OffsetX = 0;
    m_OffsetY = 0;

    _glGetIntegerv(GL_VIEWPORT, m_ViewportInit); CHECK_GL_ERROR;
    if( _WndWidth>0 && _WndHeight>0 )
    {
        GLint Vp[4];
        Vp[0] = 0;
        Vp[1] = 0;
        Vp[2] = _WndWidth-1;
        Vp[3] = _WndHeight-1;
        _glViewport(Vp[0], Vp[1], Vp[2], Vp[3]);
    }

    m_PrevVArray = 0;
    _glGetIntegerv(GL_VERTEX_ARRAY_BINDING, (GLint*)&m_PrevVArray); CHECK_GL_ERROR;
    _glBindVertexArray(0); CHECK_GL_ERROR;

    m_PrevLineWidth = 1;
    _glGetFloatv(GL_LINE_WIDTH, &m_PrevLineWidth); CHECK_GL_ERROR;
    _glLineWidth(1); CHECK_GL_ERROR;

    m_PrevLineSmooth = _glIsEnabled(GL_LINE_SMOOTH);
    _glDisable(GL_LINE_SMOOTH); CHECK_GL_ERROR;

    m_PrevCullFace = _glIsEnabled(GL_CULL_FACE);
    _glDisable(GL_CULL_FACE); CHECK_GL_ERROR;
    
    m_PrevDepthTest = _glIsEnabled(GL_DEPTH_TEST);
    _glDisable(GL_DEPTH_TEST); CHECK_GL_ERROR;

    m_PrevBlend = _glIsEnabled(GL_BLEND);
    _glEnable(GL_BLEND); CHECK_GL_ERROR;

    m_PrevScissorTest = _glIsEnabled(GL_SCISSOR_TEST);
    _glDisable(GL_SCISSOR_TEST); CHECK_GL_ERROR;

    _glGetIntegerv(GL_BLEND_SRC, &m_PrevSrcBlend); CHECK_GL_ERROR;
    _glGetIntegerv(GL_BLEND_DST, &m_PrevDstBlend); CHECK_GL_ERROR;
    _glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); CHECK_GL_ERROR;

    m_PrevTexture = 0;
    _glGetIntegerv(GL_TEXTURE_BINDING_2D, &m_PrevTexture); CHECK_GL_ERROR;
    _glBindTexture(GL_TEXTURE_2D, 0); CHECK_GL_ERROR;

    m_PrevProgramObject = 0;
    _glGetIntegerv(GL_CURRENT_PROGRAM, (GLint*)&m_PrevProgramObject); CHECK_GL_ERROR;
    _glBindVertexArray(0); CHECK_GL_ERROR;
    _glUseProgram(0); CHECK_GL_ERROR;  
}

//  ---------------------------------------------------------------------------

void CTwGraphOpenGLCore::EndDraw()
{
    assert(m_Drawing==true);
    m_Drawing = false;

    _glLineWidth(m_PrevLineWidth); CHECK_GL_ERROR;

    if( m_PrevLineSmooth )
    {
      _glEnable(GL_LINE_SMOOTH); CHECK_GL_ERROR;
    }
    else
    {
      _glDisable(GL_LINE_SMOOTH); CHECK_GL_ERROR;      
    }

    if( m_PrevCullFace )
    {
      _glEnable(GL_CULL_FACE); CHECK_GL_ERROR;
    }
    else
    {
      _glDisable(GL_CULL_FACE); CHECK_GL_ERROR;      
    }

    if( m_PrevDepthTest )
    {
      _glEnable(GL_DEPTH_TEST); CHECK_GL_ERROR;
    }
    else
    {
      _glDisable(GL_DEPTH_TEST); CHECK_GL_ERROR;      
    }

    if( m_PrevBlend )
    {
      _glEnable(GL_BLEND); CHECK_GL_ERROR;
    }
    else
    {
      _glDisable(GL_BLEND); CHECK_GL_ERROR;      
    }

    if( m_PrevScissorTest )
    {
      _glEnable(GL_SCISSOR_TEST); CHECK_GL_ERROR;
    }
    else
    {
      _glDisable(GL_SCISSOR_TEST); CHECK_GL_ERROR;      
    }

    _glBlendFunc(m_PrevSrcBlend, m_PrevDstBlend); CHECK_GL_ERROR;

    _glBindTexture(GL_TEXTURE_2D, m_PrevTexture); CHECK_GL_ERROR;

    _glUseProgram(m_PrevProgramObject); CHECK_GL_ERROR;
    
    _glBindVertexArray(m_PrevVArray); CHECK_GL_ERROR;

    RestoreViewport();
}

//  ---------------------------------------------------------------------------

bool CTwGraphOpenGLCore::IsDrawing()
{
    return m_Drawing;
}

//  ---------------------------------------------------------------------------

void CTwGraphOpenGLCore::Restore()
{
    UnbindFont(m_FontTexID);
    m_FontTexID = 0;
    m_FontTex = NULL;
}

//  ---------------------------------------------------------------------------

static inline float ToNormScreenX(int x, int wndWidth)
{
    return 2.0f*((float)x-0.5f)/wndWidth - 1.0f;
}

static inline float ToNormScreenY(int y, int wndHeight)
{
    return 1.0f - 2.0f*((float)y-0.5f)/wndHeight;
}

static inline color32 ToR8G8B8A8(color32 col)
{
    return (col & 0xff00ff00) | ((col>>16) & 0xff) | ((col<<16) & 0xff0000);
}

struct Color_4f
{
  Color_4f() : r(0.0f), g(0.0f), b(0.0f), a(0.0f) {}

  void PushToVector(std::vector<GLfloat>& vector)
  {
    vector.push_back(r);
    vector.push_back(g);
    vector.push_back(b);
    vector.push_back(a);  
  }

  float r, g, b, a;
};

static inline Color_4f ToRGBf(color32 col)
{
  Color_4f color;
  color.r = ((col >> 16) & 0xff) / 256.0f;
  color.g = ((col >> 8) & 0xff) / 256.0f;
  color.b = ((col >> 0) & 0xff) / 256.0f;
  color.a = ((col >> 24) & 0xff) / 256.0f;
  
  return color;
}

//  ---------------------------------------------------------------------------

void CTwGraphOpenGLCore::DrawLine(int _X0, int _Y0, int _X1, int _Y1, color32 _Color0, color32 _Color1, bool _AntiAliased)
{
    assert(m_Drawing==true);

    if( _AntiAliased )
    {
      _glEnable(GL_LINE_SMOOTH); CHECK_GL_ERROR;      
    }
    else
    {
      _glDisable(GL_LINE_SMOOTH); CHECK_GL_ERROR;      
    }

    GLfloat x0 = ToNormScreenX(_X0 + m_OffsetX, m_WndWidth);
    GLfloat y0 = ToNormScreenY(_Y0 + m_OffsetY, m_WndHeight);
    GLfloat x1 = ToNormScreenX(_X1 + m_OffsetX, m_WndWidth);
    GLfloat y1 = ToNormScreenY(_Y1 + m_OffsetY, m_WndHeight);
    Color_4f c0 = ToRGBf(_Color0);
    Color_4f c1 = ToRGBf(_Color1);

    GLfloat vertices[] = { x0, y0,  c0.r, c0.g, c0.b, c0.a,
                           x1, y1,  c1.r, c1.g, c1.b, c1.a,
                         };
    _glBindBuffer(GL_ARRAY_BUFFER, m_LineRectBuffer); CHECK_GL_ERROR;
    _glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW); CHECK_GL_ERROR;

    _glUseProgram(m_LineRectProgram); CHECK_GL_ERROR;
    _glBindVertexArray(m_LineRectVArray); CHECK_GL_ERROR;
    
    _glDrawArrays(GL_LINES, 0, 2); CHECK_GL_ERROR;

    _glDisable(GL_LINE_SMOOTH); CHECK_GL_ERROR;
    _glBindVertexArray(0);
    _glUseProgram(0); CHECK_GL_ERROR;  
}
  
//  ---------------------------------------------------------------------------

void CTwGraphOpenGLCore::DrawRect(int _X0, int _Y0, int _X1, int _Y1, color32 _Color00, color32 _Color10, color32 _Color01, color32 _Color11)
{
    assert(m_Drawing==true);

    // border adjustment
    if(_X0<_X1)
        ++_X1;
    else if(_X0>_X1)
        ++_X0;
    if(_Y0<_Y1)
        --_Y0;
    else if(_Y0>_Y1)
        --_Y1;

      GLfloat x0 = ToNormScreenX(_X0 + m_OffsetX, m_WndWidth);
      GLfloat y0 = ToNormScreenY(_Y0 + m_OffsetY, m_WndHeight);
      GLfloat x1 = ToNormScreenX(_X1 + m_OffsetX, m_WndWidth);
      GLfloat y1 = ToNormScreenY(_Y1 + m_OffsetY, m_WndHeight);
      Color_4f c00 = ToRGBf(_Color00);
      Color_4f c10 = ToRGBf(_Color10);
      Color_4f c01 = ToRGBf(_Color01);
      Color_4f c11 = ToRGBf(_Color11);
  
      GLfloat vertices[] = { x0, y0, c00.r, c00.g, c00.b, c00.a,
                             x1, y0, c10.r, c10.g, c10.b, c10.a,
                             x0, y1, c01.r, c01.g, c01.b, c01.a,
                             x1, y1, c11.r, c11.g, c11.b, c11.a
                           };


      _glBindBuffer(GL_ARRAY_BUFFER, m_LineRectBuffer); CHECK_GL_ERROR;
      _glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW); CHECK_GL_ERROR;

      _glUseProgram(m_LineRectProgram); CHECK_GL_ERROR;
      _glBindVertexArray(m_LineRectVArray); CHECK_GL_ERROR;

      _glDrawArrays(GL_TRIANGLE_STRIP, 0, 4 ); CHECK_GL_ERROR;
      _glBindVertexArray(0);
      _glUseProgram(0); CHECK_GL_ERROR;  
}

//  ---------------------------------------------------------------------------

void *CTwGraphOpenGLCore::NewTextObj()
{
    return new CTextObj;
}

//  ---------------------------------------------------------------------------

void CTwGraphOpenGLCore::DeleteTextObj(void *_TextObj)
{
    assert(_TextObj!=NULL);

    CTextObj* TextObj = static_cast<CTextObj *>(_TextObj);

    _glDeleteBuffers(1, &TextObj->m_Buffer); CHECK_GL_ERROR;
    _glDeleteBuffers(1, &TextObj->m_BgBuffer); CHECK_GL_ERROR;
    _glDeleteVertexArrays(1, &TextObj->m_VArray); CHECK_GL_ERROR;
    _glDeleteVertexArrays(1, &TextObj->m_BgVArray); CHECK_GL_ERROR;

    delete TextObj;
}

//  ---------------------------------------------------------------------------

void CTwGraphOpenGLCore::BuildText(void *_TextObj, const std::string *_TextLines, color32 *_LineColors, color32 *_LineBgColors, int _NbLines, const CTexFont *_Font, int _Sep, int _BgWidth)
{
    assert(m_Drawing==true);
    assert(_TextObj!=NULL);
    assert(_Font!=NULL);

    if( _Font != m_FontTex )
    {
        UnbindFont(m_FontTexID);
        m_FontTexID = BindFont(_Font);
        m_FontTex = _Font;
    }
    CTextObj *TextObj = static_cast<CTextObj *>(_TextObj);

    std::vector<GLfloat> textBuffer;
    textBuffer.reserve( _NbLines * 20 * 6);

    std::vector<GLfloat> bgBuffer;
    bgBuffer.reserve( _NbLines * 20 * 6);

    int x, x1, y, y1, i, Len;
    unsigned char ch;
    const unsigned char *Text;
    
    Color_4f LineColor;
    LineColor.r = LineColor.a = 1.0f;

    TextObj->m_VertCount = 0;
    TextObj->m_BgVertCount = 0;

    for( int Line=0; Line<_NbLines; ++Line )
    {
        x = 0;
        y = Line * (_Font->m_CharHeight+_Sep);
        y1 = y+_Font->m_CharHeight;
        Len = (int)_TextLines[Line].length();
        Text = (const unsigned char *)(_TextLines[Line].c_str());
        if( _LineColors!=NULL )
            LineColor = ToRGBf(_LineColors[Line]);

        TextObj->m_VertCount += Len * 6;
  
        for( i=0; i<Len; ++i )
        {
            ch = Text[i];
            x1 = x + _Font->m_CharWidth[ch];

            GLfloat normX = ToNormScreenX(x, m_WndWidth);
            GLfloat normY = ToNormScreenY(y, m_WndHeight);
            GLfloat normX1 = ToNormScreenX(x1, m_WndWidth);
            GLfloat normY1 = ToNormScreenY(y1, m_WndHeight);

            textBuffer.push_back(normX);  textBuffer.push_back(normY);
            textBuffer.push_back(_Font->m_CharU0[ch]); textBuffer.push_back(_Font->m_CharV0[ch]);
            LineColor.PushToVector(textBuffer);

            textBuffer.push_back(normX1); textBuffer.push_back(normY);
            textBuffer.push_back(_Font->m_CharU1[ch]); textBuffer.push_back(_Font->m_CharV0[ch]);
            LineColor.PushToVector(textBuffer);

            textBuffer.push_back(normX);  textBuffer.push_back(normY1);
            textBuffer.push_back(_Font->m_CharU0[ch]); textBuffer.push_back(_Font->m_CharV1[ch]);
            LineColor.PushToVector(textBuffer);

            textBuffer.push_back(normX1); textBuffer.push_back(normY);
            textBuffer.push_back(_Font->m_CharU1[ch]); textBuffer.push_back(_Font->m_CharV0[ch]);
            LineColor.PushToVector(textBuffer);

            textBuffer.push_back(normX1); textBuffer.push_back(normY1);
            textBuffer.push_back(_Font->m_CharU1[ch]); textBuffer.push_back(_Font->m_CharV1[ch]);
            LineColor.PushToVector(textBuffer);

            textBuffer.push_back(normX);  textBuffer.push_back(normY1);
            textBuffer.push_back(_Font->m_CharU0[ch]); textBuffer.push_back(_Font->m_CharV1[ch]);
            LineColor.PushToVector(textBuffer);

            x = x1;
        }
        if( _BgWidth>0 )
        {
          Color_4f LineBgColor;
          LineBgColor.g = LineBgColor.a = 1.0f;
          if( _LineBgColors!=NULL )
          {
              LineBgColor = ToRGBf(_LineBgColors[Line]);
          }

          TextObj->m_BgVertCount += 6;

          GLfloat normX = ToNormScreenX(-1, m_WndWidth);
          GLfloat normY = ToNormScreenY(y, m_WndHeight);
          GLfloat normX1 = ToNormScreenX(_BgWidth+1, m_WndWidth);
          GLfloat normY1 = ToNormScreenY(y1, m_WndHeight);

          bgBuffer.push_back(normX); bgBuffer.push_back(normY);
          LineBgColor.PushToVector(bgBuffer);

          bgBuffer.push_back(normX1); bgBuffer.push_back(normY);
          LineBgColor.PushToVector(bgBuffer);

          bgBuffer.push_back(normX); bgBuffer.push_back(normY1);
          LineBgColor.PushToVector(bgBuffer);

          bgBuffer.push_back(normX1); bgBuffer.push_back(normY);
          LineBgColor.PushToVector(bgBuffer);

          bgBuffer.push_back(normX1); bgBuffer.push_back(normY1);
          LineBgColor.PushToVector(bgBuffer);

          bgBuffer.push_back(normX); bgBuffer.push_back(normY1);
          LineBgColor.PushToVector(bgBuffer);
        }
    }
    
    if(!TextObj->m_VArray) _glGenVertexArrays(1, &TextObj->m_VArray); CHECK_GL_ERROR;
    _glBindVertexArray(TextObj->m_VArray); CHECK_GL_ERROR;
    
    if(!TextObj->m_Buffer) _glGenBuffers(1, &TextObj->m_Buffer); CHECK_GL_ERROR;
    _glBindBuffer(GL_ARRAY_BUFFER, TextObj->m_Buffer); CHECK_GL_ERROR;

	if (!textBuffer.empty())
		_glBufferData(GL_ARRAY_BUFFER, textBuffer.size() * sizeof(GLfloat), &textBuffer[0], GL_DYNAMIC_DRAW); CHECK_GL_ERROR;

    GLuint stride = 8 * sizeof(float);

    _glVertexAttribPointer(0, 2, GL_FLOAT, GL_TRUE, stride, NULL); CHECK_GL_ERROR;
    _glEnableVertexAttribArray(0); CHECK_GL_ERROR;

    _glVertexAttribPointer(1, 2, GL_FLOAT, GL_TRUE, stride, (const GLvoid*)(2 * sizeof(float))); CHECK_GL_ERROR;
    _glEnableVertexAttribArray(1); CHECK_GL_ERROR;

    _glVertexAttribPointer(2, 4, GL_FLOAT, GL_TRUE, stride, (const GLvoid*)(4 * sizeof(float))); CHECK_GL_ERROR;
    _glEnableVertexAttribArray(2); CHECK_GL_ERROR;

    if(!TextObj->m_BgVArray) _glGenVertexArrays(1, &TextObj->m_BgVArray); CHECK_GL_ERROR;
    _glBindVertexArray(TextObj->m_BgVArray); CHECK_GL_ERROR;
    
    if(!TextObj->m_BgBuffer) _glGenBuffers(1, &TextObj->m_BgBuffer); CHECK_GL_ERROR;
    _glBindBuffer(GL_ARRAY_BUFFER, TextObj->m_BgBuffer); CHECK_GL_ERROR;

	if (!bgBuffer.empty())
		_glBufferData(GL_ARRAY_BUFFER, bgBuffer.size() * sizeof(GLfloat), &bgBuffer[0], GL_DYNAMIC_DRAW); CHECK_GL_ERROR;

    stride = 6 * sizeof(float);

    _glVertexAttribPointer(0, 2, GL_FLOAT, GL_TRUE, stride, NULL); CHECK_GL_ERROR;
    _glEnableVertexAttribArray(0); CHECK_GL_ERROR;

    _glVertexAttribPointer(1, 4, GL_FLOAT, GL_TRUE, stride, (const GLvoid*)(2 * sizeof(float))); CHECK_GL_ERROR;
    _glEnableVertexAttribArray(1); CHECK_GL_ERROR;

    _glBindVertexArray(0);
    _glUseProgram(0); CHECK_GL_ERROR;  
}

//  ---------------------------------------------------------------------------

void CTwGraphOpenGLCore::DrawText(void *_TextObj, int _X, int _Y, color32 _Color, color32 _BgColor)
{
    assert(m_Drawing==true);
    assert(_TextObj!=NULL);
    CTextObj *TextObj = static_cast<CTextObj *>(_TextObj);

    if( TextObj->m_VertCount<4 && TextObj->m_BgVertCount<4 )
        return; // nothing to draw

    GLfloat x = 2.0f * _X / (float)m_WndWidth;
    GLfloat y = -2.0f * _Y / (float)m_WndHeight;

    _glUseProgram(m_LineRectProgram); CHECK_GL_ERROR;
    GLint offsetLoc = _glGetUniformLocation(m_LineRectProgram, "offset"); CHECK_GL_ERROR;
    _glUniform2f(offsetLoc, x, y); CHECK_GL_ERROR;
    _glBindVertexArray(TextObj->m_BgVArray); CHECK_GL_ERROR;
    
    _glDrawArrays(GL_TRIANGLES, 0, TextObj->m_BgVertCount ); CHECK_GL_ERROR;
    _glUniform2f(offsetLoc, 0.0f, 0.0f); CHECK_GL_ERROR;

    _glUseProgram(m_TextProgram); CHECK_GL_ERROR;

    offsetLoc = _glGetUniformLocation(m_TextProgram, "offset"); CHECK_GL_ERROR;
    _glUniform2f(offsetLoc, x, y); CHECK_GL_ERROR;

    offsetLoc = _glGetUniformLocation(m_TextProgram, "constant_color"); CHECK_GL_ERROR;
    Color_4f color = ToRGBf(_Color);
    _glUniform4f(offsetLoc, color.r, color.g, color.b, color.a); CHECK_GL_ERROR;


    _glBindVertexArray(TextObj->m_VArray); CHECK_GL_ERROR;
    
    _glBindTexture(GL_TEXTURE_2D, m_FontTexID); CHECK_GL_ERROR;
    _glDrawArrays(GL_TRIANGLES, 0, TextObj->m_VertCount ); CHECK_GL_ERROR;
    _glBindTexture(GL_TEXTURE_2D, 0); CHECK_GL_ERROR;

    _glBindVertexArray(0);
    _glUseProgram(0); CHECK_GL_ERROR;
}

//  ---------------------------------------------------------------------------

void CTwGraphOpenGLCore::ChangeViewport(int _X0, int _Y0, int _Width, int _Height, int _OffsetX, int _OffsetY)
{
  m_OffsetX = _X0 + _OffsetX;
  m_OffsetY = _Y0 + _OffsetY;
}

//  ---------------------------------------------------------------------------

void CTwGraphOpenGLCore::RestoreViewport()
{
    m_OffsetX = 0;
    m_OffsetY = 0;
    _glViewport(m_ViewportInit[0], m_ViewportInit[1], m_ViewportInit[2], m_ViewportInit[3]); CHECK_GL_ERROR;
}

void CTwGraphOpenGLCore::SetScissor(int _X0, int _Y0, int _Width, int _Height)
{
    if( _Width>0 && _Height>0 )
    {
        _glScissor(_X0-1, _Y0, _Width-1, _Height); CHECK_GL_ERROR;
        _glEnable(GL_SCISSOR_TEST); CHECK_GL_ERROR;
    }
    else
    {
        _glDisable(GL_SCISSOR_TEST); CHECK_GL_ERROR;
    }
}

//  ---------------------------------------------------------------------------

void CTwGraphOpenGLCore::DrawTriangles(int _NumTriangles, int *_Vertices, color32 *_Colors, Cull _CullMode)
{
    assert(m_Drawing==true);

    GLint prevCullFaceMode, prevFrontFace;
    _glGetIntegerv(GL_CULL_FACE_MODE, &prevCullFaceMode);
    _glGetIntegerv(GL_FRONT_FACE, &prevFrontFace);
    GLboolean prevCullEnable = _glIsEnabled(GL_CULL_FACE);
    _glCullFace(GL_BACK);
    _glEnable(GL_CULL_FACE);
    if( _CullMode==CULL_CW )
        _glFrontFace(GL_CCW);
    else if( _CullMode==CULL_CCW )
        _glFrontFace(GL_CW);
    else
        _glDisable(GL_CULL_FACE);

    size_t bufferSize = _NumTriangles * 3 * 6;
    GLfloat* vertices = new GLfloat[bufferSize];
    for( int i = 0; i < _NumTriangles * 3; ++i )
    {
      Color_4f color = ToRGBf(_Colors[i]);
      vertices[i * 6 + 0] = ToNormScreenX(_Vertices[2 * i + 0] + m_OffsetX, m_WndWidth);
      vertices[i * 6 + 1] = ToNormScreenY(_Vertices[2 * i + 1] + m_OffsetY, m_WndHeight);
      vertices[i * 6 + 2] = color.r;
      vertices[i * 6 + 3] = color.g;
      vertices[i * 6 + 4] = color.b;
      vertices[i * 6 + 5] = color.a;
    }

    _glBindBuffer(GL_ARRAY_BUFFER, m_LineRectBuffer); CHECK_GL_ERROR;
    _glBufferData(GL_ARRAY_BUFFER, bufferSize * sizeof(GLfloat), vertices, GL_DYNAMIC_DRAW); CHECK_GL_ERROR;

    _glUseProgram(m_LineRectProgram); CHECK_GL_ERROR;
    _glBindVertexArray(m_LineRectVArray); CHECK_GL_ERROR;

    _glDrawArrays(GL_TRIANGLES, 0, _NumTriangles * 3 ); CHECK_GL_ERROR;
    _glBindVertexArray(0);
    _glUseProgram(0); CHECK_GL_ERROR;  
    delete[] vertices;



    _glCullFace(prevCullFaceMode);
    _glFrontFace(prevFrontFace);
    if( prevCullEnable )
        _glEnable(GL_CULL_FACE);
    else
        _glDisable(GL_CULL_FACE);

}

//  ---------------------------------------------------------------------------
