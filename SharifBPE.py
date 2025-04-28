
import os
import ctypes
import ctypes.util
from typing import List
from functools import wraps
from enum import *

#--------------------------------------------------------------------------------------------------
# Load library
if os.name == "posix":
    path = os.path.dirname(os.path.abspath(__file__)) + "/SharifBPELib_shared.so"
    try:
        _lib = ctypes.cdll.LoadLibrary(path)
    except OSError:
        raise ImportError('Could not load SharifBPELib_shared at "%s"' % path)
elif os.name == "nt":
    relative_path = (
        "\\build\\Release\\SharifBPELib_shared.dll"
    )
    absolute_path = os.path.dirname(__file__) + relative_path
    try:
        _lib = ctypes.CDLL(absolute_path)
    except:
        raise ImportError("Could not load SharifBPELib_shared, make sure it is installed")
else:
    raise NotImplementedError("SharifBPELib_shared is not supported on your platform")

#--------------------------------------------------------------------------------------------------
# Set up function prototypes
_lib.BPELearner_create.restype = ctypes.c_void_p
_lib.BPELearner_destroy.argtypes = [ctypes.c_void_p]

_lib.BPELearner_LearnFromFile.restype = ctypes.c_void_p
_lib.BPELearner_LearnFromFile.argtypes = [ctypes.c_uint, ctypes.c_char_p]

_lib.BPELearner_LearnFromChunk.restype = None
_lib.BPELearner_LearnFromChunk.argtypes = [
    ctypes.c_void_p,
    ctypes.c_uint, 
    ctypes.POINTER(ctypes.c_char_p),
    ctypes.c_size_t,
]

_lib.BPELearner_Save.restype = ctypes.c_void_p
_lib.BPELearner_Save.argtypes = [ctypes.c_void_p, ctypes.c_char_p]

#--------------------------------------------------------------------------------------------------

class BPELearner:
    def __init__(self):
        self.obj = _lib.BPELearner_create()

    def __del__(self):
        _lib.BPELearner_destroy(self.obj)

    def Learn(self, vocabSize, inputFileName):
        _lib.BPELearner_LearnFromFile(self.obj, vocabSize, inputFileName)

    def Learn(self, vocabSize, textChunks: List[str]):
        # Convert Python list to C array
        c_strings = (ctypes.c_char_p * len(textChunks))()
        for i, s in enumerate(textChunks):
            c_strings[i] = s.encode('utf-8')
        
        _lib.BPELearner_LearnFromChunk(self.obj, vocabSize, c_strings, len(textChunks))
       
    def Save(self, outputFileName):
         _lib.BPELearner_Save(self.obj, outputFileName.encode('utf-8'))

#--------------------------------------------------------------------------------------------------
#--------------------------------------------------------------------------------------------------
# Set up function prototypes
_lib.BPETokenizer_create.restype = ctypes.c_void_p
_lib.BPETokenizer_destroy.argtypes = [ctypes.c_void_p]

_lib.BPETokenizer_ReadModel.restype = ctypes.c_void_p
_lib.BPETokenizer_ReadModel.argtypes = [ctypes.c_void_p, ctypes.c_char_p]

_lib.BPETokenizer_Encode.restype = ctypes.c_void_p
_lib.BPETokenizer_Encode.argtypes = [ctypes.c_void_p, ctypes.c_char_p]

_lib.BPETokenizer_EncodeFile.restype = ctypes.c_void_p
_lib.BPETokenizer_EncodeFile.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_char_p]

_lib.BPETokenizer_EncodeWords.restype = ctypes.c_void_p
_lib.BPETokenizer_EncodeWords.argtypes = [
    ctypes.c_void_p, 
    ctypes.POINTER(ctypes.c_char_p), 
    ctypes.c_size_t,
    ctypes.POINTER(ctypes.POINTER(ctypes.POINTER(ctypes.c_uint32))),
    ctypes.POINTER(ctypes.c_size_t),
    ctypes.POINTER(ctypes.POINTER(ctypes.c_size_t))
]

_lib.BPETokenizer_FreeResult.restype = ctypes.c_void_p
_lib.BPETokenizer_FreeResult.argtypes = [
    ctypes.c_void_p,
    ctypes.POINTER(ctypes.POINTER(ctypes.c_uint32)),
    ctypes.c_size_t,
    ctypes.POINTER(ctypes.c_size_t)
]

#--------------------------------------------------------------------------------------------------

class BPETokenizer:
    def __init__(self):
        self.obj = _lib.BPETokenizer_create()

    def __del__(self):
        _lib.BPETokenizer_destroy(self.obj)

    def ReadModel(self, modelFileName):
        _lib.BPETokenizer_ReadModel(self.obj, modelFileName.encode('utf-8'))

    def Encode(self, text):
        _lib.BPETokenizer_Encode(self.obj, text.encode('utf-8'))

    def EncodeFile(self, inputFileName, outputFileName):
         _lib.BPETokenizer_EncodeFile(self.obj, inputFileName.encode('utf-8'), outputFileName.encode('utf-8'))

    def EncodeWords(self, input_words):

        input_words_c = (ctypes.c_char_p * len(input_words))(*[word.encode('utf-8') for word in input_words])
        num_words = ctypes.c_size_t(len(input_words))
        out_result = ctypes.POINTER(ctypes.POINTER(ctypes.c_uint32))()
        out_num_results = ctypes.c_size_t()
        inner_sizes_ptr = ctypes.POINTER(ctypes.c_size_t)()

        _lib.BPETokenizer_EncodeWords(self.obj, input_words_c, num_words, ctypes.pointer(out_result), ctypes.pointer(out_num_results), ctypes.pointer(inner_sizes_ptr))

        # Convert the C data to Python list of lists
        result = []
        for i in range(out_num_results.value):
            inner_data = [out_result[i][j] for j in range(inner_sizes_ptr[i])]
            result.append(inner_data)

        _lib.BPETokenizer_FreeResult(self.obj, out_result, out_num_results, inner_sizes_ptr)
        return result

#--------------------------------------------------------------------------------------------------


