#include "SharifBPE_API.h"
#include "BPELearner.h"
#include "BPETokenizer.h"

//-------------------------------------------------------------------------------------------------

SHARIF_BPE_API BPELearnerHandle BPELearner_create()
{
	auto* aBPELearner = new BPELearner();
	return static_cast<BPELearnerHandle>(aBPELearner);
}

//-------------------------------------------------------------------------------------------------

SHARIF_BPE_API void BPELearner_destroy(BPELearnerHandle handle)
{
	auto* aBPELearner = static_cast<BPELearner*>(handle);
	delete aBPELearner;
}

//-------------------------------------------------------------------------------------------------

SHARIF_BPE_API void BPELearner_LearnFromFile(BPELearnerHandle handle, const unsigned int vocabSize, SharifBPE_ConstStr inputFileName)
{
	auto* aBPELearner = static_cast<BPELearner*>(handle);
	aBPELearner->Learn(vocabSize, inputFileName);
}

//-------------------------------------------------------------------------------------------------

SHARIF_BPE_API void BPELearner_LearnFromChunk(BPELearnerHandle handle, const unsigned int vocabSize, SharifBPE_ConstStr* textChunks, size_t count)
{
	auto* aBPELearner = static_cast<BPELearner*>(handle);

	std::vector<std::string> textChunksVec;
	for (size_t i = 0; i < count; ++i) 
	{
		textChunksVec.emplace_back(textChunks[i]);
	}

	aBPELearner->Learn(vocabSize, textChunksVec);
}

//-------------------------------------------------------------------------------------------------

SHARIF_BPE_API void BPELearner_Save(BPELearnerHandle handle, SharifBPE_ConstStr outputFileName)
{
	auto* aBPELearner = static_cast<BPELearner*>(handle);
	aBPELearner->Save(outputFileName);
}

//=================================================================================================

SHARIF_BPE_API BPETokenizerHandle BPETokenizer_create()
{
	auto* aBPETokenizer = new BPETokenizer();
	return static_cast<BPETokenizerHandle>(aBPETokenizer);
}

//-------------------------------------------------------------------------------------------------

SHARIF_BPE_API void BPETokenizer_destroy(BPETokenizerHandle handle)
{
	auto* aBPETokenizer = static_cast<BPETokenizer*>(handle);
	delete aBPETokenizer;
}

//-------------------------------------------------------------------------------------------------

SHARIF_BPE_API void BPETokenizer_ReadModel(BPETokenizerHandle handle, SharifBPE_ConstStr modelFileName)
{
	auto* aBPETokenizer = static_cast<BPETokenizer*>(handle);
	aBPETokenizer->ReadModel(modelFileName);
}

//-------------------------------------------------------------------------------------------------

SHARIF_BPE_API void BPETokenizer_Encode(BPETokenizerHandle handle, SharifBPE_ConstStr text)
{
	auto* aBPETokenizer = static_cast<BPETokenizer*>(handle);
	aBPETokenizer->Encode(text);
}

//-------------------------------------------------------------------------------------------------

SHARIF_BPE_API void BPETokenizer_EncodeFile(BPETokenizerHandle handle, SharifBPE_ConstStr inputFileName, SharifBPE_ConstStr outputFileName)
{
	auto* aBPETokenizer = static_cast<BPETokenizer*>(handle);
	aBPETokenizer->EncodeFile(inputFileName, outputFileName);
}

//-------------------------------------------------------------------------------------------------

SHARIF_BPE_API void BPETokenizer_EncodeWords(
	BPETokenizerHandle handle, 
	SharifBPE_ConstStr* inputWords, 
	size_t numWords, 
	uint32_t*** outResult, 
	size_t* outNumResults,
	size_t** innerResultSizes
)
{
	auto* aBPETokenizer = static_cast<BPETokenizer*>(handle);

	std::vector<std::string_view> words;
	for (size_t i = 0; i < numWords; ++i) 
	{
		words.emplace_back(inputWords[i]);
	}

	std::vector<std::vector<uint32_t>> result;

	aBPETokenizer->Encode(words, result);

	const size_t resultSize = result.size();
	// Allocate memory for outer vector
	*outNumResults = resultSize;
	*outResult = new uint32_t * [resultSize];
	*innerResultSizes = new size_t[resultSize];

	// Allocate memory for each inner vector and copy data
	for (size_t i = 0; i < result.size(); ++i) 
	{
		(*innerResultSizes)[i] = resultSize;
		(*outResult)[i] = new uint32_t[resultSize];
		std::copy(result[i].begin(), result[i].end(), (*outResult)[i]);
	}
}

//-------------------------------------------------------------------------------------------------

SHARIF_BPE_API void BPETokenizer_FreeResult(BPETokenizerHandle handle, uint32_t*** result, size_t outNumResults, size_t** innerResultSizes)
{
	for (size_t i = 0; i < outNumResults; ++i) 
	{
		delete[] result[i];
	}
	delete[] result;
	delete[] innerResultSizes;
}

//-------------------------------------------------------------------------------------------------
