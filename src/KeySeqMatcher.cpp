/*
 * Copyright 2024-2025 Jay Logue
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <string.h>
#include <stdint.h>  // For uint32_t, uint64_t

#ifndef UNIT_TEST
#include "pico/stdlib.h"
#include "hardware/clocks.h"
#else
#include <stdio.h>
#include <assert.h>

// Mock time function for unit tests
uint64_t g_mock_time_us = 0;
uint64_t time_us_64(void) {
    return g_mock_time_us;
}
#endif

#include "KeySeqMatcher.h"

/** @brief Attempts to match a received character against the configured key sequences
 * 
 * @param  ch Character to match
 * @param  queuedChars [out] Receives a pointer to an array of previously received
 *            characters that represent an incomplete partial match for a key sequence.
 * @param  numQueuedChars [out] Receives the number of characters in the queuedChars
*             array.
 * 
 * @return Index of the matched key sequence, or one of the special values:
 *            - NO_MATCH: The character did not match any key sequence.
 *            - PARTIAL_MATCH: The character was a partial match for a key sequence.
 *
 * @note   The method may return an array of previously received characters (via
 *            queueChars/numQueuedChars) in any of the three return scenarios:
 *            match, NO_MATCH or PARTIAL_MATCH.
 */
int KeySeqMatcher::Match(char ch, const char * & queuedChars, size_t & numQueuedChars)
{
    queuedChars = mKeySeqList[mCurKeySeq];
    numQueuedChars = 0;
    
    while (true) {
        if (ch == mKeySeqList[mCurKeySeq][mCurMatchLen]) {
            mCurMatchLen++;
            if (mKeySeqList[mCurKeySeq][mCurMatchLen] == 0) {
                int matchedIndex = (int)mCurKeySeq;
                Reset();
                return matchedIndex;
            }
            else {
                mLastMatchTimeUS = time_us_64();
                return PARTIAL_MATCH;
            }
        }

        do {
            if (mKeySeqList[++mCurKeySeq] == NULL) {
                if (mCurMatchLen == 0) {
                    Reset();
                    return NO_MATCH;
                }
                else {
                    numQueuedChars = mCurMatchLen;
                    Reset();
                    break;
                }
            }
        } while (mCurMatchLen > 0 && strncmp(mKeySeqList[mCurKeySeq-1], mKeySeqList[mCurKeySeq], mCurMatchLen) != 0);
    }
}

/** @brief Checks if a partial key sequence match has timed out
 * 
 * @param  queuedChars [out] Receives a pointer an array of previously received
 *             characters that had partially matched a key sequence when the timeout
 *             occurred, or an empty array if no timeout occurred.
 * @param  numQueuedChars [out] Receives the number of characters in queuedChars;
 *             will be zero if no timeout occurred.
 * 
 * @return true if a timeout occurred, false otherwise.
 * 
 * @note   When a timeout occurs, the matcher state is reset.
 */
bool KeySeqMatcher::CheckTimeout(const char * & queuedChars, size_t & numQueuedChars)
{
    if (mCurMatchLen > 0 && mTimeoutUS != 0 && time_us_64() >= (mLastMatchTimeUS + mTimeoutUS)) {
        queuedChars = mKeySeqList[mCurKeySeq];
        numQueuedChars = mCurMatchLen;
        Reset();
        return true;
    }
    else {
        queuedChars = "";
        numQueuedChars = 0;
        return false;
    }
}

#ifdef UNIT_TEST
// Unit tests for KeySeqMatcher

// Helper function to print test results
void print_test_result(int test_num, const char* test_name, bool passed) {
    printf("Test %2d: %-40s: %s\n", test_num, test_name, passed ? "PASSED" : "FAILED");
}

