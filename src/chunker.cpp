#include "aiws/chunker.hpp"
#include "aiws/text_processor.hpp"

#include <stdexcept>

namespace aiws {

Chunker::Chunker(ChunkingPolicy policy) : policy_(policy) {
    if (policy_.max_tokens == 0 || policy_.overlap >= policy_.max_tokens ||
        policy_.paragraph_window > policy_.max_tokens) {
        throw std::invalid_argument("invalid chunking policy");
    }
}


//

// maximum 120 tokens
// 20-token overlap
// if a paragraph boundary occurs in the last 20 tokens before the limit, prefer the latest one
// chunk IDs are document-id#sequence
// sequence starts at 0
// preserve document_order
// save normalized chunk text, token count, and original source-character positions
// empty documents produce no chunks

// for 121 tokens
// Chunk #0:
// tokens 0–119
// 120 tokens
// Chunk #1:
// tokens 100–120
// 21 tokens

std::vector<Chunk> Chunker::chunk(const Document& document, std::size_t document_order) const 
{
    std::vector<Chunk> generatedChunks;
    std::vector<TokenInfo> normalizedTokens = TextProcessor::tokenize(document.text());
        std::size_t chunkStartInd = 0;
        std::size_t chunkSequence = 0;
    //--empty document produces no chunks//
        if (normalizedTokens.empty()) { return generatedChunks;}
    while (chunkStartInd < normalizedTokens.size()) 
    {
        std::size_t tokensRemaining = normalizedTokens.size() - chunkStartInd;
        std::size_t chunkEndInd;
        //--last chunk contains all remaining tokens//
        if (tokensRemaining <= policy_.max_tokens) { chunkEndInd = normalizedTokens.size(); }
        else 
        {
            //--default to the hard maximum //
            chunkEndInd = chunkStartInd + policy_.max_tokens;
            //--bstart of the paragraph-preference window//
                std::size_t paragraphWindowStart = chunkStartInd + policy_.max_tokens - policy_.paragraph_window;
            //--search backward for the latest paragraph boundary//
            for (std::size_t boundaryIndex = chunkEndInd; boundaryIndex > paragraphWindowStart; --boundaryIndex) 
            {
                if (boundaryIndex < normalizedTokens.size() && normalizedTokens[boundaryIndex].paragraph != normalizedTokens[boundaryIndex - 1].paragraph) 
                {
                    //--can next chunk still move forwar?//
                    if (boundaryIndex - chunkStartInd > policy_.overlap) { chunkEndInd = boundaryIndex; break;}
                }
            }
        }

        Chunk currentChunk;
            currentChunk.id = document.id() + "#" + std::to_string(chunkSequence);
            currentChunk.document_id = document.id();
            currentChunk.document_order = document_order;
        currentChunk.sequence = chunkSequence;
        currentChunk.text =
            TextProcessor::join(normalizedTokens, chunkStartInd,chunkEndInd);
        currentChunk.token_count =chunkEndInd - chunkStartInd;
        currentChunk.source_begin =normalizedTokens[chunkStartInd].begin;
        currentChunk.source_end = normalizedTokens[chunkEndInd - 1].end;
        generatedChunks.push_back(currentChunk);
        //--reach the end of the document//
            if (chunkEndInd == normalizedTokens.size()) { break; }
            // Begin next chunk with the required overlap
            chunkStartInd = chunkEndInd - policy_.overlap;
            ++chunkSequence;
    }

    return generatedChunks;
}


}  // namespace aiws
