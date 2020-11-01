# Skill activation

For a player to successfully activate a skill we have to do this; in order:

1. Receive a **CQ_PlayerCastSkill** packet.
2. Send a **SA_CastSkill** to the caster and **SN_CastSkill** packet to everyone (when the cast is authorized).
3. Send a **SN_ExecuteSkill** to everyone when the skill *activates*.
    - The action state correspond to the animation (and probably sound file) (often SKILL_X_NEHAVIORSTATE)
    - the graphMove structure is used for movement (e.g. rolling)
4. (Optional) Create a "Remote" (projectile).
5. (Optional) Send **SN_RemoteActivated** and **SN_RemoteSyncCreateFromRemoteDoc** to everyone to signal the "Remote" creation.
    - The remote is defined in "Design/DOCUMENT/Actionbase.xml"
6. Send a **SN_BroadcastDamage** packet to everyone to signal any damage done.