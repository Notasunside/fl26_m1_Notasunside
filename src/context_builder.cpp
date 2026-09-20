
#include "aiws/context_builder.hpp"
#include "aiws/text_processor.hpp"
#include <unordered_set>

namespace aiws {


    // std::vector<ContextItem> ContextBuilder::build(const std::vector<SearchResult>&, std::size_t) const {
//     // TODO: assemble ranked results into context items within the supplied token budget.
    
//     return {};
// }
std::vector<ContextItem> ContextBuilder::build( const std::vector<SearchResult>& ranked, std::size_t token_budget) const {
    std::vector<ContextItem> contextItems;
    if (token_budget == 0) {  return contextItems; }
    std::size_t tokensLeft =token_budget;
    std::unordered_set<std::string> seenChunkIds;

    for (const SearchResult& result : ranked) {
        if (seenChunkIds.find(result.chunk_id) != seenChunkIds.end()) {  continue;
        }

        std::vector<std::string> chunkWords = TextProcessor::terms(result.text);
        std::size_t chunkWordCount = chunkWords.size();

        if (chunkWordCount == 0) { continue; }
        ContextItem item;
            item.chunk_id = result.chunk_id;
            item.document_id = result.document_id;
            item.chunk_sequence = result.chunk_sequence;
            item.score = result.score;

        if (chunkWordCount <= tokensLeft) {
            item.text = result.text;
            item.token_count =chunkWordCount;
            item.truncated =false;
                contextItems.push_back( item );
                seenChunkIds.insert( result.chunk_id );
                tokensLeft -=chunkWordCount;

            if (tokensLeft == 0) { break;  }
        } else 
        {
            item.text= TextProcessor::join(chunkWords, 0, tokensLeft);
            item.token_count = tokensLeft;
            item.truncated = true;
            contextItems.push_back( item );
            seenChunkIds.insert( result.chunk_id );
            break;
        }
    }
    return contextItems;
}

}  // namespace aiws