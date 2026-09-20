#include "aiws/processing_core.hpp"

#include "aiws/chunker.hpp"
#include "aiws/context_builder.hpp"
#include "aiws/corpus_index.hpp"
#include "aiws/retrieval_engine.hpp"
#include "aiws/text_processor.hpp"

#include <stdexcept>
#include <unordered_set>
#include <utility>

namespace aiws {

struct ProcessingCore::Impl {
    // TODO: define the internal state used by the processing core.
    std::vector<Chunk> storedChunsks;
    CorpusIndex corpusIndex;
    RetrievalEngine retrievalEngine;
    ContextBuilder contextBuilder;
};

ProcessingCore::ProcessingCore() : impl_(std::make_unique<Impl>()) { }
ProcessingCore::~ProcessingCore() = default;
ProcessingCore::ProcessingCore(ProcessingCore&&) noexcept = default;

ProcessingCore& ProcessingCore::operator=(ProcessingCore&&) noexcept = default;

std::string ProcessingCore::normalize(const std::string& text) {
    return TextProcessor::normalize(text);
}

void ProcessingCore::rebuild(const Workspace& workspace ) {
    // TODO: rebuild the processing state from the workspace.
    const std::vector<Document>& workspaceDocs = workspace.documents();
    //check doc ids before replacing corpus states at all
    std::unordered_set<std::string> docIds;
    for (const Document& currentDoc : workspaceDocs){
        bool inserted = docIds.insert(currentDoc.id()).second;
        if (!inserted) { throw std::invalid_argument("duplicate doc id: " + currentDoc.id()); }

}
    //--build new state separately//
    auto replacementState = std::make_unique<Impl>();
    ChunkingPolicy chunkingPolicy;
        chunkingPolicy.max_tokens = kMaxChunkTokens;
        chunkingPolicy.overlap = kChunkOverlap;
        chunkingPolicy.paragraph_window = kParagraphPreferenceWindow;
    Chunker documentChunker(chunkingPolicy);
    for (std::size_t documentPosition = 0; documentPosition <workspaceDocs.size();++documentPosition){
        const Document& currentDoc = workspaceDocs[documentPosition];
        std::vector<Chunk> docChunks = documentChunker.chunk(currentDoc, documentPosition);
        replacementState->storedChunsks.insert(replacementState->storedChunsks.end(), docChunks.begin(), docChunks.end());
    }

    replacementState->corpusIndex.build(replacementState->storedChunsks);
    impl_ = std::move(replacementState);
}

const std::vector<Chunk>& ProcessingCore::chunks() const noexcept {
    // static const std::vector<Chunk> empty;

    // TODO: return the chunks currently stored by the processing core.
    return impl_->storedChunsks;
}

std::size_t ProcessingCore::chunk_count() const noexcept {
    // TODO: return the number of stored chunks.
    return impl_->storedChunsks.size();
}

std::size_t ProcessingCore::document_frequency(const std::string& term ) const {
    // TODO: return the document frequency for the requested term.
    std::vector<std::string> normalTerms = TextProcessor::terms(term);
    if(normalTerms.empty()){
    return 0;
    }
    //--
    if(normalTerms.size() > 1){
        throw std::invalid_argument("requires one term");
    }

    return impl_ -> corpusIndex.document_frequency(normalTerms.front());

}

std::size_t ProcessingCore::term_frequency(const std::string& term,
                                           const std::string& chunkId) const {
    // TODO: return the term frequency for the requested chunk.
    std::vector<std::string> normalTerms = TextProcessor::terms(term);

    if (normalTerms.empty()) { return 0;}
     if(normalTerms.size() > 1){
        throw std::invalid_argument("requires one term");
    }

    return impl_->corpusIndex.term_frequency(normalTerms.front(), chunkId);
}

std::vector<SearchResult> ProcessingCore::search(const std::string& query, int k) const {
    // TODO: return the ranked results for the requested query.
    return impl_->retrievalEngine.search(query, k, impl_->storedChunsks, impl_->corpusIndex);
}

std::vector<ContextItem> ProcessingCore::build_context(const std::string& query,
                                                       int k ,
                                                       std::size_t token_budget) const {
    // TODO: build bounded context for the requested query.
    //--//
    std::vector<SearchResult> rankedResults=search( query,k );
    return impl_->contextBuilder.build(rankedResults,token_budget);
}

}  // namespace aiws
