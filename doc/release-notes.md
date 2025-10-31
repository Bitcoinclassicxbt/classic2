Classic2 version 0.14.0 Release Notes
=====================================

Classic2 0.14.0 is a feature release that delivers merge-mining (AuxPoW)
support, updated consensus parameters, and operational improvements for miners
and node operators. It is based on the Bitcoin Core 0.14 codebase with
Classic2-specific consensus rules and tooling.

Please report bugs using the Classic2 issue tracker:

  <https://github.com/Bitcoinclassicxbt/classic2/issues>

Highlights
----------

- **AuxPoW activation**: Merge-mining activates on mainnet at height `143000`
  with chain ID `0x0005`. Nodes enforce AuxPoW structure and validation rules,
  while allowing legacy headers indefinitely for historical compatibility.
- **Miner workflow updates**: `createauxblock` / `submitauxblock` RPCs now
  export fully initialised AuxPoW templates. `getauxpowinfo` reports activation
  parameters, tip status, and recent AuxPoW blocks for operational visibility.
- **Consensus safeguards**: `ContextualCheckBlockHeader()` enforces AuxPoW
  signalling rules, chain ID matching, and optional legacy grace-period limits.
  `CheckProofOfWork()` validates AuxPoW linkage and parent work.
- **Block assembly**: `BlockAssembler` automatically initialises AuxPoW payloads
  when constructing templates past activation height.
- **RPC ergonomics**: Mining-related RPCs expose AuxPoW metadata and enforce
  activation height checks to prevent premature submissions.

Activation Parameters
---------------------

- `nAuxpowStartHeight = 143000`
- `nAuxpowChainId = 0x0005`
- `fStrictChainId = false`
- `nLegacyBlocksBeforeAuxpow = -1`

New RPCs
--------

- `getauxpowinfo`: Summarises activation parameters, current tip status,
  and up to five recent AuxPoW blocks including parent header data.
- `createauxblock` / `submitauxblock`: Provide merge-mining work templates and
  accept completed AuxPoW payloads. Integrated with `AuxpowMiner` helper for
  multi-script management.

Compatibility Notes
-------------------

- Nodes must upgrade prior to height `143000` to remain in consensus.
- Merge miners should update their tooling to consume the new RPCs and supply
  valid AuxPoW payloads before submission.
- Legacy mining without AuxPoW signalling remains accepted indefinitely due to
  `nLegacyBlocksBeforeAuxpow = -1`; operators may tighten this locally if
  desired via policy.

Developer Notes
---------------

- The AuxPoW implementation introduces `auxpow.cpp`, `auxpow.h`, and supporting
  primitives in `primitives/pureheader.*`.
- `pow.cpp` and `main.cpp` contain the consensus enforcement logic.
- Mining RPC helpers are available in `rpc/auxpow_miner.*` and integrated via
  `rpc/mining.cpp`.

Acknowledgements
----------------

Thanks to the Classic2 contributors who implemented and reviewed AuxPoW
support, RPC integrations, and documentation updates.
