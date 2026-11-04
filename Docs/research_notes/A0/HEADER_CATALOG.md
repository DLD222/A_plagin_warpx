# A0：头文件类目录与公共接口

> 范围：`Source/**/*.H`，自动解析 + 人工抽检。签名来自源码启发式抽取；默认参数/模板/宏可能被简化，精确签名以头文件为准。
> 机器可读表：`classes.csv`；逐子系统签名：`api/<Subsystem>.md`。


## 0. 覆盖与读法

- 头文件：**371**；解析出 class/struct 的头文件：**235**；未解析出 class/struct：**136**（多为 free function/kernel/模板工具，见 [`COVERAGE.md`](COVERAGE.md)）。
- 类/结构体：**348**；公共方法：**2,408**（启发式抽取并去重；模板/默认参数可能简化）。
- 逐子系统签名：[`api/`](api/)；机器可读：[`classes.csv`](classes.csv)；生命周期：[`LIFECYCLE_MATRIX.md`](LIFECYCLE_MATRIX.md)。
- 读法：先看 §1 总量，再看 §2 类索引定位类，最后进 `api/<Subsystem>.md` 看完整公共签名。

### 公共方法最多的 20 个类（Route B 的重点）

| 类 | 文件 | 公共方法 |
|---|---|---:|
| `WarpX` | `Source/WarpX.H` | 217 |
| `WarpXParticleContainer` | `Source/Particles/WarpXParticleContainer.H` | 85 |
| `MultiParticleContainer` | `Source/Particles/MultiParticleContainer.H` | 67 |
| `FiniteDifferenceSolver` | `Source/FieldSolver/FiniteDifferenceSolver/FiniteDifferenceSolver.H` | 64 |
| `PhysicalParticleContainer` | `Source/Particles/PhysicalParticleContainer.H` | 54 |
| `ImplicitSolver` | `Source/FieldSolver/ImplicitSolvers/ImplicitSolver.H` | 40 |
| `HybridPICModel` | `Source/FieldSolver/FiniteDifferenceSolver/HybridPICModel/HybridPICModel.H` | 39 |
| `WarpXSolverVec` | `Source/FieldSolver/ImplicitSolvers/WarpXSolverVec.H` | 38 |
| `MacroscopicProperties` | `Source/FieldSolver/FiniteDifferenceSolver/MacroscopicProperties/MacroscopicProperties.H` | 38 |
| `MultiFabRegister` | `Source/ablastr/fields/MultiFabRegister.H` | 34 |
| `Diagnostics` | `Source/Diagnostics/Diagnostics.H` | 34 |
| `PlasmaInjector` | `Source/Initialization/PlasmaInjector.H` | 28 |
| `BTDPlotfileHeaderImpl` | `Source/Diagnostics/BTD_Plotfile_Header_Impl.H` | 27 |
| `JacobianFunctionMF` | `Source/NonlinearSolvers/JacobianFunctionMF.H` | 23 |
| `SemiImplicitDarwin` | `Source/FieldSolver/ImplicitSolvers/SemiImplicitDarwin.H` | 21 |
| `SpectralFieldDataRZ` | `Source/FieldSolver/SpectralSolver/SpectralFieldDataRZ.H` | 20 |
| `ParticleBoundaryBuffer` | `Source/Particles/ParticleBoundaryBuffer.H` | 20 |
| `WarpXFluidContainer` | `Source/Fluids/WarpXFluidContainer.H` | 18 |
| `MultiDiagnostics` | `Source/Diagnostics/MultiDiagnostics.H` | 14 |
| `MultiReducedDiags` | `Source/Diagnostics/ReducedDiags/MultiReducedDiags.H` | 12 |

## 1. 总览

| 子系统 | 类/结构体 | 公共方法 | 头文件 |
|---|---:|---:|---:|
| [FieldSolver](api/FieldSolver.md) | 53 | 541 | 47 |
| [Particles](api/Particles.md) | 97 | 527 | 54 |
| [Diagnostics](api/Diagnostics.md) | 71 | 324 | 58 |
| [NonlinearSolvers](api/NonlinearSolvers.md) | 23 | 311 | 20 |
| [Root](api/Root.md) | 1 | 217 | 1 |
| [Initialization](api/Initialization.md) | 33 | 147 | 11 |
| [ablastr](api/ablastr.md) | 17 | 103 | 10 |
| [BoundaryConditions](api/BoundaryConditions.md) | 8 | 55 | 4 |
| [MomentsSolver](api/MomentsSolver.md) | 15 | 54 | 10 |
| [Fluids](api/Fluids.md) | 5 | 50 | 4 |
| [AcceleratorLattice](api/AcceleratorLattice.md) | 9 | 26 | 6 |
| [Laser](api/Laser.md) | 5 | 21 | 1 |
| [Parallelization](api/Parallelization.md) | 1 | 15 | 1 |
| [Filter](api/Filter.md) | 3 | 9 | 3 |
| [Utils](api/Utils.md) | 3 | 7 | 3 |
| [EmbeddedBoundary](api/EmbeddedBoundary.md) | 4 | 1 | 2 |
| **合计** | **348** | **2408** | **235** |

## 2. 类索引（按子系统）

### AcceleratorLattice

| 类/结构体 | 文件 | 公共方法 | 基类 |
|---|---|---:|---|
| `AcceleratorLattice` | `Source/AcceleratorLattice/AcceleratorLattice.H` | 5 |  |
| `LatticeElementFinder` | `Source/AcceleratorLattice/LatticeElementFinder.H` | 5 |  |
| `HardEdgedPlasmaLens` | `Source/AcceleratorLattice/LatticeElements/HardEdgedPlasmaLens.H` | 4 | LatticeElementBase |
| `HardEdgedQuadrupole` | `Source/AcceleratorLattice/LatticeElements/HardEdgedQuadrupole.H` | 4 | LatticeElementBase |
| `LatticeElementBase` | `Source/AcceleratorLattice/LatticeElements/LatticeElementBase.H` | 3 |  |
| `Drift` | `Source/AcceleratorLattice/LatticeElements/Drift.H` | 2 | LatticeElementBase |
| `LatticeElementFinderDevice` | `Source/AcceleratorLattice/LatticeElementFinder.H` | 1 |  |
| `HardEdgedPlasmaLensDevice` | `Source/AcceleratorLattice/LatticeElements/HardEdgedPlasmaLens.H` | 1 |  |
| `HardEdgedQuadrupoleDevice` | `Source/AcceleratorLattice/LatticeElements/HardEdgedQuadrupole.H` | 1 |  |

### BoundaryConditions

