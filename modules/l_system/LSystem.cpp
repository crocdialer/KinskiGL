//
//  LSystem.cpp
//  gl
//
//  Created by Fabian on 27/05/14.
//
//

#include "LSystem.h"
#include "gl/Mesh.hpp"

using namespace kinski;
using namespace std;
using namespace glm;

std::pair<char, std::string> LSystem::parse_rule(const std::string &the_rule)
{
    auto ret = std::pair<char, std::string>();
    auto splits = split(remove_whitespace(the_rule), '=');
    
    if(splits.size() != 2)
    {
//        LOG_ERROR << "parse error";
    }
    else
    {
        ret.first = splits[0][0];
        ret.second = splits[1];
    }
    return ret;
}

std::string LSystem::lex_rule(const std::pair<char, std::string> &the_rule)
{
    std::stringstream ss;
    ss << the_rule.first << " = " << the_rule.second;
    return ss.str();
}

LSystem::LSystem():
m_branch_angle(90),
m_branch_randomness(0),
m_max_random_tries(5),
m_increment(2.f),
m_increment_randomness(0.f),
m_diameter(1.f),
m_diameter_shrink_factor(1.f)
{
    m_state_stack = {{glm::mat4(), false, m_diameter}};
}

const glm::vec3& LSystem::head() const
{
    return *reinterpret_cast<glm::vec3*>(&m_state_stack.back().transform[0]);
}

const glm::vec3& LSystem::left() const
{
    return *reinterpret_cast<glm::vec3*>(&m_state_stack.back().transform[1]);
}

const glm::vec3& LSystem::up() const
{
    return *reinterpret_cast<glm::vec3*>(&m_state_stack.back().transform[2]);
}

const glm::vec3& LSystem::position() const
{
    return *reinterpret_cast<glm::vec3*>(&m_state_stack.back().transform[3]);
}

glm::mat4& LSystem::turtle_transform()
{
    return m_state_stack.back().transform;
}

const glm::mat4& LSystem::turtle_transform() const
{
    return m_state_stack.back().transform;
}

void LSystem::iterate(int num_iterations)
{
    m_buffer = m_axiom;
    m_iteration_depth = num_iterations;
    
    for(int i = 0; i < num_iterations; i++)
    {
        string tmp;
        
        for (char ch : m_buffer)
        {
            auto iter = m_rules.find(ch);
            
            // if we found a corresponding rule, apply it. else append the original token
            if(iter != m_rules.end()){tmp.append(iter->second);}
            else{tmp.insert(tmp.end(), ch);}
        }
        m_buffer = tmp;
    }
    m_buffer = remove_whitespace(m_buffer);
    LOG_TRACE << m_buffer;
    
    // reset stack
    m_state_stack = {{glm::mat4(), false, m_diameter}};
}

