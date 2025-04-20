#include "BPELearner.h"
#include "BPETokenizer.h"

#include <chrono>

//-------------------------------------------------------------------------------------------------

#define Test 0

int main()
{
    using std::chrono::high_resolution_clock;
    using std::chrono::duration_cast;
    using std::chrono::duration;
    using std::chrono::milliseconds;
    

#if Test
	BPELearner aBPELearner;
    auto t1 = high_resolution_clock::now();
	aBPELearner.Learn(256 + 50 * 1000, "../TinyStoriesV2-GPT4-train.txt");
    //aBPELearner.Learn(256 + 50 * 100, "../wiki_corpus.txt");
    auto t2 = high_resolution_clock::now();
	aBPELearner.Save("vocab.model");
#else
	BPETokenizer aBPETokenizer;
	aBPETokenizer.ReadModel("vocab.model");
    auto t1 = high_resolution_clock::now();
	//aBPETokenizer.Encode("Hello World, compiler tokenizer");
    aBPETokenizer.EncodeFile("../wiki_corpus.txt", "../wikiTokens.txt");
    //aBPETokenizer.EncodeFile("../TinyStories-train.txt", "../TinyStories-Tokens.txt");
    auto t2 = high_resolution_clock::now();
#endif

    auto dur = t2 - t1;
    
    auto ms_int = duration_cast<milliseconds>(dur);
    auto ms_double = duration<double, std::milli>(dur);
    auto secs_f = duration_cast<duration<float>>(dur);

    std::cout << ms_int.count() << " ms" << std::endl;
    std::cout << ms_double.count() << " ms" << std::endl;
    std::cout << secs_f.count() << " secs" << std::endl;

}