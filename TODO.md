# Debug 3D viewer
* ✅ Actual 3D mouse camera controls
* ✅ Reset view
* TOP, BOTTOM, LEFT, RIGHT, etc, FREE VIEW modes
* ✅ Capsule mesh
* ✅ Public API to push game states (players, entities, map)
* ✅ integrate imgui (sokol_imgui)
* switch between active games with imgui

# Physics
* ✅ Capsule <-> Triangle collision
* ⚠️ Some ground triangles don't have a perfect up normal vector (I'm guessing it is the case for a lot of them), so we slide on them.
    - Compare mesh normal with triangle normal
    - Could be an epsilon issue
    - Actually just make it so if we don't move, there is no XY vel component
* Try to make rolling work

# Test
* Interpolate (extrapolate) between player positions
* ✅ Draw directions
* Try to roll without testing for collision