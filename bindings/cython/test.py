from visky import pyvisky

app = pyvisky.App()
c = app.canvas()
s = c.scene()
app.run()
del app
