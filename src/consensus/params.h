// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CONSENSUS_PARAMS_H
#define BITCOIN_CONSENSUS_PARAMS_H

#include "uint256.h"
#include <map>
#include <string>

namespace Consensus {

enum DeploymentPos
{
    DEPLOYMENT_TESTDUMMY,
    DEPLOYMENT_CSV, // Deployment of BIP68, BIP112, and BIP113.
    DEPLOYMENT_SEGWIT, // Deployment of BIP141, BIP143, and BIP147.
    // NOTE: Also add new deployments to VersionBitsDeploymentInfo in versionbits.cpp
    MAX_VERSION_BITS_DEPLOYMENTS
};

/**
 * Struct for each individual consensus rule change using BIP9.
 */
struct BIP9Deployment {
    /** Bit position to select the particular bit in nVersion. */
    int bit;
    /** Start MedianTime for version bits miner confirmation. Can be a date in the past */
    int64_t nStartTime;
    /** Timeout/expiry MedianTime for the deployment attempt. */
    int64_t nTimeout;
};

/**
 * Parameters that influence chain consensus.
 */
struct Params {
    uint256 hashGenesisBlock;
    int nSubsidyHalvingInterval;
    /** Used to check majorities for block version upgrade */
    int nMajorityEnforceBlockUpgrade;
    int nMajorityRejectBlockOutdated;
    int nMajorityWindow;
    /** Block height and hash at which BIP34 becomes active */
    int BIP34Height;
    uint256 BIP34Hash;
    /**
     * Minimum blocks including miner confirmation of the total of 2016 blocks in a retargetting period,
     * (nPowTargetTimespan / nPowTargetSpacing) which is also used for BIP9 deployments.
     * Examples: 1916 for 95%, 1512 for testchains.
     */
    uint32_t nRuleChangeActivationThreshold;
    uint32_t nMinerConfirmationWindow;
    BIP9Deployment vDeployments[MAX_VERSION_BITS_DEPLOYMENTS];
    /** Proof of work parameters */
    uint256 powLimit;
    bool fPowAllowMinDifficultyBlocks;
    bool fPowNoRetargeting;
    int64_t nPowTargetSpacing;
    int64_t nPowTargetTimespan;
    int64_t DifficultyAdjustmentInterval() const { return nPowTargetTimespan / nPowTargetSpacing; }
    uint256 nMinimumChainWork;
    uint256 pownewlimit;
    uint256 powmaxlimit;

    /** New difficulty adjustment parameters (Bellcoin v3 style) */
    int nNewPowDiffHeight;                    // Height at which new difficulty algorithm activates
    int64_t nPowAveragingWindow;              // Number of blocks to average for difficulty calculation
    int64_t nPowMaxAdjustDown;                // Maximum percentage adjustment down
    int64_t nPowMaxAdjustUp;                  // Maximum percentage adjustment up
    int64_t nPostBlossomPowTargetSpacing;     // Target spacing for new algorithm

    /** Minimum block spacing enforcement start height */
    int nMinBlockSpacingStartHeight;

    /** Helper functions for new difficulty system */
    int64_t AveragingWindowTimespan() const {
        if (nPowAveragingWindow <= 0 || nPostBlossomPowTargetSpacing <= 0) return 1;
        return nPowAveragingWindow * nPostBlossomPowTargetSpacing;
    }
    int64_t MinActualTimespan() const {
        int64_t timespan = AveragingWindowTimespan();
        if (nPowMaxAdjustUp >= 100) return timespan / 100; // Prevent negative values
        return (timespan * (100 - nPowMaxAdjustUp)) / 100;
    }
    int64_t MaxActualTimespan() const {
        int64_t timespan = AveragingWindowTimespan();
        return (timespan * (100 + nPowMaxAdjustDown)) / 100;
    }
};
} // namespace Consensus

#endif // BITCOIN_CONSENSUS_PARAMS_H
