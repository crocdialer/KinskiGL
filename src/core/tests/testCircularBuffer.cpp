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
    CircularBuffer<double> circ_buf(8);
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
    auto val = circ_buf.front();
    circ_buf.pop();
    BOOST_CHECK(val == 1);
    BOOST_CHECK(circ_buf.size() == 5);
    BOOST_CHECK(circ_buf[0] == 2);
    
    // overfill buffer
    circ_buf.push(101);
    circ_buf.push(102);
    circ_buf.push(666);
    circ_buf.push(103);
    BOOST_CHECK(circ_buf.size() == 6);

    BOOST_CHECK(circ_buf[0] == 5);
    BOOST_CHECK(circ_buf[1] == 6);
    BOOST_CHECK(circ_buf[2] == 101);
    BOOST_CHECK(circ_buf[3] == 102);
    BOOST_CHECK(circ_buf[4] == 666);
    BOOST_CHECK(circ_buf[5] == 103);
    BOOST_CHECK(kinski::median(circ_buf) == 101.5);
    
    // new capacity, buffer is empty after resize
    circ_buf.set_capacity(7);
    BOOST_CHECK(circ_buf.empty());
    
    // push 8 element into the 7-sized buffer
    circ_buf.push(89);// will fall out
    circ_buf.push(88);
    circ_buf.push(87);
    circ_buf.push(46);
    circ_buf.push(4);// will be the median
    circ_buf.push(3);
    circ_buf.push(2);
    circ_buf.push(1);
    BOOST_CHECK(kinski::median(circ_buf) == 4);
    
    
    int i = 0;
    for(const auto &v : circ_buf){ printf("val[%d]: %.2f\n", i, v); i++; }
    
    printf("median: %.2f\n", kinski::median<double>(circ_buf));
    printf("standard deviation: %.2f\n", kinski::standard_deviation(circ_buf));
    
    circ_buf.clear();
    BOOST_CHECK(circ_buf.empty());
}

//____________________________________________________________________________//

// EOF
