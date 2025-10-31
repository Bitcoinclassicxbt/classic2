// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_PRIMITIVES_BLOCK_H
#define BITCOIN_PRIMITIVES_BLOCK_H

#include "primitives/pureheader.h"
#include "primitives/transaction.h"
#include "auxpow.h"
#include "serialize.h"
#include "uint256.h"

#include <ios>
#include <memory>

/** Nodes collect new transactions into a block, hash them into a hash tree,
 * and scan through nonce values to make the block's hash satisfy proof-of-work
 * requirements.  When they solve the proof-of-work, they broadcast the block
 * to everyone and the block is added to the block chain.  The first transaction
 * in the block is a special one that creates a new coin owned by the creator
 * of the block.
 */
class CBlockHeader : public CPureBlockHeader
{
public:
    // [AUXPOW PORT] Optional AuxPoW payload for merge-mined blocks
    std::shared_ptr<CAuxPow> auxpow;

    CBlockHeader()
    {
        SetNull();
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(static_cast<CPureBlockHeader&>(*this));

        // When calculating the hash (SER_GETHASH), we only serialize the pure header
        // and exclude the auxpow data. The block hash is always calculated from just
        // the 80-byte header (nVersion, hashPrevBlock, hashMerkleRoot, nTime, nBits, nNonce).
        if (nType & SER_GETHASH) {
            return;
        }

        const int chainId = GetChainId();
        const bool expectAuxpow = IsAuxpow() && chainId > 0 && chainId < 0x0100;

        if (expectAuxpow) {
            if (ser_action.ForRead()) {
                auxpow = std::make_shared<CAuxPow>();
            }
            // Only enforce auxpow presence during READ operations (deserialization).
            // During WRITE operations (serialization), allow null auxpow to support
            // test cases that need to create invalid blocks for validation testing.
            if (ser_action.ForRead() && !auxpow) {
                throw std::ios_base::failure("AuxPow flag set but payload missing");
            }
            if (auxpow) {
                READWRITE(*auxpow);
            }
        } else if (ser_action.ForRead()) {
            auxpow.reset();
        }
    }

    void SetNull()
    {
        CPureBlockHeader::SetNull();
        auxpow.reset();
    }

    bool IsNull() const
    {
        return CPureBlockHeader::IsNull();
    }

    uint256 GetHash() const;

    void SetAuxpow(std::unique_ptr<CAuxPow> apow);
};


class CBlock : public CBlockHeader
{
public:
    // network and disk
    std::vector<CTransaction> vtx;

    // memory only
    mutable bool fChecked;

    CBlock()
    {
        SetNull();
    }

    CBlock(const CBlockHeader &header)
    {
        SetNull();
        *((CBlockHeader*)this) = header;
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(*(CBlockHeader*)this);
        READWRITE(vtx);
    }

    void SetNull()
    {
        CBlockHeader::SetNull();
        vtx.clear();
        fChecked = false;
    }

    CBlockHeader GetBlockHeader() const
    {
        CBlockHeader block;
        static_cast<CPureBlockHeader&>(block) = static_cast<const CPureBlockHeader&>(*this);
        block.auxpow = auxpow;
        return block;
    }

    std::string ToString() const;
};

/** Describes a place in the block chain to another node such that if the
 * other node doesn't have the same branch, it can find a recent common trunk.
 * The further back it is, the further before the fork it may be.
 */
struct CBlockLocator
{
    std::vector<uint256> vHave;

    CBlockLocator() {}

    CBlockLocator(const std::vector<uint256>& vHaveIn)
    {
        vHave = vHaveIn;
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        if (!(nType & SER_GETHASH))
            READWRITE(nVersion);
        READWRITE(vHave);
    }

    void SetNull()
    {
        vHave.clear();
    }

    bool IsNull() const
    {
        return vHave.empty();
    }
};

/** Compute the consensus-critical block weight (see BIP 141). */
int64_t GetBlockWeight(const CBlock& tx);

#endif // BITCOIN_PRIMITIVES_BLOCK_H
