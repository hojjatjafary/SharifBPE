//======================================================================
// 
//======================================================================

#include "catch.hpp"

#include "MaxHeap.h"

//======================================================================
//----------------------------------------------------------------------

using IntPair = std::pair<uint32_t, uint32_t>;

//======================================================================

TEST_CASE("MaxHeap TestCase1", "[MaxHeap][0]")
{
    MaxHeap maxHeap;

    IntPair p1(1, 2);
    IntPair p2(2, 3);
    IntPair p3(5, 6);

    // Insert items
    maxHeap.Push(p1, 10);
    maxHeap.Push(p2, 11);
    maxHeap.Push(p3, 14);

    IntPair maxPair;
    uint32_t Count = 0;
    maxHeap.Top(maxPair, Count);

    REQUIRE(maxPair == p3);
    REQUIRE(Count == 14);
}

TEST_CASE("Update root, bubble down root", "[MaxHeap][1]")
{
    MaxHeap maxHeap;

    IntPair p1(1, 2);
    IntPair p2(2, 3);
    IntPair p3(5, 6);

    maxHeap.Push(p1, 10);
    maxHeap.Push(p2, 11);
    maxHeap.Push(p3, 14);

    maxHeap.Update(p3, 5);

    IntPair maxPair;
    uint32_t Count = 0;
    maxHeap.Top(maxPair, Count);

    REQUIRE(maxPair == p2);
    REQUIRE(Count == 11);
}

TEST_CASE("Update left, bubble up", "[MaxHeap][1]")
{
    MaxHeap maxHeap;

    IntPair p1(1, 2);
    IntPair p2(2, 3);
    IntPair p3(5, 6);

    maxHeap.Push(p1, 10);
    maxHeap.Push(p2, 11);
    maxHeap.Push(p3, 14);

    maxHeap.Update(p1, 15);

    IntPair maxPair;
    uint32_t Count = 0;
    maxHeap.Top(maxPair, Count);

    REQUIRE(maxPair == p1);
    REQUIRE(Count == 15);
}


TEST_CASE("Update right, bubble up", "[MaxHeap][1]")
{
    MaxHeap maxHeap;

    IntPair p1(1, 2);
    IntPair p2(2, 3);
    IntPair p3(5, 6);

    maxHeap.Push(p1, 10);
    maxHeap.Push(p2, 11);
    maxHeap.Push(p3, 14);

    maxHeap.Update(p2, 15);

    IntPair maxPair;
    uint32_t Count = 0;
    maxHeap.Top(maxPair, Count);

    REQUIRE(maxPair == p2);
    REQUIRE(Count == 15);
}

TEST_CASE("pop root", "[MaxHeap][1]")
{
    MaxHeap maxHeap;

    IntPair p1(1, 2);
    IntPair p2(2, 3);
    IntPair p3(5, 6);
    IntPair p4(6, 6);

    maxHeap.Push(p1, 10);
    maxHeap.Push(p2, 11);
    maxHeap.Push(p3, 14);
    maxHeap.Push(p4, 9);

    maxHeap.Pop();

    IntPair maxPair;
    uint32_t Count = 0;
    maxHeap.Top(maxPair, Count);

    REQUIRE(maxPair == p2);
    REQUIRE(Count == 11);
}

TEST_CASE("two pops", "[MaxHeap][1]")
{
    MaxHeap maxHeap;

    IntPair p1(1, 2);
    IntPair p2(2, 3);
    IntPair p3(5, 6);
    IntPair p4(6, 6);

    maxHeap.Push(p1, 10);
    maxHeap.Push(p2, 11);
    maxHeap.Push(p3, 14);
    maxHeap.Push(p4, 9);

    maxHeap.Pop();
    maxHeap.Pop();

    IntPair maxPair;
    uint32_t Count = 0;
    maxHeap.Top(maxPair, Count);

    REQUIRE(maxPair == p1);
    REQUIRE(Count == 10);
}

TEST_CASE("push to root", "[MaxHeap][1]")
{
    MaxHeap maxHeap;

    IntPair p1(1, 2);
    IntPair p2(2, 3);
    IntPair p3(5, 6);
    IntPair p4(6, 6);

    maxHeap.Push(p1, 10);
    maxHeap.Push(p2, 11);
    maxHeap.Push(p3, 14);
    maxHeap.Push(p4, 100);

    IntPair maxPair;
    uint32_t Count = 0;
    maxHeap.Top(maxPair, Count);

    REQUIRE(maxPair == p4);
    REQUIRE(Count == 100);
}