| 类/结构体 | 文件 | 公共方法 | 基类 |
|---|---|---:|---|
| `PML` | `Source/BoundaryConditions/PML.H` | 17 |  |
| `SigmaBoxFactory` | `Source/BoundaryConditions/PML.H` | 11 | public amrex::FabFactory<SigmaBox> |
| `PEC_Insulator` | `Source/BoundaryConditions/PEC_Insulator.H` | 10 |  |
| `PML_RZ` | `Source/BoundaryConditions/PML_RZ.H` | 7 |  |
| `SigmaBox` | `Source/BoundaryConditions/PML.H` | 5 |  |
| `MultiSigmaBox` | `Source/BoundaryConditions/PML.H` | 3 | public amrex::FabArray<SigmaBox> |
| `Sigma` | `Source/BoundaryConditions/PML.H` | 2 | amrex::Gpu::DeviceVector<amrex::Real> |
| `PMLComp` | `Source/BoundaryConditions/PMLComponent.H` | 0 |  |

### Diagnostics

| 类/结构体 | 文件 | 公共方法 | 基类 |
|---|---|---:|---|
| `Diagnostics` | `Source/Diagnostics/Diagnostics.H` | 34 |  |
| `BTDPlotfileHeaderImpl` | `Source/Diagnostics/BTD_Plotfile_Header_Impl.H` | 27 |  |
| `BTDMultiFabHeaderImpl` | `Source/Diagnostics/BTD_Plotfile_Header_Impl.H` | 16 |  |
| `ReducedDiags` | `Source/Diagnostics/ReducedDiags/ReducedDiags.H` | 15 |  |
| `WarpXOpenPMDPlot` | `Source/Diagnostics/WarpXOpenPMD.H` | 12 |  |
| `MultiDiagnostics` | `Source/Diagnostics/MultiDiagnostics.H` | 11 |  |
| `ComputeDiagFunctor` | `Source/Diagnostics/ComputeDiagFunctors/ComputeDiagFunctor.H` | 11 |  |
| `FlushFormatPlotfile` | `Source/Diagnostics/FlushFormats/FlushFormatPlotfile.H` | 11 | public FlushFormat |
| `MultiReducedDiags` | `Source/Diagnostics/ReducedDiags/MultiReducedDiags.H` | 9 |  |
| `ComputeParticleDiagFunctor` | `Source/Diagnostics/ComputeDiagFunctors/ComputeParticleDiagFunctor.H` | 9 |  |
| `FlushFormatSensei` | `Source/Diagnostics/FlushFormats/FlushFormatSensei.H` | 9 | public FlushFormat |
| `BTDParticleDataHeaderImpl` | `Source/Diagnostics/BTD_Plotfile_Header_Impl.H` | 8 |  |
| `FlushFormatCatalyst` | `Source/Diagnostics/FlushFormats/FlushFormatCatalyst.H` | 8 | public FlushFormatInSitu |
| `BTDSpeciesHeaderImpl` | `Source/Diagnostics/BTD_Plotfile_Header_Impl.H` | 7 |  |
| `FieldProbeParticleContainer` | `Source/Diagnostics/ReducedDiags/FieldProbeParticleContainer.H` | 7 | public amrex::ParticleContainerPureSoA<FieldProbePIdx::nattribs, 0> |
| `FlushFormat` | `Source/Diagnostics/FlushFormats/FlushFormat.H` | 7 |  |
| `FlushFormatAscent` | `Source/Diagnostics/FlushFormats/FlushFormatAscent.H` | 7 | public FlushFormatInSitu |
| `FlushFormatInSitu` | `Source/Diagnostics/FlushFormats/FlushFormatInSitu.H` | 7 | public FlushFormat |
| `FlushFormatOpenPMD` | `Source/Diagnostics/FlushFormats/FlushFormatOpenPMD.H` | 7 | public FlushFormat |
| `FieldPoyntingFlux` | `Source/Diagnostics/ReducedDiags/FieldPoyntingFlux.H` | 6 | public ReducedDiags |
| `BackTransformFunctor` | `Source/Diagnostics/ComputeDiagFunctors/BackTransformFunctor.H` | 5 | public ComputeDiagFunctor |
| `FieldProbe` | `Source/Diagnostics/ReducedDiags/FieldProbe.H` | 4 | public ReducedDiags |
| `FieldReduction` | `Source/Diagnostics/ReducedDiags/FieldReduction.H` | 4 | public ReducedDiags |
| `ParticleDiag` | `Source/Diagnostics/ParticleDiag/ParticleDiag.H` | 4 |  |
| `BackTransformParticleFunctor` | `Source/Diagnostics/ComputeDiagFunctors/BackTransformParticleFunctor.H` | 4 | public ComputeParticleDiagFunctor |
| `DifferentialLuminosity2D` | `Source/Diagnostics/ReducedDiags/DifferentialLuminosity2D.H` | 3 | public ReducedDiags |
| `FieldEnergy` | `Source/Diagnostics/ReducedDiags/FieldEnergy.H` | 3 | public ReducedDiags |
| `LoadBalanceCosts` | `Source/Diagnostics/ReducedDiags/LoadBalanceCosts.H` | 3 | public ReducedDiags |
| `ParticleHistogram2D` | `Source/Diagnostics/ReducedDiags/ParticleHistogram2D.H` | 3 | public ReducedDiags |
| `WarpXParticleCounter` | `Source/Diagnostics/WarpXOpenPMD.H` | 2 |  |
| `BeamRelevant` | `Source/Diagnostics/ReducedDiags/BeamRelevant.H` | 2 | public ReducedDiags |
| `ChargeOnEB` | `Source/Diagnostics/ReducedDiags/ChargeOnEB.H` | 2 | public ReducedDiags |
| `ColliderRelevant` | `Source/Diagnostics/ReducedDiags/ColliderRelevant.H` | 2 | public ReducedDiags |
| `DifferentialLuminosity` | `Source/Diagnostics/ReducedDiags/DifferentialLuminosity.H` | 2 | public ReducedDiags |
| `FieldMaximum` | `Source/Diagnostics/ReducedDiags/FieldMaximum.H` | 2 | public ReducedDiags |
| `FieldMomentum` | `Source/Diagnostics/ReducedDiags/FieldMomentum.H` | 2 | public ReducedDiags |
| `LoadBalanceEfficiency` | `Source/Diagnostics/ReducedDiags/LoadBalanceEfficiency.H` | 2 | public ReducedDiags |
| `ParticleEnergy` | `Source/Diagnostics/ReducedDiags/ParticleEnergy.H` | 2 | public ReducedDiags |
| `ParticleExtrema` | `Source/Diagnostics/ReducedDiags/ParticleExtrema.H` | 2 | public ReducedDiags |
| `ParticleHistogram` | `Source/Diagnostics/ReducedDiags/ParticleHistogram.H` | 2 | public ReducedDiags |
| `ParticleMomentum` | `Source/Diagnostics/ReducedDiags/ParticleMomentum.H` | 2 | public ReducedDiags |
| `ParticleNumber` | `Source/Diagnostics/ReducedDiags/ParticleNumber.H` | 2 | public ReducedDiags |
| `RhoMaximum` | `Source/Diagnostics/ReducedDiags/RhoMaximum.H` | 2 | public ReducedDiags |
| `Timestep` | `Source/Diagnostics/ReducedDiags/Timestep.H` | 2 | public ReducedDiags |
| `CellCenterFunctor` | `Source/Diagnostics/ComputeDiagFunctors/CellCenterFunctor.H` | 2 | public ComputeDiagFunctor |
| `DarwinEfieldFunctor` | `Source/Diagnostics/ComputeDiagFunctors/DarwinEfieldFunctor.H` | 2 | public ComputeDiagFunctor |
| `DivBFunctor` | `Source/Diagnostics/ComputeDiagFunctors/DivBFunctor.H` | 2 | public ComputeDiagFunctor |
| `DivEFunctor` | `Source/Diagnostics/ComputeDiagFunctors/DivEFunctor.H` | 2 | public ComputeDiagFunctor |
| `EBCoveredFunctor` | `Source/Diagnostics/ComputeDiagFunctors/EBCoveredFunctor.H` | 2 | public ComputeDiagFunctor |
| `JFunctor` | `Source/Diagnostics/ComputeDiagFunctors/JFunctor.H` | 2 | public ComputeDiagFunctor |
| `JdispFunctor` | `Source/Diagnostics/ComputeDiagFunctors/JdispFunctor.H` | 2 | public ComputeDiagFunctor |
| `PartPerCellFunctor` | `Source/Diagnostics/ComputeDiagFunctors/PartPerCellFunctor.H` | 2 | public ComputeDiagFunctor |
| `PartPerGridFunctor` | `Source/Diagnostics/ComputeDiagFunctors/PartPerGridFunctor.H` | 2 | public ComputeDiagFunctor |
| `ParticleReductionFunctor` | `Source/Diagnostics/ComputeDiagFunctors/ParticleReductionFunctor.H` | 2 | public ComputeDiagFunctor |
| `PhiFunctor` | `Source/Diagnostics/ComputeDiagFunctors/PhiFunctor.H` | 2 | public ComputeDiagFunctor |
| `ProcessNumberFunctor` | `Source/Diagnostics/ComputeDiagFunctors/ProcessNumberFunctor.H` | 2 | public ComputeDiagFunctor |
| `RhoFunctor` | `Source/Diagnostics/ComputeDiagFunctors/RhoFunctor.H` | 2 | public ComputeDiagFunctor |
| `TemperatureFunctor` | `Source/Diagnostics/ComputeDiagFunctors/TemperatureFunctor.H` | 2 | public ComputeDiagFunctor |
| `BTDiagnostics` | `Source/Diagnostics/BTDiagnostics.H` | 1 | public Diagnostics |
| `BoundaryScrapingDiagnostics` | `Source/Diagnostics/BoundaryScrapingDiagnostics.H` | 1 | public Diagnostics |
| `FullDiagnostics` | `Source/Diagnostics/FullDiagnostics.H` | 1 | public Diagnostics |
| `SelectParticles` | `Source/Diagnostics/ComputeDiagFunctors/BackTransformParticleFunctor.H` | 1 |  |
| `LorentzTransformParticles` | `Source/Diagnostics/ComputeDiagFunctors/BackTransformParticleFunctor.H` | 1 |  |
| `aux_header_index` | `Source/Diagnostics/ReducedDiags/ColliderRelevant.H` | 0 |  |
| `aux_header_index` | `Source/Diagnostics/ReducedDiags/DifferentialLuminosity.H` | 0 |  |
| `PoyntingStaggered` | `Source/Diagnostics/ReducedDiags/FieldPoyntingFlux.H` | 0 |  |
| `PoyntingCellCentered` | `Source/Diagnostics/ReducedDiags/FieldPoyntingFlux.H` | 0 |  |
| `PoyntingNodal` | `Source/Diagnostics/ReducedDiags/FieldPoyntingFlux.H` | 0 |  |
| `FieldProbePIdx` | `Source/Diagnostics/ReducedDiags/FieldProbeParticleContainer.H` | 0 |  |
| `aux_header_index` | `Source/Diagnostics/ReducedDiags/ParticleExtrema.H` | 0 |  |
| `FlushFormatCheckpoint` | `Source/Diagnostics/FlushFormats/FlushFormatCheckpoint.H` | 0 | public FlushFormatPlotfile |

