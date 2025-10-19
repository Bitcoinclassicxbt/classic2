// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_PRIMITIVES_PUREHEADER_H
#define BITCOIN_PRIMITIVES_PUREHEADER_H

#include "serialize.h"
#include "uint256.h"

#include <assert.h>

/**
 * [AUXPOW PORT] Pure block header without AuxPoW data.
 * Used to break circular dependency between the merge-mined parent header
 * and the child header containing the AuxPoW payload.
 */
class CPureBlockHeader
{
private:
    static const int32_t VERSION_AUXPOW = (1 << 8);
    static const int32_t VERSION_CHAIN_START = (1 << 16);

public:
    int32_t nVersion;
    uint256 hashPrevBlock;
    uint256 hashMerkleRoot;
    uint32_t nTime;
    uint32_t nBits;
    uint32_t nNonce;

    CPureBlockHeader() { SetNull(); }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nSerVersion) {
        READWRITE(this->nVersion);
        READWRITE(hashPrevBlock);
        READWRITE(hashMerkleRoot);
        READWRITE(nTime);
        READWRITE(nBits);
        READWRITE(nNonce);
    }

    void SetNull()
    {
        nVersion = 0;
        hashPrevBlock.SetNull();
        hashMerkleRoot.SetNull();
        nTime = 0;
        nBits = 0;
        nNonce = 0;
    }

    bool IsNull() const
    {
        return (nBits == 0);
    }

    uint256 GetHash() const;

    int64_t GetBlockTime() const
    {
        return static_cast<int64_t>(nTime);
    }

    int32_t GetBaseVersion() const
    {
        return GetBaseVersion(nVersion);
    }

    static int32_t GetBaseVersion(int32_t ver)
    {
        return ver % VERSION_AUXPOW;
    }

    void SetBaseVersion(int32_t nBaseVersion, int32_t nChainId);

    int32_t GetChainId() const
    {
        return nVersion / VERSION_CHAIN_START;
    }

    void SetChainId(int32_t chainId)
    {
        nVersion %= VERSION_CHAIN_START;
        nVersion |= chainId * VERSION_CHAIN_START;
    }

    bool IsAuxpow() const
    {
        return (nVersion & VERSION_AUXPOW) != 0;
    }

    void SetAuxpowVersion(bool auxpow)
    {
        if (auxpow)
            nVersion |= VERSION_AUXPOW;
        else
            nVersion &= ~VERSION_AUXPOW;
    }

    bool IsLegacy() const
    {
        return nVersion == 1;
    }
};

#endif // BITCOIN_PRIMITIVES_PUREHEADER_H
