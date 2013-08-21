//  See http://www.boost.org/libs/test for the library home page.

// Boost.Test

// each test module could contain no more then one 'main' file with init function defined
// alternatively you could define init function yourself
#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>
#include "kinskiGL/Object3D.h"

using namespace kinski;
//____________________________________________________________________________//

// plane test
BOOST_AUTO_TEST_CASE( test_Object3D )
{
    gl::Object3DPtr a(new gl::Object3D()), b(new gl::Object3D());
    
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
}

//____________________________________________________________________________//

// EOF
