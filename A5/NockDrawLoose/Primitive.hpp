#pragma once

#include <glm/glm.hpp>

class Primitive {
public:
  virtual ~Primitive();
  virtual float intersects(glm::vec3 lookFrom, glm::vec3 direction) = 0;
  virtual glm::vec3 getPosition();
  double boxIntersection(glm::vec3 pos, glm::vec3 maxPos, glm::vec3 lookFrom, glm::vec3 direction);
  void getT(float posD, float lookFromD, float directionD, double maxPosD, double t[2]);
};

class Sphere : public Primitive {
public:
  virtual ~Sphere();
  float intersects(glm::vec3 lookFrom, glm::vec3 direction) override;
};

class Cube : public Primitive {
public:
  virtual ~Cube();
  float intersects(glm::vec3 lookFrom, glm::vec3 direction) override;
private:
  glm::vec3 updateNormal(glm::vec3 point);
};

class NonhierSphere : public Primitive {
public:
  NonhierSphere(const glm::vec3& pos, double radius)
    : m_pos(pos), m_radius(radius)
  {
  }
  virtual ~NonhierSphere();
  float intersects(glm::vec3 lookFrom, glm::vec3 direction) override;
  glm::vec3 getPosition() override;
private:
  glm::vec3 m_pos;
  double m_radius;
};

class NonhierBox : public Primitive {
public:
  NonhierBox(const glm::vec3& pos, double size)
    : m_pos(pos), m_size(size)
  {
  }

  virtual ~NonhierBox();
  float intersects(glm::vec3 lookFrom, glm::vec3 direction) override;
private:
  glm::vec3 updateNormal(glm::vec3 point);

  glm::vec3 m_pos;
  double m_size;
};
