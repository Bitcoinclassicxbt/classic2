// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "fastblock.h"
#include "util.h"
#include "utiltime.h"

bool IsFastBlock(const CBlock& block, const CBlockIndex* pindexPrev, const Consensus::Params& params)
{
    if (!pindexPrev) return false;

    int nextHeight = pindexPrev->nHeight + 1;
    // Disable discouragement once natural PoW timing reactivates
    if (nextHeight >= params.nNoMinSpacingActivationHeight) return false;

    // Use configurable minimum spacing (non-consensus)
    int64_t nMinSpacing = GetArg("-minblockspacing", 120); // Default 2 minutes
    if (nMinSpacing <= 0) return false;

    int64_t timeDiff = block.GetBlockTime() - pindexPrev->GetBlockTime();
    return timeDiff < nMinSpacing;
}

int GetFastBlockScore(const CBlock& block, const CBlockIndex* pindexPrev, const Consensus::Params& params)
{
    if (!IsFastBlock(block, pindexPrev, params)) return 0;

    int64_t nMinSpacing = GetArg("-minblockspacing", 120); // Default 2 minutes
    int64_t timeDiff = block.GetBlockTime() - pindexPrev->GetBlockTime();

    if (timeDiff <= 0) return 1000; // Invalid timestamp

    // Score based on how much faster than minimum
    int score = (nMinSpacing - timeDiff) * 100 / nMinSpacing;
    return std::min(score, 100); // Cap at 100
}

bool ShouldRelayBlock(const CBlock& block, const CBlockIndex* pindexPrev, const Consensus::Params& params)
{
    int score = GetFastBlockScore(block, pindexPrev, params);
    
    // Don't relay extremely fast blocks immediately
    if (score > 75) return false;
    
    // Relay other blocks normally
    return true;
}

int GetRelayDelay(const CBlock& block, const CBlockIndex* pindexPrev, const Consensus::Params& params)
{
    int score = GetFastBlockScore(block, pindexPrev, params);
    
    if (score == 0) return 0; // No delay for normal blocks
    
    // Delay relay by up to 30 seconds for fast blocks
    return (score * 30) / 100;
}
