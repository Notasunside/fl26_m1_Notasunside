#include "aiws/chunker.hpp"
#include "aiws/context_builder.hpp"
#include "aiws/corpus_index.hpp"
#include "aiws/processing_core.hpp"
#include "aiws/retrieval_engine.hpp"
#include "aiws/text_processor.hpp"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

//
//the spec says they should address text processing, chunk boundaries/overlap,
// corpus index/rebuild behavior, ranking/deterministic ordering, 
// context budgets, and a multi-document end-to-end scenario. 
//It also says at least three component boundaries must be tested directly,
// rather than only calling ProcessingCore.


namespace {

int failures = 0;
void check(bool condition, const std::string& message) 
{
    if (!condition) { ++failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

std::string makeNumberedWords(int amount ) 
{
    std::string text;
    for (int wordNumber = 0; wordNumber <amount; ++wordNumber) {
        if (!text.empty()) { text += ' ';}
        text += "word" + std::to_string(wordNumber );
    } return text;
}
} // namespace


int main() {
    using namespace aiws;
    // ---------------------------------------------------------
    // Test Uno: TextProcessor directly
    ////////////////////////////////////////////////////////

    check(
        TextProcessor::normalize("R2-D2...SHABOOYA!!!") ==
        "r2 d2 shabooya",
        "my test processor normalizes letters, digits, and separators"
    );
    check(
        TextProcessor::normalize("!!! --- \t").empty(),
        "my text processor handles separator only input"
    );


    // ---------------------------------------------------------
    // Test due chunker directly
    ////////////////////////////////////////////////////////

    ChunkingPolicy chunkingPolicy;
    chunkingPolicy.max_tokens = 120;
    chunkingPolicy.overlap = 20;
    chunkingPolicy.paragraph_window = 20;
    Chunker documentChunker(chunkingPolicy);
    Document longDocument{
        "longDoc",
        "Long Document",
        makeNumberedWords(121)
    };

    std::vector<Chunk> longDocumentChunks = documentChunker.chunk( longDocument, 0);

    check(
        longDocumentChunks.size() == 2,
        "my cunker creates two chunks for 121 tokens"
    );

    check(
        longDocumentChunks.size() == 2 &&
        longDocumentChunks[0].token_count == 120 &&
        longDocumentChunks[1].token_count == 21,
        "my chunker preserves 20 token overlap"
    );


    // ---------------------------------------------------------
    // Test tre corpusIndex directly
    ////////////////////////////////////////////////////////

    Chunk firstChunk;
    firstChunk.id = "docA#0";
    firstChunk.document_id = "docA";
    firstChunk.document_order = 0;
    firstChunk.sequence = 0;
    firstChunk.text = "splatoon splatoon control";
    firstChunk.token_count = 3;

    Chunk secondChunk;
    secondChunk.id = "docB#0";
    secondChunk.document_id = "docB";
    secondChunk.document_order = 1;
    secondChunk.sequence = 0;
    secondChunk.text = "splatoon sensor";
    secondChunk.token_count = 2;

    std::vector<Chunk> indexedChunks{  firstChunk, secondChunk };
    CorpusIndex corpusIndex(indexedChunks);

    check(
        corpusIndex.document_frequency("splatoon") == 2,
        "coprus index DF counts chunks containing term"
    );

    check(
        corpusIndex.term_frequency("splatoon", "docA#0") == 2,
        "corpus index stores repeated term frequency"
    );

    check(
        corpusIndex.term_frequency("sensor", "missing#0") == 0,
        "corpus index returns zero for missing chunk"
    );


    // ---------------------------------------------------------
    // Test quattro retrievalEngine directly
    ////////////////////////////////////////////////////////

    RetrievalEngine retrievalEngine;

    std::vector<SearchResult> retrievalResults =
        retrievalEngine.search(
            "splatoon control",
            10,
            indexedChunks,
            corpusIndex
        );

    check(
        retrievalResults.size() == 2,
        "my retrievalEngine returns candidate union!"
    );

    check(
        !retrievalResults.empty() &&
        retrievalResults[0].document_id == "docA",
        "my retrievalEngine ranks stronger match 1st!"
    );


    // ---------------------------------------------------------
    // Test cinque: contextBuilder 
    ////////////////////////////////////////////////////////

    ContextBuilder contextBuilder;
    std::vector<ContextItem> limitedContext = contextBuilder.build( retrievalResults, 2 );

    check(
        limitedContext.size() == 1,
        "my context builder stops after truncated result"
    );

    check(
        !limitedContext.empty() &&
        limitedContext[0].token_count == 2 &&
        limitedContext[0].truncated,
        "my context builder respects token budget"
    );


    // ---------------------------------------------------------
    // Test sei processingCore rebuild removes stale state
    ////////////////////////////////////////////////////////

    Workspace firstWorkspace;

    firstWorkspace.add_document
    (
        Document{
            "old",
            "Old Document",
            "potatoes molasses" //over the garden wall reference
        }
    );

    ProcessingCore processingCore;
    processingCore.rebuild(firstWorkspace);
    
    Workspace replacementWorkspace;
    check(
        processingCore.document_frequency("potatoes") == 1,
        "my initial rebuild indexes first workspace"
    );

    replacementWorkspace.add_document(
        Document{
            "new",
            "New Document",
            "deez nuts"
        }
    );

    processingCore.rebuild(replacementWorkspace);
    check(
        processingCore.document_frequency("potatoes") == 0,
        "my rebuild removes stale corpus information"
    );

    check(
        processingCore.document_frequency("deez") == 1,
        "my rebuild indexes replacement workspace"
    );


    // ---------------------------------------------------------
    // Test sette duplicate document IDs throw
    ////////////////////////////////////////////////////////

    Workspace duplicateWorkspace;
    duplicateWorkspace.add_document( Document{"same", "First", "alpha"} );
     duplicateWorkspace.add_document(    Document{"same", "Second", "beta"} );
    bool duplicateIdThrew = false;
    try {  processingCore.rebuild(duplicateWorkspace); }
    catch (const std::invalid_argument&) {   duplicateIdThrew = true; }

    check(
        duplicateIdThrew,
        "multiple/dupe document IDs throw invalid_argument"
    );

    //--previous valid corpus should still exist after failed rebuild//
    check(
        processingCore.document_frequency("deez") == 1,
        "failed rebuild preserves previous corpus"
    );


    // ---------------------------------------------------------
    // Test otto: multitoken frequency argument is invld
    ////////////////////////////////////////////////////////
    bool multipleTermsThrew = false;

    try { (void)processingCore.document_frequency(  "deez nuts" );}
    catch (const std::invalid_argument&) {multipleTermsThrew = true; }
    check(
        multipleTermsThrew,
        "my multi token frequency argument throws invalid_argument"
    );


    // ---------------------------------------------------------
    // Revealing shortcomings
    ///////////////////////////////////////////////////////

    if (failures == 0) {
        std::cout << "All my tests passed!! :) Good student. \n";
        return 0;
    }

    std::cerr
        << failures
        << " my test(s) failed. Bad student, bad student. :((( \n";

    return 1;
}

//Test 1: TextProcessor-  punctuation, capitalization, digits, empty input
// Test 2: Chunker directly
//--     exactly 120 tokens
//--    121 tokens
// Test 3: CorpusIndex directly
//--     DF and TF for repeated terms
// Test 4: rebuild behavior
//--     rebuilding removes stale data
//--     check if duplicate document IDs throw

// Test 5: RetrievalEngine
//--     ranking and other things

// Test 6: ContextBuilder
//--     exact budget and truncation

// Test 7: 
//--     mult documents -> search -> context