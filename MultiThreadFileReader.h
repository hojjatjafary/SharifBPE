#pragma once

#include <string>
#include <utility>  // For std::pair
#include <unordered_map>
#include <memory>

class MultiThreadFileReader
{
public:

	MultiThreadFileReader();
	~MultiThreadFileReader();

	void ReadText(const std::string& fileName, std::unordered_map<std::string_view, uint32_t>& wordCount);

private:
		
	using MapType = std::unordered_map<std::string_view, uint32_t>;
	using IntPair = std::pair<uint32_t, uint32_t>;

	std::unique_ptr<class MemoryMappedFile> mMappedFile;

	size_t goToLineEnd(char* data, size_t fileSize, size_t startFrom);

	void readFileSection(
		const char* data,
		const IntPair& fileSection,
		MapType& outWordCount,
		size_t& outTotalWords
	);

	void PCRETokenize(
		const char* data,
		const IntPair& fileSection,
		MapType& outWordCount,
		size_t& outTotalWords
	);

	void STDRegexTokenize(
		const char* data,
		const IntPair& fileSection,
		MapType& outWordCount,
		size_t& outTotalWords
	);

	void SimpleTokenize(
		const char* data,
		const IntPair& fileSection,
		MapType& outWordCount,
		size_t& outTotalWords
	);
};