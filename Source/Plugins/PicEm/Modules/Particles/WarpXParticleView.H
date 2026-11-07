#ifndef WARPX_PARTICLEVIEW_H_
#define WARPX_PARTICLEVIEW_H_
#include "Runtime/IParticleContainer.H"
class WarpXParticleContainer;
class WarpXParticleView final : public warpx::runtime::IParticleContainer {
public:
    explicit WarpXParticleView (WarpXParticleContainer* pc) : m_pc(pc) {}
    [[nodiscard]] const std::string& getName () const override;
    [[nodiscard]] amrex::Real getCharge () const override;
    [[nodiscard]] amrex::Real getMass () const override;
    [[nodiscard]] int finestLevel () const override;
    [[nodiscard]] std::int64_t totalNumberOfParticles () const override;
    [[nodiscard]] amrex::Real sumParticleWeight (bool local) const override;
    [[nodiscard]] std::pair<amrex::Real, amrex::Real>
    sumParticleWeightAndEnergy (bool local) const override;
    [[nodiscard]] bool isPhoton () const override;
    [[nodiscard]] bool DoQED () const override;
    [[nodiscard]] warpx::runtime::ParticleMomentumSum sumParticleMomentum () const override;
private:
    WarpXParticleContainer* m_pc;
};
#endif
