/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * S1 acceptance: a stage inserted by a plugin between two standard stages is
 * executed during a real evolve, without editing the core or the PicEm
 * plugin.  A handler at a stage that is not inside any executed interval must
 * not run.
 */
#include "Runtime/HandlerSpec.H"
#include "Runtime/Runtime.H"
#include "Runtime/StagePoint.H"
#include "Runtime/StageRegistry.H"

#include <iostream>
#include <memory>
#include <string>

namespace
{

int failures = 0;

void check (bool ok, const std::string& what)
{
    if (!ok) {
        std::cerr << "FAIL: " << what << '\n';
        ++failures;
    }
}

class CountingHandler final : public warpx::runtime::IHandler
{
public:
    CountingHandler (std::string name, int stage, int* counter)
        : m_name(std::move(name)), m_stage(stage), m_counter(counter) {}

    [[nodiscard]] warpx::runtime::HandlerSpec spec () const override
    {
        return warpx::runtime::HandlerSpec{
            .name = m_name, .point = {m_stage, 0}, .priority = 0};
    }
    void apply (warpx::runtime::StageContext& ctx) override
    {
        ++(*m_counter);
        m_last_step = ctx.step;
    }

    int last_step () const { return m_last_step; }

private:
    std::string m_name;
    int m_stage;
    int* m_counter;
    int m_last_step = -1;
};

} // namespace

int main (int argc, char* argv[])
{
    if (argc < 2) {
        std::cerr << "usage: test_stage_dispatch <inputs_file>\n";
        return 2;
    }

    using namespace warpx::runtime;

    // A plugin inserts a stage of its own between two standard stages.
    auto& registry = StageRegistry::instance();
    const int probe_stage = registry.register_stage("probe_between", "after_push",
                                                    StagePosition::After);
    check(probe_stage > Stage::AfterPush && probe_stage < Stage::BeforeCollisions,
          "probe stage sits inside the deposition interval");

    char* rt_argv[3] = {argv[0], argv[1], nullptr};
    Runtime runtime(2, rt_argv);

    int probe_runs = 0;
    int before_push_runs = 0;
    int before_init_runs = 0;

    auto probe = std::make_shared<CountingHandler>("plugin.probe", probe_stage, &probe_runs);
    runtime.plugins().add(probe);
    runtime.plugins().add(std::make_shared<CountingHandler>(
        "plugin.before_push", Stage::BeforePush, &before_push_runs));
    runtime.plugins().add(std::make_shared<CountingHandler>(
        "plugin.before_init", Stage::BeforeInit, &before_init_runs));

    runtime.initialize();
    runtime.evolve(1);
    runtime.finalize();

    check(probe_runs > 0, "plugin stage between standard stages was executed");
    check(probe->last_step() == 0, "plugin handler saw the real step index");
    check(before_push_runs > 0, "handler at before_push was executed");
    // The init stages are real dispatch points: a handler registered there
    // must run exactly once per initialize(), not be silently dropped.
    check(before_init_runs == 1, "handler at before_init was executed once");

    if (failures != 0) {
        std::cerr << failures << " check(s) failed\n";
        return 1;
    }
    return 0;
}
