//  See http://www.boost.org/libs/test for the library home page.

// Boost.Test

// each test module could contain no more then one 'main' file with init function defined
// alternatively you could define init function yourself
#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>
#include "core/core.hpp"
#include "core/CircularBuffer.hpp"

using namespace kinski;
//____________________________________________________________________________//

BOOST_AUTO_TEST_CASE( testCircularBuffer )
{
    CircularBuffer<float> circ_buf(8);
    BOOST_CHECK(circ_buf.capacity() == 8);
    BOOST_CHECK(circ_buf.empty());
    
    for(uint32_t i = 0; i < 1000; i++)
    {
        circ_buf.set_capacity(i);
        BOOST_CHECK(circ_buf.capacity() == i);
        
        circ_buf = CircularBuffer<float>(i);
        BOOST_CHECK(circ_buf.capacity() == i);
    }
    
    circ_buf.set_capacity(6);
    BOOST_CHECK(circ_buf.capacity() == 6);

    circ_buf.push_back(1);
    BOOST_CHECK(circ_buf.size() == 1);
    BOOST_CHECK(circ_buf[0] == 1);

    circ_buf.push_back(2);
    circ_buf.push_back(3);
    BOOST_CHECK(circ_buf.size() == 3);

    // fill buffer completely
    circ_buf.push_back(4);
    circ_buf.push_back(5);
    circ_buf.push_back(6);
    BOOST_CHECK(circ_buf.size() == 6);

//    // pop first element, decreasing size
    auto val = circ_buf.front();
    circ_buf.pop_front();
    BOOST_CHECK(val == 1);
    BOOST_CHECK(circ_buf.size() == 5);
    BOOST_CHECK(circ_buf[0] == 2);
    
    // overfill buffer
    circ_buf.push_back(101);
    circ_buf.push_back(102);
    circ_buf.push_back(666);
    circ_buf.push_back(103);
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
    
    // push 8 elements into the 7-sized buffer
    circ_buf.push_back(89);// will fall out
    BOOST_CHECK(circ_buf.size() == 1);
    circ_buf.push_back(2);
    BOOST_CHECK(circ_buf.size() == 2);
    circ_buf.push_back(46);
    BOOST_CHECK(circ_buf.size() == 3);
    circ_buf.push_back(4);// will be the median
    BOOST_CHECK(circ_buf.size() == 4);
    circ_buf.push_back(88);
    BOOST_CHECK(circ_buf.size() == 5);
    circ_buf.push_back(3);
    BOOST_CHECK(circ_buf.size() == 6);
    circ_buf.push_back(87);
    BOOST_CHECK(circ_buf.size() == 7);
    circ_buf.push_back(1);
    BOOST_CHECK(circ_buf.size() == 7);
    BOOST_CHECK(kinski::median(circ_buf) == 4);
    
    int i = 0;
    for(const auto &v : circ_buf){ printf("val[%d]: %.2f\n", i, v); i++; }
    
    printf("median: %.2f\n", kinski::median<float>(circ_buf));
    
    circ_buf.clear();
    BOOST_CHECK(circ_buf.empty());
    
    uint32_t num_elems = 100000;
    printf("\npushing %d elements in range (0 - 100):\n", num_elems);
    circ_buf.set_capacity(1250);
    for(uint32_t i = 0; i < num_elems; i++)
    {
        BOOST_CHECK(circ_buf.size() == std::min<uint32_t>(i, 1250));
        circ_buf.push_back(random_int(0, 100));
    }
    printf("mean: %.2f\n", kinski::mean<float>(circ_buf));
    printf("standard deviation: %.2f\n", kinski::standard_deviation(circ_buf));
    BOOST_CHECK(circ_buf.size() == 1250);
    
    for(uint32_t i = 0; i < num_elems; i++){ circ_buf.pop_front(); }
    BOOST_CHECK(circ_buf.size() == 0);
    BOOST_CHECK(circ_buf.empty());
}

//____________________________________________________________________________//

// EOF
