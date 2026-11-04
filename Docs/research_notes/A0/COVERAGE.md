# A0：头文件覆盖表

<!-- SPDX-License-Identifier: BSD-3-Clause -->

- 头文件总数：**371**
- 解析出至少一个 class/struct 的头文件：**235**
- 未解析出 class/struct 的头文件：**136**
  - 其中 **30** 个是 `_fwd.H` 前向声明（`class Foo;`），不是遗漏；
  - 其余 **106** 个是 free function / kernel / 模板工具 / 枚举，无类可列。
- 解析出的 class/struct：**348**；公共方法（清理后）：**2,408**

## 未解析出头文件的分布

| 子系统 | 数量 |
|---|---:|
| Particles | 44 |
| ablastr | 24 |
| FieldSolver | 14 |
| Utils | 13 |
| BoundaryConditions | 7 |
| Diagnostics | 7 |
| EmbeddedBoundary | 5 |
| Initialization | 5 |
| Fluids | 4 |
| NonlinearSolvers | 4 |
| MomentsSolver | 2 |
| Parallelization | 2 |
| Python | 2 |
| AcceleratorLattice | 1 |
| Fields.H | 1 |
| Filter | 1 |

## 未解析出 class/struct 的头文件

> 需要人工确认它们是“纯 free-function/kernel”还是解析遗漏。

