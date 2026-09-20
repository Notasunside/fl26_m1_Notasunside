#include "aiws/retrieval_engine.hpp"
#include "aiws/text_processor.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <unordered_map>
#include <vector>

namespace aiws {

//--pretty much the same idea as before, just rounded to a stable number
//--so the ranking doesn't get weird from floating point noise//
double RetrievalEngine::canonical_score(    double some_value   ) {
    const double scale =1e12;
    return std::round(some_value * scale) /scale; //
}

std::vector<SearchResult> RetrievalEngine::search(const std::string& query, int k,
                                                  const std::vector<Chunk>& chunks,
                                                  const CorpusIndex& index) const {
    std::vector<std::string> words = TextProcessor::terms(query);

    if (k < 0) { throw std::invalid_argument("k cannot be negative"); }
    if (k == 0 || chunks.empty()) {  return {};}
    if (words.empty()) { return {}; }
    //--quick dedupe so repeated words don't get counted twice //
    std::sort(words.begin(), words.end());
    words.erase(std::unique(words.begin(), words.end()), words.end());
    const std::size_t uniqueWordCount = words.size();
    //
    struct CandidateInfo {
        double total_score{};
        std::size_t matched_terms{};
    };

    std::unordered_map<std::size_t, CandidateInfo> score_map;
    const double chunk_count = static_cast<double>(chunks.size());

    for (const std::string& query_term : words) {
        const std::vector<CorpusIndex::Posting>* term_postings =index.postings( query_term );
        if (term_postings == nullptr) {continue; }
        
        const double document_frequency =
            static_cast<double>(index.document_frequency(query_term) );
        const double inverse_document_frequency =
            std::log((chunk_count + 1.0) /(document_frequency + 1.0)) +1.0;
            // std::log((chunk_count + 1) /(document_frequency + 1)) + 1; common mistake

        for (const CorpusIndex::Posting& posting : *term_postings ) {
            const double term_frequency = 1.0 + std::log(static_cast<double>(posting.frequency));
            CandidateInfo& candidate = score_map[posting.chunk_index];
            ++candidate.matched_terms;
            candidate.total_score += term_frequency * inverse_document_frequency;
        }
    }

    std::vector<SearchResult> results;
    results.reserve(score_map.size() );

    for (const auto& entry : score_map) {
        const std::size_t chunk_index =entry.first;
        const CandidateInfo& candidate =entry.second;
         const Chunk& current_chunk = chunks[ chunk_index ];

        const double coverage_boost =
            1.0 + 0.10 * static_cast<double>(candidate.matched_terms) /
                static_cast<double>(uniqueWordCount);
        //
        SearchResult result;
        result.matched_terms = candidate.matched_terms;
        result.chunk_id = current_chunk.id;
        result.document_id =current_chunk.document_id;
      result.chunk_sequence = current_chunk.sequence;
        result.text = current_chunk.text;
        result.score= canonical_score(candidate.total_score * coverage_boost);

        results.push_back(result);
    }

    std::sort(results.begin(), results.end(), [&chunks, &index](const SearchResult& left, const SearchResult& right) {
        if (left.score != right.score) {
            return left.score > right.score;
        }

        const std::size_t left_index = index.chunk_index(left.chunk_id);
        const std::size_t right_index = index.chunk_index(right.chunk_id);
        const Chunk& left_chunk = chunks[left_index];
        const Chunk& right_chunk= chunks[right_index];

        if (left_chunk.document_order != right_chunk.document_order) {
     return left_chunk.document_order <right_chunk.document_order;
        }

        return left_chunk.sequence < right_chunk.sequence;
    }); //bro

    if (results.size() > static_cast<std::size_t>(k)) {
        results.resize(static_cast<std::size_t>(k));
    }

    return results;
}

}  // namespace aiws
