/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * The particle system is a plugin: the kernel asks the core factory for one and only ever
 * holds the contract.  This locks the two halves of that seam that can be tested without a
 * simulation: an unregistered factory yields null, and a registered one is used exactly once
 * per call.
 *
 * The third behaviour - a run that asks for particles when nothing is registered - is the
 * kernel's own check in WarpX's constructor.  It cannot be exercised here because linking this
 * test pulls in the particle plugin, which registers the real factory; that case belongs to
 * the particle-free build, which does not exist yet (see the roadmap, stage D).
 */
#include "Runtime/IParticleSystem.H"
#include "Runtime/ServiceFactory.H"

#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

namespace
{

void check (bool condition, std::string const& what)
{
    if (!condition) { throw std::runtime_error("check failed: " + what); }
}

// A minimal stand-in: the factory only has to prove it is called and that its result is what
// the core hands back.
struct StubSystem : warpx::runtime::IParticleSystem
{
    static int built;
};

int StubSystem::built = 0;

} // namespace

int main ()
{
    try
    {
        // Nothing has registered yet in this process, so the core must say so by returning
        // null rather than handing out something half-built.
        check(warpx::runtime::create_particle_system(nullptr) == nullptr,
              "an unregistered factory yields null");

        auto factory = [] (void*) -> std::unique_ptr<warpx::runtime::IParticleSystem> {
            ++StubSystem::built;
            return nullptr;
        };
        warpx::runtime::register_particle_system_factory(factory);

        (void)warpx::runtime::create_particle_system(nullptr);
        (void)warpx::runtime::create_particle_system(nullptr);
        check(StubSystem::built == 2, "the registered factory is called once per request");
    }
    catch (std::exception const& e)
    {
        std::cerr << "test_particle_factory: " << e.what() << '\n';
        return 1;
    }
    std::cout << "test_particle_factory: ok\n";
    return 0;
}
