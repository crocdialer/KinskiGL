#include "app/App.hpp"
#include "app/imgui/imgui.h"
#include "app/imgui/imgui_util.h"
#include "imgui_impl_es2.h"
#include "gl/Mesh.hpp"

using namespace kinski;


static gl::MeshPtr g_mesh = gl::Mesh::create();
static gl::Buffer g_vertex_buffer;

void ImGui_ImplGlfwGL3_RenderDrawData(ImDrawData* draw_data)
{
    // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
    ImGuiIO& io = ImGui::GetIO();
    int fb_width = (int)(io.DisplaySize.x * io.DisplayFramebufferScale.x);
    int fb_height = (int)(io.DisplaySize.y * io.DisplayFramebufferScale.y);
    if(fb_width == 0 || fb_height == 0){ return; }
    draw_data->ScaleClipRects(io.DisplayFramebufferScale);

    // Backup GL state
    // GLenum last_active_texture; glGetIntegerv(GL_ACTIVE_TEXTURE, (GLint*)&last_active_texture);
    // glActiveTexture(GL_TEXTURE0);
    // GLint last_program; glGetIntegerv(GL_CURRENT_PROGRAM, &last_program);
    // GLint last_texture; glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
    // GLint last_sampler; glGetIntegerv(GL_SAMPLER_BINDING, &last_sampler);
    // GLint last_array_buffer; glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &last_array_buffer);
    // GLint last_element_array_buffer; glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &last_element_array_buffer);
    // GLint last_vertex_array; glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &last_vertex_array);
    // GLint last_polygon_mode[2]; glGetIntegerv(GL_POLYGON_MODE, last_polygon_mode);
    // GLint last_viewport[4]; glGetIntegerv(GL_VIEWPORT, last_viewport);
    // GLint last_scissor_box[4]; glGetIntegerv(GL_SCISSOR_BOX, last_scissor_box);
    // GLenum last_blend_src_rgb; glGetIntegerv(GL_BLEND_SRC_RGB, (GLint*)&last_blend_src_rgb);
    // GLenum last_blend_dst_rgb; glGetIntegerv(GL_BLEND_DST_RGB, (GLint*)&last_blend_dst_rgb);
    // GLenum last_blend_src_alpha; glGetIntegerv(GL_BLEND_SRC_ALPHA, (GLint*)&last_blend_src_alpha);
    // GLenum last_blend_dst_alpha; glGetIntegerv(GL_BLEND_DST_ALPHA, (GLint*)&last_blend_dst_alpha);
    // GLenum last_blend_equation_rgb; glGetIntegerv(GL_BLEND_EQUATION_RGB, (GLint*)&last_blend_equation_rgb);
    // GLenum last_blend_equation_alpha; glGetIntegerv(GL_BLEND_EQUATION_ALPHA, (GLint*)&last_blend_equation_alpha);
    // GLboolean last_enable_blend = glIsEnabled(GL_BLEND);
    // GLboolean last_enable_cull_face = glIsEnabled(GL_CULL_FACE);
    // GLboolean last_enable_depth_test = glIsEnabled(GL_DEPTH_TEST);
    // GLboolean last_enable_scissor_test = glIsEnabled(GL_SCISSOR_TEST);
    //
    // // Setup render state: alpha-blending enabled, no face culling, no depth testing, scissor enabled, polygon fill
    // glEnable(GL_BLEND);
    // glBlendEquation(GL_FUNC_ADD);
    // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // glDisable(GL_CULL_FACE);
    // glDisable(GL_DEPTH_TEST);
    // glEnable(GL_SCISSOR_TEST);
    // glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // // Setup viewport, orthographic projection matrix
    // glViewport(0, 0, (GLsizei)fb_width, (GLsizei)fb_height);
    // const float ortho_projection[4][4] =
    // {
    //     { 2.0f/io.DisplaySize.x, 0.0f,                   0.0f, 0.0f },
    //     { 0.0f,                  2.0f/-io.DisplaySize.y, 0.0f, 0.0f },
    //     { 0.0f,                  0.0f,                  -1.0f, 0.0f },
    //     {-1.0f,                  1.0f,                   0.0f, 1.0f },
    // };
    // glUseProgram(g_ShaderHandle);
    // glUniform1i(g_AttribLocationTex, 0);
    // glUniformMatrix4fv(g_AttribLocationProjMtx, 1, GL_FALSE, &ortho_projection[0][0]);
    // glBindSampler(0, 0); // Rely on combined texture/sampler state.

    // // Recreate the VAO every time
    // // (This is to easily allow multiple GL contexts. VAO are not shared among GL contexts, and we don't track creation/deletion of windows so we don't have an obvious key to use to cache them.)
    // GLuint vao_handle = 0;
    // glGenVertexArrays(1, &vao_handle);
    // glBindVertexArray(vao_handle);
    // glBindBuffer(GL_ARRAY_BUFFER, g_VboHandle);
    // glEnableVertexAttribArray(g_AttribLocationPosition);
    // glEnableVertexAttribArray(g_AttribLocationUV);
    // glEnableVertexAttribArray(g_AttribLocationColor);
    // glVertexAttribPointer(g_AttribLocationPosition, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)IM_OFFSETOF(ImDrawVert, pos));
    // glVertexAttribPointer(g_AttribLocationUV, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)IM_OFFSETOF(ImDrawVert, uv));
    // glVertexAttribPointer(g_AttribLocationColor, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ImDrawVert), (GLvoid*)IM_OFFSETOF(ImDrawVert, col));

    // Draw
    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        const ImDrawIdx* idx_buffer_offset = 0;

        glBindBuffer(GL_ARRAY_BUFFER, g_VboHandle);
        glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)cmd_list->VtxBuffer.Size * sizeof(ImDrawVert), (const GLvoid*)cmd_list->VtxBuffer.Data, GL_STREAM_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_ElementsHandle);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx), (const GLvoid*)cmd_list->IdxBuffer.Data, GL_STREAM_DRAW);

        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
        {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
            if(pcmd->UserCallback){ pcmd->UserCallback(cmd_list, pcmd); }
            else
            {
                glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)pcmd->TextureId);
                glScissor((int)pcmd->ClipRect.x, (int)(fb_height - pcmd->ClipRect.w), (int)(pcmd->ClipRect.z - pcmd->ClipRect.x), (int)(pcmd->ClipRect.w - pcmd->ClipRect.y));
                glDrawElements(GL_TRIANGLES, (GLsizei)pcmd->ElemCount, sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, idx_buffer_offset);
            }
            idx_buffer_offset += pcmd->ElemCount;
        }
    }
    // glDeleteVertexArrays(1, &vao_handle);
    //
    // // Restore modified GL state
    // glUseProgram(last_program);
    // glBindTexture(GL_TEXTURE_2D, last_texture);
    // glBindSampler(0, last_sampler);
    // glActiveTexture(last_active_texture);
    // glBindVertexArray(last_vertex_array);
    // glBindBuffer(GL_ARRAY_BUFFER, last_array_buffer);
    // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, last_element_array_buffer);
    // glBlendEquationSeparate(last_blend_equation_rgb, last_blend_equation_alpha);
    // glBlendFuncSeparate(last_blend_src_rgb, last_blend_dst_rgb, last_blend_src_alpha, last_blend_dst_alpha);
    // if (last_enable_blend) glEnable(GL_BLEND); else glDisable(GL_BLEND);
    // if (last_enable_cull_face) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
    // if (last_enable_depth_test) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    // if (last_enable_scissor_test) glEnable(GL_SCISSOR_TEST); else glDisable(GL_SCISSOR_TEST);
    // glPolygonMode(GL_FRONT_AND_BACK, (GLenum)last_polygon_mode[0]);
    // glViewport(last_viewport[0], last_viewport[1], (GLsizei)last_viewport[2], (GLsizei)last_viewport[3]);
    // glScissor(last_scissor_box[0], last_scissor_box[1], (GLsizei)last_scissor_box[2], (GLsizei)last_scissor_box[3]);
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

