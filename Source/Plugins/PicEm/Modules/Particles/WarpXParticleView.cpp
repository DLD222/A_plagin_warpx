#include "Particles/WarpXParticleView.H"
#include "Particles/WarpXParticleContainer.H"
namespace {
const std::string& nameOf (const WarpXParticleContainer* pc)
{
    return const_cast<WarpXParticleContainer*>(pc)->getName();
}
}
const std::string& WarpXParticleView::getName () const { return nameOf(m_pc); }
amrex::Real WarpXParticleView::getCharge () const { return m_pc->getCharge(); }
amrex::Real WarpXParticleView::getMass () const { return m_pc->getMass(); }
int WarpXParticleView::finestLevel () const { return m_pc->finestLevel(); }
std::int64_t WarpXParticleView::totalNumberOfParticles () const
{ return static_cast<std::int64_t>(m_pc->TotalNumberOfParticles(false)); }
amrex::Real WarpXParticleView::sumParticleWeight (bool local) const { return m_pc->sumParticleWeight(local); }
std::pair<amrex::Real, amrex::Real>
WarpXParticleView::sumParticleWeightAndEnergy (bool local) const
{ return m_pc->sumParticleWeightAndEnergy(local); }
bool WarpXParticleView::isPhoton () const { return m_pc->AmIA<PhysicalSpecies::photon>(); }
bool WarpXParticleView::DoQED () const { return m_pc->DoQED(); }

warpx::runtime::ParticleMomentumSum WarpXParticleView::sumParticleMomentum () const
{
    using PType = typename WarpXParticleContainer::SuperParticleType;
    const amrex::Real m = isPhoton() ? PhysConst::m_e : m_pc->getMass();
    amrex::ReduceOps<amrex::ReduceOpSum, amrex::ReduceOpSum,
                     amrex::ReduceOpSum, amrex::ReduceOpSum> reduce_ops;
    auto r = amrex::ParticleReduce<amrex::ReduceData<amrex::Real, amrex::Real,
                                                      amrex::Real, amrex::Real>>(
        *m_pc,
        [=] AMREX_GPU_DEVICE(const PType& p) noexcept
            -> amrex::GpuTuple<amrex::Real, amrex::Real, amrex::Real, amrex::Real>
        {
            const amrex::Real w = p.rdata(PIdx::w);
            return {w*m*p.rdata(PIdx::ux), w*m*p.rdata(PIdx::uy),
                    w*m*p.rdata(PIdx::uz), w};
        }, reduce_ops);
    return {amrex::get<0>(r), amrex::get<1>(r), amrex::get<2>(r), amrex::get<3>(r)};
}
