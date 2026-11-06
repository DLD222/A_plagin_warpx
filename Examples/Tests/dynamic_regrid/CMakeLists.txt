add_warpx_test(
    test_2d_dynamic_regrid  # name
    2  # dims
    1  # nprocs
    inputs_test_2d_dynamic_regrid  # inputs
    "analysis.py diags/diag1000080"  # analysis
    "analysis.py diags/diag1000080"  # checksum
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