bool ImGui_ImplGlfwGL3_CreateFontsTexture()
{
    // Build texture atlas
    ImGuiIO& io = ImGui::GetIO();
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);   // Load as RGBA 32-bits (75% of the memory is wasted, but default font is so small) because it is more likely to be compatible with user's existing shaders. If your ImTextureId represent a higher-level concept than just a GL texture id, consider calling GetTexDataAsAlpha8() instead to save on GPU memory.

    // Upload texture to graphics system
    GLint last_texture;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
    glGenTextures(1, &g_FontTexture);
    glBindTexture(GL_TEXTURE_2D, g_FontTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    // Store our identifier
    io.Fonts->TexID = (void *)(intptr_t)g_FontTexture;

    // Restore state
    glBindTexture(GL_TEXTURE_2D, last_texture);

    return true;
}

// bool ImGui_Impl_ES2_CreateDeviceObjects()
// {
//     // Backup GL state
//     GLint last_texture, last_array_buffer, last_vertex_array;
//     glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
//     glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &last_array_buffer);
//     glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &last_vertex_array);
//
//     const GLchar* vertex_shader =
//         "uniform mat4 ProjMtx;\n"
//         "in vec2 Position;\n"
//         "in vec2 UV;\n"
//         "in vec4 Color;\n"
//         "out vec2 Frag_UV;\n"
//         "out vec4 Frag_Color;\n"
//         "void main()\n"
//         "{\n"
//         "	Frag_UV = UV;\n"
//         "	Frag_Color = Color;\n"
//         "	gl_Position = ProjMtx * vec4(Position.xy,0,1);\n"
//         "}\n";
//
//     const GLchar* fragment_shader =
//         "uniform sampler2D Texture;\n"
//         "in vec2 Frag_UV;\n"
//         "in vec4 Frag_Color;\n"
//         "out vec4 Out_Color;\n"
//         "void main()\n"
//         "{\n"
//         "	Out_Color = Frag_Color * texture( Texture, Frag_UV.st);\n"
//         "}\n";
//
//     const GLchar* vertex_shader_with_version[2] = { g_GlslVersion, vertex_shader };
//     const GLchar* fragment_shader_with_version[2] = { g_GlslVersion, fragment_shader };
//
//     g_ShaderHandle = glCreateProgram();
//     g_VertHandle = glCreateShader(GL_VERTEX_SHADER);
//     g_FragHandle = glCreateShader(GL_FRAGMENT_SHADER);
//     glShaderSource(g_VertHandle, 2, vertex_shader_with_version, NULL);
//     glShaderSource(g_FragHandle, 2, fragment_shader_with_version, NULL);
//     glCompileShader(g_VertHandle);
//     glCompileShader(g_FragHandle);
//     glAttachShader(g_ShaderHandle, g_VertHandle);
//     glAttachShader(g_ShaderHandle, g_FragHandle);
//     glLinkProgram(g_ShaderHandle);
//
//     g_AttribLocationTex = glGetUniformLocation(g_ShaderHandle, "Texture");
//     g_AttribLocationProjMtx = glGetUniformLocation(g_ShaderHandle, "ProjMtx");
//     g_AttribLocationPosition = glGetAttribLocation(g_ShaderHandle, "Position");
//     g_AttribLocationUV = glGetAttribLocation(g_ShaderHandle, "UV");
//     g_AttribLocationColor = glGetAttribLocation(g_ShaderHandle, "Color");
//
//     glGenBuffers(1, &g_VboHandle);
//     glGenBuffers(1, &g_ElementsHandle);
//
//     ImGui_ImplGlfwGL3_CreateFontsTexture();
//
//     // Restore modified GL state
//     glBindTexture(GL_TEXTURE_2D, last_texture);
//     glBindBuffer(GL_ARRAY_BUFFER, last_array_buffer);
//     glBindVertexArray(last_vertex_array);
//
//     return true;
// }

