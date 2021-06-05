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
* Fix replication sending players moving while they don't (like when stuck in a corner)
* Make physics match the client more (for example we can go up the ramp from the side)
    - ✅ Make the phys body move based on input.moveTo
    - ✅ Fix 1s delay
    - ✅ Going up the ramp is slower on our side because we don't convert 100% of the speed, make it the same
* Try to make rolling work

# Test
* Interpolate (extrapolate) between player positions
* ✅ Draw directions
* Try to roll without testing for collision

* ✅ Make ghidra script to fix ALL flow override for function X (clear flow & repair at call site) [constbuffer_advance, _eh_prolog3]
* ✅ Reverse SN_GameEnterActor packet, ✅ use it to tag other players