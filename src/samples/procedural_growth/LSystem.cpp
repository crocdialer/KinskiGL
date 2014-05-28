//
//  LSystem.cpp
//  kinskiGL
//
//  Created by Fabian on 27/05/14.
//
//

#include "LSystem.h"
#include "kinskiGL/Geometry.h"

using namespace kinski;
using namespace std;
using namespace glm;


// + Turn left by angle δ, using rotation matrix RU(δ).
// − Turn right by angle δ, using rotation matrix RU(−δ). & Pitch down by angle δ, using rotation matrix RL(δ).
// & Pitch down by angle δ, using rotation matrix RL(δ).
// ∧ Pitch up by angle δ, using rotation matrix RL(−δ).
// \ Roll left by angle δ, using rotation matrix RH(δ).
// / Roll right by angle δ, using rotation matrix RH(−δ).
// | Turn around, using rotation matrix RU (180◦ ).

LSystem::LSystem():
m_branch_angle(90),
m_branch_distance(2.f)
{
    m_axiom = "f";
    m_rules = {{'f', "f - h"}, {'h', "f + h"}};
}

const glm::vec3& LSystem::head() const
{
    return *reinterpret_cast<glm::vec3*>(&m_transform_stack.back()[0]);
}

const glm::vec3& LSystem::up() const
{
    return *reinterpret_cast<glm::vec3*>(&m_transform_stack.back()[1]);
}

const glm::vec3& LSystem::left() const
{
    return *reinterpret_cast<glm::vec3*>(&m_transform_stack.back()[2]);
}

void LSystem::iterate(int num_iterations)
{
    m_buffer = m_axiom;
    
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
        m_buffer = remove_whitespace(tmp);
    }
    
    LOG_INFO << m_buffer;
}

gl::GeometryPtr LSystem::create_geometry() const
{
    m_transform_stack = {glm::mat4()};
    
    // start in xz plane, face upward
    
    // head
    m_transform_stack.back()[0].xyz() = gl::Y_AXIS;
    
    // up
    m_transform_stack.back()[1].xyz() = -gl::Z_AXIS;
    
    // left
    m_transform_stack.back()[2].xyz() = -gl::X_AXIS;
    
    // our output geometry
    gl::GeometryPtr ret = gl::Geometry::create();
    auto &points = ret->vertices();
    auto &colors = ret->colors();
    
    // create geometry out of our buffer string
    for (char ch : m_buffer)
    {
        // tmp save position here
        vec3 tmp_pos = m_transform_stack.back()[3].xyz();
        
        switch (ch)
        {
            // insert a line sequment in 'head direction'
            case 'f':
                points.push_back(tmp_pos);
                tmp_pos += head() * m_branch_distance;
                points.push_back(tmp_pos);
                
                colors.push_back(gl::COLOR_ORANGE);
                colors.push_back(gl::COLOR_ORANGE);
                
                break;
            case 'h':
                points.push_back(tmp_pos);
                tmp_pos += head() * m_branch_distance;
                points.push_back(tmp_pos);
                
                colors.push_back(gl::COLOR_BLUE);
                colors.push_back(gl::COLOR_BLUE);
                
                break;
            
            // rotate around 'up vector' ccw
            case '+':
                m_transform_stack.back() *= glm::rotate(mat4(),
                                                        m_branch_angle[2],
                                                        up());
                break;
                
            // rotate around 'up vector' cw
            case '-':
                m_transform_stack.back() *= glm::rotate(mat4(),
                                                        -m_branch_angle[2],
                                                        up());
                break;
                
            // rotate around 'left vector' ccw
            case '&':
                m_transform_stack.back() *= glm::rotate(mat4(),
                                                        m_branch_angle[1],
                                                        left());
                break;
                
            // rotate around 'left vector' ccw
            case '^':
                m_transform_stack.back() *= glm::rotate(mat4(),
                                                        -m_branch_angle[1],
                                                        left());
                break;
            
            // rotate around 'head vector' ccw
            case '\\':
                m_transform_stack.back() *= glm::rotate(mat4(),
                                                        m_branch_angle[0],
                                                        head());
                break;
                
            // rotate around 'head vector' ccw
            case '/':
                m_transform_stack.back() *= glm::rotate(mat4(),
                                                        -m_branch_angle[0],
                                                        head());
                break;
                
            // push state
            case '[':
                m_transform_stack.push_back(m_transform_stack.back());
                break;
                
            // pop state
            case ']':
                m_transform_stack.pop_back();
                break;
                
            default:
                break;
        }
        
        // re-insert position
        m_transform_stack.back()[3] = vec4(tmp_pos, 1.f);
    }
    
    ret->setPrimitiveType(GL_LINES);
    ret->computeBoundingBox();
    ret->createGLBuffers();
    
    return ret;
}

std::string LSystem::get_info_string() const
{
    return "not implemented";
}