### EmbeddedBoundary

| 类/结构体 | 文件 | 公共方法 | 基类 |
|---|---|---:|---|
| `ParticleBoundaryInteraction` | `Source/EmbeddedBoundary/ParticleBoundaryProcess.H` | 1 |  |
| `NoOp` | `Source/EmbeddedBoundary/ParticleBoundaryProcess.H` | 0 |  |
| `Absorb` | `Source/EmbeddedBoundary/ParticleBoundaryProcess.H` | 0 |  |
| `FaceInfoBox` | `Source/EmbeddedBoundary/WarpXFaceInfoBox.H` | 0 |  |

### FieldSolver

| 类/结构体 | 文件 | 公共方法 | 基类 |
|---|---|---:|---|
| `FiniteDifferenceSolver` | `Source/FieldSolver/FiniteDifferenceSolver/FiniteDifferenceSolver.H` | 64 |  |
| `ImplicitSolver` | `Source/FieldSolver/ImplicitSolvers/ImplicitSolver.H` | 40 |  |
| `HybridPICModel` | `Source/FieldSolver/FiniteDifferenceSolver/HybridPICModel/HybridPICModel.H` | 39 |  |
| `WarpXSolverVec` | `Source/FieldSolver/ImplicitSolvers/WarpXSolverVec.H` | 38 |  |
| `MacroscopicProperties` | `Source/FieldSolver/FiniteDifferenceSolver/MacroscopicProperties/MacroscopicProperties.H` | 38 |  |
| `SemiImplicitDarwin` | `Source/FieldSolver/ImplicitSolvers/SemiImplicitDarwin.H` | 21 | public ImplicitSolver |
| `SpectralFieldDataRZ` | `Source/FieldSolver/SpectralSolver/SpectralFieldDataRZ.H` | 20 |  |
| `DarwinLinearFieldOperator` | `Source/FieldSolver/ImplicitSolvers/DarwinLinearFieldOperator.H` | 16 | public LinearFunction<WarpXSolverVec,SemiImplicitDarwin> |
| `ThetaImplicitEM` | `Source/FieldSolver/ImplicitSolvers/ThetaImplicitEM.H` | 16 | public ImplicitSolver |
| `MassMatrixSolverOps` | `Source/FieldSolver/ImplicitSolvers/MassMatrixSolverOps.H` | 15 | public MatrixSolverOps<T> |
| `SpectralSolverRZ` | `Source/FieldSolver/SpectralSolver/SpectralSolverRZ.H` | 15 |  |
| `SemiImplicitEM` | `Source/FieldSolver/ImplicitSolvers/SemiImplicitEM.H` | 14 | public ImplicitSolver |
| `SparseJacobianMatrix` | `Source/FieldSolver/ImplicitSolvers/SparseJacobianMatrix.H` | 14 |  |
| `ElectrostaticSolver` | `Source/FieldSolver/ElectrostaticSolvers/ElectrostaticSolver.H` | 14 |  |
| `StrangImplicitSpectralEM` | `Source/FieldSolver/ImplicitSolvers/StrangImplicitSpectralEM.H` | 12 | public ImplicitSolver |
| `SpectralSolver` | `Source/FieldSolver/SpectralSolver/SpectralSolver.H` | 10 |  |
| `SpectralFieldData` | `Source/FieldSolver/SpectralSolver/SpectralFieldData.H` | 9 |  |
| `SpectralBaseAlgorithm` | `Source/FieldSolver/SpectralSolver/SpectralAlgorithms/SpectralBaseAlgorithm.H` | 9 |  |
| `SpectralBaseAlgorithmRZ` | `Source/FieldSolver/SpectralSolver/SpectralAlgorithms/SpectralBaseAlgorithmRZ.H` | 9 |  |
| `SpectralHankelTransformer` | `Source/FieldSolver/SpectralSolver/SpectralHankelTransform/SpectralHankelTransformer.H` | 8 |  |
| `ExternalVectorPotential` | `Source/FieldSolver/FiniteDifferenceSolver/HybridPICModel/ExternalVectorPotential.H` | 7 |  |
| `PoissonBoundaryHandler` | `Source/FieldSolver/ElectrostaticSolvers/PoissonBoundaryHandler.H` | 7 |  |
| `SpectralFieldIndex` | `Source/FieldSolver/SpectralSolver/SpectralFieldData.H` | 7 |  |
| `EffectivePotentialES` | `Source/FieldSolver/ElectrostaticSolvers/EffectivePotentialES.H` | 6 | public ElectrostaticSolver |
| `PsatdAlgorithmGalilean` | `Source/FieldSolver/SpectralSolver/SpectralAlgorithms/PsatdAlgorithmGalilean.H` | 6 | public SpectralBaseAlgorithm |
| `PsatdAlgorithmJRhomSecondOrder` | `Source/FieldSolver/SpectralSolver/SpectralAlgorithms/PsatdAlgorithmJRhomSecondOrder.H` | 6 | public SpectralBaseAlgorithm |
| `LabFrameExplicitES` | `Source/FieldSolver/ElectrostaticSolvers/LabFrameExplicitES.H` | 5 | public ElectrostaticSolver |
| `RelativisticExplicitES` | `Source/FieldSolver/ElectrostaticSolvers/RelativisticExplicitES.H` | 5 | public ElectrostaticSolver |
| `SpectralBinomialFilter` | `Source/FieldSolver/SpectralSolver/SpectralBinomialFilter.H` | 5 |  |
| `SpectralKSpace` | `Source/FieldSolver/SpectralSolver/SpectralKSpace.H` | 5 |  |
| `PsatdAlgorithmComoving` | `Source/FieldSolver/SpectralSolver/SpectralAlgorithms/PsatdAlgorithmComoving.H` | 5 | public SpectralBaseAlgorithm |
| `PsatdAlgorithmGalileanRZ` | `Source/FieldSolver/SpectralSolver/SpectralAlgorithms/PsatdAlgorithmGalileanRZ.H` | 5 | public SpectralBaseAlgorithmRZ |
| `PsatdAlgorithmPml` | `Source/FieldSolver/SpectralSolver/SpectralAlgorithms/PsatdAlgorithmPml.H` | 5 | public SpectralBaseAlgorithm |
| `PsatdAlgorithmPmlRZ` | `Source/FieldSolver/SpectralSolver/SpectralAlgorithms/PsatdAlgorithmPmlRZ.H` | 5 | public SpectralBaseAlgorithmRZ |
| `PsatdAlgorithmRZ` | `Source/FieldSolver/SpectralSolver/SpectralAlgorithms/PsatdAlgorithmRZ.H` | 5 | public SpectralBaseAlgorithmRZ |
| `PsatdAlgorithmJRhomFirstOrder` | `Source/FieldSolver/SpectralSolver/SpectralAlgorithms/PsatdAlgorithmJRhomFirstOrder.H` | 4 | public SpectralBaseAlgorithm |
| `HankelTransform` | `Source/FieldSolver/SpectralSolver/SpectralHankelTransform/HankelTransform.H` | 4 |  |
| `EBCalcBfromVectorPotentialPerLevel` | `Source/FieldSolver/MagnetostaticSolver/MagnetostaticSolver.H` | 3 |  |
| `WarpXSolverDOF` | `Source/FieldSolver/ImplicitSolvers/WarpXSolverDOF.H` | 3 |  |
| `CartesianCKCAlgorithm` | `Source/FieldSolver/FiniteDifferenceSolver/FiniteDifferenceAlgorithms/CartesianCKCAlgorithm.H` | 3 |  |
| `CartesianNodalAlgorithm` | `Source/FieldSolver/FiniteDifferenceSolver/FiniteDifferenceAlgorithms/CartesianNodalAlgorithm.H` | 3 |  |
| `CartesianYeeAlgorithm` | `Source/FieldSolver/FiniteDifferenceSolver/FiniteDifferenceAlgorithms/CartesianYeeAlgorithm.H` | 3 |  |
| `CylindricalYeeAlgorithm` | `Source/FieldSolver/FiniteDifferenceSolver/FiniteDifferenceAlgorithms/CylindricalYeeAlgorithm.H` | 3 |  |
| `SphericalYeeAlgorithm` | `Source/FieldSolver/FiniteDifferenceSolver/FiniteDifferenceAlgorithms/SphericalYeeAlgorithm.H` | 3 |  |
| `SpectralKSpaceRZ` | `Source/FieldSolver/SpectralSolver/SpectralKSpaceRZ.H` | 3 | public SpectralKSpace |
| `EBCalcEfromPhiPerLevel` | `Source/FieldSolver/ElectrostaticSolvers/PoissonBoundaryHandler.H` | 2 |  |
| `VectorPoissonBoundaryHandler` | `Source/FieldSolver/MagnetostaticSolver/MagnetostaticSolver.H` | 1 |  |
| `ImplicitOptions` | `Source/FieldSolver/ImplicitSolvers/ImplicitOptions.H` | 1 |  |
| `FieldAccessorMacroscopic` | `Source/FieldSolver/FiniteDifferenceSolver/FiniteDifferenceAlgorithms/FieldAccessorFunctors.H` | 0 |  |
| `LaxWendroffAlgo` | `Source/FieldSolver/FiniteDifferenceSolver/MacroscopicProperties/MacroscopicProperties.H` | 0 |  |
| `BackwardEulerAlgo` | `Source/FieldSolver/FiniteDifferenceSolver/MacroscopicProperties/MacroscopicProperties.H` | 0 |  |
| `Mat3` | `Source/FieldSolver/FiniteDifferenceSolver/MacroscopicProperties/SubpixelTensor.H` | 0 |  |
| `PhiCalculatorEB` | `Source/FieldSolver/ElectrostaticSolvers/PoissonBoundaryHandler.H` | 0 |  |

