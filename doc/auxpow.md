# AuxPoW Integration in Classic2

This document summarizes the merge-mining (AuxPoW) support that has been
ported from Namecoin into Classic2. It covers activation parameters,
expected block semantics, miner responsibilities, and the RPC interfaces
exposed for monitoring merge-mining status.

## Activation Parameters

- **`nAuxpowStartHeight`**: AuxPoW becomes mandatory at block height `143000`.
- **`nAuxpowChainId`**: All AuxPoW headers must signal chain ID `0x0005`.
- **`fStrictChainId`**: Enforced after activation; AuxPoW blocks with the wrong
  chain ID are rejected.
- **`nLegacyBlocksBeforeAuxpow`**: (Consensus) permits legacy PoW blocks to
  continue for the configured grace period after activation (use `-1` for no
  limit, or `0` for immediate enforcement). Classic2 currently permits legacy
  headers indefinitely to preserve historical compatibility.

Prior to height `143000`, legacy (non-AuxPoW) blocks remain valid and any block
attempting to signal AuxPoW is rejected. After activation, AuxPoW blocks must
carry a populated AuxPoW payload and parent mining header, while legacy headers
are only accepted within the configured grace period.

## Block Construction Notes

- `BlockAssembler::CreateNewBlock()` automatically sets the AuxPoW version bit
  and seeds a parent header once the chain height is past activation.
- Callers mining directly on Classic2 must continue to solve the child header
  nonce as usual. Merge miners should mine on the exported parent header
  recorded in `CBlockTemplate::auxMiningHeader`.
- `CAuxPow::initAuxPow()` is used to initialize a minimal AuxPoW structure. The
  AuxPoW payload is expected to be replaced with the real merge-mined proof
  before submission.

## Miner Workflow

1. Fetch a block template (e.g., via `getblocktemplate` or
   `createauxblock`).
2. If using merge mining, build the parent chain coinbase, embed the child hash
   per AuxPoW rules, and mine the parent header.
3. Attach the completed AuxPoW object to the Classic2 block header before
   submission (e.g., via `submitauxblock` or `submitblock`).

## RPC Interfaces

- **`getauxpowinfo`**: Returns the activation parameters, current tip status,
  and up to five of the most recent AuxPoW blocks (including serialized parent
  headers and coinbase data).
- **`createauxblock` / `submitauxblock`**: (Mining RPCs) Create and submit AuxPoW
  work units. `createauxblock` exposes the child hash and chain ID; miners must
  provide the AuxPoW payload back through `submitauxblock`.

Example call:

```bash
classic2-cli getauxpowinfo
```

Example result (abridged):

```json
{
  "startheight": 143000,
  "chainid": 113,
  "strictchainid": true,
  "legacyblocks": -1,
  "currentheight": 143012,
  "activated": true,
  "tip": {
    "height": 143012,
    "hash": "...",
    "version": 1074790401,
    "is_auxpow": true,
    "auxpow": {
      "coinbase": { "hash": "..." },
      "chainindex": 5,
      "merklebranch": ["..."],
      "chainmerklebranch": ["..."],
      "parenthash": "..."
    }
  },
  "recent_auxpow_blocks": [ ... ]
}
```

## Testing Considerations

- Functional tests should verify that AuxPoW blocks are rejected before height
  `143000` and accepted afterward.
- Regression tests should cover the grace-period behavior controlled by
  `nLegacyBlocksBeforeAuxpow`.
- Miner-focused tests should ensure that the exported parent header matches the
  AuxPoW payload accepted by consensus.
