// ImGui GLFW binding with OpenGL3 + shaders
// (GLFW is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan graphics context creation, etc.)
// (GL3W is a helper library to access OpenGL functions since there is no standard header to access modern OpenGL functions easily. Alternatives are GLEW, Glad, etc.)

// Implemented features:
//  [X] User texture binding. Cast 'GLuint' OpenGL texture identifier as void*/ImTextureID. Read the FAQ about ImTextureID in imgui.cpp.
//  [X] Gamepad navigation mapping. Enable with 'io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad'.

// You can copy and use unmodified imgui_impl_* files in your project. See main.cpp for an example of using this.
// If you use this binding you'll need to call 4 functions: ImGui_ImplXXXX_Init(), ImGui_ImplXXXX_NewFrame(), ImGui::Render() and ImGui_ImplXXXX_Shutdown().
// If you are new to ImGui, see examples/README.txt and documentation at the top of imgui.cpp.
// https://github.com/ocornut/imgui

// CHANGELOG
// (minor and older changes stripped away, please see git history for details)
//  2018-03-20: Misc: Setup io.BackendFlags ImGuiBackendFlags_HasMouseCursors and ImGuiBackendFlags_HasSetMousePos flags + honor ImGuiConfigFlags_NoMouseCursorChange flag.
//  2018-03-06: OpenGL: Added const char* glsl_version parameter to ImGui_ImplGlfwGL3_Init() so user can override the GLSL version e.g. "#version 150".
//  2018-02-23: OpenGL: Create the VAO in the render function so the setup can more easily be used with multiple shared GL context.
//  2018-02-20: Inputs: Added support for mouse cursors (ImGui::GetMouseCursor() value, passed to glfwSetCursor()).
//  2018-02-20: Inputs: Renamed GLFW callbacks exposed in .h to not include GL3 in their name.
//  2018-02-16: Misc: Obsoleted the io.RenderDrawListsFn callback and exposed ImGui_ImplGlfwGL3_RenderDrawData() in the .h file so you can call it yourself.
//  2018-02-06: Misc: Removed call to ImGui::Shutdown() which is not available from 1.60 WIP, user needs to call CreateContext/DestroyContext themselves.
//  2018-02-06: Inputs: Added mapping for ImGuiKey_Space.
//  2018-01-25: Inputs: Added gamepad support if ImGuiConfigFlags_NavEnableGamepad is set.
//  2018-01-25: Inputs: Honoring the io.WantSetMousePos flag by repositioning the mouse (ImGuiConfigFlags_NavEnableSetMousePos is set).
//  2018-01-20: Inputs: Added Horizontal Mouse Wheel support.
//  2018-01-18: Inputs: Added mapping for ImGuiKey_Insert.
//  2018-01-07: OpenGL: Changed GLSL shader version from 330 to 150. (Also changed GL context from 3.3 to 3.2 in example's main.cpp)
//  2017-09-01: OpenGL: Save and restore current bound sampler. Save and restore current polygon mode.
//  2017-08-25: Inputs: MousePos set to -FLT_MAX,-FLT_MAX when mouse is unavailable/missing (instead of -1,-1).
//  2017-05-01: OpenGL: Fixed save and restore of current blend function state.
//  2016-10-15: Misc: Added a void* user_data parameter to Clipboard function handlers.
//  2016-09-05: OpenGL: Fixed save and restore of current scissor rectangle.
//  2016-04-30: OpenGL: Fixed save and restore of current GL_ACTIVE_TEXTURE.

#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "gl/Mesh.hpp"
#include "app/imgui/imgui.h"
#include "app/imgui/imgui_util.h"
#include "imgui_impl_glfw_gl3.h"

// GL3W/GLFW
#include "gl/gl.hpp"
#include <GLFW/glfw3.h>
#ifdef _WIN32
#undef APIENTRY
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL
#include <GLFW/glfw3native.h>
#endif

// GLFW data
static GLFWwindow*  g_Window = NULL;
static double       g_Time = 0.0f;
static bool         g_MouseJustPressed[3] = { false, false, false };
static GLFWcursor*  g_MouseCursors[ImGuiMouseCursor_COUNT] = { 0 };

