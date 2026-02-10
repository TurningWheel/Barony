# HELO-Only Chunking Plan (Stable 16-Player Savegame Join Handshake)

## Summary
This plan introduces **HELO-only chunking** for oversized lobby join snapshots while preserving existing behavior for all other packets.

The goal is to eliminate oversized single-packet HELO joins (especially savegame joins with `MAXPLAYERS=16`) and reduce join fragility from UDP fragmentation.

Target repo: `/Users/sayhiben/dev/Barony-8p`

Primary touched files:
1. `/Users/sayhiben/dev/Barony-8p/src/ui/MainMenu.cpp`
2. `/Users/sayhiben/dev/Barony-8p/src/net.cpp`
3. `/Users/sayhiben/dev/Barony-8p/src/net.hpp`

## Scope
1. Chunk only server->client HELO success payloads when large.
2. Keep existing `HELO` (single-packet) path intact for small payloads and fallback.
3. Keep existing failure/error HELO behavior unchanged.
4. Do not modify non-HELO protocols (`JOIN`, `REDY`, gameplay packets, etc.) except adding one `JOIN` capability byte.

## Non-Goals
1. No generic packet chunking framework in this iteration.
2. No changes to gameplay packet cadence or entity sync protocol.
3. No migration of existing non-HELO chunk flows (`CSCN`, `CMPD`) to shared utilities.

## Public Interface / Protocol Changes
1. `JOIN` request adds one capability byte.
2. New packet ID for HELO chunks: `'HLCN'`.
3. `lobbyPlayerJoinRequest` signature extends to expose whether HELO chunking should be used for this accepted join.

### 1) JOIN capability extension
- Current `JOIN` length: `69`.
- New `JOIN` length: `70`.
- Byte `69` = capability bitfield.
- Bit `0` (`0x01`) means: `client supports HLCN HELO chunk reassembly`.

### 2) New packet format (`'HLCN'`)
Header layout:
1. `[0..3]` `Uint32`: `'HLCN'`
2. `[4..5]` `Uint16`: `transferId`
3. `[6]` `Uint8`: `chunkIndex` (0-based)
4. `[7]` `Uint8`: `chunkCount`
5. `[8..9]` `Uint16`: `totalHeloLen` (full reassembled HELO payload length)
6. `[10..11]` `Uint16`: `chunkLen`
7. `[12..]` bytes: `chunk payload`

Constraints:
1. `chunkCount >= 1`
2. `chunkIndex < chunkCount`
3. `totalHeloLen <= NET_PACKET_SIZE`
4. `12 + chunkLen == net_packet->len`

### 3) Function signature update
Update declaration and definition:
- From:
  - `NetworkingLobbyJoinRequestResult lobbyPlayerJoinRequest(int& outResult, bool lockedSlots[MAXPLAYERS]);`
- To:
  - `NetworkingLobbyJoinRequestResult lobbyPlayerJoinRequest(int& outResult, bool lockedSlots[MAXPLAYERS], bool& outUseChunkedHelo);`

## Design Decisions (Risk-Managed)
1. **Capability-gated chunking**
   - Server only sends chunked HELO when client advertises support in `JOIN` byte 69 bit 0.
   - If client does not advertise support, server sends legacy HELO.
2. **Chunking threshold**
   - Use chunking when HELO payload length exceeds conservative non-fragment target.
   - Default: `kHeloSinglePacketMax = 1100` bytes.
3. **Transport reliability**
   - For chunked HELO sends, use `sendPacketSafe(...)` for both direct and P2P success paths.
   - This leverages existing `SAFE/GOTP` ack path and retries.
4. **Transfer identity**
   - Include `transferId` to prevent mixing stale chunks across retries/rejoins.
5. **Strict validation at client**
   - Reject malformed chunks immediately.
   - Do not partially apply invalid snapshot.
6. **No behavior change for non-chunk HELO**
   - Preserve existing flow for compatibility and low regression risk.

## Detailed Implementation Plan

### A) Add constants and helper data structures
In `/Users/sayhiben/dev/Barony-8p/src/ui/MainMenu.cpp` and `/Users/sayhiben/dev/Barony-8p/src/net.cpp`:

1. Add shared constants (local static in each translation unit if needed):
   - `kJoinCapabilityHeloChunkV1 = 0x01`
   - `kHeloChunkHeaderSize = 12`
   - `kHeloChunkPayloadMax = 900`
   - `kHeloSinglePacketMax = 1100`
   - `kHeloChunkReassemblyTimeoutTicks = 5 * TICKS_PER_SECOND`
