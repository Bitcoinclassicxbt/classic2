// Copyright (c) 2025 XBT Foundation
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "rpc/auxpow_miner.h"

#include "arith_uint256.h"
#include "auxpow.h"
#include "chainparams.h"
#include "main.h"
#include "consensus/validation.h"
#include "net.h"
#include "pow.h"
#include "rpc/protocol.h"
#include "script/standard.h"
#include "serialize.h"
#include "txmempool.h"
#include "uint256.h"
#include "util.h"
#include "utilstrencodings.h"

#include <memory>

namespace
{

void CheckHeightForAuxpow()
{
    LOCK(cs_main);
    const Consensus::Params& consensus = Params().GetConsensus();
    if (chainActive.Height() + 1 < consensus.nAuxpowStartHeight) {
        throw JSONRPCError(RPC_INVALID_PARAMETER,
                           "Auxpow is not yet available at this height");
    }
}

} // namespace

void CheckAuxMiningAllowed()
{
    if (!Params().MineBlocksOnDemand()) {
        if (vNodes.empty()) {
            throw JSONRPCError(RPC_CLIENT_NOT_CONNECTED,
                               "Bitcoin is not connected!");
        }
        if (IsInitialBlockDownload()) {
            throw JSONRPCError(RPC_CLIENT_IN_INITIAL_DOWNLOAD,
                               "Bitcoin is downloading blocks...");
        }
    }

    CheckHeightForAuxpow();
}

const CBlock*
AuxpowMiner::getCurrentBlock(const CScript& scriptPubKey, uint256& target)
{
    AssertLockHeld(cs);

    const CBlock* pblockCur = nullptr;
    const CScriptID scriptID(scriptPubKey);

    auto it = curBlocks.find(scriptID);
    if (it != curBlocks.end()) {
        pblockCur = it->second;
    }

    const CBlockIndex* tip = nullptr;
    {
        LOCK(cs_main);
        tip = chainActive.Tip();
    }

    if (pblockCur == nullptr
        || pindexPrev != tip
        || (mempool.GetTransactionsUpdated() != txUpdatedLast
            && GetTime() - startTime > 60)) {

        if (pindexPrev != tip) {
            blocks.clear();
            templates.clear();
            curBlocks.clear();
            extraNonce = 0;
        }

        std::unique_ptr<CBlockTemplate> newBlock(
            BlockAssembler(Params()).CreateNewBlock(scriptPubKey));
        if (!newBlock) {
            throw JSONRPCError(RPC_OUT_OF_MEMORY, "Out of memory");
        }

        {
            LOCK(cs_main);
            tip = chainActive.Tip();
            if (!tip) {
                throw JSONRPCError(RPC_INTERNAL_ERROR, "No active chain tip");
            }
            IncrementExtraNonce(&newBlock->block, tip, extraNonce);
        }

        txUpdatedLast = mempool.GetTransactionsUpdated();
        pindexPrev = tip;
        startTime = GetTime();

        pblockCur = &newBlock->block;
        curBlocks.emplace(scriptID, pblockCur);
        blocks.emplace(pblockCur->GetHash(), pblockCur);
        templates.push_back(std::move(newBlock));
    }

    if (!pblockCur->IsAuxpow() || !pblockCur->auxpow)
    {
        CBlock* mutableBlock = const_cast<CBlock*>(pblockCur);
        mutableBlock->SetAuxpowVersion(true);
        CAuxPow::initAuxPow(*mutableBlock);
    }

    arith_uint256 arithTarget;
    bool fNegative = false;
    bool fOverflow = false;
    arithTarget.SetCompact(pblockCur->nBits, &fNegative, &fOverflow);
    if (fNegative || fOverflow || arithTarget == 0) {
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Invalid difficulty bits in block");
    }
    target = ArithToUint256(arithTarget);

    return pblockCur;
}

const CBlock*
AuxpowMiner::lookupSavedBlock(const std::string& hashHex) const
{
    AssertLockHeld(cs);

    uint256 hash;
    hash.SetHex(hashHex);

    const auto iter = blocks.find(hash);
    if (iter == blocks.end()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Block hash unknown");
    }

    return iter->second;
}

UniValue
AuxpowMiner::createAuxBlock(const CScript& scriptPubKey)
{
    LOCK(cs);

    uint256 target;
    const CBlock* pblock = getCurrentBlock(scriptPubKey, target);

    if (!pblock->IsAuxpow() || !pblock->auxpow)
        throw JSONRPCError(RPC_INTERNAL_ERROR, "auxpow data missing from constructed block");

    if (pblock->vtx.empty() || pblock->vtx[0].vout.empty())
        throw JSONRPCError(RPC_INTERNAL_ERROR, "constructed block has invalid coinbase");

    UniValue result(UniValue::VOBJ);
    result.pushKV("hash", pblock->GetHash().GetHex());
    result.pushKV("chainid", pblock->GetChainId());
    result.pushKV("previousblockhash", pblock->hashPrevBlock.GetHex());
    result.pushKV("coinbasevalue",
                  static_cast<int64_t>(pblock->vtx[0].vout[0].nValue));
    result.pushKV("bits", strprintf("%08x", pblock->nBits));

    const int64_t height = pindexPrev ? pindexPrev->nHeight + 1 : -1;
    result.pushKV("height", height);
    result.pushKV("_target", HexStr(target));

    return result;
}

bool
AuxpowMiner::submitAuxBlock(const std::string& hashHex,
                            const std::string& auxpowHex)
{
    CheckHeightForAuxpow();

    std::shared_ptr<CBlock> sharedBlock;
    {
        LOCK(cs);
        const CBlock* pblock = lookupSavedBlock(hashHex);
        sharedBlock = std::make_shared<CBlock>(*pblock);
    }

    const std::vector<unsigned char> vchAuxPow = ParseHex(auxpowHex);
    if (vchAuxPow.empty())
        throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "invalid auxpow data");
    CDataStream ss(vchAuxPow, SER_NETWORK, PROTOCOL_VERSION);
    std::unique_ptr<CAuxPow> pow(new CAuxPow());
    ss >> *pow;
    sharedBlock->SetAuxpow(std::move(pow));
    assert(sharedBlock->GetHash().GetHex() == hashHex);

    CValidationState state;
    return ProcessNewBlock(state, Params(), NULL, sharedBlock.get(), true, NULL, false);
}

AuxpowMiner&
AuxpowMiner::get()
{
    static AuxpowMiner instance;
    return instance;
}
