add_warpx_test(
    test_2d_dynamic_regrid  # name
    2  # dims
    1  # nprocs
    inputs_test_2d_dynamic_regrid  # inputs
    "analysis.py diags/diag1000015"  # analysis
    "analysis.py diags/diag1000015"  # checksum
    OFF  # dependency
)

add_warpx_test(
    test_2d_dynamic_regrid_remove
    2
    1
    inputs_test_2d_dynamic_regrid_remove
    "analysis_removed.py diags/diag1000004"
    "analysis_removed.py diags/diag1000004"
    OFF
)

add_warpx_test(
    test_2d_dynamic_regrid_remake
    2
    1
    inputs_test_2d_dynamic_regrid_remake
    "analysis_remake.py diags/diag1000008"
    "analysis_remake.py diags/diag1000008"
    OFF
)
