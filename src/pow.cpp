// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "pow.h"

#include "arith_uint256.h"
#include "auxpow.h"
#include "chain.h"
#include "hash.h"
#include "primitives/block.h"
#include "util.h"
#include "uint256.h"

unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& params)
{
    // Original algorithm for backward compatibility
    unsigned int nProofOfWorkLimit = UintToArith256(params.powLimit).GetCompact();
    unsigned int nProofOfWorkMin = UintToArith256(params.pownewlimit).GetCompact();
    //set diff for first block
    if (pindexLast->nHeight >= 122291 && pindexLast->nHeight <= 122310)
        return nProofOfWorkMin;
    // Switch between old and new difficulty algorithms based on height
    if (pindexLast && pindexLast->nHeight >= params.nNewPowDiffHeight)
        return GetNextWorkRequiredNew(pindexLast, pblock, params);

    // Genesis block
    if (pindexLast == NULL)
        return nProofOfWorkLimit;

    if (pindexLast->nHeight >= 112266 && pindexLast->nHeight <= 112300)
        return nProofOfWorkLimit;

    if (pindexLast->nHeight >= 112301 && pindexLast->nHeight <= 112401)
        return nProofOfWorkMin;

    // Only change once per difficulty adjustment interval
    if ((pindexLast->nHeight+1) % params.DifficultyAdjustmentInterval() != 0)
    {
        if (params.fPowAllowMinDifficultyBlocks)
        {
            // Special difficulty rule for testnet:
            // If the new block's timestamp is more than 2* 10 minutes
            // then allow mining of a min-difficulty block.
            if (pblock->GetBlockTime() > pindexLast->GetBlockTime() + params.nPowTargetSpacing*2)
                return nProofOfWorkLimit;
            else
            {
                // Return the last non-special-min-difficulty-rules-block
                const CBlockIndex* pindex = pindexLast;
                while (pindex->pprev && pindex->nHeight % params.DifficultyAdjustmentInterval() != 0 && pindex->nBits == nProofOfWorkLimit)
                    pindex = pindex->pprev;
                return pindex->nBits;
            }
        }
        return pindexLast->nBits;
    }

    // Go back by what we want to be 14 days worth of blocks
    int nHeightFirst = pindexLast->nHeight - (params.DifficultyAdjustmentInterval()-1);
    assert(nHeightFirst >= 0);
    const CBlockIndex* pindexFirst = pindexLast->GetAncestor(nHeightFirst);
    assert(pindexFirst);

    return CalculateNextWorkRequired(pindexLast, pindexFirst->GetBlockTime(), params);
}

unsigned int CalculateNextWorkRequired(const CBlockIndex* pindexLast, int64_t nFirstBlockTime, const Consensus::Params& params)
{
    if (params.fPowNoRetargeting)
        return pindexLast->nBits;

    // Limit adjustment step
    int64_t nActualTimespan = pindexLast->GetBlockTime() - nFirstBlockTime;
    if (nActualTimespan < params.nPowTargetTimespan/4)
        nActualTimespan = params.nPowTargetTimespan/4;
    if (nActualTimespan > params.nPowTargetTimespan*4)
        nActualTimespan = params.nPowTargetTimespan*4;

    // Retarget
    const arith_uint256 bnPowLimit = UintToArith256(params.powLimit);
    arith_uint256 bnNew;
    bnNew.SetCompact(pindexLast->nBits);
    bnNew *= nActualTimespan;
    bnNew /= params.nPowTargetTimespan;

    if (bnNew > bnPowLimit)
        bnNew = bnPowLimit;

    return bnNew.GetCompact();
}

