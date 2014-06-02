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
    public:
        
        //! type for a generic function to manipulate geometry in an arbitrary way
        typedef std::function<void(const gl::GeometryPtr&)> GeomFunctor;
        
        //! type for mapping symbols to geometry functions
        typedef std::map<char, GeomFunctor> FunctorMap;
        
        LSystem();
        
        void iterate(int num_iterations);
        gl::GeometryPtr create_geometry() const;
        
        const std::string axiom() const { return m_axiom;}
        void set_axiom(const std::string &the_axiom){m_axiom = the_axiom;};
        
        float increment() const {return m_increment;}
        void set_increment(float the_inc) {m_increment = the_inc;}
        
        const glm::vec3& branch_angles() const {return m_branch_angle;}
        void set_branch_angles(const glm::vec3& the_angles){m_branch_angle = the_angles;}
        
        std::map<char, std::string>& rules() {return m_rules;}
        const std::map<char, std::string>& rules() const {return m_rules;}
        void set_rules(const std::map<char, std::string> &rule_map){m_rules = rule_map;}
        
        void add_rule(const std::pair<char, string> the_rule);
        void add_rule(const std::string &the_rule);
        
        std::string get_info_string() const;
        
        friend std::ostream& operator<<(std::ostream &os,const LSystem& ls)
        {
            os << ls.get_info_string();
            return os;
        }
        
    private:
        
        struct turtle_state
        {
            glm::mat4 transform;
        };
        
        std::string m_axiom, m_buffer;
        std::map<char, std::string> m_rules;
        
        //! euler angles, in degrees, to apply when rotating (Head, Left, Up)
        glm::vec3 m_branch_angle;
        
        // increment
        float m_increment;
        
        //! turtle state: transform -> (Head, Left, Up, Pos)
        mutable std::vector<turtle_state> m_state_stack;
        
        // iteration depth of last run
        uint32_t m_iteration_depth;
        
        // convenience getters for current turtle orientation
        const glm::vec3& head() const;
        const glm::vec3& left() const;
        const glm::vec3& up() const;
        
        // helper to parse/lex rule strings
        static std::pair<char, std::string> parse_rule(const std::string &the_rule);
        static std::string lex_rule(const std::pair<char, std::string> &the_rule);
    };
}

#endif /* defined(__kinskiGL__LSystem__) */
