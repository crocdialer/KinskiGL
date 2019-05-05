//
//  SyphonConnector.h
//  gl
//
//  Created by Fabian on 5/3/13.
//
//

#pragma once

#include "gl/gl.hpp"

namespace kinski {
namespace syphon {

class Output
{
public:
    Output() {};

    Output(const std::string &theName);

    void publish_texture(const gl::Texture &theTexture);

    void setName(const std::string &theName);

    std::string getName();

private:

    std::shared_ptr<struct OutputImpl> m_impl;

    //! Emulates shared_ptr-like behavior
    explicit operator bool() const { return m_impl.get() != nullptr; }

    void reset() { m_impl.reset(); }
};

struct ServerDescription
{
    std::string name;
    std::string app_name;
};

class Input
{
public:

    static std::vector<ServerDescription> get_inputs();

    Input() {};

    Input(uint32_t the_index);

    bool has_new_frame();

    bool copy_frame(gl::Texture &tex);

    ServerDescription description();

private:

    std::shared_ptr<struct InputImpl> m_impl;

    //! Emulates shared_ptr-like behavior
    operator bool() const { return m_impl.get() != nullptr; }

    void reset() { m_impl.reset(); }
};

class SyphonNotRunningException : public std::runtime_error
{
public:
    SyphonNotRunningException() :
            std::runtime_error("No Syphon server running") {}
};

class SyphonInputOutOfBoundsException : public std::runtime_error
{
public:
    SyphonInputOutOfBoundsException() :
            std::runtime_error("Requested SyphonInput out of bounds") {}
};

}
}//namespace
