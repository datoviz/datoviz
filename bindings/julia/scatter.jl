n = 10000
x = .25 * randn(Float64, (10000, 3))
ccall((:dvz_demo_scatter, "../../build/libdatoviz.so"), Int32, (Cint, Ref{Cdouble}), 10000, x)
