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

#include "BSpline.hpp"

namespace kinski{ namespace gl {

template<typename T,int ORDER>
T rombergIntegral(T a, T b, const std::function<T(T)> &SPEEDFN)
{
    static_assert(ORDER > 2, "ORDER must be greater than 2");
    T rom[2][ORDER];
    memset(rom, 0, sizeof(rom));
    T half = b - a;
    
    rom[0][0] = ((T)0.5) * half * (SPEEDFN(a)+SPEEDFN(b));
    
    for(int i0 = 2, iP0 = 1; i0 <= ORDER; i0++, iP0 *= 2, half *= (T)0.5)
    {
        // approximations via the trapezoid rule
        T sum = 0;
        for(int i1 = 1; i1 <= iP0; i1++)
            sum += SPEEDFN(a + half * (i1 - ((T)0.5)));
        
        // Richardson extrapolation
        rom[1][0] = ((T)0.5) * (rom[0][0] + half*sum);
        for(int i2 = 1, iP2 = 4; i2 < i0; i2++, iP2 *= 4)
            rom[1][i2] = (iP2 * rom[1][i2 - 1] - rom[0][i2 - 1]) / (iP2 - 1);
        
        for(int i1 = 0; i1 < i0; i1++)
            rom[0][i1] = rom[1][i1];
    }
    return rom[0][ORDER-1];
}
    
//////////////////////////////////////////////////////////////////////////////////////////////
    
class BSplineBasis
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
    
// BSplineBasis
template <class T>
void allocate2D(int iCols, int iRows, T**& raatArray)
{
    raatArray = new T*[iRows];
    raatArray[0] = new T[iRows*iCols];
    for(int iRow = 1; iRow < iRows; iRow++) {
        raatArray[iRow] = &raatArray[0][iCols*iRow];
    }
}

template <class T>
void deallocate2D(T**& raatArray)
{
    if(raatArray)
    {
        delete raatArray[0];
        delete raatArray;
        raatArray = 0;
    }
}

BSplineBasis::BSplineBasis():
mNumCtrlPoints(-1), mKnots(0), m_aafBD0(0), m_aafBD1(0), m_aafBD2(0), m_aafBD3(0)
{
    
}

BSplineBasis::BSplineBasis(int iNumCtrlPoints, int iDegree, bool bOpen)
{
    create(iNumCtrlPoints, iDegree, bOpen);
}

BSplineBasis::BSplineBasis(const BSplineBasis &basis):
mKnots(0), m_aafBD0(0), m_aafBD1(0), m_aafBD2(0), m_aafBD3(0)
//    : mNumCtrlPoints(basis.mNumCtrlPoints), mDegree(basis.mDegree), mOpen(basis.mOpen), mUniform(basis.mUniform)
{
/*    int numKnots = initialize(mNumCtrlPoints, mDegree, mOpen);
    memcpy(mKnots, basis.mKnots, sizeof(float) * numKnots);*/
    *this = basis;
}

BSplineBasis& BSplineBasis::operator=(const BSplineBasis &basis)
{
    delete[] mKnots;
    deallocate2D(m_aafBD0);
    deallocate2D(m_aafBD1);
    deallocate2D(m_aafBD2);
    deallocate2D(m_aafBD3);

    mNumCtrlPoints = basis.mNumCtrlPoints;
    mDegree = basis.mDegree;
    mOpen = basis.mOpen;
    mUniform = basis.mUniform;
    
    if(mNumCtrlPoints > 0) {
        int numKnots = initialize(mNumCtrlPoints, mDegree, mOpen);
        memcpy(mKnots, basis.mKnots, sizeof(float) * numKnots);
    }
    else {
        mKnots = 0;
        m_aafBD0 = m_aafBD1 = m_aafBD2 = m_aafBD3 = 0;
    }

    return *this;
}

void BSplineBasis::create(int iNumCtrlPoints, int iDegree, bool bOpen)
{
    mUniform = true;

    int i, iNumKnots = initialize(iNumCtrlPoints, iDegree, bOpen);
    float fFactor = (1.0f)/(iNumCtrlPoints - mDegree);
    if (mOpen) {
        for (i = 0; i <= mDegree; i++) {
            mKnots[i] = (float)0.0;
        }

        for (/**/; i < iNumCtrlPoints; i++) {
            mKnots[i] = (i - mDegree) * fFactor;
        }

        for(/**/; i < iNumKnots; i++) {
            mKnots[i] = (float)1.0;
        }
    }
    else {
        for (i = 0; i < iNumKnots; i++) {
            mKnots[i] = (i - mDegree) * fFactor;
        }
    }
}

BSplineBasis::BSplineBasis(int aNumCtrlPoints, int iDegree, const float *afKnot)
{
    create(aNumCtrlPoints, iDegree, afKnot);
}

void BSplineBasis::create(int aNumCtrlPoints, int iDegree, const float *afKnot)
{
    mUniform = false;

    mNumCtrlPoints = aNumCtrlPoints;

    int i, iNumKnots = initialize(mNumCtrlPoints, iDegree, true);
    for(i = 0; i <= mDegree; i++) {
        mKnots[i] = (float)0.0;
    }

    for(int j = 0; i < mNumCtrlPoints; i++, j++) {
        mKnots[i] = afKnot[j];
    }

    for(/**/; i < iNumKnots; i++) {
        mKnots[i] = (float)1.0;
    }
}

BSplineBasis::~BSplineBasis()
{
    delete [] mKnots;
    deallocate2D(m_aafBD0);
    deallocate2D(m_aafBD1);
    deallocate2D(m_aafBD2);
    deallocate2D(m_aafBD3);
}

int BSplineBasis::getNumControlPoints() const
{
    return mNumCtrlPoints;
}

int BSplineBasis::getDegree() const
{
    return mDegree;
}


bool BSplineBasis::isOpen() const
{
    return mOpen;
}


bool BSplineBasis::isUniform() const
{
    return mUniform;
}


float BSplineBasis::getD0(int i) const
{
    return m_aafBD0[mDegree][i];
}


float BSplineBasis::getD1(int i) const
{
    return m_aafBD1[mDegree][i];
}


float BSplineBasis::getD2(int i) const
{
    return m_aafBD2[mDegree][i];
}


float BSplineBasis::getD3(int i) const
{
    return m_aafBD3[mDegree][i];
}


float** BSplineBasis::allocate() const
{
    int iRows = mDegree + 1;
    int iCols = mNumCtrlPoints + mDegree;
    float** aafArray;
    allocate2D<float>(iCols, iRows, aafArray);
    memset(aafArray[0],0,iRows*iCols*sizeof(float));
    return aafArray;
}

int BSplineBasis::initialize(int iNumCtrlPoints, int iDegree, bool bOpen)
{
    assert(iNumCtrlPoints >= 2);
    assert(1 <= iDegree && iDegree <= iNumCtrlPoints-1);

    mNumCtrlPoints = iNumCtrlPoints;
    mDegree = iDegree;
    mOpen = bOpen;

    int iNumKnots = mNumCtrlPoints+mDegree+1;
    mKnots = new float[iNumKnots];

    m_aafBD0 = allocate();
    m_aafBD1 = 0;
    m_aafBD2 = 0;
    m_aafBD3 = 0;

    return iNumKnots;
}

void BSplineBasis::setKnot(int i, float fKnot)
{
    if(! mUniform) {
        // access only allowed to elements d+1 <= j <= n
        int j = i + mDegree + 1;
        if(mDegree+1 <= j && j <= mNumCtrlPoints - 1) {
            mKnots[j] = fKnot;
        }
    }
}

float BSplineBasis::getKnot(int i) const
{
    if(! mUniform) {
        // access only allowed to elements d+1 <= j <= n
        int j = i + mDegree + 1;
        if ((mDegree + 1 <= j) && (j <= mNumCtrlPoints - 1)) {
            return mKnots[j];
        }
    }

    return std::numeric_limits<float>::max();
}


int BSplineBasis::getKey(float& rfTime) const
{
    if(mOpen) {
        // open splines clamp to [0,1]
        if(rfTime <= (float)0.0) {
            rfTime = (float)0.0;
            return mDegree;
        }
        else if (rfTime >= (float)1.0) {
            rfTime = (float)1.0;
            return mNumCtrlPoints - 1;
        }
    }
    else {
        // periodic splines wrap to [0,1]
        if (rfTime < (float)0.0 || rfTime >= (float)1.0) {
            rfTime -= floorf(rfTime);
        }
    }


    int i;
    if(mUniform) {
        i = mDegree + (int)((mNumCtrlPoints - mDegree) * rfTime);
    }
    else {
        for(i = mDegree + 1; i <= mNumCtrlPoints; i++) {
            if(rfTime < mKnots[i]) {
                break;
            }
        }
        i--;
    }

    return i;
}

void BSplineBasis::compute(float fTime, unsigned int uiOrder, int &riMinIndex, int &riMaxIndex) const
{
    // only derivatives through third order currently supported
    assert(uiOrder <= 3);

    if (uiOrder >= 1) {
        if (!m_aafBD1) {
            m_aafBD1 = allocate();
        }

        if (uiOrder >= 2) {
            if(! m_aafBD2) {
                m_aafBD2 = allocate();
            }

            if(uiOrder >= 3) {
                if (! m_aafBD3) {
                    m_aafBD3 = allocate();
                }
            }
        }
    }

    int i = getKey(fTime);
    m_aafBD0[0][i] = (float)1.0;

    if(uiOrder >= 1) {
        m_aafBD1[0][i] = (float)0.0;
        if (uiOrder >= 2) {
            m_aafBD2[0][i] = (float)0.0;
            if (uiOrder >= 3) {
                m_aafBD3[0][i] = (float)0.0;
            }
        }
    }

    float fN0 = fTime-mKnots[i], fN1 = mKnots[i+1] - fTime;
    float fInvD0, fInvD1;
    int j;
    for(j = 1; j <= mDegree; j++) {
        fInvD0 = ((float)1.0)/(mKnots[i+j]-mKnots[i]);
        fInvD1 = ((float)1.0)/(mKnots[i+1]-mKnots[i-j+1]);

        m_aafBD0[j][i] = fN0*m_aafBD0[j-1][i]*fInvD0;
        m_aafBD0[j][i-j] = fN1*m_aafBD0[j-1][i-j+1]*fInvD1;

        if(uiOrder >= 1) {
            m_aafBD1[j][i] = (fN0*m_aafBD1[j-1][i]+m_aafBD0[j-1][i])*fInvD0;
            m_aafBD1[j][i-j] = (fN1*m_aafBD1[j-1][i-j+1]-m_aafBD0[j-1][i-j+1])
                *fInvD1;

            if(uiOrder >= 2) {
                m_aafBD2[j][i] = (fN0*m_aafBD2[j-1][i] +
                    ((float)2.0)*m_aafBD1[j-1][i])*fInvD0;
                m_aafBD2[j][i-j] = (fN1*m_aafBD2[j-1][i-j+1] -
                    ((float)2.0)*m_aafBD1[j-1][i-j+1])*fInvD1;

                if (uiOrder >= 3) {
                    m_aafBD3[j][i] = (fN0*m_aafBD3[j-1][i] +
                        ((float)3.0)*m_aafBD2[j-1][i])*fInvD0;
                    m_aafBD3[j][i-j] = (fN1*m_aafBD3[j-1][i-j+1] -
                        ((float)3.0)*m_aafBD2[j-1][i-j+1])*fInvD1;
                }
            }
        }
    }

    for(j = 2; j <= mDegree; j++) {
        for(int k = i-j+1; k < i; k++) {
            fN0 = fTime-mKnots[k];
            fN1 = mKnots[k+j+1]-fTime;
            fInvD0 = ((float)1.0)/(mKnots[k+j]-mKnots[k]);
            fInvD1 = ((float)1.0)/(mKnots[k+j+1]-mKnots[k+1]);

            m_aafBD0[j][k] = fN0*m_aafBD0[j-1][k]*fInvD0 + fN1*
                m_aafBD0[j-1][k+1]*fInvD1;

            if(uiOrder >= 1) {
                m_aafBD1[j][k] = (fN0*m_aafBD1[j-1][k]+m_aafBD0[j-1][k])*
                    fInvD0 + (fN1*m_aafBD1[j-1][k+1]-m_aafBD0[j-1][k+1])*
                    fInvD1;

                if(uiOrder >= 2) {
                    m_aafBD2[j][k] = (fN0*m_aafBD2[j-1][k] +
                        ((float)2.0)*m_aafBD1[j-1][k])*fInvD0 +
                        (fN1*m_aafBD2[j-1][k+1]- ((float)2.0)*
                        m_aafBD1[j-1][k+1])*fInvD1;

                    if(uiOrder >= 3) {
                        m_aafBD3[j][k] = (fN0*m_aafBD3[j-1][k] +
                            ((float)3.0)*m_aafBD2[j-1][k])*fInvD0 +
                            (fN1*m_aafBD3[j-1][k+1] - ((float)3.0)*
                            m_aafBD2[j-1][k+1])*fInvD1;
                    }
                }
            }
        }
    }

    riMinIndex = i - mDegree;
    riMaxIndex = i;
}

//////////////////////////////////////////////////////////////////////////////////////////////
// BSpline

template<int D,typename T>
BSpline<D,T>::BSpline():
m_basis(std::make_unique<BSplineBasis>()),
m_num_control_points(-1),
m_control_points(nullptr)
{
    
}
    
template<int D,typename T>
BSpline<D,T>::BSpline(const std::vector<VecT> &points, int degree, bool loop, bool open):
m_basis(std::make_unique<BSplineBasis>()),
m_control_points(nullptr),
m_loop(loop)
{
    assert(points.size() >= 2);
    assert((1 <= degree) && (degree <= (int)points.size() - 1));

    m_num_control_points = (int)points.size();
    m_replicate = (m_loop ? (open ? 1 : degree) : 0);
    create_control(&points[0]);
    m_basis->create(m_num_control_points + m_replicate, degree, open);
}

template<int D,typename T>
BSpline<D,T>::BSpline(int numControlPoints, const typename BSpline<D,T>::VecT *controlPoints,
                      int degree, bool loop, const float *knots):
m_basis(std::make_unique<BSplineBasis>()),
m_num_control_points(numControlPoints),
m_control_points(nullptr),
m_loop(loop)
{
    assert(m_num_control_points >= 2);
    assert((1 <= degree) && (degree <= m_num_control_points - 1));

    m_replicate = m_loop ? 1 : 0;
    create_control(controlPoints);
    m_basis->create(m_num_control_points + m_replicate, degree, knots);
}
    
template<int D,typename T>
BSpline<D,T>::BSpline(BSpline &&other):
m_basis(std::move(other.m_basis)),
m_num_control_points(other.m_num_control_points),
m_control_points(other.m_control_points),
m_loop(other.m_loop),
m_replicate(other.m_replicate)
{
    other.m_control_points = nullptr;
    other.m_num_control_points = 0;
    other.m_loop = false;
}
    
template<int D,typename T>
BSpline<D,T>::BSpline(const BSpline &other):
m_basis(std::make_unique<BSplineBasis>()),
m_num_control_points(0),
m_control_points(nullptr)
{
    m_num_control_points = other.m_num_control_points;
    m_loop = other.m_loop;
    *m_basis = *other.m_basis;
    m_replicate = other.m_replicate;

    if(m_num_control_points > 0){ create_control(other.m_control_points); }
    else{ m_num_control_points = 0; }
}

template<int D,typename T>
BSpline<D,T>& BSpline<D,T>::operator=(BSpline other)
{
    std::swap(m_basis, other.m_basis);
    std::swap(m_num_control_points, other.m_num_control_points);
    std::swap(m_control_points, other.m_control_points);
    std::swap(m_loop, other.m_loop);
    std::swap(m_replicate, other.m_replicate);
    return *this;
}

template<int D, typename T>
BSpline<D,T>::~BSpline()
{
    delete[] m_control_points;
}
    
template<int D, typename T>
int BSpline<D,T>::num_control_points() const { return m_num_control_points; }

template<int D, typename T>
int BSpline<D,T>::degree() const { return m_basis->getDegree(); }

template<int D, typename T>
int BSpline<D,T>::num_spans() const { return m_num_control_points - m_basis->getDegree(); }

template<int D, typename T>
bool BSpline<D,T>::open() const { return m_basis->isOpen(); }

template<int D, typename T>
bool BSpline<D,T>::uniform() const { return m_basis->isUniform(); }

template<int D, typename T>
bool BSpline<D,T>::loop() const { return m_loop; }
    
template<int D, typename T>
void BSpline<D,T>::create_control(const VecT *akCtrlPoint)
{
    if(m_control_points){ delete[] m_control_points; }
    int iNewNumCtrlPoints = m_num_control_points + m_replicate;
    m_control_points = new VecT[iNewNumCtrlPoints];
    size_t uiSrcSize = m_num_control_points * sizeof(VecT);
    memcpy(m_control_points, akCtrlPoint, uiSrcSize);
    
    for(int i = 0; i < m_replicate; i++)
    {
        m_control_points[m_num_control_points + i] = akCtrlPoint[i];
    }
}

template<int D,typename T>
void BSpline<D,T>::set_control_point(int i, const VecT &rkCtrl)
{
    assert(i >= 0 && i < m_num_control_points);

    // set the control point
    m_control_points[i] = rkCtrl;

    // set the replicated control point
    if(i < m_replicate){ m_control_points[m_num_control_points + i] = rkCtrl; }
}

template<int D,typename T>
typename BSpline<D,T>::VecT BSpline<D,T>::control_point(int i) const
{
    assert(i >= 0 && i < m_num_control_points);

    return m_control_points[i];
}

template<int D,typename T>
void BSpline<D,T>::set_knot(int i, float fKnot)
{
    m_basis->setKnot(i, fKnot);
}

template<int D,typename T>
float BSpline<D,T>::knot(int i) const
{
    return m_basis->getKnot(i);
}

template<int D,typename T>
void BSpline<D,T>::get(float t, VecT *the_position, VecT *firstDerivative,
                       VecT *secondDerivative, VecT *thirdDerivative) const
{
    int i, iMin, iMax;
    
    if(thirdDerivative){ m_basis->compute(t, 3, iMin, iMax); }
    else if(secondDerivative){ m_basis->compute(t, 2, iMin, iMax); }
    else if(firstDerivative){ m_basis->compute(t, 1, iMin, iMax); }
    else{ m_basis->compute(t, 0, iMin, iMax); }

    if(the_position)
    {
        *the_position = VecT();
        
        for(i = iMin; i <= iMax; i++)
        {
            float weight = m_basis->getD0(i);
            *the_position += m_control_points[i] * weight;
        }
    }

    if(firstDerivative)
    {
        *firstDerivative = VecT();
        for(i = iMin; i <= iMax; i++){ *firstDerivative += m_control_points[i] * m_basis->getD1(i); }
    }

    if(secondDerivative)
    {
        *secondDerivative = VecT();
        for(i = iMin; i <= iMax; i++){ *secondDerivative += m_control_points[i] * m_basis->getD2(i); }
    }

    if(thirdDerivative)
    {
        *thirdDerivative = VecT();
        for(i = iMin; i <= iMax; i++){ *thirdDerivative += m_control_points[i] * m_basis->getD3(i); }
    }
}

template<int D,typename T>
float BSpline<D,T>::time(float the_length) const
{
    const size_t MAX_ITERATIONS = 32;
    const float TOLERANCE = 1.0e-03f;
    // ensure that we remain within valid parameter space
    float totalLength = length(0, 1);
    if(the_length >= totalLength)
        return 1;
    if(the_length <= 0)
        return 0;

    // initialize bisection endpoints
    float a = 0, b = 1;
    float p = the_length / totalLength;    // make first guess

    // iterate and look for zeros
    for (size_t i = 0; i < MAX_ITERATIONS; ++i)
    {
        // compute function value and test against zero
        float func = length(0, p) - the_length;
        if(std::abs(func) < TOLERANCE){ return p; }

         // update bisection endpoints
        if(func < 0){ a = p; }
        else{ b = p; }

        // get speed along curve
        float s = speed(p);

        // if result will lie outside [a,b]
        if(((p-a) * s - func) * ((p - b) * s - func) > - TOLERANCE)
        {
            // do bisection
            p = 0.5f * (a + b);
        }
        // otherwise Newton-Raphson
        else { p -= func / s; }
    }
    
    // We failed to converge, but hopefully 'p' is close enough anyway
    return p;
}

template<int D,typename T>
float BSpline<D,T>::length(float fT0, float fT1) const
{
    if(fT0 >= fT1)
        return (float)0.0;

    return rombergIntegral<T,10>(fT0, fT1, [this](float v){ return speed(v); });
}

//template<int D,typename T>
//BSplineBasis& BSpline<D,T>::basis()
//{
//    return m_basis;
//}

template<int D,typename T>
typename BSpline<D,T>::VecT BSpline<D,T>::position(float t) const
{
    VecT pos;
    get(t, &pos, 0, 0, 0);
    return pos;
}

template<int D,typename T>
typename BSpline<D,T>::VecT BSpline<D,T>::derivative(float t) const
{
    VecT d1;
    get(t, 0, &d1, 0, 0);
    return d1;
}

template<int D,typename T>
typename BSpline<D,T>::VecT BSpline<D,T>::second_derivative(float t) const
{
    VecT d2;
    get(t, 0, 0, &d2, 0);
    return d2;
}

template<int D,typename T>
typename BSpline<D,T>::VecT BSpline<D,T>::third_derivative(float t) const
{
    VecT d3;
    get(t, 0, 0, 0, &d3);
    return d3;
}

template<int D,typename T>
T BSpline<D,T>::speed(float t) const
{
    return glm::length(derivative(t));
}

// explicit template instantiations
template class KINSKI_API BSpline<2, float>;
template class KINSKI_API BSpline<3, float>;
template class KINSKI_API BSpline<4, float>;

}} // namespaces