### Filter

| 类/结构体 | 文件 | 公共方法 | 基类 |
|---|---|---:|---|
| `Filter` | `Source/Filter/Filter.H` | 4 |  |
| `NCIGodfreyFilter` | `Source/Filter/NCIGodfreyFilter.H` | 3 | public Filter |
| `BilinearFilter` | `Source/Filter/BilinearFilter.H` | 2 | public Filter |

### Fluids

| 类/结构体 | 文件 | 公共方法 | 基类 |
|---|---|---:|---|
| `WarpXFluidContainer` | `Source/Fluids/WarpXFluidContainer.H` | 18 |  |
| `MultiFluidContainer` | `Source/Fluids/MultiFluidContainer.H` | 14 |  |
| `QdsmcParticleContainer` | `Source/Fluids/QdsmcParticleContainer.H` | 13 | public amrex::ParticleContainerPureSoA<QdsmcPIdx::nattribs, 0> |
| `MomentFluidSolver` | `Source/Fluids/MomentFluidSolver.H` | 5 |  |
| `QdsmcPIdx` | `Source/Fluids/QdsmcParticleContainer.H` | 0 |  |

### Initialization

| 类/结构体 | 文件 | 公共方法 | 基类 |
|---|---|---:|---|
| `PlasmaInjector` | `Source/Initialization/PlasmaInjector.H` | 28 |  |
| `InjectorDensity` | `Source/Initialization/InjectorDensity.H` | 15 |  |
| `InjectorMomentum` | `Source/Initialization/InjectorMomentum.H` | 15 |  |
| `InjectorPosition` | `Source/Initialization/InjectorPosition.H` | 12 |  |
| `InjectorFlux` | `Source/Initialization/InjectorFlux.H` | 9 |  |
| `InjectorDensityFromFile` | `Source/Initialization/InjectorDensity.H` | 7 |  |
| `ExternalFieldReader` | `Source/Initialization/ExternalField.H` | 6 |  |
| `ProjectionDivCleaner` | `Source/Initialization/DivCleaner/ProjectionDivCleaner.H` | 5 |  |
| `InjectorMomentumConstant` | `Source/Initialization/InjectorMomentum.H` | 3 |  |
| `InjectorMomentumGaussian` | `Source/Initialization/InjectorMomentum.H` | 3 |  |
| `InjectorMomentumGaussianFlux` | `Source/Initialization/InjectorMomentum.H` | 3 |  |
| `InjectorMomentumUniform` | `Source/Initialization/InjectorMomentum.H` | 3 |  |
| `InjectorMomentumMaxwellian` | `Source/Initialization/InjectorMomentum.H` | 3 |  |
| `InjectorMomentumJuttner` | `Source/Initialization/InjectorMomentum.H` | 3 |  |
| `InjectorMomentumParser` | `Source/Initialization/InjectorMomentum.H` | 3 |  |
| `ExternalFieldVectorView` | `Source/Initialization/ExternalField.H` | 2 |  |
| `GetTemperature` | `Source/Initialization/GetTemperature.H` | 2 |  |
| `GetTemperatureVector` | `Source/Initialization/GetTemperature.H` | 2 |  |
| `GetVelocityVector` | `Source/Initialization/GetVelocity.H` | 2 |  |
| `InjectorDensityConstant` | `Source/Initialization/InjectorDensity.H` | 2 |  |
| `InjectorDensityParser` | `Source/Initialization/InjectorDensity.H` | 2 |  |
| `InjectorFluxConstant` | `Source/Initialization/InjectorFlux.H` | 2 |  |
| `InjectorFluxParser` | `Source/Initialization/InjectorFlux.H` | 2 |  |
| `InjectorPositionRandomPlane` | `Source/Initialization/InjectorPosition.H` | 2 |  |
| `InjectorPositionRegular` | `Source/Initialization/InjectorPosition.H` | 2 |  |
| `TemperatureProperties` | `Source/Initialization/TemperatureProperties.H` | 2 |  |
| `VelocityProperties` | `Source/Initialization/VelocityProperties.H` | 2 |  |
| `ExternalFieldParams` | `Source/Initialization/ExternalField.H` | 1 |  |
| `InjectorDensityDeleter` | `Source/Initialization/InjectorDensity.H` | 1 |  |
| `InjectorFluxDeleter` | `Source/Initialization/InjectorFlux.H` | 1 |  |
| `InjectorMomentumDeleter` | `Source/Initialization/InjectorMomentum.H` | 1 |  |
| `InjectorPositionRandom` | `Source/Initialization/InjectorPosition.H` | 1 |  |
| `ExternalFieldView` | `Source/Initialization/ExternalField.H` | 0 |  |

