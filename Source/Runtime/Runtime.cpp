/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "Runtime/Runtime.H"

#include "Initialization/WarpXInit.H"
#include "Runtime/WarpXRuntimeData.H"
#include "WarpX.H"

#include <stdexcept>

namespace warpx::runtime
{

namespace
{

TimeState make_time ()
{
    const auto& warpx = WarpX::GetInstance();
    TimeState t;
    t.step = warpx.getistep(0);
    t.finest_level = warpx.finestLevel();
    t.time = warpx.gett_new(0);
    t.istep = warpx.getistep();
    t.nsubsteps = warpx.getnsubsteps();
    t.dt = warpx.getdt();
    t.t_new = warpx.gett_new();
    t.t_old = warpx.gett_old();
    return t;
}

FrameState make_frame ()
{
    FrameState f;
    f.gamma_boost = WarpX::gamma_boost;
    f.beta_boost = WarpX::beta_boost;
    f.boost_direction = WarpX::boost_direction;
    f.do_moving_window = (WarpX::do_moving_window != 0);
    f.moving_window_dir = WarpX::moving_window_dir;
    f.moving_window_v = WarpX::moving_window_v;
    f.start_moving_window_step = WarpX::start_moving_window_step;
    f.end_moving_window_step = WarpX::end_moving_window_step;
    return f;
}

} // namespace

Runtime::Runtime (int argc, char* argv[])
{
    warpx::initialization::initialize_external_libraries(argc, argv);

    // Construct WarpX (reads parameters) before InitData, so diagnostics and
    // other setup-time consumers can see a config snapshot.
    auto& warpx = WarpX::GetInstance();
    m_data = std::make_unique<WarpXRuntimeData>(warpx);
    sync_config();
    sync_time();
    m_context = RuntimeContext{&m_config, &m_time, &m_frame, m_data.get(), nullptr,
                               [this] { sync_time(); }};
    warpx.SetRuntimeContext(&m_context);
}

Runtime::~Runtime ()
{
    finalize();
    warpx::initialization::finalize_external_libraries();
}

void
Runtime::initialize ()
{
    if (m_finalized) {
        throw std::runtime_error("Runtime::initialize() called after finalize()");
    }
    WarpX::GetInstance().InitData();
    m_initialized = true;
    sync_state();
}

void
Runtime::evolve (int numsteps)
{
    if (!m_initialized || m_finalized) {
        throw std::runtime_error("Runtime::evolve() called before initialize() or after finalize()");
    }
    WarpX::GetInstance().Evolve(numsteps);
    sync_state();
}

void
Runtime::finalize ()
{
    if (m_finalized) { return; }
    WarpX::Finalize();
    m_finalized = true;
}

bool
Runtime::verbose () const
{
    return m_initialized && !m_finalized && WarpX::GetInstance().Verbose();
}

const RuntimeConfig&
Runtime::config () const
{
    return m_config;
}

const TimeState&
Runtime::time () const
{
    return m_time;
}

const FrameState&
Runtime::frame () const
{
    return m_frame;
}

const RuntimeContext&
Runtime::context () const
{
    return m_context;
}

void
Runtime::sync_config ()
{
    m_config = RuntimeConfig::FromLegacy();
}

void
Runtime::sync_time ()
{
    m_time = make_time();
    m_frame = make_frame();
}

void
Runtime::sync_state ()
{
    sync_config();
    sync_time();
}

} // namespace warpx::runtime
