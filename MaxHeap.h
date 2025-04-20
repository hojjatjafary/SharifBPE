#include "PairHasher.h"

#include <vector>
#include <string>
#include <unordered_map>
#include <utility>  // For std::pair
#include <algorithm> // For std::swap
#include <stdexcept>
#include <iostream>


struct PairData
{
    std::pair<uint32_t, uint32_t> Pair;
    uint32_t Count;

    PairData(std::pair<uint32_t, uint32_t> inPair, uint32_t inCount)
    {
        Pair = inPair;
        Count = inCount;
    }

    bool operator<(const PairData& right) const
    {
        if (Count < right.Count)
        {
            return true;
        }
        else if (Count > right.Count)
        {
            return false;
        }
        else
        {
            return Pair < right.Pair;
        }
    }

    bool operator>(const PairData& right) const
    {
        return right < *this;
    }

    bool operator==(const PairData& right) const
    {
        return Pair == right.Pair && Count == right.Count;
    }
};

class MaxHeap 
{
public:

    size_t GetSize() const
    {
        return mHeap.size();
    }

    bool IsEmpty() const
    {
        return mHeap.empty();
    }

    bool Contains(const std::pair<uint32_t, uint32_t>& item) const
    {
        return mPairToIndex.find(item) != mPairToIndex.end();
    }

    void Push(const std::pair<uint32_t, uint32_t>& item, uint32_t value)
    {
        if (Contains(item))
        {
            throw std::invalid_argument("Item already exists in the mHeap");
        }

        // Place new item at the end of heap and bubble it up.
        mHeap.emplace_back(item, value);
        size_t index = mHeap.size() - 1;
        mPairToIndex[item] = index;
        bubbleUp(index);
    }

    void Pop()
    {
        if (IsEmpty())
        {
            throw std::out_of_range("Heap is empty");
        }

        mPairToIndex.erase(mHeap[0].Pair);
        mHeap[0] = mHeap.back();
        mHeap.pop_back();
        mPairToIndex[mHeap[0].Pair] = 0;
        
        if (!IsEmpty()) 
        {
            bubbleDown(0);
        }
    }

    void Top(std::pair<uint32_t, uint32_t>& maxPair, uint32_t& count)
    {
        if (IsEmpty())
        {
            throw std::out_of_range("Heap is empty");
        }

        maxPair = mHeap[0].Pair;
        count = mHeap[0].Count;
    }

    void ExtractTop(std::pair<uint32_t, uint32_t>& maxPair, uint32_t& count)
    {
        Top(maxPair, count);
        Pop();
    }

    // Update the value of an existing item
    void Update(const std::pair<uint32_t, uint32_t>& item, uint32_t newValue)
    {
        auto iter = mPairToIndex.find(item);
        if (iter == mPairToIndex.end())
        {
            throw std::invalid_argument("Item not found in the mHeap");
        }

        const size_t index = iter->second;
        const uint32_t oldValue = mHeap[index].Count;
        mHeap[index].Count = newValue;

        if (newValue > oldValue)
        {
            bubbleUp(index);
        }
        else 
        {
            bubbleDown(index);
        }
    }

    // Returns true if it is a new item.
    bool UpSert(const std::pair<uint32_t, uint32_t>& item, int value)
    {
        auto iter = mPairToIndex.find(item);
        if (iter != mPairToIndex.end())
        {
            const size_t index = iter->second;
            const uint32_t oldValue = mHeap[index].Count;
            const uint32_t newValue = addOrSubtract(oldValue, value);
            mHeap[index].Count = newValue;

            if (newValue > oldValue)
            {
                bubbleUp(index);
            }
            else
            {
                bubbleDown(index);
            }

            return false;
        }
        else if (value > 0)
        {
            // Place new item at the end of heap and bubble it up.
            mHeap.emplace_back(item, value);
            const size_t index = mHeap.size() - 1;
            mPairToIndex[item] = index;
            bubbleUp(index);
            return true;
        }

        return false;
    }

    inline unsigned int addOrSubtract(uint32_t unsignedNum, int signedNum)
    {
        if (signedNum >= 0) 
        {
            return unsignedNum + static_cast<uint32_t>(signedNum);
        }
        else 
        {
            // When subtracting a negative number, it's equivalent to adding its absolute value.
            // However, the user explicitly asked for subtraction when signedNum is negative.
            // Be aware of potential underflow with unsigned integers.
            return unsignedNum - static_cast<uint32_t>(-signedNum);
        }
    }

    void PrintHeap() const
    {
        for (const auto& item : mHeap) 
        {
            std::cout << "Item: (" << item.Pair.first << ", " << item.Pair.second << "), Count: " << item.Count << std::endl;
        }
    }

private:

    std::vector<PairData> mHeap;
    std::unordered_map<std::pair<uint32_t, uint32_t>, size_t, PairHasher> mPairToIndex; // Maps item to its index in the mHeap

    void bubbleUp(size_t index) 
    {
        size_t parentIndex = (index - 1) / 2;
        while (index > 0 && mHeap[index] > mHeap[parentIndex])
        {
            swapNodes(index, parentIndex);

            index = parentIndex;
            parentIndex = (index - 1) / 2;
        }
    }

    void bubbleDown(size_t index) 
    {
        const size_t currentSize = mHeap.size();
        bool swapped = true; // Initialize to true to ensure the loop runs at least once if needed
        
        while (swapped)
        {
            swapped = false; // Assume no swap will happen in this iteration

            size_t largest = index;
            size_t leftChildIndex = 2 * index + 1;
            size_t rightChildIndex = 2 * index + 2;

            if (leftChildIndex < currentSize && mHeap[leftChildIndex] > mHeap[largest])
            {
                largest = leftChildIndex;
            }
            
            if (rightChildIndex < currentSize && mHeap[rightChildIndex] > mHeap[largest])
            {
                largest = rightChildIndex;
            }

            // If the largest node found is NOT the current node 'index'
            if (largest != index) 
            {
                swapNodes(index, largest); // Swap with the largest child
                index = largest;          // Move down to the child's index
                swapped = true;           // Signal that a swap occurred, so the loop should continue
            }
            // If largest == index, it means the node is in the correct position
            // relative to its children. 'swapped' remains false, and the loop
            // condition will be false on the next check, terminating the loop.
        }
    }

    void swapNodes(int index1, int index2)
    {
        if (index1 == index2 || index1 >= mHeap.size() || index2 >= mHeap.size()) 
            return;

        // Swap elements in the vector
        std::swap(mHeap[index1], mHeap[index2]);

        mPairToIndex[mHeap[index1].Pair] = index1;
        mPairToIndex[mHeap[index2].Pair] = index2;
    }

};