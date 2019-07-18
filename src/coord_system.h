#ifndef _COORD_SYSTEM_H
#define _COORD_SYSTEM_H

#include <cstdint>
#include <random>
#include <utility>

#include "agl.h"
#include "types.h"

/*
   Sistema gerador das coordenadas
   ----------------------------
   Fornece funções para gerar coordenadas aleatorias
   para colocar varios elementos no jogo. Ele fornece tanto coordenadas gerais
   aleatorias e coordenadas aleatorias especificas para um dos 4 quadrantes.
 */

namespace coordinateGenerator {
// -- 2D PLANE: Y-coord = 0 -- //

// random coordinate for a specific quadrant
std::pair<float, float> genCoord2D(uint8_t quadrant);

// random coordinate generator for 2D plane
std::pair<float, float> randomCoord2D();

// coordinates for each quadrant in a 2D plane
// all are abstractions on top of genCoord2D
std::pair<float, float> firstQuadCoord2D();
std::pair<float, float> secondQuadCoord2D();
std::pair<float, float> thirdQuadCoord2D();
std::pair<float, float> fourthQuadCoord2D();

// -- 3D CUBE: Y-coord > 0 -- //

// random coordinate for a specific quadrant
agl::Point3 genCoord3D(uint8_t quadrant);

// random coordinate generator for 3D plane
agl::Point3 randomCoord3D();

// coordinates for each quadrant in 3D cube space, with y > 0
agl::Point3 firstQuadCoord3D();
agl::Point3 secondQuadCoord3D();
agl::Point3 thirdQuadCoord3D();
agl::Point3 fourthQuadCoord3D();

} // namespace coordinateGenerator

#endif // _COORD_SYSTEM_H
