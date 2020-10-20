from visky import pyvisky

app = pyvisky.App()
c = app.canvas()
s = c.scene()
p = s.panel()
p.set_controller()
p.visual('marker')
app.run()
del app