### Laser

| 类/结构体 | 文件 | 公共方法 | 基类 |
|---|---|---:|---|
| `ILaserProfile` | `Source/Laser/LaserProfiles.H` | 9 |  |
| `FromFileLaserProfile` | `Source/Laser/LaserProfiles.H` | 6 | public ILaserProfile |
| `GaussianLaserProfile` | `Source/Laser/LaserProfiles.H` | 3 | public ILaserProfile |
| `FieldFunctionLaserProfile` | `Source/Laser/LaserProfiles.H` | 3 | public ILaserProfile |
| `CommonLaserParameters` | `Source/Laser/LaserProfiles.H` | 0 |  |

### MomentsSolver

| 类/结构体 | 文件 | 公共方法 | 基类 |
|---|---|---:|---|
| `FVND` | `Source/MomentsSolver/FV/MomentFVNDDriver.H` | 14 |  |
| `Eos` | `Source/MomentsSolver/Equations/EquationOfState.H` | 10 |  |
| `Euler5` | `Source/MomentsSolver/Equations/Euler5Eqn.H` | 6 |  |
| `FV1D` | `Source/MomentsSolver/FV/MomentFVDriver.H` | 5 |  |
| `FrictionParams` | `Source/MomentsSolver/Coupling/FluidFriction.H` | 5 |  |
| `TenMoment` | `Source/MomentsSolver/Equations/TenMomentEqn.H` | 5 |  |
| `Maxwell6` | `Source/MomentsSolver/Equations/MaxwellEqn.H` | 3 |  |
| `EquationVTable` | `Source/MomentsSolver/Equations/MomentEquation.H` | 3 |  |
| `IdealGasEos` | `Source/MomentsSolver/Equations/EquationOfState.H` | 2 |  |
| `IsothermalEos` | `Source/MomentsSolver/Equations/EquationOfState.H` | 1 |  |
| `Species5` | `Source/MomentsSolver/Coupling/MomentCoupling.H` | 0 |  |
| `YeeDeposit` | `Source/MomentsSolver/Coupling/YeeDeposition.H` | 0 |  |
| `Context` | `Source/MomentsSolver/Equations/Euler5Eqn.H` | 0 |  |
| `Context` | `Source/MomentsSolver/Equations/MaxwellEqn.H` | 0 |  |
| `Context` | `Source/MomentsSolver/Equations/TenMomentEqn.H` | 0 |  |

