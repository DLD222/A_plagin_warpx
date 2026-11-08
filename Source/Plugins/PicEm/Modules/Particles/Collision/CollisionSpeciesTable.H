/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef WARPX_COLLISION_SPECIESTABLE_H_
#define WARPX_COLLISION_SPECIESTABLE_H_

#include "Runtime/IParticleContainer.H"
#include "Runtime/IParticleContainerAccess.H"
#include "Runtime/SpeciesSpec.H"

#include <functional>
#include <map>
#include <string>
#include <vector>

class MultiParticleContainer;
class WarpXParticleContainer;

/**
 * \brief Collision-internal species lookup (A3.3b8).
 *
 * Centralizes the `GetParticleContainerFromName` call sites and the few global
 * collision operations (currently CalculateNuei). Container pointers stay
 * internal to the collision package; they are not part of IHandler.
 */
/**
 * \brief Package-private view: the only place collision models obtain the
 * mutable container from a SpeciesHandle. It is deliberately not part of
 * IHandler.
 */
class CollisionSpeciesView
{
public:
    CollisionSpeciesView (warpx::runtime::SpeciesHandle handle,
                          warpx::runtime::IParticleContainer& container)
        : m_handle(std::move(handle)), m_container(&container) {}

    [[nodiscard]] const warpx::runtime::SpeciesHandle& handle () const { return m_handle; }
    [[nodiscard]] warpx::runtime::IParticleContainer& container () const { return *m_container; }

    //! Compatibility for existing model code: `*table.get(name)` / `get(name)->`.
    [[nodiscard]] warpx::runtime::IParticleContainer& operator* () const { return *m_container; }
    [[nodiscard]] warpx::runtime::IParticleContainer* operator-> () const { return m_container; }
    explicit operator bool () const { return m_container != nullptr; }

private:
    warpx::runtime::SpeciesHandle m_handle;
    warpx::runtime::IParticleContainer* m_container = nullptr;
};

class CollisionSpeciesTable
{
public:
    using NueiFunction = std::function<void(const std::string&)>;

    explicit CollisionSpeciesTable (const warpx::runtime::IParticleContainerAccess& mypc,
                                    NueiFunction calculate_nuei = {});

    [[nodiscard]] CollisionSpeciesView get (const std::string& name) const;
    [[nodiscard]] CollisionSpeciesView view (const std::string& name) const;
    [[nodiscard]] int size () const { return static_cast<int>(m_species.size()); }
    [[nodiscard]] std::vector<std::string> names () const;

    void calculateNuei (const std::string& name) const
    {
        if (m_calculate_nuei) { m_calculate_nuei(name); }
    }

private:
    std::map<std::string, warpx::runtime::IParticleContainer*> m_species;
    std::map<std::string, warpx::runtime::SpeciesHandle> m_handles;
    NueiFunction m_calculate_nuei;
};

#endif // WARPX_COLLISION_SPECIESTABLE_H_
