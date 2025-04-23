#include "BPETokenizer.h"
#include "MMFile.h"

#define USE_PCRE 1 // [USE_PCRE, STD_REGEX, NO_REGEX]
#define PCRE2_CODE_UNIT_WIDTH 8 // Match the library you linked (8, 16, or 32)
#include <pcre2.h>

#include <limits>
#include <thread>
#include <regex>
#include <iostream>
#include <fstream>

//-------------------------------------------------------------------------------------------------

BPETokenizer::BPETokenizer()
    : FileReadThreadCount(4)
    , EncodeThreadCount(4)
{
    // Initial vocabulary
    std::string oneCharString(" ");
    for (uint16_t ch = 0; ch < InitialVocabSize; ++ch)
    {
        oneCharString[0] = char(ch);
        // We should cast to uchar first and then to uint
        const uint32_t tokenId = static_cast<uint8_t>(ch);

        mIdToPair[tokenId] = oneCharString;
        mVocabulary[mIdToPair[tokenId]] = tokenId;
    }
}

//-------------------------------------------------------------------------------------------------

BPETokenizer::~BPETokenizer() = default;

//-------------------------------------------------------------------------------------------------
// Read merge rules from file.
void BPETokenizer::ReadModel(const std::string& modelFileName)
{
	std::ifstream modelFile(modelFileName);
    if (!modelFile)
    {
        fprintf(stderr, "Cannot open codes file %s\n", modelFileName.c_str());
        exit(EXIT_FAILURE);
    }

    fprintf(stderr, "Loading codes from %s ...\n", modelFileName.c_str());
    
    int rank = InitialVocabSize; // rank is the tokenId of merged ids
    int first, second;
    while (modelFile >> first >> second)
    {
        mMergeRules[IdPair(first, second)] = rank;
        const std::string str = mIdToPair[first] + mIdToPair[second];
        mIdToPair[rank] = str;
        mVocabulary[mIdToPair[rank]] = rank;
        ++rank;
    }

    fprintf(stderr, "Codes Loaded.\n");
}

//-------------------------------------------------------------------------------------------------
// Public API to encode a single word.
void BPETokenizer::Encode(const std::string& text)
{
    // split text to chunks(words)
    auto ret = encodeWord(text);

    for (const auto id : ret)
    {
        std::cout << mIdToPair[id] << ' ' << id << '\n';
    }
}

//-------------------------------------------------------------------------------------------------
// Read entire file and encode each word. Reading and Encoding are multi-threaded.
void BPETokenizer::EncodeFile(const std::string& inputFileName, const std::string& outputFileName)
{
    std::vector<std::string_view> words;
    readFile(inputFileName, words);
    
    std::vector<std::vector<uint32_t>> result;
    Encode(words, result);

    std::ofstream outFile(outputFileName);
    for (const auto& tokens : result)
    {
        for (const auto id : tokens)
        {
            outFile << mIdToPair[id] << ' ' << id << '\n';
        }
    }
}

//-------------------------------------------------------------------------------------------------
// Encode list of word in multi-threaded manner.
void BPETokenizer::Encode(const std::vector<std::string_view>& inputWords, std::vector<std::vector<uint32_t>>& outResult)
{
    uint8_t ThreadCount = EncodeThreadCount;
    const uint32_t inputSize = inputWords.size();
    const uint32_t SectionLength = inputSize / ThreadCount;

    outResult = std::vector<std::vector<uint32_t>>(inputSize);
    auto inputSections = std::vector<IdPair>(ThreadCount);

    inputSections[0].first = 0;
    inputSections[0].second = SectionLength;

    for (int i = 1; i < ThreadCount; ++i)
    {
        const uint32_t sectionStart = inputSections[i - 1].second;
        const uint32_t sectionEnd = sectionStart + SectionLength;

        inputSections[i].first = sectionStart;
        inputSections[i].second = sectionEnd;
    }
    
    // Clamp last input
    if (inputSections[ThreadCount - 1].second > inputSize)
    {
        inputSections[ThreadCount - 1].second = inputSize;
    }

    std::vector<std::thread> workers;
    workers.reserve(ThreadCount);

    for (int i = 0; i < ThreadCount; ++i)
    {
        workers.emplace_back(
            &BPETokenizer::encodeAllWords,
            this,
            std::cref(inputWords),
            std::cref(inputSections[i]),
            std::ref(outResult)
        );
    }

    // Wait for all threads to finish
    for (auto& worker : workers)
    {
        worker.join();
    }

    fprintf(stderr, "Finished encoding threads.\n");
}

//-------------------------------------------------------------------------------------------------
// Encode list of words to list of encoded result.
void BPETokenizer::encodeAllWords(const std::vector<std::string_view>& words, const IdPair& inputSection, std::vector<std::vector<uint32_t>>& outResult)
{
    for (int i = inputSection.first; i < inputSection.second; ++i)
    {
        outResult[i] = encodeWord(words[i]);
    }
}

//-------------------------------------------------------------------------------------------------
// First convert string to a list of integer and then encode it.
std::vector<uint32_t> BPETokenizer::encodeWord(const std::string_view& word)
{
    auto vocabIter = mVocabulary.find(word);
    if (vocabIter != mVocabulary.end())
    {
        return { vocabIter->second };
    }

    std::vector<uint32_t> splitedWord;
    for (const auto& ch : word)
    {
        // We should cast to unsigned char first then uint
        splitedWord.push_back(static_cast<uint8_t>(ch));
    }

    encodeWord(splitedWord);

    return splitedWord;
}