### NonlinearSolvers

| 类/结构体 | 文件 | 公共方法 | 基类 |
|---|---|---:|---|
| `JacobianFunctionMF` | `Source/NonlinearSolvers/JacobianFunctionMF.H` | 23 | public LinearFunction<T,Ops> |
| `LinearFunction` | `Source/NonlinearSolvers/LinearFunction.H` | 23 |  |
| `WeightedJacobiImpl` | `Source/NonlinearSolvers/WeightedJacobiImpl.H` | 19 |  |
| `KSP_impl` | `Source/NonlinearSolvers/WarpX_PETSc.H` | 18 | public PETScSolver_impl |
| `SNES_impl` | `Source/NonlinearSolvers/WarpX_PETSc.H` | 18 | public PETScSolver_impl |
| `LinearSolver` | `Source/NonlinearSolvers/LinearSolver.H` | 17 |  |
| `NonlinearSolver` | `Source/NonlinearSolvers/NonlinearSolver.H` | 17 |  |
| `PETScKSP` | `Source/NonlinearSolvers/PETScKSP_Wrapper.H` | 17 | public LinearSolver<Vec,LinOp> |
| `NewtonSolver` | `Source/NonlinearSolvers/NewtonSolver.H` | 15 | public NonlinearSolver<Vec,Ops> |
| `Preconditioner` | `Source/NonlinearSolvers/Preconditioner.H` | 15 |  |
| `MatrixSolverOps` | `Source/NonlinearSolvers/MatrixSolverOps.H` | 14 |  |
| `PETScSNES` | `Source/NonlinearSolvers/PETScSNES_Wrapper.H` | 14 | public NonlinearSolver<Vec,Ops> |
| `ExternLibPC` | `Source/NonlinearSolvers/ExternLibPC.H` | 13 | public Preconditioner<T,Ops> |
| `PETScSolver_impl` | `Source/NonlinearSolvers/WarpX_PETSc.H` | 12 |  |
| `AlgMGPC` | `Source/NonlinearSolvers/AlgMGPC.H` | 11 | public Preconditioner<T,Ops> |
| `CurlCurlMLMGPC` | `Source/NonlinearSolvers/CurlCurlMLMGPC.H` | 11 | public Preconditioner<T,Ops> |
| `DarwinMLMGPC` | `Source/NonlinearSolvers/DarwinMLMGPC.H` | 11 | public Preconditioner<T,Ops> |
| `JacobiPC` | `Source/NonlinearSolvers/JacobiPC.H` | 11 | public Preconditioner<T,Ops> |
| `MatrixPCAlgMG` | `Source/NonlinearSolvers/MatrixPCAlgMG.H` | 11 |  |
| `PicardSolver` | `Source/NonlinearSolvers/PicardSolver.H` | 9 | public NonlinearSolver<Vec,Ops> |
| `AMReXGMRES` | `Source/NonlinearSolvers/AMReXGMRES_Wrapper.H` | 6 | public LinearSolver<Vec,LinOp> |
| `NewtonSolverParams` | `Source/NonlinearSolvers/NewtonSolverParams.H` | 3 |  |
| `NonlinearSolverParams` | `Source/NonlinearSolvers/NonlinearSolver.H` | 3 |  |

### Parallelization

| 类/结构体 | 文件 | 公共方法 | 基类 |
|---|---|---:|---|
| `guardCellManager` | `Source/Parallelization/GuardCellManager.H` | 15 |  |

### Particles

