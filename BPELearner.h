#pragma once

#include "MaxHeap.h"
#include "PairHasher.h"

#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>

class BPELearner
{
public:

	static constexpr uint32_t InitialVocabSize = 256;
	const char* kEndWord = "</w>";

	BPELearner();

	void Learn(const uint32_t vocabSize, const char* inputFileName);
	void Learn(const uint32_t vocabSize, const std::vector<std::string>& textChunks); // chunks are words splited by regEx

	void Save(const std::string& outputFileName) const;

private:

	using IdPair = std::pair<uint32_t, uint32_t>;
	using MapType = std::unordered_map<std::string_view, uint32_t>;

	std::vector<std::vector<uint32_t>> mSplitedWords;
	std::vector<int32_t> mWordCounts;

	std::unordered_map<uint32_t, std::string> mIdToPair; // Vocabulary, Used for debugging
	std::vector<IdPair> mMergeRules;

	// Map IdPair to set of wordId, set of words that contain this pair.
	std::unordered_map<IdPair, std::unordered_set<uint32_t>, PairHasher> mWhereToUpdate;

	MaxHeap mMaxHeap;

	bool mVerbose = false;

	void internalLearn(const uint32_t vocabSize);

	void countWords(const std::vector<std::string>& textChunks, MapType& wordCount);

	void prepare(const MapType& wordCount);

	void countPairsInWord(
		const uint32_t wordIndex, 
		const std::vector<uint32_t>& splitedWord,
		const uint32_t count
	);


	void replacePairInWord(
		std::vector<uint32_t>& splitedWord,
		const uint32_t wordCount,
		const IdPair& maxPair,
		const uint32_t newTokenId,
		const uint32_t wordIndex
	);

	void updateCount(IdPair pair, int32_t count, uint32_t wordIndex);

};