// OpenGL3 data
//static char         g_GlslVersion[32] = "#version 150";
//static GLuint       g_FontTexture = 0;
//static int          g_ShaderHandle = 0, g_VertHandle = 0, g_FragHandle = 0;
//static int          g_AttribLocationTex = 0, g_AttribLocationProjMtx = 0;
//static int          g_AttribLocationPosition = 0, g_AttribLocationUV = 0, g_AttribLocationColor = 0;
//static unsigned int g_VboHandle = 0, g_ElementsHandle = 0;

static kinski::gl::MeshPtr g_mesh;
static kinski::gl::Buffer g_vertex_buffer;
static kinski::gl::Buffer g_index_buffer;

// OpenGL3 Render function.
// (this used to be set in io.RenderDrawListsFn and called by ImGui::Render(), but you can now call this directly from your main loop)
// Note that this implementation is little overcomplicated because we are saving/setting up/restoring every OpenGL state explicitly, in order to be able to run within any OpenGL engine that doesn't do so. 
void ImGui_ImplGlfwGL3_RenderDrawData(ImDrawData* draw_data)
{
    // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
    ImGuiIO& io = ImGui::GetIO();
    int fb_width = (int)(io.DisplaySize.x * io.DisplayFramebufferScale.x);
    int fb_height = (int)(io.DisplaySize.y * io.DisplayFramebufferScale.y);
    if(fb_width == 0 || fb_height == 0){ return; }
    draw_data->ScaleClipRects(io.DisplayFramebufferScale);

    kinski::gl::ScopedMatrixPush push_modelview(kinski::gl::MODEL_VIEW_MATRIX), push_proj(kinski::gl::PROJECTION_MATRIX);
    kinski::gl::load_identity(kinski::gl::MODEL_VIEW_MATRIX);
    kinski::gl::load_matrix(kinski::gl::PROJECTION_MATRIX, glm::ortho(0.f, io.DisplaySize.x, io.DisplaySize.y, 0.f));

    // Draw
    for(int n = 0; n < draw_data->CmdListsCount; n++)
    {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        auto &entry = g_mesh->entries()[0];
        entry.base_vertex = 0;
        entry.base_index = 0;
        entry.num_vertices = cmd_list->VtxBuffer.Size;
        entry.num_indices = cmd_list->IdxBuffer.Size;
        entry.primitive_type = GL_TRIANGLES;

        // upload vertex data
        auto &verts = g_mesh->geometry()->vertices();
        auto &colors = g_mesh->geometry()->colors();
        auto &tex_coords = g_mesh->geometry()->tex_coords();
        auto &indices = g_mesh->geometry()->indices();
        verts.resize(cmd_list->VtxBuffer.Size);
        colors.resize(cmd_list->VtxBuffer.Size);
        tex_coords.resize(cmd_list->VtxBuffer.Size);

        for(int i = 0; i < cmd_list->VtxBuffer.Size; ++i)
        {
            uint8_t *c = reinterpret_cast<uint8_t*>(&cmd_list->VtxBuffer.Data[i].col);
            verts[i] = glm::vec3(cmd_list->VtxBuffer[i].pos.x, cmd_list->VtxBuffer[i].pos.y, 0.f);
            colors[i] = glm::vec4(c[0], c[1], c[2], c[3]) / 255.f;
            tex_coords[i] = glm::vec2(cmd_list->VtxBuffer[i].uv.x, cmd_list->VtxBuffer[i].uv.y);
        }
        indices.assign(cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Data + cmd_list->IdxBuffer.Size);

        // upload index data
//        g_index_buffer.set_data(cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));

        for(int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
        {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
            if(pcmd->UserCallback){ pcmd->UserCallback(cmd_list, pcmd); }
            else
            {
                auto rect = kinski::Area_<uint32_t>(pcmd->ClipRect.x, fb_height - pcmd->ClipRect.w,
                                                    pcmd->ClipRect.z, fb_height - pcmd->ClipRect.y);
                g_mesh->material()->set_scissor_rect(rect);
                entry.num_indices = pcmd->ElemCount;
                kinski::gl::draw_mesh(g_mesh);
            }
            entry.base_index += pcmd->ElemCount;
        }
    }
}

static const char* ImGui_ImplGlfwGL3_GetClipboardText(void* user_data)
{
    return glfwGetClipboardString((GLFWwindow*)user_data);
}

static void ImGui_ImplGlfwGL3_SetClipboardText(void* user_data, const char* text)
{
    glfwSetClipboardString((GLFWwindow*)user_data, text);
}

void ImGui_ImplGlfw_MouseButtonCallback(GLFWwindow*, int button, int action, int /*mods*/)
{
    if (action == GLFW_PRESS && button >= 0 && button < 3)
        g_MouseJustPressed[button] = true;
}

void ImGui_ImplGlfw_ScrollCallback(GLFWwindow*, double xoffset, double yoffset)
{
    ImGuiIO& io = ImGui::GetIO();
    io.MouseWheelH += (float)xoffset;
    io.MouseWheel += (float)yoffset;
}

void ImGui_ImplGlfw_KeyCallback(GLFWwindow*, int key, int, int action, int mods)
{
    ImGuiIO& io = ImGui::GetIO();
    if (action == GLFW_PRESS)
        io.KeysDown[key] = true;
    if (action == GLFW_RELEASE)
        io.KeysDown[key] = false;

    (void)mods; // Modifiers are not reliable across systems
    io.KeyCtrl = io.KeysDown[GLFW_KEY_LEFT_CONTROL] || io.KeysDown[GLFW_KEY_RIGHT_CONTROL];
    io.KeyShift = io.KeysDown[GLFW_KEY_LEFT_SHIFT] || io.KeysDown[GLFW_KEY_RIGHT_SHIFT];
    io.KeyAlt = io.KeysDown[GLFW_KEY_LEFT_ALT] || io.KeysDown[GLFW_KEY_RIGHT_ALT];
    io.KeySuper = io.KeysDown[GLFW_KEY_LEFT_SUPER] || io.KeysDown[GLFW_KEY_RIGHT_SUPER];
}

void ImGui_ImplGlfw_CharCallback(GLFWwindow*, unsigned int c)
{
    ImGuiIO& io = ImGui::GetIO();
    if (c > 0 && c < 0x10000)
        io.AddInputCharacter((unsigned short)c);
}

bool ImGui_ImplGlfwGL3_CreateDeviceObjects()
{
    // buffer objects
//    g_vertex_buffer = kinski::gl::Buffer(GL_ARRAY_BUFFER, GL_STREAM_DRAW);
//    g_vertex_buffer.set_stride(sizeof(ImDrawVert));
//    g_index_buffer = kinski::gl::Buffer(GL_ELEMENT_ARRAY_BUFFER, GL_STREAM_DRAW);

    // font texture
    ImGuiIO& io = ImGui::GetIO();
    unsigned char* pixels;
    int width, height, num_components;
//    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height, &num_components);
    io.Fonts->GetTexDataAsAlpha8(&pixels, &width, &height, &num_components);

#if defined(KINSKI_ARM)
    GLint tex_format = GL_LUMINANCE_ALPHA;

    // create data
    size_t num_bytes = width * height * 2;
    auto luminance_alpha_data = std::unique_ptr<uint8_t>(new uint8_t[num_bytes]);
    uint8_t *src_ptr = static_cast<uint8_t*>(pixels);
    uint8_t *out_ptr = luminance_alpha_data.get(), *data_end = luminance_alpha_data.get() + num_bytes;

    for (; out_ptr < data_end; out_ptr += 2, ++src_ptr)
    {
        out_ptr[0] = 255;
        out_ptr[1] = *src_ptr;
    }

    // create a new texture object for our glyphs
    gl::Texture::Format fmt;
    fmt.internal_format = tex_format;
    auto font_texture = gl::Texture(luminance_alpha_data.get(), tex_format, width, height, fmt);
#else
    auto font_img = kinski::Image_<uint8_t>::create(pixels, width, height, num_components, true);
    auto font_texture = kinski::gl::create_texture_from_image(font_img, false, false);
    font_texture.set_flipped(false);
    font_texture.set_swizzle(GL_ONE, GL_ONE, GL_ONE, GL_RED);
#endif

    // create mesh instance
    g_mesh = kinski::gl::Mesh::create();

    // add texture
    auto &mat = g_mesh->material();
    mat->add_texture(font_texture, kinski::gl::Texture::Usage::COLOR);
    mat->set_depth_test(false);
    mat->set_depth_write(false);
    mat->set_blending(true);
    mat->set_culling(kinski::gl::Material::CULL_NONE);

    // vertex attrib -> position
//    kinski::gl::Mesh::VertexAttrib position_attrib;
//    position_attrib.type = GL_FLOAT;
//    position_attrib.size = 2;
//    position_attrib.name = "a_vertex";
//    position_attrib.buffer = g_vertex_buffer;
//    position_attrib.stride = sizeof(ImDrawVert);
//    position_attrib.offset = offsetof(ImDrawVert, pos);
//    position_attrib.normalize = false;
//
//    // vertex attrib -> color
//    kinski::gl::Mesh::VertexAttrib color_attrib;
//    color_attrib.type = GL_UNSIGNED_BYTE;
//    color_attrib.size = 4;
//    color_attrib.name = "a_color";
//    color_attrib.buffer = g_vertex_buffer;
//    color_attrib.stride = sizeof(ImDrawVert);
//    color_attrib.offset = offsetof(ImDrawVert, col);
//    position_attrib.normalize = true;
//
//    // vertex attrib -> texcoords
//    kinski::gl::Mesh::VertexAttrib tex_coord_attrib;
//    tex_coord_attrib.type = GL_FLOAT;
//    tex_coord_attrib.size = 2;
//    tex_coord_attrib.name = "a_texCoord";
//    tex_coord_attrib.buffer = g_vertex_buffer;
//    tex_coord_attrib.stride = sizeof(ImDrawVert);
//    tex_coord_attrib.offset = offsetof(ImDrawVert, uv);
//    tex_coord_attrib.normalize = false;
//
//    g_mesh->add_vertex_attrib(position_attrib);
//    g_mesh->add_vertex_attrib(color_attrib);
//    g_mesh->add_vertex_attrib(tex_coord_attrib);
//
//    g_mesh->set_index_buffer(g_index_buffer);
    return true;
}

void ImGui_ImplGlfwGL3_InvalidateDeviceObjects()
{
    g_mesh.reset();
    g_vertex_buffer.reset();
    g_index_buffer.reset();
}

//static void ImGui_ImplGlfw_InstallCallbacks(GLFWwindow* window)
//{
//    glfwSetMouseButtonCallback(window, ImGui_ImplGlfw_MouseButtonCallback);
//    glfwSetScrollCallback(window, ImGui_ImplGlfw_ScrollCallback);
//    glfwSetKeyCallback(window, ImGui_ImplGlfw_KeyCallback);
//    glfwSetCharCallback(window, ImGui_ImplGlfw_CharCallback);
//}

bool ImGui_ImplGlfwGL3_Init(GLFWwindow* window, bool install_callbacks, const char* glsl_version)
{
    g_Window = window;

    // Setup back-end capabilities flags
    ImGuiIO& io = ImGui::GetIO();
    io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;   // We can honor GetMouseCursor() values (optional)
    io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;    // We can honor io.WantSetMousePos requests (optional, rarely used)

    // Keyboard mapping. ImGui will use those indices to peek into the io.KeysDown[] array.
    io.KeyMap[ImGuiKey_Tab] = GLFW_KEY_TAB;
    io.KeyMap[ImGuiKey_LeftArrow] = GLFW_KEY_LEFT;
    io.KeyMap[ImGuiKey_RightArrow] = GLFW_KEY_RIGHT;
    io.KeyMap[ImGuiKey_UpArrow] = GLFW_KEY_UP;
    io.KeyMap[ImGuiKey_DownArrow] = GLFW_KEY_DOWN;
    io.KeyMap[ImGuiKey_PageUp] = GLFW_KEY_PAGE_UP;
    io.KeyMap[ImGuiKey_PageDown] = GLFW_KEY_PAGE_DOWN;
    io.KeyMap[ImGuiKey_Home] = GLFW_KEY_HOME;
    io.KeyMap[ImGuiKey_End] = GLFW_KEY_END;
    io.KeyMap[ImGuiKey_Insert] = GLFW_KEY_INSERT;
    io.KeyMap[ImGuiKey_Delete] = GLFW_KEY_DELETE;
    io.KeyMap[ImGuiKey_Backspace] = GLFW_KEY_BACKSPACE;
    io.KeyMap[ImGuiKey_Space] = GLFW_KEY_SPACE;
    io.KeyMap[ImGuiKey_Enter] = GLFW_KEY_ENTER;
    io.KeyMap[ImGuiKey_Escape] = GLFW_KEY_ESCAPE;
    io.KeyMap[ImGuiKey_A] = GLFW_KEY_A;
    io.KeyMap[ImGuiKey_C] = GLFW_KEY_C;
    io.KeyMap[ImGuiKey_V] = GLFW_KEY_V;
    io.KeyMap[ImGuiKey_X] = GLFW_KEY_X;
    io.KeyMap[ImGuiKey_Y] = GLFW_KEY_Y;
    io.KeyMap[ImGuiKey_Z] = GLFW_KEY_Z;

    io.SetClipboardTextFn = ImGui_ImplGlfwGL3_SetClipboardText;
    io.GetClipboardTextFn = ImGui_ImplGlfwGL3_GetClipboardText;
    io.ClipboardUserData = g_Window;
#ifdef _WIN32
    io.ImeWindowHandle = glfwGetWin32Window(g_Window);
#endif

    // Load cursors
    // FIXME: GLFW doesn't expose suitable cursors for ResizeAll, ResizeNESW, ResizeNWSE. We revert to arrow cursor for those.
    g_MouseCursors[ImGuiMouseCursor_Arrow] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
    g_MouseCursors[ImGuiMouseCursor_TextInput] = glfwCreateStandardCursor(GLFW_IBEAM_CURSOR);
    g_MouseCursors[ImGuiMouseCursor_ResizeAll] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
    g_MouseCursors[ImGuiMouseCursor_ResizeNS] = glfwCreateStandardCursor(GLFW_VRESIZE_CURSOR);
    g_MouseCursors[ImGuiMouseCursor_ResizeEW] = glfwCreateStandardCursor(GLFW_HRESIZE_CURSOR);
    g_MouseCursors[ImGuiMouseCursor_ResizeNESW] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
    g_MouseCursors[ImGuiMouseCursor_ResizeNWSE] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);

//    if(install_callbacks){ ImGui_ImplGlfw_InstallCallbacks(window); }
    return true;
}