// void    ImGui_Impl_ES2_InvalidateDeviceObjects()
// {
//     if (g_VboHandle) glDeleteBuffers(1, &g_VboHandle);
//     if (g_ElementsHandle) glDeleteBuffers(1, &g_ElementsHandle);
//     g_VboHandle = g_ElementsHandle = 0;
//
//     if (g_ShaderHandle && g_VertHandle) glDetachShader(g_ShaderHandle, g_VertHandle);
//     if (g_VertHandle) glDeleteShader(g_VertHandle);
//     g_VertHandle = 0;
//
//     if (g_ShaderHandle && g_FragHandle) glDetachShader(g_ShaderHandle, g_FragHandle);
//     if (g_FragHandle) glDeleteShader(g_FragHandle);
//     g_FragHandle = 0;
//
//     if (g_ShaderHandle) glDeleteProgram(g_ShaderHandle);
//     g_ShaderHandle = 0;
//
//     if (g_FontTexture)
//     {
//         glDeleteTextures(1, &g_FontTexture);
//         ImGui::GetIO().Fonts->TexID = 0;
//         g_FontTexture = 0;
//     }
// }

// static void ImGui_ImplGlfw_InstallCallbacks(GLFWwindow* window)
// {
//     glfwSetMouseButtonCallback(window, ImGui_ImplGlfw_MouseButtonCallback);
//     glfwSetScrollCallback(window, ImGui_ImplGlfw_ScrollCallback);
//     glfwSetKeyCallback(window, ImGui_ImplGlfw_KeyCallback);
//     glfwSetCharCallback(window, ImGui_ImplGlfw_CharCallback);
// }

