-- Simple Scene:
-- An extremely simple scene that will render right out of the box
-- with the provided skeleton code.  It doesn't rely on hierarchical
-- transformations.

-- Create the top level root node named 'root'.
rootNode = gr.node('root')

ground = gr.node('ground')
obstacles = gr.node('obstacles')

rootNode:add_child(ground);
rootNode:add_child(obstacles)

grassMesh = gr.mesh('grass', 'grass')
grassMesh:rotate('y', -90.0);
grassMesh:scale(1.25, 1.25, 1.25);
grassMesh:translate(0.0, -3.0, -17.0)
grassMesh:set_material(gr.material({1.0, 1.0, 1.0}, {0.0, 0.0, 0.0}, 100.0))
ground:add_child(grassMesh)

-- Obstacles
------------------------------------------------------------------------------------------
crateId = 0

for i = 0, 3, 1
do
	for j = 0, 1, 1
	do
		crateMesh = gr.mesh('crate', 'crate' .. crateId)
		crateMesh:translate(-5.0 + i, -2.5 + j, -13.0)
		crateMesh:set_material(gr.material({1.0, 1.0, 1.0}, {0.0, 0.0, 0.0}, 100.0))
		obstacles:add_child(crateMesh)
		crateId = crateId + 1
	end
end

for i = 0, 4, 1
do
	for j = 0, 4 - i, 1
	do
		crateMesh = gr.mesh('crate', 'crate' .. crateId)
		crateMesh:translate(2.0 + i, -2.5 + j, -17.0)
		crateMesh:set_material(gr.material({1.0, 1.0, 1.0}, {0.0, 0.0, 0.0}, 100.0))
		obstacles:add_child(crateMesh)
		crateId = crateId + 1
	end
end

crateMesh = gr.mesh('crate', 'crate' .. crateId)
crateMesh:translate(5.0, -2.5, -10.0)
crateMesh:set_material(gr.material({1.0, 1.0, 1.0}, {0.0, 0.0, 0.0}, 100.0))
obstacles:add_child(crateMesh)

-- Targets
------------------------------------------------------------------------------------------
-- circle is radius 0.5, and stick is length 0.71
targetMesh = gr.mesh('target', 'target')
--targetMesh:rotate('y', 90.0)
--targetMesh:rotate('x', 90.0)
targetMesh:translate(0.0, -3.0, 0.0)
--targetMesh:translate(-2.0, -1.79, -8.0)
targetMesh:set_material(gr.material({1.0, 1.0, 1.0}, {0.0, 0.0, 0.0}, 100.0))
rootNode:add_child(targetMesh)

-- targetMesh2 = gr.mesh('target', 'target2')
-- targetMesh2:rotate('y', 90.0)
-- targetMesh2:rotate('z', -90.0)
-- targetMesh2:translate(2.5, -1.79, -20.0)
-- targetMesh2:set_material(gr.material({1.0, 1.0, 1.0}, {0.0, 0.0, 0.0}, 100.0))
-- objects:add_child(targetMesh2)

-- targetMesh3 = gr.mesh('target', 'target3')
-- targetMesh3:rotate('y', -90.0)
-- targetMesh3:rotate('x', 90.0)
-- targetMesh3:translate(0.5, -0.3, -1.8)
-- targetMesh3:set_material(gr.material({1.0, 1.0, 1.0}, {0.0, 0.0, 0.0}, 100.0))

-- rootNode:add_child(targetMesh3)
------------------------------------------------------------------------------------------

-- arrowMesh2 = gr.mesh('arrow', 'testArrow')
-- -- orig scale was 0.3, 0.3 0.3
-- arrowMesh2:scale(0.3, 0.3, 0.3)
-- arrowMesh2:rotate('x', -90.0)
-- arrowMesh2:translate(0.36, -0.3, -1.2)
-- arrowMesh2:set_material(gr.material({1.0, 1.0, 1.0}, {0.0, 0.0, 0.0}, 100.0))

-- rootNode:add_child(arrowMesh2);

-- Bow and arrows
------------------------------------------------------------------------------------------
bowMesh = gr.mesh('bow', 'bow')
bowMesh:translate(0.4, -0.3, -1.0)
bowMesh:set_material(gr.material({1.0, 1.0, 1.0}, {0.0, 0.0, 0.0}, 100.0))
rootNode:add_child(bowMesh)

-- 0.9 in length
arrowMesh = gr.mesh('arrow', 'arrow')
-- orig scale was 0.3, 0.3 0.3
arrowMesh:translate(-0.04, 0.0, -0.3)
arrowMesh:set_material(gr.material({1.0, 1.0, 1.0}, {0.0, 0.0, 0.0}, 100.0))
bowMesh:add_child(arrowMesh);
------------------------------------------------------------------------------------------

-- Return the root with all of it's childern.  The SceneNode A3::m_rootNode will be set
-- equal to the return value from this Lua script.
return rootNode
