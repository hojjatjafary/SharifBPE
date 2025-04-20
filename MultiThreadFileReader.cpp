#include "MultiThreadFileReader.h"
#include "MMFile.h"

#define USE_PCRE 1 // [USE_PCRE, STD_REGEX, NO_REGEX]
#define PCRE2_CODE_UNIT_WIDTH 8 // Match the library you linked (8, 16, or 32)
#include <pcre2.h>

#include <thread>
#include <regex>
#include <iostream>

//-------------------------------------------------------------------------------------------------

MultiThreadFileReader::MultiThreadFileReader() = default;

//-------------------------------------------------------------------------------------------------

MultiThreadFileReader::~MultiThreadFileReader() = default;

//-------------------------------------------------------------------------------------------------

void MultiThreadFileReader::ReadText(const std::string& fileName, std::unordered_map<std::string_view, uint32_t>& outWordCount)
{
	mMappedFile = std::make_unique<MemoryMappedFile>(fileName);

	if (!mMappedFile->isValid())
	{
		std::cout << "Mapped file is not valid (likely zero size)." << std::endl;
		// Handle zero-size file case appropriately here if needed
		return;
	}

	std::cout << "File '" << fileName << "' mapped successfully." << std::endl;
	std::cout << "Size: " << mMappedFile->getSize() << " bytes" << std::endl;

	// Treat the mapped data as a char array
	char* data = static_cast<char*>(mMappedFile->getData());
	const uint32_t fileSize = mMappedFile->getSize();

	const uint8_t ThreadCount = 4;
	const uint32_t SectionLength = fileSize / ThreadCount;

	auto fileSections = std::vector<IntPair>(ThreadCount);
	auto wordCounts = std::vector<MapType>(ThreadCount);
	auto totalWords = std::vector<size_t>(ThreadCount);

	fileSections[0].first = 0;
	fileSections[0].second = goToLineEnd(data, fileSize, SectionLength);

	for (int i = 1; i < ThreadCount; ++i)
	{
		const uint32_t sectionStart = fileSections[i - 1].second;
		const uint32_t sectionEnd = sectionStart + SectionLength;

		fileSections[i].first = sectionStart;
		fileSections[i].second = goToLineEnd(data, fileSize, sectionEnd);
	}

	std::vector<std::thread> workers;
	workers.reserve(ThreadCount);

	for (int i = 0; i < ThreadCount; ++i)
	{
		workers.emplace_back(
			&MultiThreadFileReader::readFileSection,
			this,
			data,
			std::cref(fileSections[i]),
			std::ref(wordCounts[i]),
			std::ref(totalWords[i])
		);
	}

	// Wait for all threads to finish
	for (auto& worker : workers)
	{
		worker.join();
	}

	uint64_t totalProcessedWords = 0;
	for (int i = 0; i < ThreadCount; ++i)
	{
		auto& currentCount = wordCounts[i];
		for (const auto& pairItem : currentCount)
		{
			outWordCount[pairItem.first] += pairItem.second;
		}

		totalProcessedWords += totalWords[i];
	}

	fprintf(stderr, "Read %llu words (%zu unique) from text file.\n", totalProcessedWords, outWordCount.size());
}

//-------------------------------------------------------------------------------------------------

size_t MultiThreadFileReader::goToLineEnd(char* data, size_t fileSize, size_t startFrom)
{
	if (startFrom >= fileSize)
	{
		return fileSize;
	}

	for (int i = startFrom; i < fileSize; ++i)
	{
		if (data[i] == '\n' || data[i] == EOF)
		{
			return i;
		}
	}

	return -1;
}

//-------------------------------------------------------------------------------------------------

void MultiThreadFileReader::readFileSection(const char* data, const IntPair& fileSection, MapType& outWordCount, size_t& outTotalWords)
{
#if USE_PCRE
	PCRETokenize(data, fileSection, outWordCount, outTotalWords);
#elif STD_REGEX
	STDRegexTokenize(data, fileSection, outWordCount, outTotalWords);
#elif NO_REGEX
	SimpleTokenize(data, fileSection, outWordCount, outTotalWords);
#endif
}

//-------------------------------------------------------------------------------------------------

