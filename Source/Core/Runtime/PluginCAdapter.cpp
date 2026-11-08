/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Host side of the C plugin ABI (v3): wraps a WarpXHandlerDesc in an IHandler
 * and implements the exported C functions.  No C type here contains a C++
 * object, so a plugin built by another compiler/STL can register handlers.
 */
#include "Runtime/PluginCAdapter.H"

#include "Runtime/DataBus.H"
#include "Runtime/HandlerSpec.H"
#include "Runtime/StageRegistry.H"

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

/** \brief Definition of the opaque C stage context.
 *
 * Plugins only see accessors; the layout stays private to the host.
 */
struct WarpXStageContext
{
    const warpx::runtime::RuntimeContext* context = nullptr;
    warpx::runtime::DataBus* bus = nullptr;
    warpx::runtime::HostServices* host = nullptr;
    const warpx::runtime::RuntimeData* data = nullptr;
    const warpx::runtime::RuntimeConfigSnapshot* config = nullptr;
    int step = 0;
    double time = 0.0;
    double dt = 0.0;
};

namespace warpx::runtime
{
namespace
{

// The C enum must track warpx::runtime::Stage; a silent reorder would move
// every external handler to a different stage.
static_assert(Stage::BeforeInit == WARPX_STAGE_BEFORE_INIT, "Stage drift");
static_assert(Stage::Init == WARPX_STAGE_INIT, "Stage drift");
static_assert(Stage::AfterInit == WARPX_STAGE_AFTER_INIT, "Stage drift");
static_assert(Stage::BeforePush == WARPX_STAGE_BEFORE_PUSH, "Stage drift");
static_assert(Stage::Push == WARPX_STAGE_PUSH, "Stage drift");
static_assert(Stage::AfterPush == WARPX_STAGE_AFTER_PUSH, "Stage drift");
static_assert(Stage::BeforeCollisions == WARPX_STAGE_BEFORE_COLLISIONS, "Stage drift");
static_assert(Stage::Collisions == WARPX_STAGE_COLLISIONS, "Stage drift");
static_assert(Stage::AfterCollisions == WARPX_STAGE_AFTER_COLLISIONS, "Stage drift");
static_assert(Stage::BeforeFieldSolve == WARPX_STAGE_BEFORE_FIELD_SOLVE, "Stage drift");
static_assert(Stage::FieldSolve == WARPX_STAGE_FIELD_SOLVE, "Stage drift");
static_assert(Stage::AfterFieldSolve == WARPX_STAGE_AFTER_FIELD_SOLVE, "Stage drift");
static_assert(Stage::Diagnostics == WARPX_STAGE_DIAGNOSTICS, "Stage drift");
static_assert(Stage::Finalize == WARPX_STAGE_FINALIZE, "Stage drift");

static_assert(std::is_standard_layout_v<WarpXHandlerDesc>,
              "WarpXHandlerDesc must stay a C struct");
static_assert(std::is_trivially_copyable_v<WarpXHandlerDesc>,
              "WarpXHandlerDesc must stay trivially copyable");
static_assert(std::is_standard_layout_v<WarpXPluginDesc>,
              "WarpXPluginDesc must stay a C struct");
static_assert(std::is_trivially_copyable_v<WarpXPluginDesc>,
              "WarpXPluginDesc must stay trivially copyable");

std::vector<std::string>
copy_list (const char* const* items, std::uint32_t count)
{
    std::vector<std::string> out;
    if (items == nullptr) { return out; }
    out.reserve(count);
    for (std::uint32_t i = 0; i < count; ++i) {
        if (items[i] != nullptr) { out.emplace_back(items[i]); }
    }
    return out;
}

//! IHandler implementation owned by the host, backed by a C descriptor.
class CApiHandler final : public IHandler
{
public:
    explicit CApiHandler (const WarpXHandlerDesc& desc)
        : m_name(desc.name != nullptr ? desc.name : ""),
          m_stage(desc.stage),
          m_sub(desc.sub),
          m_priority(desc.priority),
          m_reads_fields(copy_list(desc.reads_fields, desc.n_reads_fields)),
          m_writes_fields(copy_list(desc.writes_fields, desc.n_writes_fields)),
          m_reads_species(copy_list(desc.reads_species, desc.n_reads_species)),
          m_writes_species(copy_list(desc.writes_species, desc.n_writes_species)),
          m_reads_scalars(copy_list(desc.reads_scalars, desc.n_reads_scalars)),
          m_writes_scalars(copy_list(desc.writes_scalars, desc.n_writes_scalars)),
          m_apply(desc.apply),
          m_on_regrid(desc.on_regrid),
          m_user_data(desc.user_data)
    {
        if (!StageRegistry::instance().contains(desc.stage)) {
            throw std::runtime_error("CApiHandler '" + m_name + "': stage " +
                                     StageRegistry::instance().name_of(desc.stage) +
                                     " is not registered");
        }
        if (m_apply == nullptr) {
            throw std::runtime_error("CApiHandler '" + m_name + "': null apply()");
        }
    }

