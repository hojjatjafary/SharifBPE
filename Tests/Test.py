from random import choice, randint
from string import ascii_lowercase
from typing import Optional, Sequence, Dict
import tempfile
import os

import sys
sys.path.append('..')

from SharifBPE import BPELearner 
from SharifBPE import BPETokenizer 


def test_init():
    
    bpeLearner = BPELearner()
    bpeLearner.Learn(256 + 1, ["Hello", "He", "is", "Programmer"])
    bpeLearner.Save("Test.model")

def test_tokenizer():

    bpeTokenizer = BPETokenizer();
    bpeTokenizer.ReadModel("vocab.model");
    bpeTokenizer.Encode("This text will be tokenized!")

    tokens = bpeTokenizer.EncodeWords(["This", "is", "a", "list", "of", "words"])

    print ("Python Results:")
    for token in tokens:
        print (token)

if __name__ == "__main__":
    test_init()
    test_tokenizer()

    
