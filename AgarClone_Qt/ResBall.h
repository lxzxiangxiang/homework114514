#pragma once

#include "Entity.h"

class ResBall : public Entity {
public:
    ResBall(qreal mass, QColor color) : Entity(mass, color) {}
};
