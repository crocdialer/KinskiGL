//  See http://www.boost.org/libs/test for the library home page.

// Boost.Test

// each test module could contain no more then one 'main' file with init function defined
// alternatively you could define init function yourself
#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>
#include "gl/Object3D.hpp"

using namespace kinski;
//____________________________________________________________________________//

BOOST_AUTO_TEST_CASE( test_Object3D )
{
    gl::Object3DPtr a(new gl::Object3D()), b(new gl::Object3D()), c(new gl::Object3D());

    a->set_parent(b);
    BOOST_CHECK(a->parent() == b);
    BOOST_CHECK(b->children().size() == 1);

    b->remove_child(a);
    BOOST_CHECK(!a->parent());

    b->remove_child(a);

    a->add_child(b);
    BOOST_CHECK(a->children().size() == 1);
    BOOST_CHECK(b->parent() == a);

    b->set_parent(gl::Object3DPtr());
    BOOST_CHECK(a->children().empty());
    BOOST_CHECK(!b->parent());

    // a -> b -> c
    c->set_parent(b);
    a->add_child(b);
    BOOST_CHECK(c->parent() == b);
    BOOST_CHECK(b->parent() == a);

    //test scaling
    b->set_scale(0.5);
    c->set_scale(0.2);
    BOOST_CHECK(a->global_scale() == glm::vec3(1));
    BOOST_CHECK(b->global_scale() == glm::vec3(0.5));
    BOOST_CHECK(c->global_scale() == glm::vec3(0.1));

    a->set_position(glm::vec3(0, 100, 0));
    b->set_position(glm::vec3(0, 50, 0));
    BOOST_CHECK(b->position() == glm::vec3(0, 50, 0));
    BOOST_CHECK(b->global_position() == glm::vec3(0, 150, 0));

    // rotations
    b->set_rotation(48.f, 10.f, 5.f);
    BOOST_CHECK(b->global_scale() == glm::vec3(0.5));
    BOOST_CHECK(b->global_position() == glm::vec3(0, 150, 0));
}

//____________________________________________________________________________//

// EOF
