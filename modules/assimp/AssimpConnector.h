//
//  AssimpConnector.h
//  gl
//
//  Created by Fabian on 12/15/12.
//
//

#pragma once

#include "gl/gl.hpp"

namespace kinski { namespace assimp{

//! load a single 3D model from file
gl::MeshPtr load_model(const std::string &thePath);
    
//! load a scene from file
gl::ScenePtr load_scene(const std::string &thePath);
    
//! load animations from file and add to existing mesh
size_t add_animations_to_mesh(const std::string &thePath, gl::MeshPtr m);
    
}}//namespaces
