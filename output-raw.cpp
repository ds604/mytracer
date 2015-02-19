#include <iostream>
#include <list>
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
      red = r;
      green = g;
      blue = b;
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

std::list<std::pair<Vector, Color>> spheres;
int resolution = 128;

float pixelCoordinateToWorldCoordinate(int coordinate) {
  return ((coordinate / (float)resolution) - 0.5f) * 2.0f;
}

Vector spherePoint(Vector rayOrigin, Vector rayDirection, float t) {
  return rayOrigin + (rayDirection * t);
}

std::pair<bool, Vector> calculateSphereIntersection(
    Vector sphereCenter,
    Vector rayOrigin,
    Vector rayDirection) {
  float sphereRadius = 0.5f;
  Vector l = sphereCenter - rayOrigin;
  float s = l.dot(rayDirection);
  float lSquared = l.dot(l);
  float sphereRadiusSquared = sphereRadius * sphereRadius;
  if (s < 0 && lSquared > sphereRadiusSquared) return std::make_pair(false, Vector());
  float mSquared = lSquared - (s * s);
  if (mSquared > sphereRadiusSquared) return std::make_pair(false, Vector());
  float q = sqrt(sphereRadiusSquared - mSquared);
  float t = 0.0;
  if (lSquared > sphereRadiusSquared) t = s - q;
  else t = s + q;
  if (t > 0.00001f) {
    return std::make_pair(true, spherePoint(rayOrigin, rayDirection, t));
  } else {
    return std::make_pair(false, Vector());
  }
}

float calculateLambert(Vector sphereCenter, Vector intersection) {
  Vector lightPosition(0.5f, 0.5f, 0.0f);
  Vector lightDirection = (lightPosition - intersection).normalized();
  Vector sphereNormal = (intersection - sphereCenter).normalized();
  return std::max(0.0f, lightDirection.dot(sphereNormal));
}

bool isShadowed(Vector point, std::list<std::pair<Vector, Color>> spheres) {
  Vector lightPosition(0.5f, 0.5f, 0.0f);
  Vector lightDirection = (lightPosition - point).normalized();
  for(std::pair<Vector, Color> sphere : spheres) {
    if(calculateSphereIntersection(sphere.first, point, lightDirection).first) {
      return true;
    }
  }
  return false;
}

void renderImage(uint8_t* pixels) {
  spheres.push_back(std::make_pair(Vector(0.0f, 0.3f, -1.0f), Color(1.0f, 0.0f, 0.0f)));
  spheres.push_back(std::make_pair(Vector(0.0f, -0.3f, -1.0f), Color(0.96f, 0.94f, 0.32f)));
  uint8_t* p = pixels;
  for(int i = 0; i < resolution; ++i) {
    for(int j = 0; j < resolution; ++j) {
      Color pixelColor;
      Vector rayOrigin(
          pixelCoordinateToWorldCoordinate(j),
          pixelCoordinateToWorldCoordinate(i),
          0.0f);
      Vector rayDirection(0.0f, 0.0f, -1.0f);
      std::list<std::pair<Vector, Color>>::iterator sphereIt = spheres.begin();
      while(sphereIt != spheres.end()) {
        std::pair<Vector, Color> sphere = *sphereIt;
        std::pair<bool, Vector> sphereIntersection = calculateSphereIntersection(
            sphere.first,
            rayOrigin,
            rayDirection);
        if(sphereIntersection.first) {
          if(isShadowed(sphereIntersection.second, spheres)) {
            pixelColor = pixelColor + Color(0.0f, 0.0f, 0.0f);
          } else {
            pixelColor = sphere.second *
              calculateLambert(sphere.first, sphereIntersection.second);
          }
        }
        sphereIt++;
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

int main() {
  FILE* outputFile = fopen("output.tga", "wb");

  uint8_t* pixels = (uint8_t*)malloc(resolution * resolution * 3);
  uint8_t* p = pixels;
  uint8_t blue = 0;
  for(int i = 0; i < resolution; ++i) {
    uint8_t green = 0;
    for(int j = 0; j < resolution; ++j) {
      *p = blue & 0xFF; p++;
      *p = green & 0xFF; p++;
      *p = 0x0; p++;
      green += (uint8_t)(255.0f / (float)resolution);
    }
    blue += (uint8_t)(255.0f / (float)resolution);
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
