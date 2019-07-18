#include "coord_system.h"

namespace coordinateGenerator {

std::pair<float, float> genCoord2D(uint8_t quadrant) {
  // C++11 new random lib
  std::random_device rd;
  std::mt19937 mt(rd()); // random generator
  
  std::uniform_real_distribution<double> dist(0.0, elements::FLOOR_SIZE - 50);

  int angle = quadrant * 90;            // angulo base relativo ao quadrante
  angle += ((int)dist(mt) % (int)91.0); // random angulo entre 0-90
  angle *= M_PI / 180.0f;               // passando para rads
  float d =
      dist(mt) + 30.0f; // permite colocar a distancia 30 a partir(0,0)

  // return the pair of coordinates
  return {
      d * cos(angle), // x
      d * sin(angle)  // z
  };
}

std::pair<float, float> randomCoord2D() {
  std::random_device rd;
  std::mt19937 mt(rd()); 

  // select a random quadrant
  uint8_t quadrant = mt() % 4;

  return genCoord2D(quadrant);
}

std::pair<float, float> firstQuadCoord2D() {
  // select 1 quadrant
  uint8_t quadrant = 0;
  return genCoord2D(quadrant);
}

std::pair<float, float> secondQuadCoord2D() {
  // select 2 quadrant
  uint8_t quadrant = 1;
  return genCoord2D(quadrant);
}

std::pair<float, float> thirdQuadCoord2D() {
  // select 3 quadrant
  uint8_t quadrant = 2;
  return genCoord2D(quadrant);
}

std::pair<float, float> fourthQuadCoord2D() {
  // select 4 quadrant
  uint8_t quadrant = 3;
  return genCoord2D(quadrant);
}

// 3D CUBE

agl::Point3 genCoord3D(uint8_t quadrant) {
  float x, y, z;

  std::random_device rd;
  std::mt19937 mt(rd()); // random generator
  
  std::uniform_real_distribution<double> space(0.0, elements::FLOOR_SIZE - 50);
  std::uniform_real_distribution<double> height(0.0, elements::SKY_RADIUS);

  double angle = quadrant * 90;          
  angle += ((int)space(mt) % (int)91.0); 
  angle *= M_PI / 180.0f;                
  float d = space(mt) +
            30.0f; 

  // project coords to cartesian
  x = d * cos(angle), z = d * sin(angle);
  // Y is plain easy
  y = height(mt);
  +5;
  agl::Point3 coord = agl::Point3(x, y, z);

  return coord;
}

agl::Point3 randomCoord3D() {
  
  std::random_device rd;
  std::mt19937 mt(rd()); //random generator

   uint8_t quadrant = mt() % 4;

  return genCoord3D(quadrant);
}

agl::Point3 firstQuadCoord3D() {
  // select 1 quadrant
  uint8_t quadrant = 0;
  return genCoord3D(quadrant);
}

agl::Point3 secondQuadCoord3D() {
  // select 2 quadrant
  uint8_t quadrant = 1;
  return genCoord3D(quadrant);
}

agl::Point3 thirdQuadCoord3D() {
  // select 3 quadrant
  uint8_t quadrant = 2;
  return genCoord3D(quadrant);
}

agl::Point3 fourthQuadCoord3D() {
  // select 4 quadrant
  uint8_t quadrant = 3;
  return genCoord3D(quadrant);
}

} 
