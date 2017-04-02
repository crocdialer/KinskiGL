//
// Created by crocdialer on 3/31/17.
//


#include "SyphonConnector.h"
#include "gl/Texture.hpp"
#include "gl/Buffer.hpp"

namespace kinski{ namespace syphon{

struct OutputImpl
{

};

Output::Output(const std::string &theName){}

void Output::publish_texture(const gl::Texture &theTexture)
{}

void Output::setName(const std::string &theName)
{}

std::string Output::getName()
{
    return "syphon dummy impl";
}

///////////////////////////////////////////////////////////////////////////////////////////

std::vector<ServerDescription> Input::get_inputs()
{
    return {};
}

struct InputImpl
{

};

Input::Input(uint32_t the_index){}

bool Input::has_new_frame()
{
    return false;
}

bool Input::copy_frame(gl::Texture &tex)
{
    if(!has_new_frame()) return false;
    return true;
}

ServerDescription Input::description()
{
    ServerDescription ret;
    ret.name = "syphon dummy impl";
    return ret;
}

///////////////////////////////////////////////////////////////////////////////////////////
}}//namespace
