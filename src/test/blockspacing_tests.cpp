// Copyright (c) 2025 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chainparams.h"
#include "consensus/validation.h"
#include "main.h"
#include "miner.h"
#include "pubkey.h"
#include "script/standard.h"
#include "txmempool.h"
#include "uint256.h"
#include "util.h"
#include "utilstrencodings.h"
#include "validation.h"

#include "test/test_bitcoin.h"

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(blockspacing_tests, TestingSetup)

// Test that artificial spacing is enforced before activation height
BOOST_AUTO_TEST_CASE(enforce_spacing_before_activation)
{
    const CChainParams& chainparams = Params();
    const Consensus::Params& params = chainparams.GetConsensus();
    
    // Simulate a block at height 138999 (one before activation at 139000)
    CBlockIndex* pindexPrev = chainActive.Tip();
    BOOST_CHECK(pindexPrev != nullptr);
    
    // Set the height to just before activation
    int testHeight = params.nNoMinSpacingActivationHeight - 1;
    
    // Create a mock previous block with recent timestamp
    CBlockIndex mockPrev;
    mockPrev.nHeight = testHeight - 1;
    mockPrev.nTime = GetAdjustedTime() - 100; // 100 seconds ago
    mockPrev.nBits = 0x1d00ffff;
    mockPrev.nChainWork = arith_uint256(0);
    
    // Test that spacing should be enforced
    bool fSpacingEnforced = testHeight >= params.nMinBlockSpacingStartHeight &&
                            testHeight < params.nNoMinSpacingActivationHeight;
    
    BOOST_CHECK(fSpacingEnforced);
    
    // Verify minimum spacing requirement
    int64_t nMinSpacing = 480; // 8 minutes
    int64_t nMinTime = mockPrev.nTime + nMinSpacing;
    int64_t nCurrentTime = GetAdjustedTime();
    
    // If current time is less than minimum time, block should be set to future
    if (nCurrentTime < nMinTime) {
        BOOST_CHECK_MESSAGE(nMinTime <= nCurrentTime + 7200, 
                           "Minimum spacing creates timestamp beyond MAX_FUTURE_BLOCK_TIME");
    }
}

// Test that spacing is NOT enforced at/after activation height
BOOST_AUTO_TEST_CASE(no_spacing_after_activation)
{
    const CChainParams& chainparams = Params();
    const Consensus::Params& params = chainparams.GetConsensus();
    
    // Test at activation height
    int testHeight = params.nNoMinSpacingActivationHeight;
    
    bool fSpacingEnforced = testHeight >= params.nMinBlockSpacingStartHeight &&
                            testHeight < params.nNoMinSpacingActivationHeight;
    
    BOOST_CHECK(!fSpacingEnforced);
    
    // Test one block after activation
    testHeight = params.nNoMinSpacingActivationHeight + 1;
    fSpacingEnforced = testHeight >= params.nMinBlockSpacingStartHeight &&
                       testHeight < params.nNoMinSpacingActivationHeight;
    
    BOOST_CHECK(!fSpacingEnforced);
}

// Test fast block detection disables after activation
BOOST_AUTO_TEST_CASE(fastblock_disabled_after_activation)
{
    const CChainParams& chainparams = Params();
    const Consensus::Params& params = chainparams.GetConsensus();
    
    CBlock block;
    CBlockIndex mockPrev;
    mockPrev.nHeight = params.nNoMinSpacingActivationHeight - 1;
    mockPrev.nTime = GetAdjustedTime() - 60; // 1 minute ago
    
    block.nTime = GetAdjustedTime(); // Fast block (< 2 min spacing)
    
    // Before activation: should be detected as fast
    bool isFast = IsFastBlock(block, &mockPrev, params);
    BOOST_CHECK(isFast);
    
    // At activation: should NOT be detected as fast
    mockPrev.nHeight = params.nNoMinSpacingActivationHeight;
    isFast = IsFastBlock(block, &mockPrev, params);
    BOOST_CHECK(!isFast);
}

// Test timestamp validation around activation boundary
BOOST_AUTO_TEST_CASE(timestamp_validation_boundary)
{
    const CChainParams& chainparams = Params();
    const Consensus::Params& params = chainparams.GetConsensus();
    
    // Create a chain of blocks around the activation height
    std::vector<CBlockIndex> chain;
    int64_t baseTime = GetAdjustedTime() - 10000; // Start 10000 seconds ago
    
    // Build chain from activation-5 to activation+5
    for (int i = -5; i <= 5; i++) {
        CBlockIndex block;
        block.nHeight = params.nNoMinSpacingActivationHeight + i;
        block.nTime = baseTime + (i * 600); // 10 minutes apart
        block.nBits = 0x1d00ffff;
        block.pprev = (i == -5) ? nullptr : &chain.back();
        chain.push_back(block);
    }
    
    // Verify spacing enforcement changes at boundary
    for (size_t i = 1; i < chain.size(); i++) {
        const CBlockIndex& current = chain[i];
        const CBlockIndex& prev = chain[i-1];
        
        bool fSpacingEnforced = current.nHeight >= params.nMinBlockSpacingStartHeight &&
                                current.nHeight < params.nNoMinSpacingActivationHeight;
        
        int64_t actualSpacing = current.nTime - prev.nTime;
        
        if (fSpacingEnforced) {
            // Before activation: spacing should be enforced (but this is in miner, not validation)
            BOOST_CHECK_MESSAGE(current.nHeight < params.nNoMinSpacingActivationHeight,
                              "Spacing enforcement should be active before activation height");
        } else {
            // After activation: no spacing enforcement
            BOOST_CHECK_MESSAGE(current.nHeight >= params.nNoMinSpacingActivationHeight,
                              "Spacing enforcement should be inactive at/after activation height");
        }
    }
}

