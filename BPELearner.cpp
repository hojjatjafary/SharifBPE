#include "BPELearner.h"
#include "MultiThreadFileReader.h"

#include <iostream>
#include <fstream>
#include <cassert>

//-------------------------------------------------------------------------------------------------

BPELearner::BPELearner()
{
	//mVerbose = true;

	// Initial vocabulary
	std::string oneCharString(" ");
	for (uint16_t ch = 0; ch < InitialVocabSize; ++ch)
	{
		oneCharString[0] = char(ch);
		// We should cast to uchar first and then to uint
		mIdToPair[static_cast<uint8_t>(ch)] = oneCharString;
	}
}

//-------------------------------------------------------------------------------------------------

void BPELearner::Learn(const uint32_t vocabSize, const char* inputFileName)
{
	MapType wordCountHashTable;
	MultiThreadFileReader MTFRead;
	MTFRead.ReadText(inputFileName, wordCountHashTable);

	prepare(wordCountHashTable);

	internalLearn(vocabSize);
}

//-------------------------------------------------------------------------------------------------

void BPELearner::Learn(const uint32_t vocabSize, const std::vector<std::string>& textChunks)
{
	MapType wordCountHashTable;
	countWords(textChunks, wordCountHashTable);

	prepare(wordCountHashTable);

	internalLearn(vocabSize);
}

//-------------------------------------------------------------------------------------------------

void BPELearner::Save(const std::string& outputFileName) const
{
	std::ofstream outFile(outputFileName);
	outFile << std::noskipws;  // Prevent any potential skipping of whitespace

	for (const auto& [first, second] : mMergeRules)
	{
		outFile << first << ' ' << second << '\n';  // Use single char instead of strings
	}
}

//-------------------------------------------------------------------------------------------------

void BPELearner::countWords(const std::vector<std::string>& textChunks, MapType& wordCount)
{
	for (const auto& word : textChunks)
	{
		wordCount[word]++;
	}
}

//-------------------------------------------------------------------------------------------------
// Split words to a list of Ids (unsigned int), also flattens the word counts.
void BPELearner::prepare(const MapType& wordCount)
{
	mSplitedWords.reserve(wordCount.size());
	mWordCounts.reserve(wordCount.size());
	
	for (const auto& item : wordCount)
	{
		const auto& word = item.first;

		auto& currentSplitedWord = mSplitedWords.emplace_back(word.size(), 0);
		for (size_t i = 0; i < word.size(); ++i)
		{
			// We should cast to uchar first and then to uint
			currentSplitedWord[i] = static_cast<uint8_t>(word[i]);
		}

		mWordCounts.push_back(item.second);
	}

	for (uint32_t wi = 0; wi < mSplitedWords.size(); ++wi)
	{
		countPairsInWord(wi,
			mSplitedWords[wi],
			mWordCounts[wi]
		);
	}
}

//-------------------------------------------------------------------------------------------------

void BPELearner::countPairsInWord(
	const uint32_t wordIndex,
	const std::vector<uint32_t>& splitedWord,
	const uint32_t countOfWord
)
{
	IdPair curPair;
	curPair.second = splitedWord[0];

	for (size_t i = 1; i < splitedWord.size(); ++i)
	{
		curPair.first = curPair.second;
		curPair.second = splitedWord[i];

		mMaxHeap.UpSert(curPair, countOfWord);

		mWhereToUpdate[curPair].insert(wordIndex);
	}
}

//-------------------------------------------------------------------------------------------------
// The main BPE algorithm implementation.
void BPELearner::internalLearn(const uint32_t vocabSize)
{
	assert(vocabSize >= InitialVocabSize);
	const int numMerges = vocabSize - InitialVocabSize;
	
	IdPair maxPair;
	uint32_t maxCount = 0;
	mMaxHeap.ExtractTop(maxPair, maxCount);

	for (size_t i = 0; i < numMerges; ++i)
	{
		const uint32_t newPairId = i + InitialVocabSize;
		mMergeRules.push_back(maxPair);

		// Debug info
		if (mVerbose)
		{
			const std::string pairStr = mIdToPair[maxPair.first] + mIdToPair[maxPair.second];
			std::cout << mIdToPair[maxPair.first] << " " << mIdToPair[maxPair.second] << " -> " << pairStr << " " << maxCount << std::endl;
			mIdToPair[newPairId] = pairStr;
		}

		// merge
		for (const auto wordIndex : mWhereToUpdate[maxPair])
		{
			auto& splitedWord = mSplitedWords[wordIndex];
			const auto wordCount = mWordCounts[wordIndex];

			replacePairInWord(splitedWord, wordCount, maxPair, newPairId, wordIndex);
		}

		mWhereToUpdate.erase(maxPair);

		mMaxHeap.ExtractTop(maxPair, maxCount);
	}
}

//-------------------------------------------------------------------------------------------------

void BPELearner::replacePairInWord(
	std::vector<uint32_t>& splitedWord,
	const uint32_t wordCount,
	const IdPair& maxPair,
	const uint32_t newTokenId,
	const uint32_t wordIndex
)
{
	const auto wordLength = splitedWord.size();

	size_t write = 0, read = 0;
	while (read < wordLength)
	{
		if (read < wordLength - 1 &&
			splitedWord[read] == maxPair.first &&
			splitedWord[read + 1] == maxPair.second)
		{
			// Update previous pair (if it exists)
			if (write > 0) 
			{
				updateCount(IdPair(splitedWord[write - 1], maxPair.first), -wordCount, wordIndex);
				updateCount(IdPair(splitedWord[write - 1], newTokenId), wordCount, wordIndex);
			}

			// Update next pair (if it exists)
			if (read + 2 < wordLength)
			{
				updateCount(IdPair(maxPair.second, splitedWord[read + 2]), -wordCount, wordIndex);
				updateCount(IdPair(newTokenId, splitedWord[read + 2]), wordCount, wordIndex);
			}

			splitedWord[write++] = newTokenId; // Replace the pair
			read += 2;
		}
		else 
		{
			splitedWord[write++] = splitedWord[read++];
		}
	}
	splitedWord.resize(write);
}

//-------------------------------------------------------------------------------------------------

void BPELearner::updateCount(IdPair pair, int32_t count, uint32_t wordIndex)
{
	bool inserted = mMaxHeap.UpSert(pair, count);
	if (count > 0)
		mWhereToUpdate[pair].insert(wordIndex);
}

//-------------------------------------------------------------------------------------------------