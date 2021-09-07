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

* ✅ Fix replication sending players moving while they don't (like when stuck in a corner)
    - This is hard to fix properly, the sent move direction is not perfect so when we run into a wall it does not match 1:1 on the server side
    - One solution is to not send the "running into a wall" animation

* Make physics match the client more (for example we can go up the ramp from the side)
    - ✅ Make the phys body move based on input.moveTo
    - ✅ Fix 1s delay
    - ✅ Going up the ramp is slower on our side because we don't convert 100% of the speed, make it the same
    - ✅ Fix losing speed on flat ground
* ✅ Try to make rolling work

* ✅ Make the client send more position updates (like rotation updates)
    - ✅ Hypothesis: the 0.5s delay is due to a bug. Since we send a position update every 0.5s, the first one is delayed by that amount.

# Test
* ✅ Draw directions
* ✅ Try to roll without testing for collision

* ✅ Make ghidra script to fix ALL flow override for function X (clear flow & repair at call site) [constbuffer_advance, _eh_prolog3]
* ✅ Reverse SN_GameEnterActor packet, ✅ use it to tag other players

* Edit SKILL.xml in DOCUMENTS to make Sizuka's dodge not test terrain.

# Inner communication
* 🔥🔥🔥 Send message from hub to game server to create a game (pvp 3v3) instance, then connect to it
    - Need an inner communication protocol
    - ✅ Need to connect to game servers (make a game_servers.txt list)
    - Have a timeout for each request in case a game server goes down (crash, network, whatever else). The request needs to be resent elsewhere then.

# Networking
* Check packet length when parsing / receiving? And drop client instead of crashing (damn you russian bots)
* ✅ Make a unique identifier for each client and a mapping between Local client ID and Unique client ID
    - ✅ Everything except server uses unique client ID
    - ✅ Make only one local mapping per instance and pass it down to game and replication?
* 🔥 De-duplicate coordinator/lane code

# NavMesh
* Make a nav mesh representing all the possible movement positions and interpolate between them (linked nodes, see navmesh.png)
    - This would replace the "physics engine" while navigating

# Matchmaking
* When 6 players are in queue, consume them.
    - Ask a game server to create a match instance.
    - When created, send packet to players to connect to said server.
    - When all players are connected, start the picking phase
    - When picking phase is done, send load map
