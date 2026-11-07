/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef WARPX_RUNTIME_DATABUS_H_
#define WARPX_RUNTIME_DATABUS_H_

#include "Runtime/FieldRegistry.H"
#include "Runtime/SpeciesRegistry.H"

#include <ablastr/fields/MultiFabRegister.H>

#include <AMReX_MultiFab.H>

#include <map>
#include <string>

namespace warpx::runtime
{

/**
 * \brief Thin schema + forwarding layer over the authoritative MultiFabRegister.
 *
 * There is exactly one storage: `WarpX::GetMultiFabRegister()`. DataBus only
 * stores `FieldSpec` declarations and forwards field access/alias creation.
 * It must never copy or re-allocate a field (audit D1/D9).
 */
class DataBus
{
public:
    explicit DataBus (ablastr::fields::MultiFabRegister& fields) : m_fields(fields) {}
    DataBus (ablastr::fields::MultiFabRegister& fields, FieldRegistry& registry)
        : m_fields(fields), m_registry(&registry) {}

    void declare (FieldSpec spec);
    [[nodiscard]] const FieldSpec* spec (const std::string& name) const;
    [[nodiscard]] const FieldRegistry& registry () const { return *m_registry; }

    [[nodiscard]] ablastr::fields::MultiFabRegister& fields () { return m_fields; }
    [[nodiscard]] const ablastr::fields::MultiFabRegister& fields () const { return m_fields; }

    [[nodiscard]] bool has (const std::string& name, int lev) const;
    [[nodiscard]] bool has (const std::string& name, int dir, int lev) const;

    [[nodiscard]] amrex::MultiFab* field (const std::string& name, int lev);
    [[nodiscard]] const amrex::MultiFab* field (const std::string& name, int lev) const;
    [[nodiscard]] amrex::MultiFab* field (const std::string& name, int dir, int lev);
    [[nodiscard]] const amrex::MultiFab* field (const std::string& name, int dir, int lev) const;

    /**
     * \brief Create/validate an alias via Register::alias_init.
     *
     * The target is a new name pointing at the same MultiFab; no data is copied.
     */
    amrex::MultiFab* alias (const std::string& dst, const std::string& src, int dir, int lev);

    void register_species (const SpeciesSpec& spec, WarpXParticleContainer* pc);
    [[nodiscard]] bool has_species (const std::string& name) const;
    [[nodiscard]] SpeciesHandle species (const std::string& name) const;
    [[nodiscard]] const SpeciesRegistry& speciesRegistry () const { return m_species; }

private:
    ablastr::fields::MultiFabRegister& m_fields;
    FieldRegistry m_owned_registry;
    FieldRegistry* m_registry = &m_owned_registry;
    SpeciesRegistry m_species;
};

} // namespace warpx::runtime

#endif // WARPX_RUNTIME_DATABUS_H_
