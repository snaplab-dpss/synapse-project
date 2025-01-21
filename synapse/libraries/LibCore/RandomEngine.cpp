#include <LibCore/RandomEngine.h>

#include <memory>

namespace LibCore {

std::unique_ptr<SingletonRandomEngine> SingletonRandomEngine::engine;

void SingletonRandomEngine::seed(u32 rand_seed) { engine = std::unique_ptr<SingletonRandomEngine>(new SingletonRandomEngine(rand_seed)); }

i32 SingletonRandomEngine::generate() {
  assert(engine && "RandomEngine not seeded");
  return engine->generate();
}

} // namespace LibCore