#include <LibCore/RandomEngine.h>

#include <memory>

namespace LibCore {

std::unique_ptr<RandomUniformEngine> SingletonRandomEngine::engine;

void SingletonRandomEngine::seed(u32 rand_seed) { engine = std::unique_ptr<RandomUniformEngine>(new RandomUniformEngine(rand_seed)); }

i32 SingletonRandomEngine::generate() {
  assert(engine && "RandomEngine not seeded");
  return engine->generate();
}

} // namespace LibCore