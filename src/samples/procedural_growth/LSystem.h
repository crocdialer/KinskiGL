//
//  LSystem.h
//  kinskiGL
//
//  Created by Fabian on 27/05/14.
//
//

#ifndef __kinskiGL__LSystem__
#define __kinskiGL__LSystem__

#include "kinskiGL/KinskiGL.h"

namespace kinski
{
    class LSystem
    {
    private:
        
        std::string m_axiom, m_buffer;
        std::map<char, std::string> m_rules;
        
        //! euler angles to apply when rotating (Head, Left, Up)
        glm::vec3 m_branch_angle;
        
        // turtle state
        mutable std::vector<glm::mat4> m_transform_stack;
        
        // convenience getters
        const glm::vec3& head() const;
        const glm::vec3& left() const;
        const glm::vec3& up() const;
        
        float m_branch_distance;
        
    public:
        
        LSystem();
        
        void iterate(int num_iterations);
        gl::GeometryPtr create_geometry() const;
        
        const std::string axiom() const { return m_axiom;}
        void set_axiom(const std::string &the_axiom);
        
        const glm::vec3& branch_angles() const {return m_branch_angle;}
        void set_branch_angles(const glm::vec3& the_angles){m_branch_angle = the_angles;}
        
        std::map<char, std::string>& rules() {return m_rules;}
        const std::map<char, std::string>& rules() const {return m_rules;}
        void set_rules(const std::map<char, std::string> &rule_map){m_rules = rule_map;}
        void add_rule(const std::pair<char, string> the_rule);
        
        std::string get_info_string() const;
    };
}

#endif /* defined(__kinskiGL__LSystem__) */
