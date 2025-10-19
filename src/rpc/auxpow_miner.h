// Copyright (c) 2025 XBT Foundation
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_RPC_AUXPOW_MINER_H
#define BITCOIN_RPC_AUXPOW_MINER_H

#include "miner.h"
#include "script/standard.h"
#include "sync.h"
#include "uint256.h"

#include <univalue.h>

#include <map>
#include <memory>

class CBlock;
class CScript;

/**
 * Singleton helper that manages AuxPoW block templates for merge-mining
 * RPCs.
 */
class AuxpowMiner
{
private:
    mutable CCriticalSection cs;

    /** Active block templates we constructed for auxpow miners. */
    std::vector<std::unique_ptr<CBlockTemplate>> templates;
    /** Maps block hashes to the corresponding block pointer in templates. */
    std::map<uint256, const CBlock*> blocks;
    /** Maps coinbase script hashes to blocks we handed out. */
    std::map<CScriptID, const CBlock*> curBlocks;

    unsigned int extraNonce = 0;
    unsigned int txUpdatedLast = 0;
    const CBlockIndex* pindexPrev = nullptr;
    int64_t startTime = 0;

    const CBlock* getCurrentBlock(const CScript& scriptPubKey, uint256& target);
    const CBlock* lookupSavedBlock(const std::string& hashHex) const;

public:
    AuxpowMiner() = default;

    UniValue createAuxBlock(const CScript& scriptPubKey);
    bool submitAuxBlock(const std::string& hashHex, const std::string& auxpowHex);

    static AuxpowMiner& get();
};

/** Throw if merge-mining RPCs are not currently allowed. */
void CheckAuxMiningAllowed();

#endif // BITCOIN_RPC_AUXPOW_MINER_H
