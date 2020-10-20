from visky import pyvisky

app = pyvisky.App()
c = app.canvas()
s = c.scene()
p = s.panel()
p.set_controller()
app.run()
del app
