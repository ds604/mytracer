#include <iostream>
#include <list>
#include <queue>
#include <cmath>
#include <stdint.h>

class Vector {
  private:
    float x;
    float y;
    float z;
  public:
    Vector() : x(0), y(0), z(0) {}
    Vector(float x, float y, float z) : x(x), y(y), z(z) {}
    Vector(const Vector & v) : x(v.x), y(v.y), z(v.z) {}
    float length() const {
      return sqrt(x * x + y * y + z * z);
    }
    float dot(const Vector& v) const {
      return (x * v.x) + (y * v.y) + (z * v.z);
    }
    Vector normalized() const {
      return (*this) * (1.0f / length());
    }
    Vector operator - (const Vector& v) const {
      return Vector(
          x - v.x,
          y - v.y,
          z - v.z);
    }
    Vector operator + (const Vector& v) const {
      return Vector(
          x + v.x,
          y + v.y,
          z + v.z);
    }
    Vector operator * (float scale) const {
      return Vector(
          x * scale,
          y * scale,
          z * scale);
    }
    std::string toString() const {
      char buffer[100];
      snprintf(buffer, 100, "x: %.3f y: %.3f z: %.3f", x, y, z);
      return buffer;
    }
};

class Color {
  private:
    bool defined;
    float red;
    float green;
    float blue;
  public:
    Color() : defined(false), red(0.0f), green(0.0f), blue(0.0f) {}
    Color(float r, float g, float b) {
      defined = true;
      red = std::max(std::min(r, 1.0f), 0.0f);
      green = std::max(std::min(g, 1.0f), 0.0f);
      blue = std::max(std::min(b, 1.0f), 0.0f);
    }
    Color operator * (float scale) const {
      return Color(
          red * scale,
          green * scale,
          blue * scale);
    }
    Color operator + (const Color& color) const {
      return Color(
          red + color.red,
          green + color.green,
          blue + color.blue);
    }
    bool isDefined() const { return defined; }
    uint8_t redByte() const { return red * 0xFF; }
    uint8_t greenByte() const { return green * 0xFF; }
    uint8_t blueByte() const { return blue * 0xFF; }
};

struct Material {
  float specValue;
  float specPower;
};

typedef std::pair<Vector, Color> Sphere;
typedef std::pair<Sphere, Vector> IntersectionPoint;
int resolution = 512;

std::list<Sphere> spheres;
std::list<Vector> lights;

float pixelCoordinateToWorldCoordinate(int coordinate) {
  return ((coordinate / (float)resolution) - 0.5f) * 2.0f;
}

Vector spherePoint(Vector rayOrigin, Vector rayDirection, float t) {
  return rayOrigin + (rayDirection * t);
}

std::vector<float> raySphereIntersections(
    const Sphere& sphere,
    const Vector& rayOrigin,
    const Vector& rayDirection) {
  Vector sphereCenter = sphere.first;
  float sphereRadius = 0.5f;
  Vector l = sphereCenter - rayOrigin;
  float s = l.dot(rayDirection);
  float lSquared = l.dot(l);
  float sphereRadiusSquared = sphereRadius * sphereRadius;
  if (s < 0 && lSquared > sphereRadiusSquared) return {};
  float mSquared = lSquared - (s * s);
  if (mSquared > sphereRadiusSquared) return {};
  float q = sqrt(sphereRadiusSquared - mSquared);
  float t = 0.0;
  return { s - q, s + q };
}

std::pair<bool, IntersectionPoint> closestSphereIntersection(
    std::list<Sphere> spheres,
    Vector rayOrigin,
    Vector rayDirection) {
  bool intersectionFound = false;
  float tMin = 0.0;
  std::pair<bool, IntersectionPoint> ret = std::make_pair(
      false, std::make_pair(std::make_pair(Vector(), Color()), Vector()));
  for(Sphere sphere : spheres) {
    std::vector<float> intersections = raySphereIntersections(sphere, rayOrigin, rayDirection);
    for(float t : intersections) {
      if (t > 0.00001f && (!intersectionFound || t < tMin)) {
        intersectionFound = true;
        tMin = t;
        ret = std::make_pair(
            true,
            std::make_pair(sphere, spherePoint(rayOrigin, rayDirection, t)));
      }
    }
  }
  return ret;
}

float calculateLambert(Vector sphereCenter, Vector intersection, Vector lightPosition) {
  Vector lightDirection = (lightPosition - intersection).normalized();
  Vector sphereNormal = (intersection - sphereCenter).normalized();
  return std::max(0.0f, lightDirection.dot(sphereNormal));
}

float calculatePhong(Vector sphereCenter, Vector intersection, Vector lightPosition, Vector rayOrigin, Material sphereMaterial) {
  Vector sphereNormal = (intersection - sphereCenter).normalized();
  Vector lightDirection = (lightPosition - intersection).normalized();
  Vector viewDirection = (intersection - rayOrigin).normalized();
  Vector blinnDirection = (lightDirection - viewDirection).normalized();
  float blinnTerm = std::max(blinnDirection.dot(sphereNormal), 0.0f);
  return sphereMaterial.specValue * powf(blinnTerm, sphereMaterial.specPower);
}

