#ifndef __GEOMETRY_UTILS_HPP
#define __GEOMETRY_UTILS_HPP

#include "common/types.hpp"

namespace GeometryUtils
{

std::tuple<Real,Real,Real> barycentricCoords(const Vec3r& p, const Vec3r& a, const Vec3r& b, const Vec3r& c);
Mat3r quatToMat(const Vec4r& quat);
Vec4r quatMult(const Vec4r& a, const Vec4r& b);
Vec3r rotateVectorByQuat(const Vec3r& v, const Vec4r& quat);
Vec4r inverseQuat(const Vec4r& quat);
Vec4r eulXYZ2Quat(const Real x, const Real y, const Real z);

}

#endif // __GEOMETRY_UTILS_HPP