bool ImGui_Impl_ES2_Init()
{
    // Setup back-end capabilities flags
    ImGuiIO& io = ImGui::GetIO();
    io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;   // We can honor GetMouseCursor() values (optional)
    io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;    // We can honor io.WantSetMousePos requests (optional, rarely used)

    // Keyboard mapping. ImGui will use those indices to peek into the io.KeysDown[] array.
    io.KeyMap[ImGuiKey_Tab] = Key::_TAB;
    io.KeyMap[ImGuiKey_LeftArrow] = Key::_LEFT;
    io.KeyMap[ImGuiKey_RightArrow] = Key::_RIGHT;
    io.KeyMap[ImGuiKey_UpArrow] = Key::_UP;
    io.KeyMap[ImGuiKey_DownArrow] = Key::_DOWN;
    io.KeyMap[ImGuiKey_PageUp] = Key::_PAGE_UP;
    io.KeyMap[ImGuiKey_PageDown] = Key::_PAGE_DOWN;
    io.KeyMap[ImGuiKey_Home] = Key::_HOME;
    io.KeyMap[ImGuiKey_End] = Key::_END;
    io.KeyMap[ImGuiKey_Insert] = Key::_INSERT;
    io.KeyMap[ImGuiKey_Delete] = Key::_DELETE;
    io.KeyMap[ImGuiKey_Backspace] = Key::_BACKSPACE;
    io.KeyMap[ImGuiKey_Space] = Key::_SPACE;
    io.KeyMap[ImGuiKey_Enter] = Key::_ENTER;
    io.KeyMap[ImGuiKey_Escape] = Key::_ESCAPE;
    io.KeyMap[ImGuiKey_A] = Key::_A;
    io.KeyMap[ImGuiKey_C] = Key::_C;
    io.KeyMap[ImGuiKey_V] = Key::_V;
    io.KeyMap[ImGuiKey_X] = Key::_X;
    io.KeyMap[ImGuiKey_Y] = Key::_Y;
    io.KeyMap[ImGuiKey_Z] = Key::_Z;

    return true;
}

void ImGui_Impl_ES2_Shutdown()
{
    g_mesh.reset();
    g_buffer.reset();
}

