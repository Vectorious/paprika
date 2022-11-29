Bugs
====


TODO
====
- Draw lines behind nodes.
- Display match result (if available) in `Current Match` window when bot is disabled.
- Add saving to a temp file and replacing the original when saving match data.
- Add way to open windows on launch.
- Consider switching to docking branch of Dear ImGui.
- Consider adding auto-update.
- Add formatting to LogWrite function.
    - Consider adding separator commas here.
- Refactor `Paprika_Input`.
- Improve matchup history system.
    - Probably have entire matchup history loaded, but plot uses a dynamic window into it.
- Add Salty Bet chat support. Pretty sure this is just Twitch chat (IRC?).

Security
--------
- Node systems operate under the assumption that the first node in the array is the output node. This could be untrue if the serialized data is directly manipulated.
    - Simple solution: add an assertion.
    - The node system will mostly function without this assumption.
    - For checking if the output node is dirty, add an `output_dirty` field to the node
    system.
    - Add check of node type for deleting nodes.
    - Possibly check if there is more than one output node.

Low Priority
------------
- Add match sorting/filtering for combining multiple match data files.
- Add node system editor depth sorting on nodes.
- Add switch for a full static build for release mode.
- Add WebSocket support to replace polling.
    - curl does not have WebSocket support yet.
    - WinHTTP
        - Windows only.
    - libwebsockets
        - Kinda big.
- Improve JSON parsing errors.
    - Already includes character position.
    - Can include message.
- Make sure bot handles case of match not ending.
    - Not sure how the API responses look in this case.
    - I think this is already working.


Presentation
============
- Audio.

Visuals
-------
- Glass balls (see `visuals.png`) are lowered into a pit (or pits) during betting phase, and are lifted when bets are locked.
- Glass balls fall and shatter into physics-activated shards.
- Show matchup details (see `matchup.png`) on a virtual screen.
- Salty Bet livestream visible.