    [[nodiscard]] HandlerSpec spec () const override
    {
        return HandlerSpec{
            .name = m_name,
            .point = {m_stage, m_sub},
            .reads_fields = m_reads_fields,
            .writes_fields = m_writes_fields,
            .reads_species = m_reads_species,
            .writes_species = m_writes_species,
            .priority = m_priority,
            .reads_scalars = m_reads_scalars,
            .writes_scalars = m_writes_scalars};
    }

    void apply (StageContext& ctx) override
    {
        WarpXStageContext cctx{ctx.context, ctx.bus, ctx.host, ctx.data, ctx.config,
                               ctx.step, ctx.time, ctx.dt};
        m_apply(&cctx, m_user_data);
    }

    void on_regrid (StageContext& ctx) override
    {
        if (m_on_regrid == nullptr) { return; }
        WarpXStageContext cctx{ctx.context, ctx.bus, ctx.host, ctx.data, ctx.config,
                               ctx.step, ctx.time, ctx.dt};
        m_on_regrid(&cctx, m_user_data);
    }

private:
    std::string m_name;
    int m_stage;
    int m_sub;
    int m_priority;
    std::vector<std::string> m_reads_fields;
    std::vector<std::string> m_writes_fields;
    std::vector<std::string> m_reads_species;
    std::vector<std::string> m_writes_species;
    std::vector<std::string> m_reads_scalars;
    std::vector<std::string> m_writes_scalars;
    void (*m_apply) (WarpXStageContext*, void*);
    void (*m_on_regrid) (WarpXStageContext*, void*);
    void* m_user_data;
};

} // namespace

WarpXPluginHost*
plugin_host_c_handle (PluginHost& host)
{
    return reinterpret_cast<WarpXPluginHost*>(&host);
}

} // namespace warpx::runtime

extern "C" int
warpx_host_add_handler (WarpXPluginHost* host, const WarpXHandlerDesc* desc)
{
    if (host == nullptr || desc == nullptr) { return 1; }
    if (desc->struct_size != sizeof(WarpXHandlerDesc)) { return 2; }
    if (desc->name == nullptr) { return 3; }
    auto* plugin_host = reinterpret_cast<warpx::runtime::PluginHost*>(host);
    try {
        plugin_host->add(std::make_shared<warpx::runtime::CApiHandler>(*desc));
    } catch (const std::exception&) {
        // Duplicate writer, duplicate name, invalid stage, ...
        return 4;
    }
    return 0;
}

extern "C" int32_t
warpx_stage_step (const WarpXStageContext* ctx)
{
    return ctx != nullptr ? static_cast<int32_t>(ctx->step) : 0;
}

extern "C" double
warpx_stage_time (const WarpXStageContext* ctx)
{
    return ctx != nullptr ? ctx->time : 0.0;
}

extern "C" double
warpx_stage_dt (const WarpXStageContext* ctx)
{
    return ctx != nullptr ? ctx->dt : 0.0;
}

extern "C" int32_t
warpx_stage_has_field (WarpXStageContext* ctx, const char* name, int32_t lev)
{
    if (ctx == nullptr || ctx->bus == nullptr || name == nullptr) { return 0; }
    return ctx->bus->has(std::string(name), lev) ? 1 : 0;
}

extern "C" int32_t
warpx_stage_read_field (WarpXStageContext* ctx, const char* name, int32_t lev, void** multifab)
{
    if (ctx == nullptr || ctx->bus == nullptr || name == nullptr) { return 0; }
    const amrex::MultiFab* field = ctx->bus->field(std::string(name), lev);
    if (field == nullptr) { return 0; }
    if (multifab != nullptr) { *multifab = const_cast<amrex::MultiFab*>(field); }
    return 1;
}

extern "C" int32_t
warpx_stage_scalar (WarpXStageContext* ctx, const char* name, double* value)
{
    if (ctx == nullptr || ctx->bus == nullptr || name == nullptr) { return 0; }
    // Exceptions must not cross the C boundary: report failure instead.
    try {
        const double v = ctx->bus->scalar(std::string(name));
        if (value != nullptr) { *value = v; }
        return 1;
    } catch (const std::exception&) {
        return 0;
    }
}

extern "C" int32_t
warpx_stage_set_scalar (WarpXStageContext* ctx, const char* name, double value)
{
    if (ctx == nullptr || ctx->bus == nullptr || name == nullptr) { return 0; }
    try {
        ctx->bus->set_scalar(std::string(name), value);
        return 1;
    } catch (const std::exception&) {
        return 0;
    }
}

extern "C" int32_t
warpx_stage_field (WarpXStageContext* ctx, const char* name, int32_t lev, void** multifab)
{
    if (ctx == nullptr || ctx->bus == nullptr || name == nullptr) { return 0; }
    amrex::MultiFab* field = ctx->bus->field(std::string(name), lev);
    if (field == nullptr) { return 0; }
    if (multifab != nullptr) { *multifab = static_cast<void*>(field); }
    return 1;
}
