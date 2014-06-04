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
m_max_random_tries(50),
m_increment(2.f),
m_increment_randomness(0.f)
{
//    m_position_check = [&](const vec3& p) -> bool {return glm::length(p) < 10;};
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
    m_state_stack = {{glm::mat4(), false}};
    
    // start in xz plane, face upward
    
    // head
    m_state_stack.back().transform[0].xyz() = gl::Y_AXIS;
    
    // left
    m_state_stack.back().transform[1].xyz() = -gl::X_AXIS;
    
    // up
    m_state_stack.back().transform[2].xyz() = gl::Z_AXIS;
    
    // our output geometry
    gl::GeometryPtr ret = gl::Geometry::create();
    auto &points = ret->vertices();
    auto &colors = ret->colors();
    auto &indices = ret->indices();
    int i = 0;
    vec3 current_pos, new_pos;
    
    // create geometry out of our buffer string
    for (char ch : m_buffer)
    {
        int num_grow_tries = 0;
        
        gl::Color current_color(gl::COLOR_WHITE);
        
        // color / material / stack state here
        switch (ch)
        {
            //TODO: move those in a colour lookup map
            case 'F':
                current_color = gl::COLOR_ORANGE;
                break;
            case 'H':
                current_color = gl::COLOR_BLUE;
                break;
                
            // push state
            case '[':
                m_state_stack.push_back(m_state_stack.back());
                break;
                
            // pop state
            case ']':
                m_state_stack.pop_back();
                break;
        }
        
        // this branch should not continue growing -> move on with iteration
        if(m_state_stack.back().abort_branch)
            continue;
        
        // save current position here
        current_pos = new_pos = m_state_stack.back().transform[3].xyz();
        
        // our current branch angles
        vec3 current_branch_angles = m_branch_angle + glm::linearRand(-m_branch_randomness,
                                                                      m_branch_randomness);
        
        // our current increment
        float current_increment = m_increment + kinski::random(-m_increment_randomness,
                                                               m_increment_randomness);
        
        switch (ch)
        {
            // already handled above
            case '[':
            case ']':
                break;
                
            // rotate around 'up vector' ccw
            case '+':
                m_state_stack.back().transform *= glm::rotate(mat4(),
                                                              current_branch_angles[2],
                                                              up());
                break;
                
            // rotate around 'up vector' cw
            case '-':
                m_state_stack.back().transform *= glm::rotate(mat4(),
                                                              -current_branch_angles[2],
                                                              up());
                break;
            
            // rotate around 'up vector' 180 deg
            case '|':
                m_state_stack.back().transform *= glm::rotate(mat4(),
                                                              180.f,
                                                              up());
                break;
                
            // rotate around 'left vector' ccw
            case '&':
                m_state_stack.back().transform *= glm::rotate(mat4(),
                                                              current_branch_angles[1],
                                                              left());
                break;
                
            // rotate around 'left vector' cw
            case '^':
                m_state_stack.back().transform *= glm::rotate(mat4(),
                                                              -current_branch_angles[1],
                                                              left());
                break;
            
            // rotate around 'head vector' ccw
            case '\\':
                m_state_stack.back().transform *= glm::rotate(mat4(),
                                                              current_branch_angles[0],
                                                              head());
                break;
                
            // rotate around 'head vector' cw
            case '/':
                m_state_stack.back().transform *= glm::rotate(mat4(),
                                                              -current_branch_angles[0],
                                                              head());
                break;
                
            // all other symbols will insert a line sequment in 'head direction'
            default:
                
                //geometry check here
                do
                {
                    new_pos = current_pos + head() * current_increment / (float) std::max<int>(m_iteration_depth, 1);
                    num_grow_tries++;
                }
                while(!is_position_valid(new_pos) && num_grow_tries < m_max_random_tries);
                
                if(num_grow_tries >= m_max_random_tries)
                {
                    m_state_stack.back().abort_branch = true;
                    break;
                }
                points.push_back(current_pos);
                points.push_back(new_pos);
                colors.push_back(current_color);
                colors.push_back(current_color);
                indices.push_back(i++);
                indices.push_back(i++);
                break;
        }
        // re-insert position
        m_state_stack.back().transform[3] = vec4(new_pos, 1.f);
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
