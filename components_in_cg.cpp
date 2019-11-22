//matrix
#include <iostream>
#include <cmath>
#include <cstdio>
#include <ctime>
#include "drand48.h"

#define MAX_ITER 10e8
#define M_PI 3.14159265358979323846

using namespace std;

template<typename T>
class Vec3
{
public:
    T x, y, z;
    Vec3(): x(T(0)), y(T(0)), z(T(0)) {}
    Vec3(T xx): x(xx), y(xx), z(xx) {}
    Vec3(T xx, T yy, T zz): x(xx), y(yy), z(zz) {}
    Vec3& normalize()
    {
        T nor2 = length2();
        if(nor2 > 0)
        {
            T invNor = 1/sqrt(nor2);
            x *= invNor, y *= invNor, z *= invNor;
        }
        return *this;
    }

    Vec3<T> operator * (const T &f) const { return Vec3<T>(x*f, y*f, z*f); }
    Vec3<T> operator * (const Vec3<T> &v) const { return Vec3<T>(x*v.x, y*v.y, z*v.z); }
    T dot(const Vec3<T> &v) const { return x*v.x + y*v.y + z*v.z; }
    Vec3<T> cross(const Vec3<T> &v) const { return Vec3<T>(y*v.z-z*v.y, z*v.x-x*v.z, x*v.y-y*v.x); }
    Vec3<T> operator - (const Vec3<T> &v) const { return Vec3<T>(x-v.x, y-v.y, z-v.z); }
    Vec3<T> operator + (const Vec3<T> &v) const { return Vec3<T>(x+v.x, y+v.y, z+v.z); }
    Vec3<T>& operator += (const Vec3<T> &v) { x += v.x, y += v.y, z += v.z; return *this; }
    Vec3<T>& operator *= (const Vec3<T> &v) { x *= v.x, y *= v.y, z *= v.z; return *this; }
    Vec3<T> operator - () const { return Vec3<T>(-x, -y, -z); }
    T length2() const { return x*x + y*y + z*z; }
    T length() const { return sqrt(length2()); }
    
    friend ostream & operator << (ostream &os, const Vec3<T> &v)
    {
        os << "[" << v.x << " " << v.y << " " << v.z << "]";
        return os;
    }

    Vec3<T> sphericalToCartesian(const T &theta, const T &phi)
    {
        return Vec3<T>(cos(phi)*sin(theta), sin(phi)*sin(theta), cos(theta));
    }

    inline T sphericalTheta(const Vec3<T> &v)
    {
        // more safer, v should be normalised before being input
        return acos(clamp<T>(v[2], -1, 1));
    }

    inline T sphericalPhi(const Vec3<T> &v)
    {
        T p = atan2(v[1], v[0]);
        return (p < 0) ? p + 2*M_PI : p;
    }
    
    inline T cosTheta(const Vec3<T> &w) { return w[2]; }
    
    inline T sinTheta2(const Vec3<T> &w)
    {
        return max(T(0), 1 - cosTheta(w)*cosTheta(w));
    }
    inline T sinTheta(const Vec3<T> &w)
    {
        return sqrt(sinTheta2(w));
    }

    inline T cosPhi(const Vec3<T> &w)
    {
        // why not use sphericalPhi? The reason lays in my notebook
        T sintheta = sinTheta(w);
        if(sintheta == 0) 
            return 1;
        return clamp<T>(w[0] / sintheta, -1, 1);
    }
    inline T sinPhi(const Vec3<T> &w)
    {
        T sintheta = sinTheta(w);
        if(sintheta == 0)
            return 0;
        // cosPhi or sinPhi who gets 1, I think it's random
        return clamp<T>(w[1] / sintheta, -1, 1);
    }
};

typedef Vec3<float> Vec3f;


template<typename T>
class Matrix44
{
public:
    Matrix44() {}
    // the two below are accessor
    const T* operator [] (uint8_t i) const { return m[i]; }
    T* operator [] (uint8_t i) { return m[i]; }

    Matrix44 operator * (const Matrix44& rhs) const {
        Matrix44 mult;
        for(uint8_t i=0; i<4; i++)
            for(uint8_t j=0; j<4; j++)
            {
                mult[i][j] = m[i][0] * m[0][j] +
                            m[i][1] * m[1][j] +
                            m[i][2] * m[2][j] +
                            m[i][3] * m[3][j];
            }
        
        return mult;
    }

    /* x,y,z,w de w'
        homogeneous point
    */
    void multVecMatrix(const Vec3<T> &src, Vec3<T> &dst) const
    {
        dst.x = src.x*m[0][0] + src.y*m[1][0] + src.z*m[2][0] + m[3][0];
        dst.y = src.x*m[0][1] + src.y*m[1][1] + src.z*m[2][1] + m[3][1];
        dst.z = src.x*m[0][2] + src.y*m[1][2] + src.z*m[2][2] + m[3][2];
        T w = dst.x = src.x*m[0][3] + src.y*m[1][3] + src.z*m[2][3] + m[3][3];
        if(w != 1 && w != 0)
        {
            // why not 
            dst.x /= w;
            dst.y /= w;
            dst.z /= w;
        }
    }

    // as vectors represent direction, so position is meaningless
    void multDirMatrix(const Vec3<T> &src, Vec3<T> &dst) const
    {
        dst.x = src.x*m[0][0] + src.y*m[1][0] + src.z*m[2][0];
        dst.y = src.x*m[0][1] + src.y*m[1][1] + src.z*m[2][1];
        dst.z = src.x*m[0][2] + src.y*m[1][2] + src.z*m[2][2];
    }
    
    Vec3<T> multVecMatrix(const Vec3<T> &v)
    {
        // row-major order, not recommended
        #ifndef ROWMAJOR
        return Vec3<T>(
            v.x*m[0][0] + v.y*m[1][0] + v.z*m[2][0],
            v.x*m[0][1] + v.y*m[1][1] + v.z*m[2][1],
            v.x*m[0][2] + v.y*m[1][2] + v.z*m[2][2]);
        #else
        // column-major order
        return Vec3<T>(
            v.x*m[0][0] + v.y*m[0][1] + v.z*m[0][2],
            v.x*m[1][0] + v.y*m[1][1] + v.z*m[1][2],
            v.x*m[2][0] + v.y*m[2][1] + v.z*m[2][2]);
        #endif

    }

    Matrix44 transpose() const
    {
        Matrix44 transpMat;
        for(uint8_t i=0; i<4; i++){
            for(uint8_t j=0; j<4; j++){
                transpMat[i][j] = m[j][i];
            }
        }
        return transpMat;
    }


    // initialize the coefficients of the matrix 
    // with the coefficients of the identity matrix
    T m[4][4] = {{1,0,0,0},{0,1,0,0},{0,0,1,0},{0,0,0,1}};
};



typedef Matrix44<float> Matrix44f;

void funcMatrix()
{
    // normal way to use
    Matrix44f mat;
    mat.m[0][3] = 1.f;
    // accessor
    mat[0][3] = 1.f;
}


int main(int argc, char **argv)
{
    clock_t start = clock();
    Vec3<float> v(1, 2, 3);
    Matrix44<float> M;
    float *tmp = &M.m[0][0];
    for(int i=0; i<16; i++) *(tmp + i) = drand48();
    for(int i=0; i<MAX_ITER; i++)
    {
        Vec3<float> vt = M.multVecMatrix(v);
    }
    fprintf(stderr, "Clock time %f\n", (clock() - start) / float(CLOCKS_PER_SEC));

    getchar();

    return 0;
}