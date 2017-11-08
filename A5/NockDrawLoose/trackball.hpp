#pragma once

#include <glm/glm.hpp>

/* Function prototypes */
glm::vec3 vCalcRotVec(float fNewX, float fNewY,
                 float fOldX, float fOldY,
                 float fDiameter);
glm::mat4 vAxisRotMatrix(float fVecX, float fVecY, float fVecZ);