//-------------------------------------------------------------------------------------------------
// Real encode algorithm for each word converted to a list of ids.
// I can use a minHeap and do same as learn algorithm but seems it is overkill and has no gain.
void BPETokenizer::encodeWord(std::vector<uint32_t>& splitedWord)
{
    static const auto mergeRulesEnd = mMergeRules.end();

    while (splitedWord.size() > 1)
    {
        // Min rank is the most frequent pair in the training phase.
        // rank is token id or the order by frequency of pairs in training phase.
        bool minPairFound = false;
        IdPair minRankPair;
        uint32_t minRank = std::numeric_limits< uint32_t>::max();
        for (size_t i = 0; i < splitedWord.size() - 1; ++i)
        {
            const IdPair currentPair(splitedWord[i], splitedWord[i + 1]);
            auto mergeIter = mMergeRules.find(currentPair);
            if (mergeIter != mergeRulesEnd)
            {
                minPairFound = true;
                const uint32_t rank = mergeIter->second;
                if (rank < minRank)
                {
                    minRank = rank;
                    minRankPair = currentPair;
                    // Early exit if the minimal possible rank is found
                    if (minRank == 0)
                    {
                        break;
                    }
                }
            }
        }

        // If no mergeable pairs found, exit
        if (!minPairFound)
        {
            break;
        }

        // Step 2: Replace all occurrences of minPair with its token ID in-place
        size_t write = 0, read = 0;
        while (read < splitedWord.size())
        {
            if (read < splitedWord.size() - 1 &&
                splitedWord[read] == minRankPair.first &&
                splitedWord[read + 1] == minRankPair.second)
            {
                splitedWord[write++] = minRank;
                read += 2;
            }
            else
            {
                splitedWord[write++] = splitedWord[read++];
            }
        }
        splitedWord.resize(write);
    }
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
// Multi-threaded read file, pre-tokenize using regex.
void BPETokenizer::readFile(const std::string& fileName, std::vector<std::string_view>& outAllWords)
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

    const uint8_t ThreadCount = FileReadThreadCount;
    const uint32_t SectionLength = fileSize / ThreadCount;

    auto fileSections = std::vector<IdPair>(ThreadCount);
    auto threadOutWords = std::vector<std::vector<std::string_view>>(ThreadCount);
    auto numProcessedWords = std::vector<size_t>(ThreadCount);

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
            &BPETokenizer::readFileSection,
            this,
            data,
            std::cref(fileSections[i]),
            std::ref(threadOutWords[i]),
            std::ref(numProcessedWords[i])
        );
    }

    // Wait for all threads to finish
    for (auto& worker : workers)
    {
        worker.join();
    }

    // Consolidate results.
    uint64_t totalWords = 0;
    for (int i = 0; i < ThreadCount; ++i)
    {
        totalWords += threadOutWords[i].size();
    }

    outAllWords.reserve(totalWords);
    
    for (int i = 0; i < ThreadCount; ++i)
    {
        outAllWords.insert(
            outAllWords.end(),
            std::make_move_iterator(threadOutWords[i].begin()),
            std::make_move_iterator(threadOutWords[i].end())
        );
    }

    fprintf(stderr, "Read %llu words from text file.\n", totalWords);
}

//-------------------------------------------------------------------------------------------------

size_t BPETokenizer::goToLineEnd(char* data, size_t fileSize, size_t startFrom)
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

void BPETokenizer::readFileSection(const char* data, const IdPair& fileSection, std::vector<std::string_view>& outWords, size_t& outTotalWords)
{
#if USE_PCRE
    PCRETokenize(data, fileSection, outWords, outTotalWords);
#elif STD_REGEX
    STDRegexTokenize(data, fileSection, outWords, outTotalWords);
#elif NO_REGEX
    SimpleTokenize(data, fileSection, outWords, outTotalWords);
#endif
}

//-------------------------------------------------------------------------------------------------
// TODO: here we have duplicated code to MultiThreadFileReader::PCRETokenize
void BPETokenizer::PCRETokenize(const char* data, const IdPair& fileSection, std::vector<std::string_view>& outWords, size_t& outTotalWords)
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

        const size_t token_len = static_cast<size_t>(ovector[1] - ovector[0]);
        outWords.emplace_back(dataStart + ovector[0], token_len);
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
// TODO: here we have duplicated code to MultiThreadFileReader::STDRegexTokenize
void BPETokenizer::STDRegexTokenize(const char* data, const IdPair& fileSection, std::vector<std::string_view>& outWords, size_t& outTotalWords)
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
        outWords.emplace_back(match[0].first, match[0].length());
        outTotalWords++;
    }
}

//-------------------------------------------------------------------------------------------------
// TODO: here we have duplicated code to MultiThreadFileReader::SimpleTokenize
void BPETokenizer::SimpleTokenize(const char* data, const IdPair& fileSection, std::vector<std::string_view>& outWords, size_t& outTotalWords)
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
            outWords.emplace_back(data + word_start, i - word_start);
            outTotalWords++;
            word_start = i + 1;
        }
    }

    // Add the last word if there's one
    if (word_start < fileSection.second)
    {
        outWords.emplace_back(data + word_start, fileSection.second - word_start);
        outTotalWords++;
    }
}

//-------------------------------------------------------------------------------------------------