bool isShadowed(Vector point, std::list<Sphere> spheres, Vector lightPosition) {
  Vector lightDirection = (lightPosition - point).normalized();
  return closestSphereIntersection(spheres, point, lightDirection).first;
}

Color contributionFromLight(IntersectionPoint intersectionPoint, Sphere intersectionSphere, std::list<Sphere> spheres, Vector lightPosition, Vector rayOrigin, Material sphereMaterial) {
  if(isShadowed(intersectionPoint.second, spheres, lightPosition)) {
    return Color(0.0f, 0.0f, 0.0f);
  } else {
    float phongTerm = calculatePhong(intersectionSphere.first, intersectionPoint.second, lightPosition, rayOrigin, sphereMaterial);
    float lambertTerm = calculateLambert(intersectionSphere.first, intersectionPoint.second, lightPosition);
    return (intersectionSphere.second * lambertTerm) + (intersectionSphere.second * phongTerm);
  }
}

Color ambientLight(Sphere intersectionSphere) {
  float ambientStrength = 0.1f;
  return intersectionSphere.second * ambientStrength;
}

void renderImage(uint8_t* pixels) {
  Material sphereMaterial = { 5.0, 100.0 };
  spheres.push_back(std::make_pair(Vector(0.0f, 0.5f, -1.0f), Color(1.0f, 0.0f, 0.0f)));
  spheres.push_back(std::make_pair(Vector(0.0f, -0.5f, -1.0f), Color(0.96f, 0.94f, 0.32f)));
  lights.push_back(Vector(0.5f, 0.5f, 0.0f));
  lights.push_back(Vector(-3.0f, -0.0f, -2.0f));

  uint8_t* p = pixels;
  for(int i = 0; i < resolution; ++i) {
    for(int j = 0; j < resolution; ++j) {
      int currentDepth = 0;
      Color pixelColor;
      float reflectionFactor = 1.0f;
      Vector rayOrigin(
          pixelCoordinateToWorldCoordinate(j),
          pixelCoordinateToWorldCoordinate(i),
          0.0f);
      Vector rayDirection(0.0f, 0.0f, -1.0f);
      while(currentDepth < 10) {
        std::pair<bool, IntersectionPoint> sphereIntersection = closestSphereIntersection(
            spheres,
            rayOrigin,
            rayDirection);
        if(sphereIntersection.first && currentDepth == 0) {
          IntersectionPoint intersectionPoint = sphereIntersection.second;
          Sphere intersectionSphere = intersectionPoint.first;
          pixelColor = pixelColor + ambientLight(intersectionSphere);
        }
        if(sphereIntersection.first) {
          IntersectionPoint intersectionPoint = sphereIntersection.second;
          Sphere intersectionSphere = intersectionPoint.first;
          for(Vector light : lights) {
            pixelColor = pixelColor + (contributionFromLight(intersectionPoint, intersectionSphere, spheres, light, rayOrigin, sphereMaterial) * reflectionFactor);
          }
          reflectionFactor = reflectionFactor * 0.6f;
          Vector sphereNormal = (intersectionPoint.second - intersectionSphere.first).normalized();
          float reflect = 2.0f * (rayDirection.dot(sphereNormal));
          rayOrigin = intersectionPoint.second;
          rayDirection = rayDirection - (sphereNormal * reflect);
          currentDepth++;
        } else {
          currentDepth = 10;
        }
      }
      if(pixelColor.isDefined()) {
        *p = pixelColor.blueByte() & 0xFF; p++;
        *p = pixelColor.greenByte() & 0xFF; p++;
        *p = pixelColor.redByte() & 0xFF; p++;
      } else {
        p += 3;
      }
    }
  }
}

uint8_t roundToInt(float value) {
  return (uint8_t)(value + 0.5);
}

int main() {
  FILE* outputFile = fopen("output.tga", "wb");

  uint8_t* pixels = (uint8_t*)malloc(resolution * resolution * 3);
  uint8_t* p = pixels;
  float blue = 0;
  for(int i = 0; i < resolution; ++i) {
    float green = 0;
    for(int j = 0; j < resolution; ++j) {
      *p = roundToInt(blue) & 0xFF; p++;
      *p = roundToInt(green) & 0xFF; p++;
      *p = 0x0; p++;
      green += 255.0f / (float)resolution;
    }
    blue += 255.0f / (float)resolution;
  }

  uint8_t tgaHeader[18] = {0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

  tgaHeader[12] = resolution & 0xFF;
  tgaHeader[13] = (resolution >> 8) & 0xFF;
  tgaHeader[14] = (resolution) & 0xFF;
  tgaHeader[15] = (resolution >> 8) & 0xFF;
  tgaHeader[16] = 24; 
  
  renderImage(pixels);

  fwrite(tgaHeader, sizeof(uint8_t), 18, outputFile);
  fwrite(pixels, sizeof(uint8_t), resolution * resolution * 3, outputFile);
  fclose(outputFile);

  free(pixels);

  return 0;
}
