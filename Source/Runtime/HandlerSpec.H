/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef WARPX_RUNTIME_HANDLERSPEC_H_
#define WARPX_RUNTIME_HANDLERSPEC_H_

#include "Runtime/StagePoint.H"

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

namespace warpx::runtime
{

class DataBus;
struct RuntimeContext;

/** Per-handler data contract. Declarations only; the bus owns no storage. */
struct HandlerSpec
{
    std::string name;
    StagePoint point;
    std::vector<std::string> reads_fields;
    std::vector<std::string> writes_fields;
    std::vector<std::string> reads_species;
    std::vector<std::string> writes_species;
    int priority = 0;
};

/** Execution context passed to a handler (A3.3a skeleton). */
struct StageContext
{
    const RuntimeContext* context = nullptr;
    DataBus* bus = nullptr;
};

class IHandler
{
public:
    virtual ~IHandler () = default;
    [[nodiscard]] virtual HandlerSpec spec () const = 0;
    virtual void apply (StageContext&) = 0;
};

/**
 * \brief Minimal handler registry (A3.3a).
 *
 * A3.3a only stores handlers and orders them by StagePoint/priority. Data
 * dependency topological sorting is added in A3.3c.
 */
class PluginHost
{
public:
    void add (std::shared_ptr<IHandler> handler)
    {
        m_handlers.push_back(std::move(handler));
    }

    [[nodiscard]] std::size_t size () const { return m_handlers.size(); }

    [[nodiscard]] std::vector<IHandler*> ordered () const
    {
        std::vector<IHandler*> out;
        out.reserve(m_handlers.size());
        for (const auto& h : m_handlers) { out.push_back(h.get()); }
        std::stable_sort(out.begin(), out.end(), [](const IHandler* a, const IHandler* b) {
            const HandlerSpec sa = a->spec();
            const HandlerSpec sb = b->spec();
            if (!(sa.point == sb.point)) { return sa.point < sb.point; }
            return sa.priority < sb.priority;
        });
        return out;
    }

    [[nodiscard]] std::vector<IHandler*> at (Stage stage) const
    {
        std::vector<IHandler*> out;
        for (auto* h : ordered()) {
            if (h->spec().point.stage == stage) { out.push_back(h); }
        }
        return out;
    }

private:
    std::vector<std::shared_ptr<IHandler>> m_handlers;
};

} // namespace warpx::runtime

#endif // WARPX_RUNTIME_HANDLERSPEC_H_
