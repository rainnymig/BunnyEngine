#include "Vertex.h"

namespace Bunny::Render
{
size_t NormalVertex::Hash::operator()(const NormalVertex& v) const
{
    using hashf = std::hash<float>;

    return hashf{}(v.mPosition.x) ^ hashf {}(v.mPosition.y) ^ hashf {}(v.mPosition.z) ^ hashf {}(v.mNormal.x) ^
           hashf {}(v.mNormal.y) ^ hashf {}(v.mNormal.z);
};
} // namespace Bunny::Render
