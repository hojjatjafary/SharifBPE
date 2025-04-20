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

		mSplitedWords.emplace_back();
		auto& currentSplitedWord = mSplitedWords.back();
		for (const auto& ch : word)
		{
			// We should cast to uchar first and then to uint
			currentSplitedWord.push_back(static_cast<uint8_t>(ch));
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
	const std::list<uint32_t>& splitedWord,
	const uint32_t countOfWord
)
{
	auto iter = splitedWord.begin();
	IdPair curPair;
	curPair.second = *iter;

	for (iter = std::next(iter); iter != splitedWord.end(); ++iter)
	{
		const uint32_t tokenId = *iter;
		curPair.first = curPair.second;
		curPair.second = tokenId;

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
	std::list<uint32_t>& splitedWord,
	const uint32_t wordCount,
	const IdPair& maxPair,
	const uint32_t newTokenId,
	const uint32_t wordIndex
)
{
	auto iter = splitedWord.begin();
	IdPair curPair;
	curPair.second = *iter;
	iter = std::next(iter);

	while (iter != splitedWord.end())
	{
		curPair.first = curPair.second;
		curPair.second = *iter;

		// maxPair is present in this word
		if (curPair == maxPair)
		{
			iter--; // points to first element of pair

			// if there is a token before us
			if (iter != splitedWord.begin())
			{
				iter--;
				updateCount(IdPair(*iter, curPair.first), -wordCount, wordIndex);
				updateCount(IdPair(*iter, newTokenId), wordCount, wordIndex);
				iter++;
			}

			// Insert the new token before the first element of the pair
            splitedWord.insert(iter, newTokenId);

            // Erase the original pair (next two elements after the inserted token)
            iter = splitedWord.erase(iter, std::next(iter, 2));

			// if there is a token after the one we inserted
			if (iter != splitedWord.end())
			{
				updateCount(IdPair(curPair.second, *iter), -wordCount, wordIndex);
				updateCount(IdPair(newTokenId, *iter), wordCount, wordIndex);
			}

			curPair.second = newTokenId;
		}
		else
		{
			iter++;
		}
	}
}

//-------------------------------------------------------------------------------------------------

void BPELearner::updateCount(IdPair pair, int32_t count, uint32_t wordIndex)
{
	bool inserted = mMaxHeap.UpSert(pair, count);
	if (count > 0)
		mWhereToUpdate[pair].insert(wordIndex);
}

//-------------------------------------------------------------------------------------------------