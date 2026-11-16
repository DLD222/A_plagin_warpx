# Boundary-condition plugin: PEC and dielectric on embedded surfaces.
#
# Independent from the field-solver plugin and from the PicEm/EB module: it includes
# only core headers (Runtime/IBoundaryConditions.H, Runtime/IEBGeometry.H).  Enable with
#     -DWarpX_PLUGIN_BC=ON
option(WarpX_PLUGIN_BC "Build the boundary-condition plugin (PEC + dielectric on EB)" ON)

if(WarpX_PLUGIN_BC)
    foreach(SD IN LISTS WarpX_DIMS)
        add_library(warpx_boundaryconditions_${SD} STATIC PlasmaBoundary.cpp)
        target_include_directories(warpx_boundaryconditions_${SD} PUBLIC
            ${CMAKE_SOURCE_DIR}/Source/Core
            ${CMAKE_SOURCE_DIR}/Source/Plugins/BoundaryConditions)
    endforeach()
endif()
