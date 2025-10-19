// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2011 Vince Durham
// Copyright (c) 2009-2014 The Bitcoin developers
// Copyright (c) 2014-2019 Daniel Kraft
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "auxpow.h"

#include "consensus/consensus.h"
#include "consensus/merkle.h"
#include "hash.h"
#include "primitives/block.h"
#include "script/script.h"
#include "utilstrencodings.h"
#include "util.h"

#include <algorithm>

namespace {

static uint32_t DecodeLE32(const unsigned char* bytes)
{
    uint32_t res = 0;
    for (int i = 0; i < 4; ++i) {
        res <<= 8;
        res |= bytes[3 - i];
    }
    return res;
}

} // namespace

bool CAuxPow::check(const uint256& hashAuxBlock, int nChainId,
                    const Consensus::Params& params) const
{
    if (params.fStrictChainId && parentBlock.GetChainId() == nChainId)
        return error("Aux POW parent has our chain ID");

    if (vChainMerkleBranch.size() > 30)
        return error("Aux POW chain merkle branch too long");

    const uint256 nRootHash = CheckMerkleBranch(hashAuxBlock, vChainMerkleBranch, nChainIndex);
    std::vector<unsigned char> vchRootHash(UBEGIN(nRootHash), UEND(nRootHash));
    std::reverse(vchRootHash.begin(), vchRootHash.end());

    if (CheckMerkleBranch(coinbaseTx.GetHash(), vMerkleBranch, 0)
            != parentBlock.hashMerkleRoot)
        return error("Aux POW merkle root incorrect");

    if (coinbaseTx.vin.empty())
        return error("Aux POW coinbase has no inputs");

    const CScript& script = coinbaseTx.vin[0].scriptSig;

    const unsigned char* mmHeaderBegin = pchMergedMiningHeader;
    const unsigned char* mmHeaderEnd = mmHeaderBegin + sizeof(pchMergedMiningHeader);
    CScript::const_iterator pcHead =
        std::search(script.begin(), script.end(), mmHeaderBegin, mmHeaderEnd);

    CScript::const_iterator pc =
        std::search(script.begin(), script.end(), vchRootHash.begin(), vchRootHash.end());

    if (pc == script.end())
        return error("Aux POW missing chain merkle root in parent coinbase");

    if (pcHead != script.end()) {
        if (script.end() != std::search(pcHead + 1, script.end(), mmHeaderBegin, mmHeaderEnd))
            return error("Multiple merged mining headers in coinbase");
        if (pcHead + sizeof(pchMergedMiningHeader) != pc)
            return error("Merged mining header is not just before chain merkle root");
    } else {
        if (pc - script.begin() > 20)
            return error("Aux POW chain merkle root must start in the first 20 bytes of the parent coinbase");
    }

    pc += vchRootHash.size();
    if (script.end() - pc < 8)
        return error("Aux POW missing chain merkle tree size and nonce in parent coinbase");

    const uint32_t nSize = DecodeLE32(&pc[0]);
    const unsigned merkleHeight = vChainMerkleBranch.size();
    if (nSize != (1u << merkleHeight))
        return error("Aux POW merkle branch size does not match parent coinbase");

    const uint32_t nNonce = DecodeLE32(&pc[4]);
    if (nChainIndex != getExpectedIndex(nNonce, nChainId, merkleHeight))
        return error("Aux POW wrong index");

    return true;
}

int CAuxPow::getExpectedIndex(uint32_t nNonce, int nChainId, unsigned h)
{
    uint32_t rand = nNonce;
    rand = rand * 1103515245 + 12345;
    rand += nChainId;
    rand = rand * 1103515245 + 12345;
    return rand % (1u << h);
}

uint256 CAuxPow::CheckMerkleBranch(uint256 hash,
                                   const std::vector<uint256>& branch,
                                   int nIndex)
{
    if (nIndex == -1)
        return uint256();
    for (const uint256& item : branch) {
        if (nIndex & 1)
            hash = Hash(BEGIN(item), END(item), BEGIN(hash), END(hash));
        else
            hash = Hash(BEGIN(hash), END(hash), BEGIN(item), END(item));
        nIndex >>= 1;
    }
    return hash;
}

std::unique_ptr<CAuxPow> CAuxPow::createAuxPow(const CPureBlockHeader& header)
{
    assert(header.IsAuxpow());

    const uint256 blockHash = header.GetHash();
    std::vector<unsigned char> inputData(blockHash.begin(), blockHash.end());
    std::reverse(inputData.begin(), inputData.end());
    inputData.push_back(1);
    inputData.insert(inputData.end(), 7, 0);

    CMutableTransaction coinbase;
    coinbase.vin.resize(1);
    coinbase.vin[0].prevout.SetNull();
    coinbase.vin[0].scriptSig = (CScript() << inputData);
    coinbase.vout.clear();

    CBlock parent;
    parent.nVersion = 1;
    parent.vtx.resize(1);
    parent.vtx[0] = coinbase;
    parent.hashMerkleRoot = BlockMerkleRoot(parent);

    std::unique_ptr<CAuxPow> auxpow(new CAuxPow());
    auxpow->coinbaseTx = CTransaction(coinbase);
    auxpow->vMerkleBranch.clear();
    auxpow->vChainMerkleBranch.clear();
    auxpow->nChainIndex = 0;
    auxpow->parentBlock = parent;

    return auxpow;
}

CPureBlockHeader& CAuxPow::initAuxPow(CBlockHeader& header)
{
    header.SetAuxpowVersion(true);

    std::unique_ptr<CAuxPow> apow = createAuxPow(header);
    CPureBlockHeader& parentRef = apow->parentBlock;
    header.SetAuxpow(std::move(apow));

    return parentRef;
}