void ImGui_ImplGlfwGL3_Shutdown()
{
    // Destroy GLFW mouse cursors
    for (ImGuiMouseCursor cursor_n = 0; cursor_n < ImGuiMouseCursor_COUNT; cursor_n++)
        glfwDestroyCursor(g_MouseCursors[cursor_n]);
    memset(g_MouseCursors, 0, sizeof(g_MouseCursors));

    // Destroy OpenGL objects
    ImGui_ImplGlfwGL3_InvalidateDeviceObjects();
}

void ImGui_ImplGlfwGL3_NewFrame()
{
    if(!g_mesh){ ImGui_ImplGlfwGL3_CreateDeviceObjects(); }

    ImGuiIO& io = ImGui::GetIO();

    // Setup display size (every frame to accommodate for window resizing)
    int w, h;
    int display_w, display_h;
    glfwGetWindowSize(g_Window, &w, &h);
    glfwGetFramebufferSize(g_Window, &display_w, &display_h);
    io.DisplaySize = ImVec2((float)w, (float)h);
    io.DisplayFramebufferScale = ImVec2(w > 0 ? ((float)display_w / w) : 0, h > 0 ? ((float)display_h / h) : 0);

    // Setup time step
    double current_time = glfwGetTime();
    io.DeltaTime = g_Time > 0.0 ? (float)(current_time - g_Time) : (float)(1.0f/60.0f);
    g_Time = current_time;

    // Setup inputs
    // (we already got mouse wheel, keyboard keys & characters from glfw callbacks polled in glfwPollEvents())
    if(glfwGetWindowAttrib(g_Window, GLFW_FOCUSED))
    {
        // Set OS mouse position if requested (only used when ImGuiConfigFlags_NavEnableSetMousePos is enabled by user)
        if(io.WantSetMousePos){ glfwSetCursorPos(g_Window, (double)io.MousePos.x, (double)io.MousePos.y); }
        else
        {
            double mouse_x, mouse_y;
            glfwGetCursorPos(g_Window, &mouse_x, &mouse_y);
            io.MousePos = ImVec2((float)mouse_x, (float)mouse_y);
        }
    }
    else{ io.MousePos = ImVec2(-FLT_MAX,-FLT_MAX); }

    for (int i = 0; i < 3; i++)
    {
        // If a mouse press event came, always pass it as "mouse held this frame", so we don't miss click-release events that are shorter than 1 frame.
        io.MouseDown[i] = g_MouseJustPressed[i] || glfwGetMouseButton(g_Window, i) != 0;
        g_MouseJustPressed[i] = false;
    }

    // Start the frame. This call will update the io.WantCaptureMouse, io.WantCaptureKeyboard flag that you can use to dispatch inputs (or not) to your application.
    ImGui::NewFrame();
}
