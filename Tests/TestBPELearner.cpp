//======================================================================
// 
//======================================================================

#include "catch.hpp"

#include <iostream>

#include "BPELearner.h"
#include "BPETokenizer.h"

//======================================================================
//----------------------------------------------------------------------

using IntPair = std::pair<uint32_t, uint32_t>;

//======================================================================

TEST_CASE("BPELearner TestCase 1", "[BPELearner][0]")
{
    BPELearner aBPELearner;
    aBPELearner.Learn(256 + 1, { "Hello", "He", "is", "Programmer" });

    BPETokenizer bpeTokenizer;
    bpeTokenizer.ReadModel("vocab.model");
    //bpeTokenizer.Encode("This text will be tokenized!");

    std::vector<std::vector<uint32_t>> outResult;
    bpeTokenizer.Encode({ "This", "is", "a", "list", "of", "words" }, outResult);

    for (const auto& res : outResult)
    {
        std::cout << '[';
        bool first = true;
        for (const auto token : res)
        {
            std::cout << (first ? "" : " ") << token;
            first = false;
        }
        std::cout << ']' << '\n';
    }

}