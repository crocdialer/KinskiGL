//  ---------------------------------------------------------------------------
//
//  @file       TwOpenGLCore.h
//  @brief      OpenGL Core graph functions
//  @author     Philippe Decaudin - http://www.antisphere.com
//  @license    This file is part of the AntTweakBar library.
//              For conditions of distribution and use, see License.txt
//
//  notes:      Private header,
//              Work In Progress, Disabled.
//
//  ---------------------------------------------------------------------------


#if !defined ANT_TW_OPENGL_CORE_INCLUDED
#define ANT_TW_OPENGL_CORE_INCLUDED

#include "TwGraph.h"

//  ---------------------------------------------------------------------------

class CTwGraphOpenGLCore : public ITwGraph
{
public:
    virtual int         Init();
    virtual int         Shut();
    virtual void        BeginDraw(int _WndWidth, int _WndHeight);
    virtual void        EndDraw();
    virtual bool        IsDrawing();
    virtual void        Restore();
    virtual void        DrawLine(int _X0, int _Y0, int _X1, int _Y1, color32 _Color0, color32 _Color1, bool _AntiAliased=false);
    virtual void        DrawLine(int _X0, int _Y0, int _X1, int _Y1, color32 _Color, bool _AntiAliased=false) { DrawLine(_X0, _Y0, _X1, _Y1, _Color, _Color, _AntiAliased); }
    virtual void        DrawRect(int _X0, int _Y0, int _X1, int _Y1, color32 _Color00, color32 _Color10, color32 _Color01, color32 _Color11);
    virtual void        DrawRect(int _X0, int _Y0, int _X1, int _Y1, color32 _Color) { DrawRect(_X0, _Y0, _X1, _Y1, _Color, _Color, _Color, _Color); }
    virtual void        DrawTriangles(int _NumTriangles, int *_Vertices, color32 *_Colors, Cull _CullMode);

    virtual void *      NewTextObj();
    virtual void        DeleteTextObj(void *_TextObj);
    virtual void        BuildText(void *_TextObj, const std::string *_TextLines, color32 *_LineColors, color32 *_LineBgColors, int _NbLines, const CTexFont *_Font, int _Sep, int _BgWidth);
    virtual void        DrawText(void *_TextObj, int _X, int _Y, color32 _Color, color32 _BgColor);

    virtual void        ChangeViewport(int _X0, int _Y0, int _Width, int _Height, int _OffsetX, int _OffsetY);
    virtual void        RestoreViewport();
    virtual void        SetScissor(int _X0, int _Y0, int _Width, int _Height);

protected:
    bool                m_Drawing;
    GLuint              m_FontTexID;
    const CTexFont *    m_FontTex;
    
    GLboolean           m_PrevLineSmooth;
    GLboolean           m_PrevCullFace;
    GLboolean           m_PrevDepthTest;
    GLboolean           m_PrevBlend;
    GLboolean           m_PrevScissorTest;
    GLint               m_PrevSrcBlend;
    GLint               m_PrevDstBlend;

    GLuint               m_PrevVArray;

    GLfloat             m_PrevLineWidth;
    GLint               m_PrevTexture;
    GLuint              m_PrevProgramObject;
    GLint               m_ViewportInit[4];
    GLuint              m_LineRectVS;
    GLuint              m_LineRectFS;
    GLuint              m_LineRectProgram;
    GLuint              m_LineRectVArray;
    GLuint              m_LineRectBuffer;

    GLuint              m_TextVS;
    GLuint              m_TextFS;
    GLuint              m_TextProgram;

    int                 m_WndWidth;
    int                 m_WndHeight;
    int                 m_OffsetX;
    int                 m_OffsetY;

    struct Vec2         { GLfloat x, y; Vec2(){} Vec2(GLfloat _X, GLfloat _Y):x(_X),y(_Y){} Vec2(int _X, int _Y):x(GLfloat(_X)),y(GLfloat(_Y)){} };
    struct CTextObj
    {
        GLuint               m_VArray;
        GLuint               m_Buffer;
        GLuint               m_BgVArray;
        GLuint               m_BgBuffer;
        GLuint               m_VertCount;
        GLuint               m_BgVertCount;
        
        CTextObj():m_VArray(0),m_Buffer(0),m_BgVArray(0),
        m_BgBuffer(0),m_VertCount(0),m_BgVertCount(0){};
    };
};

//  ---------------------------------------------------------------------------


#endif // !defined ANT_TW_OPENGL_CORE_INCLUDED