/*!
 
 + Turn left by angle δ, using rotation matrix RU(δ).
 − Turn right by angle δ, using rotation matrix RU(−δ).
 & Pitch down by angle δ, using rotation matrix RL(δ).
 ∧ Pitch up by angle δ, using rotation matrix RL(−δ).
 \ Roll left by angle δ, using rotation matrix RH(δ).
 / Roll right by angle δ, using rotation matrix RH(−δ).
 | Turn around, using rotation matrix RU (180◦ ).
 
*/
gl::MeshPtr LSystem::create_mesh() const
{
//    m_state_stack = {{glm::mat4(), false, m_diameter}};
    
    // use subgeometries for different branch depths
    bool use_mesh_entries = true;
    
    // our output geometry
    gl::MeshPtr ret = gl::Mesh::create(gl::Geometry::create(), gl::Material::create());
    auto &points = ret->geometry()->vertices();
    auto &normals = ret->geometry()->normals();
    auto &colors = ret->geometry()->colors();
    auto &indices = ret->geometry()->indices();
    auto &point_sizes = ret->geometry()->point_sizes();
    
    uint32_t max_branch_depth = 0;
    
    // vertex info for the different branch depths
    std::vector<std::vector<glm::vec3>> verts_vec;
    std::vector<std::vector<glm::vec3>> normals_vec;
    std::vector<std::vector<glm::vec4>> colors_vec;
    std::vector<std::vector<uint32_t>>  indices_vec;
    
    // will hold the current branch diameter
    std::vector<std::vector<float>>  point_sizes_vec;
    
    // helper to have seperate indices for each branch depth
    std::vector<uint32_t> index_increments;
    
    // tmp values
    int i = 0;
    vec3 current_pos, new_pos, current_normal;
    vec3 branch_angle;
    float increment, diameter_shrink_factor;
    
    // create geometry out of our buffer string
    for (auto iter = m_buffer.begin(), end = m_buffer.end(); iter != end; ++iter)
    {
        char ch = *iter, next_ch = (iter + 1) != m_buffer.end() ? *(iter + 1) : 0;
        
        gl::Color current_color(gl::COLOR_WHITE);
        
        // upcoming char starts a parameter block
        bool has_parameter = next_ch && next_ch == '(';
        
        // color / material / stack state here
        switch (ch)
        {
            // push state
            case '[':
                m_state_stack.push_back(m_state_stack.back());
                break;
                
            // pop state
            case ']':
                m_state_stack.pop_back();
                break;
        }
        
        // this optional parameter will only be used if not NaN
        float optional_param = std::nanf("0");
        
        if(has_parameter)
        {
            // jump forward to first position in parameter block
            iter += 2;
            
            string accum;
            
            // increment the iterator until a ')' char is encountered, build up accum string
            while (iter != m_buffer.end())
            {
                // parameter string ended
                if(*iter == ')'){ break; }
                
                accum.insert(accum.end(), *iter);
                ++iter;
            }
            optional_param = kinski::string_to<float>(accum);
        }
        
        // this branch should not continue growing -> move on with iteration
        if(m_state_stack.back().abort_branch)
            continue;
        
        // save current position here
        current_pos = new_pos = position();
        
        // derive normal from up-vector
        current_normal = up();
        
        // the possible base values to apply, either predefined or from supplied parameter
        branch_angle = m_branch_angle;
        increment = m_increment;
        diameter_shrink_factor = m_diameter_shrink_factor;
        
        // a custom parameter was provided
        if(!std::isnan(optional_param))
        {
            branch_angle = vec3(optional_param);
            increment = optional_param;
            diameter_shrink_factor = optional_param;
        }
        
        // our current branch angles
        vec3 current_branch_angles = glm::radians(branch_angle + glm::linearRand(-m_branch_randomness,
                                                                                 m_branch_randomness));
        
        // our current increment
        float current_increment = increment + kinski::random(-m_increment_randomness,
                                                               m_increment_randomness);
        
        switch (ch)
        {
            // already handled above
            case '[':
            case ']':
            case '(':
            case ')':
                break;
            
            // rotate around 'up vector' ccw
            case '+':
                m_state_stack.back().transform = glm::rotate(m_state_stack.back().transform,
                                                             current_branch_angles[2],
                                                             up());
                break;
                
            // rotate around 'up vector' cw
            case '-':
                m_state_stack.back().transform = glm::rotate(m_state_stack.back().transform,
                                                             -current_branch_angles[2],
                                                             up());
                break;
            
            // rotate around 'up vector' 180 deg
            case '|':
                m_state_stack.back().transform = glm::rotate(m_state_stack.back().transform,
                                                             180.f,
                                                             up());
                break;
                
            // rotate around 'left vector' ccw
            case '&':
                m_state_stack.back().transform = glm::rotate(m_state_stack.back().transform,
                                                             current_branch_angles[1],
                                                             left());
                break;
                
            // rotate around 'left vector' cw
            case '^':
                m_state_stack.back().transform = glm::rotate(m_state_stack.back().transform,
                                                             -current_branch_angles[1],
                                                             left());
                break;
            
            // rotate around 'head vector' ccw
            case '\\':
                m_state_stack.back().transform = glm::rotate(m_state_stack.back().transform,
                                                             current_branch_angles[0],
                                                             head());
                break;
                
            // rotate around 'head vector' cw
            case '/':
                m_state_stack.back().transform = glm::rotate(m_state_stack.back().transform,
                                                             -current_branch_angles[0],
                                                             head());
                break;
            
            // shrink diameter
            case '!':
                m_state_stack.back().diameter *= diameter_shrink_factor;
                break;
                
            // all other symbols will insert a line sequment in 'head direction'
            default:
                
                // current branch depth
                size_t current_branch_depth = m_state_stack.size();
                
                // create seperate vectors for each branch_depth to store vertex infos
                if(current_branch_depth > max_branch_depth)
                {
                    max_branch_depth = current_branch_depth;
                    verts_vec.resize(max_branch_depth);
                    normals_vec.resize(max_branch_depth);
                    colors_vec.resize(max_branch_depth);
                    indices_vec.resize(max_branch_depth);
                    point_sizes_vec.resize(max_branch_depth);
                    index_increments.resize(max_branch_depth, 0);
                }
                
                // try to grow in head direction
                vec3 grow_dir = head();
                
                // number of tries to grow in this direction
                int num_grow_tries = 0;
                
                //geometry check here
                while(true)
                {
                    new_pos = current_pos + grow_dir * current_increment;
                    num_grow_tries++;
                    
                    // exit condition
                    if(is_position_valid(new_pos) || num_grow_tries >= m_max_random_tries)
                        break;
                    
                    // get new random vals
                    // our current branch angles
                    current_branch_angles = branch_angle + glm::linearRand(-m_branch_randomness,
                                                                           m_branch_randomness);
                    
                    
                    // our current increment
                    current_increment = increment + kinski::random(-m_increment_randomness,
                                                                   m_increment_randomness);
                    
                    // random direction
                    grow_dir = glm::sphericalRand(1.f);
                    
                    // rotate around 'up vector' 180 deg
//                    m_state_stack.back().transform = glm::rotate(m_state_stack.back().transform,
//                                                                 current_branch_angles[2],
//                                                                 up());
                }
                
                
                // no way to grow from this point
                if(num_grow_tries >= m_max_random_tries)
                {
                    // TODO: come up with something useful here
                    
                    m_state_stack.back().abort_branch = true;
                    LOG_DEBUG << "aborting branch";
                    continue;

                }
                
                if(!use_mesh_entries)
                {
                    points.push_back(current_pos);
                    points.push_back(new_pos);
                    normals.push_back(current_normal);
                    normals.push_back(current_normal);
                    colors.push_back(current_color);
                    colors.push_back(current_color);
                    point_sizes.push_back(m_state_stack.back().diameter);
                    point_sizes.push_back(m_state_stack.back().diameter);
                    indices.push_back(i++);
                    indices.push_back(i++);
                }
                // seperate entries implementation
                else
                {
                    int idx = current_branch_depth - 1;
                    verts_vec[idx].push_back(current_pos);
                    verts_vec[idx].push_back(new_pos);
                    normals_vec[idx].push_back(current_normal);
                    normals_vec[idx].push_back(current_normal);
                    colors_vec[idx].push_back(current_color);
                    colors_vec[idx].push_back(current_color);
                    point_sizes_vec[idx].push_back(m_state_stack.back().diameter);
                    point_sizes_vec[idx].push_back(m_state_stack.back().diameter);
                    indices_vec[idx].push_back(index_increments[idx]++);
                    indices_vec[idx].push_back(index_increments[idx]++);
                }
                break;
        }
        // re-insert position
        m_state_stack.back().transform[3] = vec4(new_pos, 1.f);
    }
    
    // merge together all sub entries
    if(use_mesh_entries)
    {
        uint32_t current_index = 0, current_vertex = 0, material_index = 0;
        std::vector<gl::Mesh::Entry> entries;
        std::vector<gl::MaterialPtr> materials;
        
        gl::GeometryPtr merged_geom = gl::Geometry::create();
        
        for (int i = 0; i < max_branch_depth; i++)
        {
            // merge vertices
            merged_geom->vertices().insert(merged_geom->vertices().end(),
                                           verts_vec[i].begin(),
                                           verts_vec[i].end());
            
            // merge normals
            merged_geom->normals().insert(merged_geom->normals().end(),
                                          normals_vec[i].begin(),
                                          normals_vec[i].end());
            
            // merge colors
            merged_geom->colors().insert(merged_geom->colors().end(),
                                         colors_vec[i].begin(),
                                         colors_vec[i].end());
            
            // merge indices
            merged_geom->indices().insert(merged_geom->indices().end(),
                                          indices_vec[i].begin(),
                                          indices_vec[i].end());
            
            // merge point sizes (aka diameters)
            merged_geom->point_sizes().insert(merged_geom->point_sizes().end(),
                                              point_sizes_vec[i].begin(),
                                              point_sizes_vec[i].end());
            
            gl::Mesh::Entry m;
            m.num_vertices = verts_vec[i].size();
            m.num_indices = indices_vec[i].size();
            m.base_index = current_index;
            m.base_vertex = current_vertex;
            m.material_index = material_index;
            entries.push_back(m);
            current_vertex += verts_vec[i].size();
            current_index += indices_vec[i].size();
            material_index++;
            materials.push_back(gl::Material::create());
        }
        ret->geometry() = merged_geom;
        ret->entries() = entries;
        ret->materials() = materials;
    }
    else
    {
        ret->entries().front().num_vertices = ret->geometry()->vertices().size();
        ret->entries().front().num_indices = ret->geometry()->indices().size();
    }
    ret->geometry()->setPrimitiveType(GL_LINES);
    ret->geometry()->computeBoundingBox();
//    ret->geometry()->createGLBuffers();
    
    return ret;
}

void LSystem::add_rule(const std::pair<char, string> the_rule)
{
    m_rules.insert(the_rule);
}

void LSystem::add_rule(const std::string &the_rule)
{
    add_rule(parse_rule(the_rule));
}

std::string LSystem::get_info_string() const
{
    std::stringstream ss("LSystem:\n");
    ss << "Axiom: " << m_axiom << std::endl;
    
    for (const auto &rule : m_rules)
    {
        ss << "Rule: " << lex_rule(rule) << std::endl;
    }
    ss << "Angles (Head / Left / Up): " << glm::to_string(m_branch_angle) << std::endl;
    ss << "Increment: " << m_increment << std::endl;
    return ss.str();
}