unsigned int GetNextWorkRequiredNew(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& params)
{
    unsigned int nProofOfWorkLimit = UintToArith256(params.powLimit).GetCompact();
    unsigned int nProofOfWorkMin = UintToArith256(params.pownewlimit).GetCompact();
    unsigned int nProofOfWorkMax = UintToArith256(params.powmaxlimit).GetCompact();
    unsigned int nProofOfWorkMid = UintToArith256(params.powdinlimit).GetCompact();

    // Genesis block
    if (pindexLast == NULL)
    {
        return nProofOfWorkLimit;
    }

    // Regtest
    if (params.fPowNoRetargeting)
    {
        return pindexLast->nBits;
    }

    // Validate parameters to prevent division by zero
    if (params.nPowAveragingWindow <= 0 || params.nPostBlossomPowTargetSpacing <= 0)
    {
        return nProofOfWorkMin;
    }

    // Emergency difficulty rule: if block time is more than 6x target spacing, allow min difficulty
    /*if (pblock && pblock->GetBlockTime() > pindexLast->GetBlockTime() + params.nPostBlossomPowTargetSpacing * 6)
    {
        //return nProofOfWorkMin;
    }  //old emergency 
    if (pblock) {
        int64_t time_diff = pblock->GetBlockTime() - pindexLast->GetBlockTime();
        int64_t spacing = params.nPostBlossomPowTargetSpacing;
        
        arith_uint256 lastTarget;
        lastTarget.SetCompact(pindexLast->nBits);
        
        arith_uint256 maxTarget;
        maxTarget.SetCompact(nProofOfWorkMax);
        
        if (time_diff > spacing * 8) {
            // 8x delay: minimum difficulty
            return nProofOfWorkMax;
        } else if (time_diff > spacing * 6) {
            // 6x delay: 65% easier (35% of current difficulty)
            lastTarget = lastTarget * 100 / 35;
        } else if (time_diff > spacing * 3) {
            // 3x delay: 50% easier (50% of current difficulty)
            lastTarget = lastTarget * 100 / 50;
        } else {
            // Normal case: no emergency adjustment
            return pindexLast->nBits;
        }
        
        // Cap at minimum difficulty (maximum target)
        if (lastTarget > maxTarget) {
            return nProofOfWorkMax;
        }
        
        return lastTarget.GetCompact();
    }*/

    // Old emergency rule (before height 126800)
    if (pindexLast->nHeight < 126800 && pblock && 
        pblock->GetBlockTime() > pindexLast->GetBlockTime() + params.nPostBlossomPowTargetSpacing * 6) {
        return nProofOfWorkMin;
    }

    // New emergency rule (height 126800 and above)
    if (pindexLast->nHeight >= 127464 && pindexLast->nHeight <= 127927 && pblock) {
    int64_t time_diff = pblock->GetBlockTime() - pindexLast->GetBlockTime();
    int64_t spacing = params.nPostBlossomPowTargetSpacing;
    
    arith_uint256 lastTarget;
    lastTarget.SetCompact(pindexLast->nBits);
    
    arith_uint256 maxTarget;
    maxTarget.SetCompact(nProofOfWorkMax);
    
    if (time_diff > spacing * 8) {
        // 8x delay: minimum difficulty
        //LogPrintf("Emergency: 8x delay triggered, using minimum difficulty\n");
        return nProofOfWorkMax;
    } else if (time_diff > spacing * 6) {
        // 6x delay: 65% easier (35% of current difficulty)
        //LogPrintf("Emergency: 6x delay triggered, making 65%% easier\n");
        lastTarget = lastTarget * 100 / 35;
    } else if (time_diff > spacing * 3) {
        // 3x delay: 50% easier (50% of current difficulty)
        //LogPrintf("Emergency: 3x delay triggered, making 50%% easier\n");
        lastTarget = lastTarget * 100 / 50;
    }
    
    // Cap at minimum difficulty (maximum target)
    if (lastTarget > maxTarget) {
        return nProofOfWorkMax;
    }
    
    return lastTarget.GetCompact();
}

if (pindexLast->nHeight >= 127926 && pblock) {
        int64_t time_diff = pblock->GetBlockTime() - pindexLast->GetBlockTime();
        int64_t spacing = params.nPostBlossomPowTargetSpacing;

        arith_uint256 lastTarget;
        lastTarget.SetCompact(pindexLast->nBits);

        arith_uint256 maxTarget;
        maxTarget.SetCompact(nProofOfWorkMid);

        if (time_diff > spacing * 8) {
            // 8x delay: minimum difficulty
            //LogPrintf("Emergency: 8x delay triggered, using minimum difficulty\n");
            return nProofOfWorkMid;
        } else if (time_diff > spacing * 6) {
            // 6x delay: 65% easier (35% of current difficulty)
            //LogPrintf("Emergency: 6x delay triggered, making 65%% easier\n");
            lastTarget = lastTarget * 100 / 35;
        } else if (time_diff > spacing * 3) {
            // 3x delay: 50% easier (50% of current difficulty)
            //LogPrintf("Emergency: 3x delay triggered, making 50%% easier\n");
            lastTarget = lastTarget * 100 / 50;
        }

        // Cap at minimum difficulty (maximum target)
        if (lastTarget > maxTarget) {
            return nProofOfWorkMid;
        }

        return lastTarget.GetCompact();
    }


    // Find the first block in the averaging window and calculate average target
    const CBlockIndex* pindexFirst = pindexLast;
    arith_uint256 bnTot {0};
    for (int i = 0; pindexFirst && i < params.nPowAveragingWindow; i++) {
        arith_uint256 bnTmp;
        bnTmp.SetCompact(pindexFirst->nBits);
        bnTot += bnTmp;
        pindexFirst = pindexFirst->pprev;
    }

    // Check we have enough blocks
    if (pindexFirst == NULL)
    {
        return nProofOfWorkMin;
    }

    // Calculate average target
    arith_uint256 bnAvg {bnTot / params.nPowAveragingWindow};

    return CalculateNextWorkRequiredNew(bnAvg, pindexFirst->GetBlockTime(), pindexLast->GetBlockTime(), params);
}

