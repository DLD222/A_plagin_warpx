# Boundary-condition plugin: PEC and dielectric on embedded surfaces.
#
# Independent from the field-solver plugin and from the PicEm/EB module: it includes
# only core headers (Runtime/IBoundaryConditions.H, Runtime/IEBGeometry.H, Core/Fields.H).
#
# The object target `warpx_boundaryconditions_<SD>` is created by the top-level
# CMakeLists (it is one of WARPX_MODULE_LAYERS); a plugin only adds its sources.
option(WarpX_PLUGIN_BC "Build the boundary-condition plugin (PEC + dielectric on EB)" ON)

if(WarpX_PLUGIN_BC)
    foreach(SD IN LISTS WarpX_DIMS)
        if(TARGET warpx_boundaryconditions_${SD})
            target_sources(warpx_boundaryconditions_${SD} PRIVATE
                ${CMAKE_CURRENT_SOURCE_DIR}/PlasmaBoundary.cpp
                ${CMAKE_CURRENT_SOURCE_DIR}/Register.cpp)
            target_include_directories(warpx_boundaryconditions_${SD} PRIVATE
                ${CMAKE_SOURCE_DIR}/Source/Core
                ${CMAKE_CURRENT_SOURCE_DIR})
        endif()
    endforeach()
endif()
