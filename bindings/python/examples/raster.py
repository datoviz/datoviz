import numpy as np
import visky

n = 10000
visky.markers(
    .25 * np.random.randn(n, 3),
    np.random.randint(size=(n, 4), low=0, high=255))