unsigned int CalculateNextWorkRequiredNew(arith_uint256 bnAvg, int64_t nFirstBlockTime, int64_t nLastBlockTime, const Consensus::Params& params)
{
    int64_t averagingWindowTimespan = params.AveragingWindowTimespan();
    int64_t minActualTimespan = params.MinActualTimespan();
    int64_t maxActualTimespan = params.MaxActualTimespan();

    // Validate parameters
    if (averagingWindowTimespan <= 0) {
        return UintToArith256(params.pownewlimit).GetCompact();
    }

    // Calculate actual timespan with dampening
    int64_t nActualTimespan = nLastBlockTime - nFirstBlockTime;
    nActualTimespan = averagingWindowTimespan + (nActualTimespan - averagingWindowTimespan)/4;

    // Apply adjustment limits
    if (nActualTimespan < minActualTimespan) {
        nActualTimespan = minActualTimespan;
    }
    if (nActualTimespan > maxActualTimespan) {
        nActualTimespan = maxActualTimespan;
    }

    // Retarget using the provided average target
    const arith_uint256 bnPowLimit = UintToArith256(params.powLimit);
    arith_uint256 bnNew {bnAvg};
    bnNew /= averagingWindowTimespan;
    bnNew *= nActualTimespan;

    if (bnNew > bnPowLimit) {
        bnNew = bnPowLimit;
    }

    return bnNew.GetCompact();
}