| 类/结构体 | 文件 | 公共方法 | 基类 |
|---|---|---:|---|
| `WarpXParticleContainer` | `Source/Particles/WarpXParticleContainer.H` | 85 | public amrex::ParticleContainerPureSoA<PIdx::nattribs, 0, amrex::PolymorphicAren |
| `MultiParticleContainer` | `Source/Particles/MultiParticleContainer.H` | 67 |  |
| `PhysicalParticleContainer` | `Source/Particles/PhysicalParticleContainer.H` | 54 | public WarpXParticleContainer |
| `ParticleBoundaryBuffer` | `Source/Particles/ParticleBoundaryBuffer.H` | 20 |  |
| `ScatteringProcess` | `Source/Particles/Collision/ScatteringProcess.H` | 17 |  |
| `LaserParticleContainer` | `Source/Particles/LaserParticleContainer.H` | 15 | public WarpXParticleContainer |
| `PhotonParticleContainer` | `Source/Particles/PhotonParticleContainer.H` | 15 | public PhysicalParticleContainer |
| `RigidInjectedParticleContainer` | `Source/Particles/RigidInjectedParticleContainer.H` | 13 | public PhysicalParticleContainer |
| `CollisionBase` | `Source/Particles/Collision/CollisionBase.H` | 13 |  |
| `WarpXParIter` | `Source/Particles/WarpXParticleContainer.H` | 12 | public amrex::ParIterSoA<PIdx::nattribs, 0, amrex::PolymorphicArenaAllocator> |
| `BreitWheelerEngine` | `Source/Particles/ElementaryProcess/QEDInternals/BreitWheelerEngineWrapper.H` | 11 |  |
| `QuantumSynchrotronEngine` | `Source/Particles/ElementaryProcess/QEDInternals/QuantumSyncEngineWrapper.H` | 11 |  |
| `BackgroundMCCCollision` | `Source/Particles/Collision/BackgroundMCC/BackgroundMCCCollision.H` | 10 | public CollisionBase |
| `ParticleBoundaries` | `Source/Particles/ParticleBoundaries.H` | 9 |  |
| `BinaryCollision` | `Source/Particles/Collision/BinaryCollision/BinaryCollision.H` | 9 | public CollisionBase |
| `BackgroundStopping` | `Source/Particles/Collision/BackgroundStopping/BackgroundStopping.H` | 9 | public CollisionBase |
| `InverseBremsstrahlung` | `Source/Particles/Collision/InverseBremsstrahlung/InverseBremsstrahlung.H` | 8 | public CollisionBase |
| `ResamplingAlgorithm` | `Source/Particles/Resampling/Resampling.H` | 7 |  |
| `CollisionFuncBase` | `Source/Particles/Collision/CollisionFuncBase.H` | 7 |  |
| `PulsedDecay` | `Source/Particles/Collision/PulsedDecay/PulsedDecay.H` | 7 | public CollisionBase |
| `HybridResistiveDrag` | `Source/Particles/Collision/HybridResistiveDrag/HybridResistiveDrag.H` | 7 | public CollisionBase |
| `GetParticlePosition` | `Source/Particles/Pusher/GetAndSetPosition.H` | 5 |  |
| `PlasmaParserHelper` | `Source/Particles/ParticleCreation/AddPlasmaUtilities.H` | 5 |  |
| `VarianceAccumulationBuffer` | `Source/Particles/Deposition/VarianceAccumulationBuffer.H` | 5 |  |
| `IndependentPairHelper` | `Source/Particles/Collision/BinaryCollision/ParticleShufflers.H` | 5 |  |
| `NuclearFusionFunc` | `Source/Particles/Collision/BinaryCollision/NuclearFusion/NuclearFusionFunc.H` | 5 | public CollisionFuncBase |
| `LevelingThinning` | `Source/Particles/Resampling/LevelingThinning.H` | 4 | public ResamplingAlgorithm |
| `Resampling` | `Source/Particles/Resampling/Resampling.H` | 4 |  |
| `ResamplingTrigger` | `Source/Particles/Resampling/ResamplingTrigger.H` | 4 |  |
| `ParticleThermalizer` | `Source/Particles/ParticleThermalizer/ParticleThermalizer.H` | 4 |  |
| `PairWiseCoulombCollisionFunc` | `Source/Particles/Collision/BinaryCollision/Coulomb/PairWiseCoulombCollisionFunc.H` | 4 | public CollisionFuncBase |
| `DSMCFunc` | `Source/Particles/Collision/BinaryCollision/DSMC/DSMCFunc.H` | 4 | public CollisionFuncBase |
| `LinearComptonCollisionFunc` | `Source/Particles/Collision/BinaryCollision/LinearCompton/LinearComptonCollisionFunc.H` | 4 | public CollisionFuncBase |
| `LinearBreitWheelerCollisionFunc` | `Source/Particles/Collision/BinaryCollision/LinearBreitWheeler/LinearBreitWheelerCollisionFunc.H` | 4 | public CollisionFuncBase |
| `BremsstrahlungFunc` | `Source/Particles/Collision/BinaryCollision/Bremsstrahlung/BremsstrahlungFunc.H` | 4 | public CollisionFuncBase |
| `SmartCopyFactory` | `Source/Particles/ParticleCreation/SmartCopy.H` | 3 |  |
| `SmartCreateFactory` | `Source/Particles/ParticleCreation/SmartCreate.H` | 3 |  |
| `VelocityCoincidenceThinning` | `Source/Particles/Resampling/VelocityCoincidenceThinning.H` | 3 | public ResamplingAlgorithm |
| `PicsarQedVector` | `Source/Particles/ElementaryProcess/QEDInternals/QedWrapperCommons.H` | 3 | public std::vector<Real> |
| `GetExternalEBField` | `Source/Particles/Gather/GetExternalFields.H` | 3 |  |
| `CollisionHandler` | `Source/Particles/Collision/CollisionHandler.H` | 3 |  |
| `CopyParticleAttribs` | `Source/Particles/Pusher/CopyParticleAttribs.H` | 2 |  |
| `BreitWheelerEvolveOpticalDepth` | `Source/Particles/ElementaryProcess/QEDInternals/BreitWheelerEngineWrapper.H` | 2 |  |
| `BreitWheelerGeneratePairs` | `Source/Particles/ElementaryProcess/QEDInternals/BreitWheelerEngineWrapper.H` | 2 |  |
| `QuantumSynchrotronEvolveOpticalDepth` | `Source/Particles/ElementaryProcess/QEDInternals/QuantumSyncEngineWrapper.H` | 2 |  |
| `QuantumSynchrotronPhotonEmission` | `Source/Particles/ElementaryProcess/QEDInternals/QuantumSyncEngineWrapper.H` | 2 |  |
| `ScaleFields` | `Source/Particles/Gather/ScaleFields.H` | 2 |  |
| `ParticleCreationFunc` | `Source/Particles/Collision/BinaryCollision/ParticleCreationFunc.H` | 2 |  |
| `NoParticleCreationFunc` | `Source/Particles/Collision/BinaryCollision/ParticleCreationFunc.H` | 2 |  |
| `SplitAndScatterFunc` | `Source/Particles/Collision/BinaryCollision/DSMC/SplitAndScatterFunc.H` | 2 |  |
| `PhotonCreationFunc` | `Source/Particles/Collision/BinaryCollision/Bremsstrahlung/PhotonCreationFunc.H` | 2 |  |
| `ExternalParticleFields` | `Source/Particles/ExternalParticleFields.H` | 1 |  |
| `SetParticlePosition` | `Source/Particles/Pusher/GetAndSetPosition.H` | 1 |  |
| `PlasmaParserWrapper` | `Source/Particles/ParticleCreation/AddPlasmaUtilities.H` | 1 |  |
| `QEDHelper` | `Source/Particles/ParticleCreation/AddPlasmaUtilities.H` | 1 |  |
| `SmartCopyTag` | `Source/Particles/ParticleCreation/SmartUtils.H` | 1 |  |
| `RandomFilter` | `Source/Particles/Filter/FilterFunctors.H` | 1 |  |
| `UniformFilter` | `Source/Particles/Filter/FilterFunctors.H` | 1 |  |
| `ParserFilter` | `Source/Particles/Filter/FilterFunctors.H` | 1 |  |
| `GeometryFilter` | `Source/Particles/Filter/FilterFunctors.H` | 1 |  |
| `fillBufferFlag` | `Source/Particles/Sorting/SortingUtils.H` | 1 |  |
| `fillBufferFlagRemainingParticles` | `Source/Particles/Sorting/SortingUtils.H` | 1 |  |
| `copyAndReorder` | `Source/Particles/Sorting/SortingUtils.H` | 1 |  |
| `IonizationFilterFunc` | `Source/Particles/ElementaryProcess/Ionization.H` | 1 |  |
| `PairGenerationFilterFunc` | `Source/Particles/ElementaryProcess/QEDPairGeneration.H` | 1 |  |
| `PairGenerationTransformFunc` | `Source/Particles/ElementaryProcess/QEDPairGeneration.H` | 1 |  |
| `PhotonEmissionFilterFunc` | `Source/Particles/ElementaryProcess/QEDPhotonEmission.H` | 1 |  |
| `PhotonEmissionTransformFunc` | `Source/Particles/ElementaryProcess/QEDPhotonEmission.H` | 1 |  |
| `BreitWheelerGetOpticalDepth` | `Source/Particles/ElementaryProcess/QEDInternals/BreitWheelerEngineWrapper.H` | 1 |  |
| `QuantumSynchrotronGetOpticalDepth` | `Source/Particles/ElementaryProcess/QEDInternals/QuantumSyncEngineWrapper.H` | 1 |  |
| `Executor` | `Source/Particles/Collision/ScatteringProcess.H` | 1 |  |
| `ImpactIonizationFilterFunc` | `Source/Particles/Collision/BackgroundMCC/ImpactIonization.H` | 1 |  |
| `ImpactIonizationTransformFunc` | `Source/Particles/Collision/BackgroundMCC/ImpactIonization.H` | 1 |  |
| `ParticleFieldMetaData` | `Source/Particles/ExternalParticleFields.H` | 0 |  |
| `ParticleBoundariesData` | `Source/Particles/ParticleBoundaries.H` | 0 |  |
| `Compute_shape_factor` | `Source/Particles/ShapeFactors.H` | 0 |  |
| `Compute_shifted_shape_factor` | `Source/Particles/ShapeFactors.H` | 0 |  |
| `Compute_shape_factor_pair` | `Source/Particles/ShapeFactors.H` | 0 |  |
| `PIdx` | `Source/Particles/WarpXParticleContainer.H` | 0 |  |
| `IntIdx` | `Source/Particles/WarpXParticleContainer.H` | 0 |  |
| `DiagIdx` | `Source/Particles/WarpXParticleContainer.H` | 0 |  |
| `FieldGatherOrders` | `Source/Particles/WarpXParticleContainer.H` | 0 |  |
| `SmartCopy` | `Source/Particles/ParticleCreation/SmartCopy.H` | 0 |  |
| `SmartCreate` | `Source/Particles/ParticleCreation/SmartCreate.H` | 0 |  |
| `HeapSort` | `Source/Particles/Resampling/VelocityCoincidenceThinning.H` | 0 |  |
| `VelocityBinCalculator` | `Source/Particles/Resampling/VelocityCoincidenceThinning.H` | 0 |  |
| `IonizationTransformFunc` | `Source/Particles/ElementaryProcess/Ionization.H` | 0 |  |
| `SchwingerFilterFunc` | `Source/Particles/ElementaryProcess/QEDSchwingerProcess.H` | 0 |  |
| `SchwingerTransformFunc` | `Source/Particles/ElementaryProcess/QEDSchwingerProcess.H` | 0 |  |
| `PicsarBreitWheelerCtrl` | `Source/Particles/ElementaryProcess/QEDInternals/BreitWheelerEngineWrapper.H` | 0 |  |
| `PicsarQuantumSyncCtrl` | `Source/Particles/ElementaryProcess/QEDInternals/QuantumSyncEngineWrapper.H` | 0 |  |
| `Executor` | `Source/Particles/Collision/BinaryCollision/Coulomb/PairWiseCoulombCollisionFunc.H` | 0 |  |
| `Executor` | `Source/Particles/Collision/BinaryCollision/NuclearFusion/NuclearFusionFunc.H` | 0 |  |
| `Executor` | `Source/Particles/Collision/BinaryCollision/DSMC/DSMCFunc.H` | 0 |  |
| `Executor` | `Source/Particles/Collision/BinaryCollision/LinearCompton/LinearComptonCollisionFunc.H` | 0 |  |
| `Executor` | `Source/Particles/Collision/BinaryCollision/LinearBreitWheeler/LinearBreitWheelerCollisionFunc.H` | 0 |  |
| `Executor` | `Source/Particles/Collision/BinaryCollision/Bremsstrahlung/BremsstrahlungFunc.H` | 0 |  |

