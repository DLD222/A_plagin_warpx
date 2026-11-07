# FieldSolver 实现清单

文件总数：60

| 模块 | 单元 | 文件 | 行数 | WarpX.H | Particles |
|---|---|---|---:|---:|---:|
| Electrostatic | ElectrostaticFieldSolve | `Electrostatic/EffectivePotentialES.cpp` | 299 | 1 | 1 |
| Electrostatic | ElectrostaticFieldSolve | `Electrostatic/ElectrostaticSolver.cpp` | 616 | 1 | 0 |
| Electrostatic | ElectrostaticFieldSolve | `Electrostatic/LabFrameExplicitES.cpp` | 349 | 1 | 1 |
| Electrostatic | ElectrostaticFieldSolve | `Electrostatic/PoissonBoundaryHandler.cpp` | 205 | 1 | 0 |
| Electrostatic | ElectrostaticFieldSolve | `Electrostatic/RelativisticExplicitES.cpp` | 185 | 1 | 2 |
| FiniteDifference/Boundaries | ApplySilverMuellerBoundary | `FiniteDifference/ApplySilverMuellerBoundary.cpp` | 394 | 0 | 0 |
| FiniteDifference/Operators | ComputeCurlA | `FiniteDifference/ComputeCurlA.cpp` | 461 | 1 | 0 |
| FiniteDifference/Operators | ComputeCurlB | `FiniteDifference/ComputeCurlB.cpp` | 232 | 1 | 0 |
| FiniteDifference/Operators | ComputeDivE | `FiniteDifference/ComputeDivE.cpp` | 257 | 0 | 0 |
| FiniteDifference/Operators | ComputeGradient | `FiniteDifference/ComputeGradient.cpp` | 183 | 1 | 0 |
| FiniteDifference/Operators | ComputeLaplacian | `FiniteDifference/ComputeLaplacian.cpp` | 532 | 1 | 0 |
| FiniteDifference/Units | EvolveElectromagneticField | `FiniteDifference/EvolveB.cpp` | 616 | 1 | 0 |
| FiniteDifference/Units | EvolveElectromagneticField | `FiniteDifference/EvolveBPML.cpp` | 192 | 0 | 0 |
| FiniteDifference/Units | EvolveElectromagneticField | `FiniteDifference/EvolveE.cpp` | 757 | 1 | 0 |
| FiniteDifference/Units | EvolveElectromagneticField | `FiniteDifference/EvolveECTRho.cpp` | 169 | 1 | 0 |
| FiniteDifference/Units | EvolveElectromagneticField | `FiniteDifference/EvolveEPML.cpp` | 260 | 0 | 0 |
| FiniteDifference/Units | EvolveF | `FiniteDifference/EvolveF.cpp` | 299 | 0 | 0 |
| FiniteDifference/Units | EvolveF | `FiniteDifference/EvolveFPML.cpp` | 130 | 0 | 0 |
| FiniteDifference/Units | EvolveG | `FiniteDifference/EvolveG.cpp` | 113 | 0 | 0 |
| FiniteDifference/Driver | FiniteDifferenceFieldSolver | `FiniteDifference/FiniteDifferenceSolver.cpp` | 148 | 1 | 0 |
| FiniteDifference/HybridPIC | HybridPICSolveElectricField | `FiniteDifference/HybridPICModel/ExternalVectorPotential.cpp` | 379 | 1 | 0 |
| FiniteDifference/HybridPIC | HybridPICSolveElectricField | `FiniteDifference/HybridPICModel/HybridPICModel.cpp` | 3002 | 1 | 1 |
| FiniteDifference/HybridPIC | HybridPICSolveElectricField | `FiniteDifference/HybridPICSolveE.cpp` | 1585 | 1 | 0 |
| FiniteDifference/Macroscopic | MacroscopicFieldUpdate | `FiniteDifference/MacroscopicEvolveE.cpp` | 434 | 1 | 0 |
| FiniteDifference/Macroscopic | MacroscopicFieldUpdate | `FiniteDifference/MacroscopicProperties/MacroscopicProperties.cpp` | 1142 | 1 | 0 |
| Implicit | ImplicitFieldSolve | `Implicit/DarwinLinearFieldOperator.cpp` | 218 | 1 | 0 |
| Implicit | ImplicitFieldSolve | `Implicit/ImplicitSolver.cpp` | 1556 | 1 | 1 |
| Implicit | ImplicitFieldSolve | `Implicit/SemiImplicitDarwin.cpp` | 678 | 1 | 0 |
| Implicit | ImplicitFieldSolve | `Implicit/SemiImplicitEM.cpp` | 208 | 1 | 0 |
| Implicit | ImplicitFieldSolve | `Implicit/SparseJacobianMatrix.cpp` | 651 | 0 | 0 |
| Implicit | ImplicitFieldSolve | `Implicit/StrangImplicitSpectralEM.cpp` | 153 | 1 | 0 |
| Implicit | ImplicitFieldSolve | `Implicit/ThetaImplicitEM.cpp` | 532 | 1 | 0 |
| Implicit | ImplicitFieldSolve | `Implicit/WarpXImplicitOps.cpp` | 388 | 1 | 2 |
| Implicit | ImplicitFieldSolve | `Implicit/WarpXSolverDOF.cpp` | 220 | 1 | 0 |
| Implicit | ImplicitFieldSolve | `Implicit/WarpXSolverVec.cpp` | 269 | 1 | 0 |
| Magnetostatic | MagnetostaticFieldSolve | `Magnetostatic/MagnetostaticSolver.cpp` | 491 | 1 | 2 |
| PSATD/SpectralAlgorithms | SpectralFieldUpdate | `PSATD/SpectralAlgorithms/PsatdAlgorithmComoving.cpp` | 512 | 0 | 0 |
| PSATD/SpectralAlgorithms | SpectralFieldUpdate | `PSATD/SpectralAlgorithms/PsatdAlgorithmGalilean.cpp` | 795 | 0 | 0 |
| PSATD/SpectralAlgorithms | SpectralFieldUpdate | `PSATD/SpectralAlgorithms/PsatdAlgorithmGalileanRZ.cpp` | 364 | 1 | 0 |
| PSATD/SpectralAlgorithms | SpectralFieldUpdate | `PSATD/SpectralAlgorithms/PsatdAlgorithmJRhomFirstOrder.cpp` | 375 | 0 | 0 |
| PSATD/SpectralAlgorithms | SpectralFieldUpdate | `PSATD/SpectralAlgorithms/PsatdAlgorithmJRhomSecondOrder.cpp` | 681 | 0 | 0 |
| PSATD/SpectralAlgorithms | SpectralFieldUpdate | `PSATD/SpectralAlgorithms/PsatdAlgorithmPml.cpp` | 469 | 0 | 0 |
| PSATD/SpectralAlgorithms | SpectralFieldUpdate | `PSATD/SpectralAlgorithms/PsatdAlgorithmPmlRZ.cpp` | 173 | 1 | 0 |
| PSATD/SpectralAlgorithms | SpectralFieldUpdate | `PSATD/SpectralAlgorithms/PsatdAlgorithmRZ.cpp` | 494 | 1 | 0 |
| PSATD/SpectralAlgorithms | SpectralFieldUpdate | `PSATD/SpectralAlgorithms/SpectralBaseAlgorithm.cpp` | 115 | 0 | 0 |
| PSATD/SpectralAlgorithms | SpectralFieldUpdate | `PSATD/SpectralAlgorithms/SpectralBaseAlgorithmRZ.cpp` | 80 | 0 | 0 |
| PSATD | SpectralFieldData | `PSATD/SpectralBinomialFilter.cpp` | 52 | 0 | 0 |
| PSATD | SpectralFieldData | `PSATD/SpectralFieldData.cpp` | 441 | 1 | 0 |
| PSATD | SpectralFieldData | `PSATD/SpectralFieldDataRZ.cpp` | 920 | 1 | 0 |
| PSATD/Hankel | HankelTransform | `PSATD/SpectralHankelTransform/BesselRoots.cpp` | 153 | 0 | 0 |
| PSATD/Hankel | HankelTransform | `PSATD/SpectralHankelTransform/HankelTransform.cpp` | 261 | 0 | 0 |
| PSATD/Hankel | HankelTransform | `PSATD/SpectralHankelTransform/SpectralHankelTransformer.cpp` | 192 | 0 | 0 |
| PSATD | SpectralFieldData | `PSATD/SpectralKSpace.cpp` | 277 | 0 | 0 |
| PSATD | SpectralFieldData | `PSATD/SpectralKSpaceRZ.cpp` | 53 | 0 | 0 |
| PSATD | SpectralFieldData | `PSATD/SpectralSolver.cpp` | 184 | 0 | 0 |
| PSATD | SpectralFieldData | `PSATD/SpectralSolverRZ.cpp` | 206 | 1 | 0 |
| Electromagnetic/Driver | PushElectromagneticFields | `WarpXPushFieldsEM.cpp` | 2113 | 1 | 0 |
| FiniteDifference/HybridPIC | HybridPICSolveElectricField | `WarpXPushFieldsHybridPIC.cpp` | 608 | 1 | 1 |
| Electrostatic/Driver | SolveElectrostaticFields | `WarpXSolveFieldsES.cpp` | 37 | 1 | 0 |
| QEDFieldPush | QEDFieldPush | `WarpX_QED_Field_Pushers.cpp` | 196 | 1 | 0 |

## 汇总

- Electromagnetic/Driver: 1 个 CPP, 2113 行
- Electrostatic: 5 个 CPP, 1654 行
- Electrostatic/Driver: 1 个 CPP, 37 行
- FiniteDifference/Boundaries: 1 个 CPP, 394 行
- FiniteDifference/Driver: 1 个 CPP, 148 行
- FiniteDifference/HybridPIC: 4 个 CPP, 5574 行
- FiniteDifference/Macroscopic: 2 个 CPP, 1576 行
- FiniteDifference/Operators: 5 个 CPP, 1665 行
- FiniteDifference/Units: 8 个 CPP, 2536 行
- Implicit: 10 个 CPP, 4873 行
- Magnetostatic: 1 个 CPP, 491 行
- PSATD: 7 个 CPP, 2133 行
- PSATD/Hankel: 3 个 CPP, 606 行
- PSATD/SpectralAlgorithms: 10 个 CPP, 4058 行
- QEDFieldPush: 1 个 CPP, 196 行

## 未分类

- 无