bool PermittedDifficultyTransition(const Consensus::Params& params, int64_t height, uint32_t old_nbits, uint32_t new_nbits)
{
    // Always allow transitions on networks where min difficulty blocks are allowed
    if (params.fPowAllowMinDifficultyBlocks) return true;

    // Check if we're using the new difficulty algorithm
    if (height >= params.nNewPowDiffHeight) {
        // For new algorithm, validate against averaging window constraints
        const arith_uint256 pow_limit = UintToArith256(params.powLimit);
        arith_uint256 observed_new_target;
        observed_new_target.SetCompact(new_nbits);

        // Calculate maximum allowed difficulty change
        arith_uint256 max_target;
        max_target.SetCompact(old_nbits);
        max_target *= (100 + params.nPowMaxAdjustDown);
        max_target /= 100;

        if (max_target > pow_limit) {
            max_target = pow_limit;
        }

        // Calculate minimum allowed difficulty change
        arith_uint256 min_target;
        min_target.SetCompact(old_nbits);
        // Prevent underflow if nPowMaxAdjustUp >= 100
        if (params.nPowMaxAdjustUp >= 100) {
            min_target /= 100; // Very small target (high difficulty)
        } else {
            min_target *= (100 - params.nPowMaxAdjustUp);
            min_target /= 100;
        }

        // Check if new target is within allowed range
        if (observed_new_target > max_target || observed_new_target < min_target) {
            return false;
        }
    } else {
        // For old algorithm, use original Bitcoin validation
        if (height % params.DifficultyAdjustmentInterval() == 0) {
            int64_t smallest_timespan = params.nPowTargetTimespan/4;
            int64_t largest_timespan = params.nPowTargetTimespan*4;

            const arith_uint256 pow_limit = UintToArith256(params.powLimit);
            arith_uint256 observed_new_target;
            observed_new_target.SetCompact(new_nbits);

            // Calculate the largest difficulty value possible
            arith_uint256 largest_difficulty_target;
            largest_difficulty_target.SetCompact(old_nbits);
            largest_difficulty_target *= largest_timespan;
            largest_difficulty_target /= params.nPowTargetTimespan;

            if (largest_difficulty_target > pow_limit) {
                largest_difficulty_target = pow_limit;
            }

            arith_uint256 maximum_new_target;
            maximum_new_target.SetCompact(largest_difficulty_target.GetCompact());
            if (maximum_new_target < observed_new_target) {
                return false;
            }

            // Calculate the smallest difficulty value possible
            arith_uint256 smallest_difficulty_target;
            smallest_difficulty_target.SetCompact(old_nbits);
            smallest_difficulty_target *= smallest_timespan;
            smallest_difficulty_target /= params.nPowTargetTimespan;

            if (smallest_difficulty_target > pow_limit) {
                smallest_difficulty_target = pow_limit;
            }

            arith_uint256 minimum_new_target;
            minimum_new_target.SetCompact(smallest_difficulty_target.GetCompact());
            if (minimum_new_target > observed_new_target) {
                return false;
            }
        } else if (old_nbits != new_nbits) {
            return false;
        }
    }
    return true;
}

static bool CheckProofOfWorkHash(uint256 hash, unsigned int nBits, const Consensus::Params& params)
{
    bool fNegative;
    bool fOverflow;
    arith_uint256 bnTarget;

    bnTarget.SetCompact(nBits, &fNegative, &fOverflow);

    // Check range
    if (fNegative || bnTarget == 0 || fOverflow || bnTarget > UintToArith256(params.powLimit))
        return false;

    // Check proof of work matches claimed amount
    if (UintToArith256(hash) > bnTarget)
        return false;

    return true;
}

bool CheckProofOfWork(uint256 hash, unsigned int nBits, const Consensus::Params& params)
{
    return CheckProofOfWorkHash(hash, nBits, params);
}

bool CheckProofOfWork(const CBlockHeader& block, const Consensus::Params& params)
{
    /* Legacy blocks (without chain ID encoded) are permitted before AuxPoW
       activation height, controlled in ContextualCheck logic.  */
    const bool signalsAuxpow = block.IsAuxpow();
    const bool chainMatches = (block.GetChainId() == params.nAuxpowChainId);

    if (!signalsAuxpow || !chainMatches) {
        if (signalsAuxpow && block.auxpow)
            return error("%s : auxpow payload present without auxpow flag", __func__);

        return CheckProofOfWorkHash(block.GetHash(), block.nBits, params);
    }

    if (!block.auxpow)
        return error("%s : auxpow flag set but payload missing", __func__);

    if (params.fStrictChainId && block.GetChainId() != params.nAuxpowChainId)
        return error("%s : wrong chain ID in auxpow block", __func__);

    const CPureBlockHeader& parent = block.auxpow->getParentBlock();
    if (parent.IsAuxpow())
        return error("%s : parent block must not signal auxpow", __func__);

    if (!CheckProofOfWorkHash(block.auxpow->getParentBlockHash(), block.nBits, params))
        return error("%s : parent proof of work failed", __func__);

    if (!block.auxpow->check(block.GetHash(), block.GetChainId(), params))
        return error("%s : auxpow merkle linkage invalid", __func__);

    return true;
}
