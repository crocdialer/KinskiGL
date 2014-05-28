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

std::pair<char, std::string> LSystem::parse_rule(const std::string &the_rule)
{
    auto ret = std::pair<char, std::string>();
    auto splits = split(remove_whitespace(the_rule), '=');
    
    if(splits.size() != 2)
    {
        LOG_ERROR << "parse error";
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
m_increment(2.f)
{

}

const glm::vec3& LSystem::head() const
{
    return *reinterpret_cast<glm::vec3*>(&m_transform_stack.back()[0]);
}

const glm::vec3& LSystem::left() const
{
    return *reinterpret_cast<glm::vec3*>(&m_transform_stack.back()[1]);
}

const glm::vec3& LSystem::up() const
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
        m_buffer = tmp;
    }
    m_buffer = remove_whitespace(m_buffer);
    LOG_DEBUG << m_buffer;
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
gl::GeometryPtr LSystem::create_geometry() const
{
    m_transform_stack = {glm::mat4()};
    
    // start in xz plane, face upward
    
    // head
    m_transform_stack.back()[0].xyz() = gl::Y_AXIS;
    
    // left
    m_transform_stack.back()[1].xyz() = -gl::X_AXIS;
    
    // up
    m_transform_stack.back()[2].xyz() = -gl::Z_AXIS;
    
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
                tmp_pos += head() * m_increment;
                points.push_back(tmp_pos);
                
                colors.push_back(gl::COLOR_ORANGE);
                colors.push_back(gl::COLOR_ORANGE);
                break;
                
            case 'h':
                points.push_back(tmp_pos);
                tmp_pos += head() * m_increment;
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
                
            // rotate around 'left vector' cw
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
                
            // rotate around 'head vector' cw
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