2. Add a per-player transfer counter on server side:
   - `static Uint16 g_heloTransferId[MAXPLAYERS] = {0};`

### B) Update JOIN request writer (client)
In `/Users/sayhiben/dev/Barony-8p/src/ui/MainMenu.cpp` `sendJoinRequest()`:

1. Set `net_packet->data[69] = kJoinCapabilityHeloChunkV1`.
2. Change `net_packet->len = 70`.

Code example:

```cpp
memcpy(net_packet->data, "JOIN", 4);
// existing fields...
SDLNet_Write32(loadinglobbykey, &net_packet->data[65]);
net_packet->data[69] = kJoinCapabilityHeloChunkV1;
net_packet->len = 70;
```

### C) Update JOIN request parser / decision point (server)
In `/Users/sayhiben/dev/Barony-8p/src/net.cpp` `lobbyPlayerJoinRequest(...)`:

1. Read client capability:
   - `bool clientSupportsHeloChunk = (net_packet->len >= 70) && (net_packet->data[69] & kJoinCapabilityHeloChunkV1);`
2. Build HELO payload exactly as today (legacy bytes unchanged).
3. Compute `outUseChunkedHelo`:
   - `outUseChunkedHelo = clientSupportsHeloChunk && loadingsavegame && (net_packet->len > kHeloSinglePacketMax);`
4. For direct-connect success path:
   - If `outUseChunkedHelo` true, send chunked HELO directly from here.
   - Else keep existing single `sendPacketSafe`.

### D) Add server-side HELO chunk sender helper
Implement helper in `/Users/sayhiben/dev/Barony-8p/src/net.cpp` (or `MainMenu.cpp` static if preferred):

1. Input:
   - `hostnum`, destination address, source HELO bytes, source HELO length, `transferId`.
2. Split HELO payload into chunks of `kHeloChunkPayloadMax`.
3. For each chunk:
   - Write `'HLCN'` header.
   - Copy chunk bytes.
   - Set `net_packet->len = kHeloChunkHeaderSize + chunkLen`.
   - Send with `sendPacketSafe(...)`.

Code example:

```cpp
static void sendChunkedHeloToHost(int hostnum, const Uint8* heloData, int heloLen, Uint16 transferId)
{
    const int chunkCount = (heloLen + kHeloChunkPayloadMax - 1) / kHeloChunkPayloadMax;
    for (int chunkIndex = 0; chunkIndex < chunkCount; ++chunkIndex)
    {
        const int offset = chunkIndex * kHeloChunkPayloadMax;
        const int chunkLen = std::min(kHeloChunkPayloadMax, heloLen - offset);

        memcpy(net_packet->data, "HLCN", 4);
        SDLNet_Write16(transferId, &net_packet->data[4]);
        net_packet->data[6] = static_cast<Uint8>(chunkIndex);
        net_packet->data[7] = static_cast<Uint8>(chunkCount);
        SDLNet_Write16(static_cast<Uint16>(heloLen), &net_packet->data[8]);
        SDLNet_Write16(static_cast<Uint16>(chunkLen), &net_packet->data[10]);
        memcpy(&net_packet->data[12], heloData + offset, chunkLen);
        net_packet->len = 12 + chunkLen;

        sendPacketSafe(net_sock, -1, net_packet, hostnum);
    }
}
```

### E) P2P success-path changes in MainMenu server join handling
In `/Users/sayhiben/dev/Barony-8p/src/ui/MainMenu.cpp` where `lobbyPlayerJoinRequest(...)` result is processed:

1. Capture `outUseChunkedHelo`.
2. Keep existing P2P failure behavior unchanged.
3. On P2P success:
   - Keep current remote-ID assignment (`steamIDRemote`/EOS peer index).
   - If `outUseChunkedHelo` true:
     - Copy current HELO payload to local temp buffer.
     - Increment/use `transferId` for this player.
     - Send chunked HELO via `sendPacketSafe` helper using `hostnum = playerNum - 1`.
   - Else:
     - Keep current legacy single-HELO send behavior.

### F) Client-side HLCN reassembly (pre-clientnum phase)
In `/Users/sayhiben/dev/Barony-8p/src/ui/MainMenu.cpp` `handlePacketsAsClient()` branch `receivedclientnum == false`:

1. Add local static reassembly state:
   - `active`, `transferId`, `chunkCount`, `totalLen`, `lastChunkTick`.
   - `std::vector<std::vector<Uint8>> chunks`.
   - `std::vector<bool> received`.