### Root

| 类/结构体 | 文件 | 公共方法 | 基类 |
|---|---|---:|---|
| `WarpX` | `Source/WarpX.H` | 217 | public amrex::AmrCore |

### Utils

| 类/结构体 | 文件 | 公共方法 | 基类 |
|---|---|---:|---|
| `BTDIntervalsParser` | `Source/Utils/Parser/BTDIntervalsParser.H` | 6 |  |
| `AnisotropicCoefficientTable` | `Source/Utils/ScatteringUtils.H` | 1 |  |
| `HeapSortDecreasing` | `Source/Utils/ParticleUtils.H` | 0 |  |

### ablastr

| 类/结构体 | 文件 | 公共方法 | 基类 |
|---|---|---:|---|
| `MultiFabRegister` | `Source/ablastr/fields/MultiFabRegister.H` | 34 |  |
| `WarnManager` | `Source/ablastr/warn_manager/WarnManager.H` | 15 |  |
| `SliceParser` | `Source/ablastr/utils/text/IntervalsParser.H` | 8 |  |
| `IntervalsParser` | `Source/ablastr/utils/text/IntervalsParser.H` | 8 |  |
| `SignalHandling` | `Source/ablastr/utils/SignalHandling.H` | 6 |  |
| `Logger` | `Source/ablastr/utils/msg_logger/MsgLogger.H` | 6 |  |
| `KernelTimer` | `Source/ablastr/parallelization/KernelTimer.H` | 5 |  |
| `Timer` | `Source/ablastr/utils/timer/Timer.H` | 5 |  |
| `MLMGOptions` | `Source/ablastr/fields/MLMGOptions.H` | 4 |  |
| `Msg` | `Source/ablastr/utils/msg_logger/MsgLogger.H` | 3 |  |
| `MsgWithCounter` | `Source/ablastr/utils/msg_logger/MsgLogger.H` | 3 |  |
| `MsgWithCounterAndRanks` | `Source/ablastr/utils/msg_logger/MsgLogger.H` | 3 |  |
| `Direction` | `Source/ablastr/fields/MultiFabRegister.H` | 2 |  |
| `PoissonInterpCPtoFP` | `Source/ablastr/fields/Interpolate.H` | 1 |  |
| `FFTplan` | `Source/ablastr/math/fft/AnyFFT.H` | 0 |  |
| `is_castable_to_string` | `Source/ablastr/fields/MultiFabRegister.H` | 0 | std::false_type |
| `MultiFabOwner` | `Source/ablastr/fields/MultiFabRegister.H` | 0 |  |

