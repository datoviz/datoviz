n = 10000
x = .25 * randn(Float64, (10000, 2))
ccall((:vky_demo_scatter, "../../build/libvisky.so"), Int32, (Cint, Ref{Cdouble}), 10000, x)
