/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef WARPX_RUNTIME_MATERIALREGISTRY_H_
#define WARPX_RUNTIME_MATERIALREGISTRY_H_

#include "Runtime/MaterialSpec.H"

#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace warpx::runtime
{

/** \brief Named material coefficient bundles (A4.3). */
class MaterialRegistry
{
public:
    void declare (const MaterialSpec& spec)
    {
        auto [it, inserted] = m_materials.emplace(spec.name, spec);
        if (inserted) { return; }
        const MaterialSpec& old = it->second;
        auto same6 = [](const amrex::GpuArray<amrex::Real, 6>& a,
                        const amrex::GpuArray<amrex::Real, 6>& b) {
            for (int i = 0; i < 6; ++i) {
                if (a[i] != b[i]) { return false; }
            }
            return true;
        };
        if (old.epsilon != spec.epsilon || old.sigma != spec.sigma ||
            old.mu != spec.mu || old.epsilon2 != spec.epsilon2 ||
            old.has_interface != spec.has_interface ||
            old.has_eps_tensor != spec.has_eps_tensor ||
            old.has_eps2_tensor != spec.has_eps2_tensor ||
            old.has_sigma_tensor != spec.has_sigma_tensor ||
            !same6(old.eps_tensor, spec.eps_tensor) ||
            !same6(old.eps2_tensor, spec.eps2_tensor) ||
            !same6(old.sigma_tensor, spec.sigma_tensor)) {
            throw std::runtime_error(
                "MaterialRegistry: conflicting declaration for material '" + spec.name + "'");
        }
    }

    [[nodiscard]] bool contains (const std::string& name) const
    {
        return m_materials.find(name) != m_materials.end();
    }

    [[nodiscard]] const MaterialSpec& get (const std::string& name) const
    {
        const auto it = m_materials.find(name);
        if (it == m_materials.end()) {
            throw std::runtime_error("MaterialRegistry: unknown material " + name);
        }
        return it->second;
    }

    [[nodiscard]] std::size_t size () const { return m_materials.size(); }

    [[nodiscard]] std::vector<std::string> names () const
    {
        std::vector<std::string> out;
        out.reserve(m_materials.size());
        for (const auto& [name, spec] : m_materials) {
            (void)spec;
            out.push_back(name);
        }
        return out;
    }

private:
    std::map<std::string, MaterialSpec> m_materials;
};

} // namespace warpx::runtime

#endif // WARPX_RUNTIME_MATERIALREGISTRY_H_
