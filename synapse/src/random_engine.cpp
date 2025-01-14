#include <memory>

#include "random_engine.hpp"

namespace synapse {
std::unique_ptr<RandomEngine> RandomEngine::engine;
}