puddle = gr.mesh('puddle', 'puddle')
puddle:set_material(gr.material({1.0, 1.0, 1.0}, {0.0, 0.0, 0.0}, 100.0))
puddle:scale(1.5, 1.0, 2.0)
puddle:translate(-9.0, -2.99, -15.0)
return puddle