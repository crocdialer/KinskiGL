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
        
        //! signature for a generic function to manipulate geometry in an arbitrary way
        typedef std::function<void(const gl::GeometryPtr&)> GeomFunctor;
        
        //! signature for a generic function to check if a newly grown position is valid
        typedef std::function<bool(const glm::vec3&)> PositionCheckFunctor;
        
        //! type for mapping symbols to geometry functions
        typedef std::map<char, GeomFunctor> FunctorMap;
        
        LSystem();
        
        void iterate(int num_iterations);
        gl::MeshPtr create_mesh() const;
        
        const std::string axiom() const { return m_axiom;}
        void set_axiom(const std::string &the_axiom){m_axiom = the_axiom;};
        
        float increment() const {return m_increment;}
        void set_increment(float the_inc) {m_increment = the_inc;}
        
        float increment_randomness() const {return m_increment_randomness;}
        void set_increment_randomness(float the_inc) {m_increment_randomness = the_inc;}
        
        const glm::vec3& branch_angles() const {return m_branch_angle;}
        void set_branch_angles(const glm::vec3& the_angles){m_branch_angle = the_angles;}
        
        const glm::vec3& branch_randomness() const {return m_branch_randomness;}
        void set_branch_randomness(const glm::vec3& the_angles){m_branch_randomness = the_angles;}
        
        float diameter() const {return m_diameter;}
        void set_diameter(float d) {m_diameter = d;}
        
        float diameter_shrink_factor() const {return m_diameter_shrink_factor;}
        void set_diameter_shrink_factor(float factor) {m_diameter_shrink_factor = factor;}
        
        std::map<char, std::string>& rules() {return m_rules;}
        const std::map<char, std::string>& rules() const {return m_rules;}
        void set_rules(const std::map<char, std::string> &rule_map){m_rules = rule_map;}
        
        void add_rule(const std::pair<char, string> the_rule);
        void add_rule(const std::string &the_rule);
        
        void set_position_check(PositionCheckFunctor pf){m_position_check = pf;}
        
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
            bool abort_branch;
            float diameter;
        };
        
        std::string m_axiom, m_buffer;
        std::map<char, std::string> m_rules;
        
        //! euler angles, in degrees, to apply when rotating (Head, Left, Up)
        glm::vec3 m_branch_angle;
        
        //! euler angles, in degrees, representing max randomness (Head, Left, Up)
        glm::vec3 m_branch_randomness;
        
        //! maximum number of random grow tries before a branch is aborted
        uint32_t m_max_random_tries;
        
        // increment
        float m_increment;
        
        // increment randomness
        float m_increment_randomness;
        
        //! factor to multiply current diameter when shrinking
        float m_diameter;
        
        //! factor to multiply current diameter when shrinking
        float m_diameter_shrink_factor;
        
        //! turtle state: transform -> (Head, Left, Up, Pos)
        mutable std::vector<turtle_state> m_state_stack;
        
        // iteration depth of last run
        uint32_t m_iteration_depth;
        
        /*! an optional function object, responsible for performing validity checks for
         *  newly created geometry
         */
        PositionCheckFunctor m_position_check;
        
        // convenience getters for current turtle orientation
        const glm::vec3& head() const;
        const glm::vec3& left() const;
        const glm::vec3& up() const;
        
        /*! perform a validity check for a new position
         *  if no functor is defined the check will always succeed
         */
        inline bool is_position_valid(const glm::vec3& p) const
        {return m_position_check ? m_position_check(p) : true;};
        
        // helper to parse/lex rule strings
        static std::pair<char, std::string> parse_rule(const std::string &the_rule);
        static std::string lex_rule(const std::pair<char, std::string> &the_rule);
    };
}

#endif /* defined(__kinskiGL__LSystem__) */
