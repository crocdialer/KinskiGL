//  See http://www.boost.org/libs/test for the library home page.

// Boost.Test

// each test module could contain no more then one 'main' file with init function defined
// alternatively you could define init function yourself
#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>
#include "core/Utils.hpp"

using namespace kinski;
//____________________________________________________________________________//

BOOST_AUTO_TEST_CASE( testCircularBuffer )
{
    CircularBuffer<int> circ_buf(8);
    BOOST_CHECK(circ_buf.capacity() == 8);
    BOOST_CHECK(circ_buf.empty());
    circ_buf.set_capacity(6);
    BOOST_CHECK(circ_buf.capacity() == 6);

    circ_buf.push(1);
    BOOST_CHECK(circ_buf.size() == 1);
    BOOST_CHECK(circ_buf[0] == 1);

    circ_buf.push(2);
    circ_buf.push(3);
    BOOST_CHECK(circ_buf.size() == 3);

    // fill buffer completely
    circ_buf.push(4);
    circ_buf.push(5);
    circ_buf.push(6);
    BOOST_CHECK(circ_buf.size() == 6);

//    // pop first element, decreasing size
    auto val = circ_buf.pop();
    BOOST_CHECK(val == 1);
    BOOST_CHECK(circ_buf.size() == 5);
    BOOST_CHECK(circ_buf[0] == 2);

    // overfill buffer
    circ_buf.push(101);
    circ_buf.push(102);
    circ_buf.push(103);
    BOOST_CHECK(circ_buf.size() == 6);

    BOOST_CHECK(circ_buf[0] == 4);
    BOOST_CHECK(circ_buf[1] == 5);
    BOOST_CHECK(circ_buf[2] == 6);
    BOOST_CHECK(circ_buf[3] == 101);
    BOOST_CHECK(circ_buf[4] == 102);
    BOOST_CHECK(circ_buf[5] == 103);

    circ_buf.clear();
    BOOST_CHECK(circ_buf.empty());
}

//____________________________________________________________________________//

// EOF
