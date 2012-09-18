//  See http://www.boost.org/libs/test for the library home page.

// Boost.Test

// each test module could contain no more then one 'main' file with init function defined
// alternatively you could define init function yourself
#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>
#include "kinskiCore/Property.h"

using namespace kinski;
//____________________________________________________________________________//

class TestObserver : public Property::Observer
{
public:
    TestObserver():m_triggered(false){};
    bool m_triggered;
    
    void update(const Property::Ptr &theProperty){m_triggered = true;};
};

// most frequently you implement test cases as a free functions with automatic registration
BOOST_AUTO_TEST_CASE( testObserver )
{
    _Property<uint32_t>::Ptr intProp(_Property<uint32_t>::create("intProp", 5));
    
    boost::shared_ptr<TestObserver> obs(new TestObserver);
    intProp->addObserver(obs);
    
    *intProp = 69;
    
    BOOST_CHECK(intProp->val() == 69);
    BOOST_CHECK(obs->m_triggered);
    
    obs->m_triggered = false;
    intProp->removeObserver(obs);
    
    intProp->val(23);
    BOOST_CHECK(intProp->val() == 23);
    BOOST_CHECK(!obs->m_triggered);

    intProp->set(111);
    BOOST_CHECK(intProp->val() == 111);
}

//____________________________________________________________________________//

// each test file may contain any number of test cases; each test case has to have unique name
BOOST_AUTO_TEST_CASE( testRangedProp )
{
    _RangedProperty<int32_t>::Ptr rangeProp
        (_RangedProperty<int32_t>::create("rangedProp", 5, -3, 10));

    *rangeProp = -5;
    BOOST_CHECK_EQUAL( rangeProp->val(), -3 );

    *rangeProp = 1001;
    BOOST_CHECK_EQUAL( rangeProp->val(), 10 );
    
    rangeProp->setRange(-40, 5000);
    int min, max;
    rangeProp->getRange(min, max);
    BOOST_CHECK_EQUAL(min, -40);
    BOOST_CHECK_EQUAL(max, 5000);
    
    rangeProp->set(-999);
    BOOST_CHECK_EQUAL(rangeProp->val(), -40);
}

//____________________________________________________________________________//

// EOF
