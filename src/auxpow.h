// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Copyright (c) 2014-2019 Daniel Kraft
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CLASSIC2_AUXPOW_H
#define CLASSIC2_AUXPOW_H

#include "consensus/params.h"
#include "primitives/pureheader.h"
#include "primitives/transaction.h"
#include "serialize.h"
#include "uint256.h"

#include <memory>
#include <vector>

class CBlock;
class CBlockHeader;
class CBlockIndex;
class CValidationState;
class UniValue;

// Forward declaration for test friend class
namespace auxpow_tests {
    class CAuxPowForTest;
}

/** Header identifying merge-mining data inside coinbase scripts. */
static const unsigned char pchMergedMiningHeader[] = {0xfa, 0xbe, 'm', 'm'};

/**
 * [AUXPOW PORT] Merge-mining auxiliary proof-of-work payload.
 * Stores the parent coinbase transaction, merkle branches, and the
 * parent header which performed the actual PoW.
 */
class CAuxPow
{
    // Friend declaration for test helper class (in auxpow_tests namespace)
    friend class auxpow_tests::CAuxPowForTest;

private:
    CTransaction coinbaseTx;
    std::vector<uint256> vMerkleBranch;
    std::vector<uint256> vChainMerkleBranch;
    int nChainIndex;
    CPureBlockHeader parentBlock;

    static uint256 CheckMerkleBranch(uint256 hash,
                                     const std::vector<uint256>& vMerkleBranch,
                                     int nIndex);

public:
  /* Prevent accidental conversion.  */
  inline explicit CAuxPow (CTransaction&& txIn)
    : coinbaseTx (std::move (txIn))
  {}

  CAuxPow () = default;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        uint256 hashBlock;
        int nIndex = 0;

        if (!ser_action.ForRead()) {
            hashBlock.SetNull();
        }

        READWRITE(coinbaseTx);
        READWRITE(hashBlock);
        READWRITE(vMerkleBranch);
        READWRITE(nIndex);
        READWRITE(vChainMerkleBranch);
        READWRITE(nChainIndex);
        READWRITE(parentBlock);
    }

    bool check(const uint256& hashAuxBlock, int nChainId,
               const Consensus::Params& params) const;

    uint256 getParentBlockHash() const
    {
        return parentBlock.GetHash();
    }

    const CPureBlockHeader& getParentBlock() const
    {
        return parentBlock;
    }

    static int getExpectedIndex(uint32_t nNonce, int nChainId, unsigned h);

    static std::unique_ptr<CAuxPow> createAuxPow (const CPureBlockHeader& header);

  /**
   * Initialises the auxpow of the given block header.  This builds a minimal
   * auxpow object like createAuxPow and sets it on the block header.  Returns
   * a reference to the parent header so it can be mined as a follow-up.
   */
  static CPureBlockHeader& initAuxPow (CBlockHeader& header);

    /**
     * Accessors for testing and RPC.
     */
    const CTransaction& getCoinbaseTransaction() const { return coinbaseTx; }
    const std::vector<uint256>& getMerkleBranch() const { return vMerkleBranch; }
    const std::vector<uint256>& getChainMerkleBranch() const { return vChainMerkleBranch; }
    int getChainIndex() const { return nChainIndex; }
};

#endif // CLASSIC2_AUXPOW_H
