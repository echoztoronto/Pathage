#pragma once
#include <iostream>
#include <vector>
#include <string>
#include <cmath>


#ifndef MAX_VEC_PRINT
#define MAX_VEC_PRINT 200
#endif

template<typename T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& vec) {
    os << "{";
    for(size_t i = 0; i < vec.size(); i++) {
        os << vec[i];
#ifdef MAX_VEC_PRINT
        if(i > MAX_VEC_PRINT) {
            os << "...";
            break;
        }
#endif
        if(i != vec.size() - 1) {
            os << ", " ;
        }
    }
    os << "}";
    return os;
}

namespace ece297test {

template<typename T>
float relative_error(T A, T B) {
    if (A == B) {
        return 0.;
    }

    if (fabs(B) > fabs(A)) {
        return fabs((A - B) / B);
    } else {
        return fabs((A - B) / A);
    }
}

template<typename T>
float absolute_error(T A, T B) {
    return fabs(A - B);
}

}

#ifdef ECE297_TIME_CONSTRAINT
    #error unit_test_util.h redefines ECE297_TIME_CONSTRAINT
#endif

//We define a new time constraint ECE297_TIME_CONSTRAINT which
//is almost identical to UNITTEST_TIME_CONSTRAINT, but doesn't
//rely upon const rvalue life extension to maintian the test 
//details. Instead it creates a new stack variable to store the
//test details. This avoids issues with printing corrupted
//file and test names on failures.
#define ECE297_TIME_CONSTRAINT(ms) \
    UnitTest::TestDetails unitTest__timeConstraint_details__(m_details, __LINE__); \
	UnitTest::TimeConstraint unitTest__timeConstraint__(ms, unitTest__timeConstraint_details__)
