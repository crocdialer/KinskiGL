//
//  Visitor.h
//  kinskiGL
//
//  Created by Croc Dialer on 31/03/15.
//
//

#ifndef kinskiGL_Visitor_h
#define kinskiGL_Visitor_h

#include "Mesh.h"
#include "Light.h"
#include "Camera.h"

namespace kinski { namespace gl {
    
    class Visitor
    {
    public:
        Visitor()
        {
            m_transform_stack.push(glm::mat4());
        }
        
        inline const std::stack<glm::mat4>& transform_stack() const {return m_transform_stack;};
        inline std::stack<glm::mat4>& transform_stack() {return m_transform_stack;};
        
        virtual void visit(Object3D &theNode)
        {
            if(!theNode.enabled()) return;
            m_transform_stack.push(m_transform_stack.top() * theNode.transform());
            for (Object3DPtr &child : theNode.children()){child->accept(*this);}
            m_transform_stack.pop();
        }
        virtual void visit(gl::Mesh &theNode){ visit(static_cast<Object3D&>(theNode)); };
        virtual void visit(gl::Light &theNode){ visit(static_cast<Object3D&>(theNode)); };
        virtual void visit(gl::Camera &theNode){ visit(static_cast<Object3D&>(theNode)); };
        
    private:
        std::stack<glm::mat4> m_transform_stack;
    };
    
    template<typename T>
    class SelectVisitor : public Visitor
    {
    public:
        SelectVisitor():Visitor(){};
        
        void visit(T &theNode) override
        {
            if(theNode.enabled())
            {
                m_objects.push_back(&theNode);
                Visitor::visit(static_cast<gl::Object3D&>(theNode));
            }
        };
        
        void clear(){ m_objects.clear(); }
        const std::list<T*>& getObjects() const {return m_objects;};
        
    private:
        std::list<T*> m_objects;
    };
    
}}//namespace
#endif