void ImGui_Impl_ES2_NewFrame()
{
    ImGuiIO& io = ImGui::GetIO();

    // Setup display size (every frame to accommodate for window resizing)
    // int w, h;
    // int display_w, display_h;
    // glfwGetWindowSize(g_Window, &w, &h);
    // glfwGetFramebufferSize(g_Window, &display_w, &display_h);

    io.DisplaySize = im_vec_cast(gl::window_dimension());
    io.DisplayFramebufferScale = ImVec2(1.f, 1.f);

    // Setup time step
    double current_time =  glfwGetTime();
    io.DeltaTime = g_Time > 0.0 ? (float)(current_time - g_Time) : (float)(1.0f/60.0f);
    g_Time = current_time;

    // Setup inputs
    // (we already got mouse wheel, keyboard keys & characters from glfw callbacks polled in glfwPollEvents())
    if (glfwGetWindowAttrib(g_Window, GLFW_FOCUSED))
    {
        // Set OS mouse position if requested (only used when ImGuiConfigFlags_NavEnableSetMousePos is enabled by user)
        if (io.WantSetMousePos)
        {
            glfwSetCursorPos(g_Window, (double)io.MousePos.x, (double)io.MousePos.y);
        }
        else
        {
            double mouse_x, mouse_y;
            glfwGetCursorPos(g_Window, &mouse_x, &mouse_y);
            io.MousePos = ImVec2((float)mouse_x, (float)mouse_y);
        }
    }
    else
    {
        io.MousePos = ImVec2(-FLT_MAX,-FLT_MAX);
    }

    for (int i = 0; i < 3; i++)
    {
        // If a mouse press event came, always pass it as "mouse held this frame", so we don't miss click-release events that are shorter than 1 frame.
        io.MouseDown[i] = g_MouseJustPressed[i] || glfwGetMouseButton(g_Window, i) != 0;
        g_MouseJustPressed[i] = false;
    }

    // Update OS/hardware mouse cursor if imgui isn't drawing a software cursor
//    if ((io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange) == 0 && glfwGetInputMode(g_Window, GLFW_CURSOR) != GLFW_CURSOR_DISABLED)
//    {
//        ImGuiMouseCursor cursor = ImGui::GetMouseCursor();
//        if (io.MouseDrawCursor || cursor == ImGuiMouseCursor_None)
//        {
//            glfwSetInputMode(g_Window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
//        }
//        else
//        {
//            glfwSetCursor(g_Window, g_MouseCursors[cursor] ? g_MouseCursors[cursor] : g_MouseCursors[ImGuiMouseCursor_Arrow]);
//            glfwSetInputMode(g_Window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
//        }
//    }

//    // Gamepad navigation mapping [BETA]
//    memset(io.NavInputs, 0, sizeof(io.NavInputs));
//    if (io.ConfigFlags & ImGuiConfigFlags_NavEnableGamepad)
//    {
//        // Update gamepad inputs
//        #define MAP_BUTTON(NAV_NO, BUTTON_NO)       { if (buttons_count > BUTTON_NO && buttons[BUTTON_NO] == GLFW_PRESS) io.NavInputs[NAV_NO] = 1.0f; }
//        #define MAP_ANALOG(NAV_NO, AXIS_NO, V0, V1) { float v = (axes_count > AXIS_NO) ? axes[AXIS_NO] : V0; v = (v - V0) / (V1 - V0); if (v > 1.0f) v = 1.0f; if (io.NavInputs[NAV_NO] < v) io.NavInputs[NAV_NO] = v; }
//        int axes_count = 0, buttons_count = 0;
//        const float* axes = glfwGetJoystickAxes(GLFW_JOYSTICK_1, &axes_count);
//        const unsigned char* buttons = glfwGetJoystickButtons(GLFW_JOYSTICK_1, &buttons_count);
//        MAP_BUTTON(ImGuiNavInput_Activate,   0);     // Cross / A
//        MAP_BUTTON(ImGuiNavInput_Cancel,     1);     // Circle / B
//        MAP_BUTTON(ImGuiNavInput_Menu,       2);     // Square / X
//        MAP_BUTTON(ImGuiNavInput_Input,      3);     // Triangle / Y
////        MAP_BUTTON(ImGuiNavInput_DpadLeft,   13);    // D-Pad Left
////        MAP_BUTTON(ImGuiNavInput_DpadRight,  11);    // D-Pad Right
////        MAP_BUTTON(ImGuiNavInput_DpadUp,     10);    // D-Pad Up
////        MAP_BUTTON(ImGuiNavInput_DpadDown,   12);    // D-Pad Down
//        MAP_BUTTON(ImGuiNavInput_FocusPrev,  4);     // L1 / LB
//        MAP_BUTTON(ImGuiNavInput_FocusNext,  5);     // R1 / RB
//        MAP_BUTTON(ImGuiNavInput_TweakSlow,  4);     // L1 / LB
//        MAP_BUTTON(ImGuiNavInput_TweakFast,  5);     // R1 / RB
//        MAP_ANALOG(ImGuiNavInput_LStickLeft, 0,  -0.3f,  -0.9f);
//        MAP_ANALOG(ImGuiNavInput_LStickRight,0,  +0.3f,  +0.9f);
//        MAP_ANALOG(ImGuiNavInput_LStickUp,   1,  +0.3f,  +0.9f);
//        MAP_ANALOG(ImGuiNavInput_LStickDown, 1,  -0.3f,  -0.9f);
//
//        MAP_ANALOG(ImGuiNavInput_DpadLeft, 6,  -0.3f,  -0.9f);
//        MAP_ANALOG(ImGuiNavInput_DpadRight, 6,  +0.3f,  +0.9f);
//        MAP_ANALOG(ImGuiNavInput_DpadUp, 7,  -0.3f,  -0.9f);
//        MAP_ANALOG(ImGuiNavInput_DpadDown, 7,  0.3f,  0.9f);
//
//        #undef MAP_BUTTON
//        #undef MAP_ANALOG
//        if (axes_count > 0 && buttons_count > 0)
//            io.BackendFlags |= ImGuiBackendFlags_HasGamepad;
//        else
//            io.BackendFlags &= ~ImGuiBackendFlags_HasGamepad;
//    }

    // Start the frame. This call will update the io.WantCaptureMouse, io.WantCaptureKeyboard flag that you can use to dispatch inputs (or not) to your application.
    ImGui::NewFrame();
}
