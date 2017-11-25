/*
 Copyright (c) 2010, The Barbarian Group
 All rights reserved.

 Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and
    the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
    the following disclaimer in the documentation and/or other materials provided with the distribution.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.

 Significant Portions Copyright:
 Geometric Tools, LLC
 Copyright (c) 1998-2010
 Distributed under the Boost Software License, Version 1.0.
 http://www.boost.org/LICENSE_1_0.txt
 http://www.geometrictools.com/License/Boost/LICENSE_1_0.txt
*/

#pragma once

#include "gl.hpp"

namespace kinski{ namespace gl {

class KINSKI_API BSplineBasis
{
 public:
    BSplineBasis();

    // Open uniform or periodic uniform.  The knot array is internally
    // generated with equally spaced elements.
    BSplineBasis(int aNumCtrlPoints, int iDegree, bool bOpen);
    void create(int aNumCtrlPoints, int iDegree, bool bOpen);

    // Open nonuniform.  The knot array must have n-d elements.  The elements
    // must be nondecreasing.  Each element must be in [0,1].  The caller is
    // responsible for deleting afKnot.  An internal copy is made, so to
    // dynamically change knots you must use the setKnot function.
    BSplineBasis(int aNumCtrlPoints, int iDegree, const float* afKnot);
    void create(int aNumCtrlPoints, int iDegree, const float* afKnot);

    BSplineBasis(const BSplineBasis &basis);
    BSplineBasis& operator=(const BSplineBasis &basis);

    ~BSplineBasis();

    int getNumControlPoints() const;
    int getDegree() const;
    bool isOpen() const;
    bool isUniform() const;

    // The knot values can be changed only if the basis function is nonuniform
    // and the input index is valid (0 <= i <= n-d-1).  If these conditions
    // are not satisfied, getKnot returns MAX_REAL.
    void setKnot(int i, float fKnot);
    float getKnot(int i) const;

    // access basis functions and their derivatives
    float getD0(int i) const;
    float getD1(int i) const;
    float getD2(int i) const;
    float getD3(int i) const;

    // evaluate basis functions and their derivatives
    void compute(float fTime, unsigned int uiOrder, int &riMinIndex, int &riMaxIndex) const;

 protected:
    int initialize(int iNumCtrlPoints, int iDegree, bool bOpen);
    float** allocate() const;
    void deallocate(float** aafArray);

    // Determine knot index i for which knot[i] <= rfTime < knot[i+1].
    int getKey(float& rfTime) const;

    int mNumCtrlPoints;    // n+1
    int mDegree;           // d
    float *mKnots;          // knot[n+d+2]
    bool mOpen, mUniform;

    // Storage for the basis functions and their derivatives first three
    // derivatives.  The basis array is always allocated by the constructor
    // calls.  A derivative basis array is allocated on the first call to a
    // derivative member function.
    float **m_aafBD0;             // bd0[d+1][n+d+1]
    mutable float **m_aafBD1;     // bd1[d+1][n+d+1]
    mutable float **m_aafBD2;     // bd2[d+1][n+d+1]
    mutable float **m_aafBD3;     // bd3[d+1][n+d+1]
};

template<int D, typename T> class KINSKI_API BSpline
{
  public:
    
    using VecT = typename Vector<D, T>::Type;
    
    // Construction and destruction.  The caller is responsible for deleting
    // the input arrays if they were dynamically allocated.  Internal copies
    // of the arrays are made, so to dynamically change control points or
    // knots you must use the 'setControlPoint', 'getControlPoint', and
    // 'Knot' member functions.

    // Uniform spline.  The number of control points is n+1 >= 2.  The degree
    // of the B-spline is d and must satisfy 1 <= d <= n.  The knots are
    // implicitly calculated in [0,1].  If bOpen is 'true', the spline is
    // open and the knots are
    //   t[i] = 0,               0 <= i <= d
    //          (i-d)/(n+1-d),   d+1 <= i <= n
    //          1,               n+1 <= i <= n+d+1
    // If bOpen is 'false', the spline is periodic and the knots are
    //   t[i] = (i-d)/(n+1-d),   0 <= i <= n+d+1
    // If bLoop is 'true', extra control points are added to generate a closed
    // curve.  For an open spline, the control point array is reallocated and
    // one extra control point is added, set to the first control point
    // C[n+1] = C[0].  For a periodic spline, the control point array is
    // reallocated and the first d points are replicated.  In either case the
    // knot array is calculated accordingly.
    BSpline(const std::vector<VecT> &points, int degree, bool loop, bool open);
    
    // Open, nonuniform spline.  The knot array must have n-d elements.  The
    // elements must be nondecreasing.  Each element must be in [0,1].
    BSpline() : m_num_control_points(-1), m_control_points(0){}
    
    BSpline(int numControlPoints, const VecT *controlPoints, int degree, bool loop,
            const float *knots);
    
    BSpline(const BSpline &bspline);
    BSpline& operator=(const BSpline &bspline);

    ~BSpline();

    int num_control_points() const;
    
    int degree() const;
    
    int num_spans() const;
    
    bool open() const;
    
    bool uniform() const;
    
    bool loop() const;

    // Control points may be changed at any time.  The input index should be
    // valid (0 <= i <= n).  If it is invalid, getControlPoint returns a
    // vector whose components are all MAX_REAL.
    VecT control_point(int i) const;
    void set_control_point(int i, const VecT &rkCtrl);

    // The knot values can be changed only if the basis function is nonuniform
    // and the input index is valid (0 <= i <= n-d-1).  If these conditions
    // are not satisfied, getKnot returns MAX_REAL.
    void set_knot(int i, float fKnot);
    float knot(int i) const;

    // The spline is defined for 0 <= t <= 1.  If a t-value is outside [0,1],
    // an open spline clamps t to [0,1].  That is, if t > 1, t is set to 1;
    // if t < 0, t is set to 0.  A periodic spline wraps to to [0,1].  That
    // is, if t is outside [0,1], then t is set to t-floor(t).
    VecT position(float t) const;
    VecT derivative(float t) const;
    VecT second_derivative(float t) const;
    VecT third_derivative(float t) const;

    T speed(float t) const;

    float length(float fT0, float fT1) const;

    // If you need position and derivatives at the same time, it is more
    // efficient to call these functions.  Pass the addresses of those
    // quantities whose values you want.  You may pass 0 in any argument
    // whose value you do not want.
    void get(float t, VecT *position, VecT *first_derivative = nullptr,
             VecT *second_derivative = nullptr, VecT *third_derivative = nullptr) const;
    
    //! Returns the time associated with an arc length in the range [0,getLength(0,1)]
    float time(float length) const;

    // Access the basis function to compute it without control points.  This
    // is useful for least squares fitting of curves.
    BSplineBasis& basis();

 private:
    // Replicate the necessary number of control points when the create
    // function has bLoop equal to true, in which case the spline curve must
    // be a closed curve.
    void create_control(const VecT *akCtrlPoint);

    int m_num_control_points;
    VecT *m_control_points;  // ctrl[n+1]
    bool m_loop;
    BSplineBasis m_basis;
    int m_replicate;  // the number of replicated control points
};
    
extern template class KINSKI_API BSpline<2, float>;
extern template class KINSKI_API BSpline<3, float>;
extern template class KINSKI_API BSpline<4, float>;
    
using BSpline2f = BSpline<2, float>;
using BSpline3f = BSpline<3, float>;

}} // namespaces

