//======================================================================
// Sharif BPE API
//======================================================================
#pragma once

#include <stdint.h>

#if !defined(_SHARIF_BPE_API_HEADER_)
#define _SHARIF_BPE_API_HEADER_

//======================================================================

#if defined(__cplusplus)
extern "C" {
#endif

//======================================================================

#if defined(SHARIF_BPE_SHARED)
	#if defined(_WIN32)
		#if SHARIF_BPE_BUILDING_DLL
			#define SHARIF_BPE_API	__declspec(dllexport)
		#else
			#define SHARIF_BPE_API	__declspec(dllimport)
		#endif
	#else
		#define SHARIF_BPE_API	/**/
	#endif
#else
	#define SHARIF_BPE_API	/**/
#endif

//----------------------------------------------------------------------

#define SHARIF_BPE_API_REVISION			1

//----------------------------------------------------------------------

#define SHARIF_BPE_MAX_PATH				1024

//----------------------------------------------------------------------

SHARIF_BPE_API
typedef char SharifBPE_Byte;

SHARIF_BPE_API
typedef char SharifBPE_Char;

SHARIF_BPE_API
typedef SharifBPE_Char const * SharifBPE_ConstStr;

//======================================================================

//----------------------------------------------------------------------
// BPELearner
//----------------------------------------------------------------------

// Opaque pointer to hide C++ implementation details
typedef void* BPELearnerHandle;

// Constructor and destructor
SHARIF_BPE_API BPELearnerHandle BPELearner_create();
SHARIF_BPE_API void BPELearner_destroy(BPELearnerHandle handle);

// Member functions
SHARIF_BPE_API void BPELearner_LearnFromFile(BPELearnerHandle handle, const unsigned int vocabSize, SharifBPE_ConstStr inputFileName);
SHARIF_BPE_API void BPELearner_LearnFromChunk(BPELearnerHandle handle, const unsigned int vocabSize, SharifBPE_ConstStr* textChunks, size_t count); // chunks are words splited by regEx
SHARIF_BPE_API void BPELearner_Save(BPELearnerHandle handle, SharifBPE_ConstStr outputFileName);

//----------------------------------------------------------------------
// BPETokenizer
//----------------------------------------------------------------------
// 
// Opaque pointer to hide C++ implementation details
typedef void* BPETokenizerHandle;

// Constructor and destructor
SHARIF_BPE_API BPETokenizerHandle BPETokenizer_create();
SHARIF_BPE_API void BPETokenizer_destroy(BPETokenizerHandle handle);

// Member functions
SHARIF_BPE_API void BPETokenizer_ReadModel(BPETokenizerHandle handle, SharifBPE_ConstStr modelFileName);
SHARIF_BPE_API void BPETokenizer_Encode(BPETokenizerHandle handle, SharifBPE_ConstStr text);
SHARIF_BPE_API void BPETokenizer_EncodeFile(BPETokenizerHandle handle, SharifBPE_ConstStr inputFileName, SharifBPE_ConstStr outputFileName);
SHARIF_BPE_API void BPETokenizer_EncodeWords(BPETokenizerHandle handle, SharifBPE_ConstStr* inputWords, size_t numWords, uint32_t*** outResult, size_t* outNumResults, size_t** innerResultSizes);
SHARIF_BPE_API void BPETokenizer_FreeResult(BPETokenizerHandle handle, uint32_t*** result, size_t outNumResults, size_t** innerResultSizes);

//======================================================================

#if defined(__cplusplus)
}
#endif

#endif	//_SHARIF_BPE_API_HEADER_
