/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef WARPX_RUNTIME_RUNTIME_H_
#define WARPX_RUNTIME_RUNTIME_H_

namespace warpx::runtime
{

/**
 * \brief Process-level owner of the simulation lifetime.
 *
 * This is the first step of the plugin-runtime refactor (see
 * Docs/research_notes/最终的技术路线.md). It currently forwards to the legacy
 * WarpX singleton, so behaviour is unchanged. Later steps move configuration,
 * time state, services, and the plugin host into this class.
 */
class Runtime
{
public:
    Runtime (int argc, char* argv[]);
    ~Runtime ();

    Runtime (Runtime const &) = delete;
    Runtime& operator= (Runtime const &) = delete;
    Runtime (Runtime &&) = delete;
    Runtime& operator= (Runtime &&) = delete;

    /** Allocate state and run the package initialization (currently WarpX::InitData). */
    void initialize ();

    /** Advance the simulation. */
    void evolve (int numsteps = -1);

    /** Release simulation state. Idempotent. */
    void finalize ();

    [[nodiscard]] bool verbose () const;

private:
    bool m_initialized = false;
    bool m_finalized = false;
};

} // namespace warpx::runtime

#endif // WARPX_RUNTIME_RUNTIME_H_