void MultiThreadFileReader::PCRETokenize(const char* data, const IntPair& fileSection, MapType& outWordCount, size_t& outTotalWords)
{
	// This is equivalent to original GPT-2 pattern but executes faster (r50k)
	const char* pattern =
		R"('(?:[sdmt]|ll|ve|re)| ?\p{L}++| ?\p{N}++| ?[^\s\p{L}\p{N}]++|\s++$|\s+(?!\S)|\s)";

	int errornumber;
	PCRE2_SIZE erroroffset;
	uint32_t compile_options = PCRE2_UCP;
	pcre2_code* re = pcre2_compile(
		(PCRE2_SPTR)pattern,
		PCRE2_ZERO_TERMINATED,
		compile_options,
		&errornumber,
		&erroroffset,
		nullptr
	);

	if (!re)
	{
		PCRE2_UCHAR buffer[256];
		pcre2_get_error_message(errornumber, buffer, sizeof(buffer));
		throw std::runtime_error("Failed to compile pattern: " + std::string((char*)buffer));
	}

	// Compile to native machine code
	int jit_ret = pcre2_jit_compile(re, PCRE2_JIT_COMPLETE);
	if (jit_ret != 0)
	{
		pcre2_code_free(re);
		throw std::runtime_error("JIT compilation failed");
	}

	const char* dataStart = data + fileSection.first;
	const PCRE2_SIZE length = fileSection.second - fileSection.first;
	PCRE2_SIZE start_offset = 0;

	pcre2_match_data* match_data = pcre2_match_data_create_from_pattern(re, nullptr);

	while (start_offset < length)
	{
		int rc = pcre2_match(
			re,
			(PCRE2_SPTR)dataStart,
			length,
			start_offset,
			0,
			match_data,
			nullptr
		);

		if (rc < 0)
		{
			if (rc != PCRE2_ERROR_NOMATCH)
			{
				throw std::runtime_error("PCRE match error!");
			}
			break;
		}

		PCRE2_SIZE* ovector = pcre2_get_ovector_pointer(match_data);
		// Validate match bounds
		if (ovector[0] > ovector[1] || ovector[1] > length || ovector[0] > length)
		{
			throw std::runtime_error("Invalid match bounds!");
		}

		size_t token_len = static_cast<size_t>(ovector[1] - ovector[0]);
		const std::string_view match_view(dataStart + ovector[0], token_len);
		outWordCount[match_view]++;
		outTotalWords++;

		// Prevent infinite loop if no progress
		if (ovector[1] <= start_offset)
		{
			break;
		}
		start_offset = ovector[1];
	}

	pcre2_match_data_free(match_data);
	pcre2_code_free(re);
}

//-------------------------------------------------------------------------------------------------

void MultiThreadFileReader::STDRegexTokenize(const char* data, const IntPair& fileSection, MapType& outWordCount, size_t& outTotalWords)
{
	// Portable alternative without Unicode properties
	const std::regex token_pattern(
		R"('(?:[sdmt]|ll|ve|re)| ?[a-zA-Z]+| ?[0-9]+| ?[^\s\w]| +\n| +$| +|[\n])",
		std::regex_constants::optimize
	);

	const char* start_ptr = data + fileSection.first;
	const char* end_ptr = data + fileSection.second;

	std::cregex_iterator it(start_ptr, end_ptr, token_pattern);
	std::cregex_iterator end_it;
	for (; it != end_it; ++it)
	{
		const auto& match = *it;
		const std::string_view match_view(match[0].first, match[0].length());
		outWordCount[match_view]++;
		outTotalWords++;
	}
}

//-------------------------------------------------------------------------------------------------

void MultiThreadFileReader::SimpleTokenize(const char* data, const IntPair& fileSection, MapType& outWordCount, size_t& outTotalWords)
{
	size_t word_start = fileSection.first;
	for (size_t i = fileSection.first; i < fileSection.second; ++i)
	{
		const char cur_char = data[i];
		if (cur_char == ' ' || cur_char == '\n')
		{
			if (i == word_start)
			{
				word_start = i + 1;
				continue;
			}

			// end of word
			const std::string_view match_view(data + word_start, i - word_start);
			outWordCount[match_view]++;
			outTotalWords++;
			word_start = i + 1;
		}
	}

	// Add the last word if there's one
	if (word_start < fileSection.second)
	{
		const std::string_view match_view(data + word_start, fileSection.second - word_start);
		outWordCount[match_view]++;
		outTotalWords++;
	}
}

//-------------------------------------------------------------------------------------------------