- `Source/AcceleratorLattice/LatticeElements/HardEdged_K.H`
- `Source/BoundaryConditions/FieldBoundaries.H`
- `Source/BoundaryConditions/PEC_Insulator_fwd.H`
- `Source/BoundaryConditions/PML_RZ_fwd.H`
- `Source/BoundaryConditions/PML_current.H`
- `Source/BoundaryConditions/PML_fwd.H`
- `Source/BoundaryConditions/WarpX_PEC.H`
- `Source/BoundaryConditions/WarpX_PML_kernels.H`
- `Source/Diagnostics/ComputeDiagFunctors/ComputeDiagFunctor_fwd.H`
- `Source/Diagnostics/FieldIO.H`
- `Source/Diagnostics/FlushFormats/FlushFormat_fwd.H`
- `Source/Diagnostics/MultiDiagnostics_fwd.H`
- `Source/Diagnostics/OpenPMDHelpFunction.H`
- `Source/Diagnostics/ParticleDiag/ParticleDiag_fwd.H`
- `Source/Diagnostics/ReducedDiags/MultiReducedDiags_fwd.H`
- `Source/EmbeddedBoundary/DistanceToEB.H`
- `Source/EmbeddedBoundary/EmbeddedBoundaryInit.H`
- `Source/EmbeddedBoundary/Enabled.H`
- `Source/EmbeddedBoundary/ParticleScraper.H`
- `Source/EmbeddedBoundary/WarpXFaceInfoBox_fwd.H`
- `Source/FieldSolver/ElectrostaticSolvers/ElectrostaticSolver_fwd.H`
- `Source/FieldSolver/FiniteDifferenceSolver/FiniteDifferenceSolver_fwd.H`
- `Source/FieldSolver/FiniteDifferenceSolver/HybridPICModel/HybridPICModel_fwd.H`
- `Source/FieldSolver/FiniteDifferenceSolver/MacroscopicProperties/MacroscopicProperties_fwd.H`
- `Source/FieldSolver/ImplicitSolvers/ImplicitSolverLibrary.H`
- `Source/FieldSolver/ImplicitSolvers/ImplicitSolver_fwd.H`
- `Source/FieldSolver/SpectralSolver/SpectralFieldData_fwd.H`
- `Source/FieldSolver/SpectralSolver/SpectralHankelTransform/BesselRoots.H`
- `Source/FieldSolver/SpectralSolver/SpectralKSpace_fwd.H`
- `Source/FieldSolver/SpectralSolver/SpectralSolverRZ_fwd.H`
- `Source/FieldSolver/SpectralSolver/SpectralSolver_fwd.H`
- `Source/FieldSolver/WarpXPushFieldsEM_K.H`
- `Source/FieldSolver/WarpX_FDTD.H`
- `Source/FieldSolver/WarpX_QED_K.H`
- `Source/Fields.H`
- `Source/Filter/NCIGodfreyFilter_fwd.H`
- `Source/Fluids/MultiFluidContainer_fwd.H`
- `Source/Fluids/MusclHancockUtils.H`
- `Source/Fluids/QdsmcParticleContainer_fwd.H`
- `Source/Fluids/WarpXFluidContainer_fwd.H`
- `Source/Initialization/ExternalField_fwd.H`
- `Source/Initialization/InjectorPosition_fwd.H`
- `Source/Initialization/SampleGaussianFluxDistribution.H`
- `Source/Initialization/WarpXAMReXInit.H`
- `Source/Initialization/WarpXInit.H`
- `Source/MomentsSolver/Coupling/TenMomentClosure.H`
- `Source/MomentsSolver/Equations/MomentsConfig.H`
- `Source/NonlinearSolvers/LinearSolverLibrary.H`
- `Source/NonlinearSolvers/NonlinearSolverLibrary.H`
- `Source/NonlinearSolvers/PreconditionerLibrary.H`
- `Source/NonlinearSolvers/PreconditionerTypes.H`
- `Source/Parallelization/WarpXComm_K.H`
- `Source/Parallelization/WarpXSumGuardCells.H`
- `Source/Particles/Algorithms/KineticEnergy.H`
- `Source/Particles/Collision/BinaryCollision/BinaryCollisionUtils.H`
- `Source/Particles/Collision/BinaryCollision/Coulomb/ElasticCollisionPerez.H`
- `Source/Particles/Collision/BinaryCollision/Coulomb/UpdateMomentumPerezElastic.H`
- `Source/Particles/Collision/BinaryCollision/DSMC/CollisionFilterFunc.H`
- `Source/Particles/Collision/BinaryCollision/LinearBreitWheeler/LinearBreitWheelerCrossSection.H`
- `Source/Particles/Collision/BinaryCollision/LinearBreitWheeler/LinearBreitWheelerInitializeMomentum.H`
- `Source/Particles/Collision/BinaryCollision/LinearBreitWheeler/LinearBreitWheelerUtil.H`
- `Source/Particles/Collision/BinaryCollision/LinearBreitWheeler/SingleLinearBreitWheelerCollisionEvent.H`
- `Source/Particles/Collision/BinaryCollision/LinearCompton/LinearComptonInitializeMomentum.H`
- `Source/Particles/Collision/BinaryCollision/LinearCompton/SingleLinearComptonCollisionEvent.H`
- `Source/Particles/Collision/BinaryCollision/NuclearFusion/BoschHaleFusionCrossSection.H`
- `Source/Particles/Collision/BinaryCollision/NuclearFusion/ProtonBoronFusionCrossSection.H`
- `Source/Particles/Collision/BinaryCollision/NuclearFusion/ProtonBoronFusionInitializeMomentum.H`
- `Source/Particles/Collision/BinaryCollision/NuclearFusion/SingleNuclearFusionEvent.H`
- `Source/Particles/Collision/BinaryCollision/NuclearFusion/TwoProductFusionInitializeMomentum.H`
- `Source/Particles/Collision/BinaryCollision/TwoProductUtil.H`
- `Source/Particles/Collision/BinaryCollision/VirtualPhotonCreation.H`
- `Source/Particles/Deposition/ChargeDeposition.H`
- `Source/Particles/Deposition/CurrentDeposition.H`
- `Source/Particles/Deposition/MassMatricesDeposition.H`
- `Source/Particles/Deposition/SharedDepositionUtils.H`
- `Source/Particles/Deposition/TemperatureDeposition.H`
- `Source/Particles/Deposition/TemperatureDepositionTypes.H`
- `Source/Particles/ElementaryProcess/QEDInternals/BreitWheelerEngineWrapper_fwd.H`
- `Source/Particles/ElementaryProcess/QEDInternals/QedChiFunctions.H`
- `Source/Particles/ElementaryProcess/QEDInternals/QuantumSyncEngineWrapper_fwd.H`
- `Source/Particles/ElementaryProcess/QEDInternals/SchwingerProcessWrapper.H`
- `Source/Particles/Gather/FieldGather.H`
- `Source/Particles/MultiParticleContainer_fwd.H`
- `Source/Particles/ParticleBoundaries_K.H`
- `Source/Particles/ParticleBoundaryBuffer_fwd.H`
- `Source/Particles/ParticleCreation/DefaultInitialization.H`
- `Source/Particles/ParticleCreation/FilterCopyTransform.H`
- `Source/Particles/ParticleCreation/FilterCreateTransformFromFAB.H`
- `Source/Particles/ParticleIO.H`
- `Source/Particles/Pusher/PushSelector.H`
- `Source/Particles/Pusher/UpdateMomentumBoris.H`
- `Source/Particles/Pusher/UpdateMomentumBorisWithRadiationReaction.H`
- `Source/Particles/Pusher/UpdateMomentumHigueraCary.H`
- `Source/Particles/Pusher/UpdateMomentumVay.H`
- `Source/Particles/Pusher/UpdatePosition.H`
- `Source/Particles/SpeciesPhysicalProperties.H`
- `Source/Particles/WarpXParticleContainer_fwd.H`
- `Source/Python/callbacks.H`
- `Source/Python/pyWarpX.H`
- `Source/Utils/Algorithms/IsIn.H`
- `Source/Utils/Interpolate.H`
- `Source/Utils/Interpolate_K.H`
- `Source/Utils/Logo/GetLogo.H`
- `Source/Utils/NCIGodfreyTables.H`
- `Source/Utils/Parser/ParserUtils.H`
- `Source/Utils/Physics/IonizationEnergiesTable.H`
- `Source/Utils/SpeciesUtils.H`
- `Source/Utils/TextMsg.H`
- `Source/Utils/WarpXAlgorithmSelection.H`
- `Source/Utils/WarpXConst.H`
- `Source/Utils/WarpXUtil.H`
- `Source/Utils/WarpX_Complex.H`
- `Source/ablastr/coarsen/average.H`
- `Source/ablastr/coarsen/sample.H`
- `Source/ablastr/constant.H`
- `Source/ablastr/fields/EffectivePotentialPoissonSolver.H`
- `Source/ablastr/fields/IntegratedGreenFunctionSolver.H`
- `Source/ablastr/fields/PoissonSolver.H`
- `Source/ablastr/fields/VectorPoissonSolver.H`
- `Source/ablastr/math/FiniteDifference.H`
- `Source/ablastr/math/LinearInterpolation.H`
- `Source/ablastr/math/RandomSeed.H`
- `Source/ablastr/parallelization/MPIInitHelpers.H`
- `Source/ablastr/particles/DepositCharge.H`
- `Source/ablastr/particles/NodalFieldGather.H`
- `Source/ablastr/particles/ParticleMoments.H`
- `Source/ablastr/profiler/ProfilerWrapper.H`
- `Source/ablastr/utils/Communication.H`
- `Source/ablastr/utils/Enums.H`
- `Source/ablastr/utils/RelativeCellPosition.H`
- `Source/ablastr/utils/Serialization.H`
- `Source/ablastr/utils/TextMsg.H`
- `Source/ablastr/utils/UsedInputsFile.H`
- `Source/ablastr/utils/msg_logger/MsgLogger_fwd.H`
- `Source/ablastr/utils/text/StreamUtils.H`
- `Source/ablastr/utils/text/StringUtils.H`