int main() {
    printf("Running KeySeqMatcher unit tests...\n\n");
    bool all_tests_passed = true;
    bool test_result;
    const char* queued_chars;
    size_t num_queued_chars;
    int result;

    // Test 1: Basic match with single sequence
    {
        const char* sequences[] = {"ABC", NULL};
        KeySeqMatcher matcher(sequences, 1000000);
        
        g_mock_time_us = 0;
        result = matcher.Match('A', queued_chars, num_queued_chars);
        result = matcher.Match('B', queued_chars, num_queued_chars);
        result = matcher.Match('C', queued_chars, num_queued_chars);
        
        test_result = (result == 0); // Should match the first (and only) sequence
        print_test_result(1, "Basic match with single sequence", test_result);
        all_tests_passed &= test_result;
    }
    
    // Test 2: No match with single sequence
    {
        const char* sequences[] = {"ABC", NULL};
        KeySeqMatcher matcher(sequences, 1000000);
        
        g_mock_time_us = 0;
        result = matcher.Match('A', queued_chars, num_queued_chars);
        result = matcher.Match('B', queued_chars, num_queued_chars);
        result = matcher.Match('D', queued_chars, num_queued_chars); // 'D' instead of 'C'
        
        test_result = (result == KeySeqMatcher::NO_MATCH);
        print_test_result(2, "No match with single sequence", test_result);
        all_tests_passed &= test_result;
    }
    
    // Test 3: Partial match with single sequence and verify output params
    {
        const char* sequences[] = {"ABC", NULL};
        KeySeqMatcher matcher(sequences, 1000000);
        
        g_mock_time_us = 0;
        result = matcher.Match('A', queued_chars, num_queued_chars);
        
        // In a partial match, queued_chars should be empty and num_queued_chars should be 0
        test_result = (result == KeySeqMatcher::PARTIAL_MATCH && 
                      num_queued_chars == 0);
        print_test_result(3, "Partial match with single sequence (queued_chars check)", test_result);
        all_tests_passed &= test_result;
    }
    
    // Test 4: Multiple sequences with unique match
    {
        const char* sequences[] = {"ABC", "DEF", "GHI", NULL};
        KeySeqMatcher matcher(sequences, 1000000);
        
        g_mock_time_us = 0;
        result = matcher.Match('D', queued_chars, num_queued_chars);
        result = matcher.Match('E', queued_chars, num_queued_chars);
        result = matcher.Match('F', queued_chars, num_queued_chars);
        
        test_result = (result == 1); // Should match the second sequence
        print_test_result(4, "Multiple sequences with unique match", test_result);
        all_tests_passed &= test_result;
    }
    
    // Test 5: Multiple sequences with common prefix
    {
        const char* sequences[] = {"ABC", "ABD", NULL};
        KeySeqMatcher matcher(sequences, 1000000);
        
        g_mock_time_us = 0;
        // First try "ABC"
        result = matcher.Match('A', queued_chars, num_queued_chars);
        result = matcher.Match('B', queued_chars, num_queued_chars);
        result = matcher.Match('C', queued_chars, num_queued_chars);
        test_result = (result == 0);
        print_test_result(5, "Multiple sequences with common prefix (ABC)", test_result);
        all_tests_passed &= test_result;
        
        // Then try "ABD"
        matcher.Reset();
        result = matcher.Match('A', queued_chars, num_queued_chars);
        result = matcher.Match('B', queued_chars, num_queued_chars);
        result = matcher.Match('D', queued_chars, num_queued_chars);
        test_result = (result == 1);
        print_test_result(6, "Multiple sequences with common prefix (ABD)", test_result);
        all_tests_passed &= test_result;
    }
    
    // Test 6: Timeout of partial match
    {
        const char* sequences[] = {"ABC", NULL};
        KeySeqMatcher matcher(sequences, 1000000); // 1 second timeout
        
        g_mock_time_us = 0;
        result = matcher.Match('A', queued_chars, num_queued_chars);
        
        // Simulate time passing beyond the timeout
        g_mock_time_us = 2000000; // 2 seconds later
        
        bool timeout = matcher.CheckTimeout(queued_chars, num_queued_chars);
        
        test_result = (timeout == true && num_queued_chars == 1 && queued_chars[0] == 'A');
        print_test_result(7, "Timeout of partial match", test_result);
        all_tests_passed &= test_result;
    }
    
    // Test 7: No timeout before threshold
    {
        const char* sequences[] = {"ABC", NULL};
        KeySeqMatcher matcher(sequences, 1000000); // 1 second timeout
        
        g_mock_time_us = 0;
        result = matcher.Match('A', queued_chars, num_queued_chars);
        
        // Simulate time passing but not beyond the timeout
        g_mock_time_us = 500000; // 0.5 seconds later
        
        bool timeout = matcher.CheckTimeout(queued_chars, num_queued_chars);
        
        test_result = (timeout == false && num_queued_chars == 0);
        print_test_result(8, "No timeout before threshold", test_result);
        all_tests_passed &= test_result;
    }
    
    // Test 8: Reset after partial match
    {
        const char* sequences[] = {"ABC", NULL};
        KeySeqMatcher matcher(sequences, 1000000);
        
        g_mock_time_us = 0;
        result = matcher.Match('A', queued_chars, num_queued_chars);
        matcher.Reset();
        
        // After reset, should be able to match 'A' again
        result = matcher.Match('A', queued_chars, num_queued_chars);
        
        test_result = (result == KeySeqMatcher::PARTIAL_MATCH);
        print_test_result(9, "Reset after partial match", test_result);
        all_tests_passed &= test_result;
    }
    
    // Test 9: SetTimeout changes timeout duration
    {
        const char* sequences[] = {"ABC", NULL};
        KeySeqMatcher matcher(sequences, 1000000); // 1 second timeout
        
        // Change timeout to 2 seconds
        matcher.SetTimeout(2000000);
        
        g_mock_time_us = 0;
        result = matcher.Match('A', queued_chars, num_queued_chars);
        
        // Simulate time passing to 1.5 seconds (should not timeout yet)
        g_mock_time_us = 1500000;
        bool timeout = matcher.CheckTimeout(queued_chars, num_queued_chars);
        
        test_result = (timeout == false);
        print_test_result(10, "SetTimeout changes timeout duration", test_result);
        all_tests_passed &= test_result;
    }
    
    // Test 10: Complete sequence followed by another character
    {
        const char* sequences[] = {"ABC", NULL};
        KeySeqMatcher matcher(sequences, 1000000);
        
        g_mock_time_us = 0;
        result = matcher.Match('A', queued_chars, num_queued_chars);
        result = matcher.Match('B', queued_chars, num_queued_chars);
        result = matcher.Match('C', queued_chars, num_queued_chars);
        
        // After complete match, matcher should be reset
        result = matcher.Match('A', queued_chars, num_queued_chars);
        
        test_result = (result == KeySeqMatcher::PARTIAL_MATCH);
        print_test_result(11, "Complete sequence followed by another character", test_result);
        all_tests_passed &= test_result;
    }
    
    // Test 11: Verify queued_chars in NO_MATCH case with invalid first character
    {
        const char* sequences[] = {"ABC", "DEF", NULL};
        KeySeqMatcher matcher(sequences, 1000000);
        
        g_mock_time_us = 0;
        result = matcher.Match('X', queued_chars, num_queued_chars); // Should not match any sequence
        
        test_result = (result == KeySeqMatcher::NO_MATCH && 
                      num_queued_chars == 0);
        print_test_result(12, "NO_MATCH case with invalid char (queued_chars check)", test_result);
        all_tests_passed &= test_result;
    }
    
    // Test 12: Verify queued_chars in NO_MATCH case after partial match
    {
        const char* sequences[] = {"ABC", NULL};
        KeySeqMatcher matcher(sequences, 1000000);
        
        g_mock_time_us = 0;
        result = matcher.Match('A', queued_chars, num_queued_chars); // Partial match
        result = matcher.Match('X', queued_chars, num_queued_chars); // Invalid character
        
        // When a partial match fails, queued_chars should contain the partial match characters
        test_result = (result == KeySeqMatcher::NO_MATCH && 
                      num_queued_chars == 1 && 
                      strncmp(queued_chars, "A", 1) == 0);
        print_test_result(13, "NO_MATCH after partial match (queued_chars check)", test_result);
        all_tests_passed &= test_result;
    }
    
    // Test 13: Verify queued_chars in full match case
    {
        const char* sequences[] = {"ABC", NULL};
        KeySeqMatcher matcher(sequences, 1000000);
        
        g_mock_time_us = 0;
        result = matcher.Match('A', queued_chars, num_queued_chars);
        result = matcher.Match('B', queued_chars, num_queued_chars);
        result = matcher.Match('C', queued_chars, num_queued_chars); // Complete match
        
        // In a complete match, queued_chars should be empty and num_queued_chars should be 0
        test_result = (result == 0 && 
                      num_queued_chars == 0);
        print_test_result(14, "Full match (queued_chars check)", test_result);
        all_tests_passed &= test_result;
    }
    
    // Test 14: Partial match then different character - verify queued_chars behavior
    {
        const char* sequences[] = {"ABC", "D", NULL};
        KeySeqMatcher matcher(sequences, 1000000);
        
        g_mock_time_us = 0;
        result = matcher.Match('A', queued_chars, num_queued_chars); // Partial match with "ABC"
        result = matcher.Match('B', queued_chars, num_queued_chars); // Partial match with "ABC"
        result = matcher.Match('D', queued_chars, num_queued_chars); // Fails "ABC", should try "D"
        
        // When a partial match of "ABC" fails but the character matches "D" sequence:
        // 1. We should match the "D" sequence (index 1)
        // 2. num_queued_chars should be 2
        // 3. queued_chars should contain "AB" (the previously matched characters)
        test_result = (result == 1 && // Index 1 for "D" sequence
                     num_queued_chars == 2 &&
                     queued_chars[0] == 'A' &&
                     queued_chars[1] == 'B');
        
        print_test_result(15, "Partial match then different sequence (queued_chars check)", test_result);
        all_tests_passed &= test_result;
    }
    
    // Test 15: Partial match with common prefix then match with a different sequence
    {
        const char* sequences[] = {"ABC", "ABD", "XYZ", NULL};
        KeySeqMatcher matcher(sequences, 1000000);
        
        g_mock_time_us = 0;
        result = matcher.Match('A', queued_chars, num_queued_chars); // Partial match with "ABC" and "ABD"
        result = matcher.Match('B', queued_chars, num_queued_chars); // Partial match with "ABC" and "ABD"
        result = matcher.Match('X', queued_chars, num_queued_chars); // Fails both "ABC" and "ABD", should try "XYZ"
        
        // When a partial match of "ABC"/"ABD" fails but the character matches "XYZ" sequence:
        // 1. We should match the "XYZ" sequence (index 2)
        // 2. num_queued_chars should be greater than 0 (containing the abandoned partial match)
        // 3. queued_chars should contain "AB" (the previously matched characters)
        test_result = (result == KeySeqMatcher::PARTIAL_MATCH && // First char of "XYZ" is a partial match
                     num_queued_chars == 2 &&
                     queued_chars[0] == 'A' &&
                     queued_chars[1] == 'B');
        
        print_test_result(16, "Partial match with common prefix then different sequence", test_result);
        all_tests_passed &= test_result;
    }

    printf("\nSummary: %s\n", all_tests_passed ? "ALL TESTS PASSED" : "SOME TESTS FAILED");
    
    return all_tests_passed ? 0 : 1;
}
#endif // UNIT_TEST
