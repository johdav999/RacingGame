#pragma once
#include "engine/mesh_optimizer.hpp"
#include <memory>

namespace racing::art {
// Prepare once before actors publish the immutable resource. All instances and
// LODs share the imported materials/maps; level zero retains every source face.
inline std::shared_ptr<const engine::Mesh> prepareStatic(const engine::Mesh& source) {
    auto mesh=std::make_shared<engine::Mesh>(source);
    engine::MeshOptimizer::optimize(*mesh);
    engine::MeshOptimizer::generateLods(*mesh);
    return mesh;
}
}
