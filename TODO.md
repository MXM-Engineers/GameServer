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
* ✅ Don't slide on triangles
* ✅ Go up the ramp
* ⚠️ Spatial queries:
    - I'd like to move from A to B, where do I end up?
* Dynamic body <-> Dynamic body collision (needed for AI mobs such as rozark)
* Collision groups (only collide with AI mobs not players)
* Try to make rolling work

# Test
* Interpolate (extrapolate) between player positions
* ✅ Draw directions
* Try to roll without testing for collision