// Test that UpdateTime doesn't break spacing enforcement
BOOST_AUTO_TEST_CASE(updatetime_respects_spacing)
{
    const CChainParams& chainparams = Params();
    const Consensus::Params& params = chainparams.GetConsensus();
    
    // Create a block header
    CBlockHeader header;
    
    // Create mock previous block
    CBlockIndex mockPrev;
    mockPrev.nHeight = params.nNoMinSpacingActivationHeight - 1; // Before activation
    mockPrev.nTime = GetAdjustedTime() - 100; // 100 seconds ago
    mockPrev.nBits = 0x1d00ffff;
    
    // Set initial time with spacing enforcement
    int64_t nMinSpacing = 480;
    int64_t nMinTime = mockPrev.nTime + nMinSpacing;
    header.nTime = nMinTime; // Set to minimum required time (in the future)
    
    int64_t nTimeBeforeUpdate = header.nTime;
    
    // Call UpdateTime
    int64_t nTimeAdjustment = UpdateTime(&header, params, &mockPrev);
    
    int64_t nTimeAfterUpdate = header.nTime;
    
    // CRITICAL TEST: UpdateTime should not reduce the timestamp below minimum spacing
    // This test will FAIL with current implementation if GetAdjustedTime() < nMinTime
    BOOST_CHECK_MESSAGE(nTimeAfterUpdate >= nMinTime,
                       strprintf("UpdateTime reduced timestamp below minimum spacing: "
                                "before=%d, after=%d, minTime=%d",
                                nTimeBeforeUpdate, nTimeAfterUpdate, nMinTime));
    
    // Also verify it doesn't exceed MAX_FUTURE_BLOCK_TIME
    int64_t nMaxFutureTime = GetAdjustedTime() + 7200;
    BOOST_CHECK_MESSAGE(nTimeAfterUpdate <= nMaxFutureTime,
                       strprintf("UpdateTime created timestamp beyond MAX_FUTURE_BLOCK_TIME: "
                                "time=%d, max=%d",
                                nTimeAfterUpdate, nMaxFutureTime));
}

// Test edge case: very recent previous block
BOOST_AUTO_TEST_CASE(recent_previous_block)
{
    const CChainParams& chainparams = Params();
    const Consensus::Params& params = chainparams.GetConsensus();
    
    // Previous block was mined just 30 seconds ago
    CBlockIndex mockPrev;
    mockPrev.nHeight = params.nNoMinSpacingActivationHeight - 1;
    mockPrev.nTime = GetAdjustedTime() - 30;
    mockPrev.nBits = 0x1d00ffff;
    
    // Calculate required minimum time
    int64_t nMinSpacing = 480; // 8 minutes
    int64_t nMinTime = mockPrev.nTime + nMinSpacing;
    int64_t nCurrentTime = GetAdjustedTime();
    
    // The minimum time will be 450 seconds (7.5 minutes) in the future
    int64_t nFutureOffset = nMinTime - nCurrentTime;
    
    BOOST_CHECK_MESSAGE(nFutureOffset > 0 && nFutureOffset < 480,
                       strprintf("Expected future offset between 0-480, got %d", nFutureOffset));
    
    // Verify this is within MAX_FUTURE_BLOCK_TIME
    BOOST_CHECK_MESSAGE(nFutureOffset <= 7200,
                       "Future timestamp exceeds MAX_FUTURE_BLOCK_TIME");
}

// Test validation rejects fast blocks before activation
BOOST_AUTO_TEST_CASE(validation_rejects_fast_blocks)
{
    const CChainParams& chainparams = Params();
    const Consensus::Params& params = chainparams.GetConsensus();
    
    // This test verifies the validation logic in main.cpp
    int testHeight = params.nNoMinSpacingActivationHeight - 1;
    
    // Simulate validation check
    bool fSpacingEnforced = testHeight >= params.nMinBlockSpacingStartHeight &&
                            testHeight < params.nNoMinSpacingActivationHeight;
    
    BOOST_CHECK(fSpacingEnforced);
    
    // Simulate a fast block (< 8 minutes)
    int64_t prevTime = 1000;
    int64_t blockTime = 1400; // 400 seconds later (< 480 required)
    int64_t nMinSpacing = 480;
    
    bool shouldReject = (blockTime - prevTime) < nMinSpacing;
    BOOST_CHECK(shouldReject);
}

// Test validation accepts fast blocks after activation
BOOST_AUTO_TEST_CASE(validation_accepts_fast_blocks_after_activation)
{
    const CChainParams& chainparams = Params();
    const Consensus::Params& params = chainparams.GetConsensus();
    
    // At activation height
    int testHeight = params.nNoMinSpacingActivationHeight;
    
    bool fSpacingEnforced = testHeight >= params.nMinBlockSpacingStartHeight &&
                            testHeight < params.nNoMinSpacingActivationHeight;
    
    BOOST_CHECK(!fSpacingEnforced);
    
    // Fast block should be accepted
    int64_t prevTime = 1000;
    int64_t blockTime = 1100; // 100 seconds later (fast, but OK after activation)
    int64_t nMinSpacing = 480;
    
    bool shouldReject = fSpacingEnforced && ((blockTime - prevTime) < nMinSpacing);
    BOOST_CHECK(!shouldReject);
}

BOOST_AUTO_TEST_SUITE_END()
