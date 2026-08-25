#include "object.h"

const Vector* Object::GetNormal(size_t index) const {
    if (index < normals.size() && normals[index].has_value()) {
        return &normals[index].value();
    }
    return nullptr;
}
