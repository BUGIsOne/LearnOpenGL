#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <fstream>
#include <vector>
#include <iostream>
#include <cassert>
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
    Vec3<T> cross(const Vec3f<T> &v) const { return Vec3f<T>(y*v.z-z*v.y, z*v.x-x*v.z, x*v.y-y*v.x); }
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
};

typedef Vec3<float> Vec3f;

class Sphere
{
public:
    Vec3f center;
    float radius, radius2;
    // surface color and emission (light)
    Vec3f surfaceColor, emissionColor; 
    // surface transparency and reflectivity
    float transparency, reflection;

    Sphere(
        const Vec3f &c, 
        const float &r, 
        const Vec3f &sc, 
        const float &refl =0, 
        const float &transp =0, 
        const Vec3f &ec =0): center(c), radius(r), radius2(r * r),
            surfaceColor(sc), emissionColor(ec), transparency(transp), 
            reflection(refl) {}

    bool intersect(const Vec3f &rayOrigin, const Vec3f &rayDirection, float &t0, float &t1) const 
    {
        Vec3f l = center - rayOrigin;
        float tca = l.dot(rayDirection);
        if(tca < 0) 
            return false;
        // when rayDireaction is 1, below is right
        float d2 = l.dot(l) - tca * tca;
        if(d2 > radius2)
            return false;
        float thc = sqrt(radius2 - d2);
        t0 = tca - thc;
        t1 = tca + thc;

        return true;
    }
};

#define MAX_RAY_DEPTH 5

float mix(const float &a, const float &b, const float &mix)
{
    // linear interpolation
    return mix*b + (1-mix)*a;
}


// test if ray intersect a object, if it does, compute the shadeing
// shading depends on the surface property (transparent, reflective, diffuse..)
//
// returns a color for the ray, 
// if ray hits the object, the color is the color of the obj at the point
// otherwise returns the background color
Vec3f trace(
    const Vec3f &rayOrigin,
    const Vec3f &rayDirection,
    const vector<Sphere> &spheres,
    const int &depth)
{
    float tnear = INFINITY;
    const Sphere* sphere = NULL;
    // find intersection of ray with the sphere
    for(unsigned i = 0; i<spheres.size(); i++)
    {
        float t0 = INFINITY, t1 = INFINITY;
        if(spheres[i].intersect(rayOrigin, rayDirection, t0, t1))
        {
            // t0 < 0, there will be only one intersected point
            if(t0 < 0)
                t0 = t1;
            if(t0 < tnear)
            {
                tnear = t0;
                sphere = &spheres[i];
            }
        }
    }

    // if there's intersection return black or backgroud colo
    if(!sphere)
        return Vec3f(2);
    
    // color of the ray at the intersected point
    Vec3f surfaceColor = 0;
    // intersected point at the sphere
    Vec3f pHit = rayOrigin + rayDirection * tnear;
    Vec3f nHit = pHit - sphere->center;
    nHit.normalize();
    
    // if the normal and the view direction are not opposite to each other
    // reverse the normal direction
    // we are inside the sphere
    // finally reverse the sign of IdotN which we want positive
    float bias = 1e-4;
    bool inside = false;
    if(rayDirection.dot(nHit) > 0)
    {
        nHit = -nHit;
        inside = true;
    }
    
    if((sphere->transparency > 0 || sphere->reflection > 0 ) && depth < MAX_RAY_DEPTH )
    {
        float facingRatio = -rayDirection.dot(nHit);
        //change the mix value to tweak the effect
        float fresnelEffect = mix(pow(1 - facingRatio, 3), 1, 0.1);

        // compute reflection direction (all vectors are normalized)
        Vec3f reflectDirection = rayDirection - nHit*2*rayDirection.dot(nHit);
        reflectDirection.normalize();
        Vec3f reflection = trace(pHit + nHit*bias, reflectDirection, spheres, depth+1);
        
        Vec3f refraction = 0;
        // if the sphere is also transparent, compute refraction ray (transmission)
        if(sphere->transparency)
        {
            float ior = 1.1, eta = (inside)? ior : 1/ior;
            float cosi = -nHit.dot(rayDirection);
            float k = 1 - eta*eta*(1 - cosi*cosi);
            Vec3f refractDirection = rayDirection*eta + nHit*(eta*cosi - sqrt(k));
            refractDirection.normalize();
            // bias the ray's direction, ?????????
            refraction = trace(pHit - nHit*bias, refractDirection, spheres, depth+1);
        }

        // the result is a mix of reflection and refraction (if transparent)
        surfaceColor = (reflection * fresnelEffect + 
            refraction * (1-fresnelEffect) * sphere->transparency) *
            sphere->surfaceColor;
    }

    else
    {
        // it's a diffuse object, no need to raytrace any further
        for(unsigned i=0; i< spheres.size(); i++)
            // ???????
            if(spheres[i].emissionColor.x > 0)
            {
                // this is a light
                Vec3f transmission = 1;
                Vec3f lightDirection = spheres[i].center - pHit;
                lightDirection.normalize();
                for(unsigned j=0; j<spheres.size(); j++)
                {
                    if(i != j)
                    {
                        float t0, t1;
                        // if ray is a shadow ray
                        if(spheres[j].intersect(pHit + nHit*bias, lightDirection, t0, t1))
                        {
                            transmission = 0;
                            break;
                        }
                    }
                }
                surfaceColor += sphere->surfaceColor * transmission *
                    max(float(0), nHit.dot(lightDirection)) * spheres[i].emissionColor;
            }
    }

    return surfaceColor + sphere->emissionColor;
}