2. When packet is `'HELO'`:
   - Clear chunk state.
   - Continue existing legacy parse flow unchanged.
3. When packet is `'HLCN'`:
   - Validate header and bounds.
   - Start or switch reassembly session by `transferId`.
   - Store chunk only if first time received for that index.
   - Update `lastChunkTick`.
   - If all chunks received:
     - Reassemble to contiguous buffer.
     - Validate reconstructed packet begins with `'HELO'`.
     - Copy reconstructed bytes into `net_packet->data`, set `net_packet->len`.
     - Set `gotPacket = true` to reuse existing HELO parser.
4. Timeout:
   - If active and stale (`ticks - lastChunkTick > kHeloChunkReassemblyTimeoutTicks`), reset state.

Code example:

```cpp
if (packetId == 'HELO')
{
    resetHeloChunkState();
    gotPacket = true;
}
else if (packetId == 'HLCN')
{
    if (ingestHeloChunkAndMaybeAssemble(*net_packet))
    {
        // ingest function overwrote net_packet with full HELO bytes
        gotPacket = true;
    }
}
```

### G) Validation / guardrails
Add hard checks:

1. Reject chunk packets where `totalHeloLen > NET_PACKET_SIZE`.
2. Reject chunk packets where `chunkLen` exceeds per-chunk max or packet length mismatch.
3. Reject chunk packets where `chunkCount == 0` or `chunkCount > 32`.
4. Reject out-of-range `chunkIndex`.
5. On any malformed chunk, reset active state and continue waiting (do not crash, do not partially apply).

### H) Logging (stability-first)
Add `printlog` lines (debug-friendly, concise):

1. Server:
   - `sending chunked HELO: player=%d transfer=%u chunks=%d total=%d`
2. Client:
   - `recv HLCN: transfer=%u idx=%u/%u len=%u`
   - `HELO reassembled: transfer=%u total=%u`
   - `HELO chunk timeout/reset transfer=%u`

## Test Plan

### 1) Functional matrix
1. Direct connect, non-savegame join:
   - Expect legacy single HELO path unchanged.
2. Direct connect, savegame join, 16 players:
   - Expect chunked HELO path when client advertises capability.
3. Steam join, non-savegame:
   - Legacy path unchanged.
4. Steam join, savegame 16 players:
   - Chunked HELO path; successful join.
5. EOS join, savegame 16 players:
   - Chunked HELO path; successful join.
6. Legacy-capability client simulation (no capability bit):
   - Server sends legacy HELO fallback.
7. Error HELO path (`MAXPLAYERS + n` codes):
   - Ensure unchanged behavior and prompts.

### 2) Robustness scenarios
1. Inject duplicate HLCN chunks:
   - Reassembler ignores duplicates.
2. Out-of-order chunk arrival:
   - Reassembler completes successfully.
3. Missing chunk:
   - No malformed parse; timeout reset occurs; overall connect flow times out gracefully.
4. Mixed/stale transfer IDs:
   - Old transfer chunks ignored/reset correctly.
5. Malformed header fields:
   - Packet safely rejected; no crash.

### 3) Acceptance criteria
1. No HELO handshake packet exceeding chunk threshold is sent when chunking is enabled.
2. Client can reassemble and parse chunked HELO deterministically.
3. Legacy HELO behavior remains unchanged for small payloads and no-capability clients.
4. No regressions in `JOIN`, `REDY`, `DISC`, `SVFL`, or lobby prompt flow.
5. Build success for affected targets.

## Rollout / Verification Instructions
1. Implement in small commits:
   - Commit 1: protocol constants + JOIN capability byte.
   - Commit 2: server chunk sender + decision logic.
   - Commit 3: client reassembly + timeout + logs.
   - Commit 4: cleanup and guard assertions.
2. Run build:
   - `cmake --build /Users/sayhiben/dev/Barony-8p/build-mac -j8`
3. Manual smoke tests:
   - Host/join for direct + Steam + EOS in both savegame and non-savegame.
4. Keep logs enabled during first multiplayer verification pass.
5. If instability appears, temporarily force legacy HELO by setting chunk decision false server-side while keeping receiver code in place.

## Assumptions and Defaults
1. This is a **HELO-only** chunking change.
2. Capability negotiation is via `JOIN` byte 69 bit 0.
3. Chunking only triggers for large HELO (default threshold `1100` bytes).
4. Chunk transport uses existing SAFE/GOTP reliability path.
5. Existing legacy parser remains source of truth for final HELO interpretation.
6. No version string bump is required for this patch; capability bit handles handshake behavior selection.
