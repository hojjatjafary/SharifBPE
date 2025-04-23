#pragma once

#include "PairHasher.h"

#include <string>
#include <vector>
#include <unordered_map>
#include <utility>  // For std::pair
#include <list>
#include <memory>

class BPETokenizer
{
public:

	static constexpr uint32_t InitialVocabSize = 256;

	BPETokenizer();
	~BPETokenizer();

	void ReadModel(const std::string& modelFileName);

	void Encode(const std::string& text);

	void EncodeFile(const std::string& inputFileName, const std::string& outputFileName);

	void Encode(const std::vector<std::string_view>& inputWords, std::vector<std::vector<uint32_t>>& outResult);
	
private:

	using IdPair = std::pair<uint32_t, uint32_t>;

	std::unordered_map<uint32_t, std::string> mIdToPair; // Vocabulary, Used for debugging
	std::unordered_map<std::string_view, uint32_t> mVocabulary;
	std::unordered_map<IdPair, uint32_t, PairHasher> mMergeRules;

	const uint8_t FileReadThreadCount;
	const uint8_t EncodeThreadCount;

	std::unique_ptr<class MemoryMappedFile> mMappedFile;

	std::vector<uint32_t> encodeWord(const std::string_view& word);

	void encodeAllWords(
		const std::vector<std::string_view>& words,
		const IdPair& fileSection, 
		std::vector<std::vector<uint32_t>>& outResult
	);

	void encodeWord(std::vector<uint32_t>& splitedWord);

	// --- Read file methods ---

	void readFile(const std::string& fileName, std::vector<std::string_view>& outAllWords);

	size_t goToLineEnd(char* data, size_t fileSize, size_t startFrom);

	void readFileSection(
		const char* data,
		const IdPair& fileSection,
		std::vector<std::string_view>& outWords,
		size_t& outTotalWords
	);

	void PCRETokenize(
		const char* data,
		const IdPair& fileSection,
		std::vector<std::string_view>& outWords,
		size_t& outTotalWords
	);

	void STDRegexTokenize(
		const char* data,
		const IdPair& fileSection,
		std::vector<std::string_view>& outWords,
		size_t& outTotalWords
	);

	void SimpleTokenize(
		const char* data,
		const IdPair& fileSection,
		std::vector<std::string_view>& outWords,
		size_t& outTotalWords
	);
};