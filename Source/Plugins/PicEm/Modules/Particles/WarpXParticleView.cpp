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