// rendering
// compute a camera ray for each pixel, trace it and return color,
// if the ray hits a shpere, return the color of intersection point,
// else return the background color
void render(const vector<Sphere> &spheres)
{
    unsigned width = 640, height = 480;
    Vec3f *image = new Vec3f[width * height], *pixel = image;
    float invWidth = 1/(float)width, invHeight = 1/(float)height;
    float fov = 30, aspectRatio = width/(float)height;
    float angle = tan(M_PI * 0.5 * fov/180.);

    // trace rays
    for(unsigned y=0; y<height; y++)
    {
        for(unsigned x=0; x<width; x++, pixel++)
        {
            float xx = (2 * ((x+0.5)*invWidth) - 1) * angle * aspectRatio;
            float yy = (1 - 2*((y+0.5)*invHeight)) * angle;

            Vec3f rayDirection(xx, yy, -1);
            rayDirection.normalize();
            *pixel = trace(Vec3f(0), rayDirection, spheres, 0);
        }
    }

    // save result to a ppm
    ofstream ofs("./out.ppm", ios::out | ios::binary);
    ofs << "P6\n" << width << " " << height << "\n255\n";
    for(unsigned i=0; i<width*height; i++)
    {
        ofs << (unsigned char)(min(float(1), image[i].x) * 255) <<
            (unsigned char)(min(float(1), image[i].y) * 255) <<
            (unsigned char)(min(float(1), image[i].z) * 255);
    }
    ofs.close();
    delete []image;
}


int main()
{
    srand(13);
    vector<Sphere> spheres;
    // position, radius, surface color, reflectivity, transparency, emission color
    spheres.push_back(Sphere(Vec3f( 0.0, -10004, -20), 10000, Vec3f(0.20, 0.20, 0.20), 0, 0.0)); 
    spheres.push_back(Sphere(Vec3f( 0.0,      0, -20),     4, Vec3f(1.00, 0.32, 0.36), 1, 0.5)); 
    spheres.push_back(Sphere(Vec3f( 5.0,     -1, -15),     2, Vec3f(0.90, 0.76, 0.46), 1, 0.0)); 
    spheres.push_back(Sphere(Vec3f( 5.0,      0, -25),     3, Vec3f(0.65, 0.77, 0.97), 1, 0.0)); 
    spheres.push_back(Sphere(Vec3f(-5.5,      0, -15),     3, Vec3f(0.90, 0.90, 0.90), 1, 0.0)); 
    // light
    spheres.push_back(Sphere(Vec3f( 0.0,     20, -30),     3, Vec3f(0.00, 0.00, 0.00), 0, 0.0, Vec3f(3))); 
    render(spheres); 
 
    return 0; 
}





