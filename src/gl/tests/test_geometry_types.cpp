//  See http://www.boost.org/libs/test for the library home page.

// Boost.Test

// each test module could contain no more then one 'main' file with init function defined
// alternatively you could define init function yourself
#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>
#include "gl/geometry_types.hpp"
#include "gl/Camera.hpp"

using namespace kinski;
//____________________________________________________________________________//

// plane test
BOOST_AUTO_TEST_CASE( test_Plane )
{
    glm::vec3 v(10, 20, 30);
    gl::Plane p;

    // test contructors

    // foot, normal
    p = gl::Plane(glm::vec3(99, 0, -69), glm::vec3(0, 1, 0));
    BOOST_CHECK(p.distance(v) == 20);

    // coefficients
    p = gl::Plane(0, 1, 0, 0);
    BOOST_CHECK(p.distance(v) == 20);

    // 3 vertices
    p = gl::Plane(glm::vec3(10, 0, 30), glm::vec3(99, 0, -222), glm::vec3(69, 0, -69));
    BOOST_CHECK(p.distance(v) == 20);

    for(int i = 0; i < 5000; i++)
    {
        // generate random plane
        glm::vec3 v1 = v + glm::sphericalRand(50.f);
        p = gl::Plane(v, v1 - v);
        BOOST_CHECK_CLOSE(p.distance(v1), 50.f, 0.0001f);

        // transform the plane
        p.transform(glm::translate(glm::mat4(), glm::normalize(v1 - v) * 20.f));
        BOOST_CHECK_CLOSE(p.distance(v1), 30.f, 0.0001f);
    }

}

BOOST_AUTO_TEST_CASE( test_scale )
{
    gl::Object3DPtr test_object(new gl::Object3D());
    test_object->transform() = glm::rotate(test_object->transform(), 47.f, glm::vec3(1, 1, 1));
    test_object->transform() = glm::translate(test_object->transform(), glm::vec3(100, -1221, 1));
    test_object->transform() = glm::scale(test_object->transform(), glm::vec3(2, 3, 4));

    glm::vec3 scale = test_object->scale();
    BOOST_CHECK_CLOSE(scale.x, 2.f, 0.0001f);
    BOOST_CHECK_CLOSE(scale.y, 3.f, 0.0001f);
    BOOST_CHECK_CLOSE(scale.z, 4.f, 0.0001f);

    test_object->set_scale(glm::vec3(5));
    scale = test_object->scale();
    BOOST_CHECK_CLOSE(scale.x, 5.f, 0.0001f);
    BOOST_CHECK_CLOSE(scale.y, 5.f, 0.0001f);
    BOOST_CHECK_CLOSE(scale.z, 5.f, 0.0001f);
}

//____________________________________________________________________________//

// EOF
