#include "aiws/corpus_index.hpp"
#include "aiws/text_processor.hpp"

#include <stdexcept>
#include <unordered_map>

namespace aiws {

    
CorpusIndex::CorpusIndex(const std::vector<Chunk>& chunks) {
    build(chunks);
}

// void CorpusIndex::build(const std::vector<Chunk>&) {
//     // TODO: build the searchable index from the supplied chunks.
// }
void CorpusIndex::build(const std::vector<Chunk>& chunks) 
{
    postings_.clear();
    chunk_by_id_.clear();
    for (std::size_t chunkPosition = 0; chunkPosition <chunks.size(); ++chunkPosition) 
    {
        const Chunk& currentChunk = chunks[     chunkPosition    ];
        //--where is this chunk located and save it//
        chunk_by_id_[   currentChunk.id ] = chunkPosition;
        // turn normalized chunk text into individual terms
         std::vector<std::string> chunkTerms =TextProcessor::terms(    currentChunk.text   );
        //--count num times term occurs//
            std::unordered_map<std::string, std::size_t> termCounts;
        for ( const std::string& currentTerm : chunkTerms ) {  ++termCounts[currentTerm]; }
        //--+1 posting for each distinct term in this chunk//
        for ( const auto& termAndCount :termCounts )
         {
            const std::string& currentTerm =termAndCount.first;
            std::size_t currentFrequency =termAndCount.second;
                postings_[currentTerm].push_back({  chunkPosition, currentFrequency }   );
        }
    }
}


// std::size_t CorpusIndex::document_frequency(
//     const std::string&) const noexcept {
//     // TODO: return how many chunks contain the requested term.
//     return 0;
// }
std::size_t CorpusIndex::document_frequency(const std::string& normalized_term) const noexcept 
{ //
    auto postingLocation = postings_.find(normalized_term);
    if (postingLocation == postings_.end()) { return 0; }
        return postingLocation->second.size();
}


// std::size_t CorpusIndex::term_frequency(
//     const std::string&,
//     const std::string&) const noexcept {
//     // TODO: return the requested term's frequency in the specified chunk.
//     return 0;
// }

// std::size_t CorpusIndex::term_frequency(const std::string& normalized_term, const std::string& chunk_id) const noexcept 
// {
//     auto chunkLocation = chunk_by_id_.find( chunk_id );
//        auto postingLocation = postings_.find( normalized_term );
//        std::size_t requestedChunkIndex = chunkLocation->second;
//     if (chunkLocation == chunk_by_id_.end() ) {return 0; }
//     if (postingLocation == postings_.end()) {return 0;}
//         for (const Posting& currentPosting : postingLocation->second) {
//             if (currentPosting.chunk_index == requestedChunkIndex ) { return currentPosting.frequency;}
//         } //--//
//         return 0;
// }

//--crash fix//
std::size_t CorpusIndex::term_frequency(
    const std::string& normalized_term,
    const std::string& chunk_id) const noexcept {
    auto chunkLocation = chunk_by_id_.find(chunk_id);
     auto postingLocation = postings_.find(normalized_term);
    //--missing chunk ID has frequency 0//
    if (chunkLocation ==chunk_by_id_.end()) { return 0; }
    if (postingLocation == postings_.end()) { return 0;} 
    std::size_t requestedChunkIndex =  chunkLocation->second;

    for (const Posting& currentPosting : postingLocation->second) {
        if (currentPosting.chunk_index ==  requestedChunkIndex) { return currentPosting.frequency;  }
    }

    return 0;
}


// const std::vector<CorpusIndex::Posting>* CorpusIndex::postings(
//     const std::string&) const noexcept {
//     // TODO: return the postings associated with the requested term.
//     return nullptr;
// }


const std::vector<CorpusIndex::Posting>* CorpusIndex::postings( const std::string& normalized_term) const noexcept 
{
    auto postingLocation = postings_.find( normalized_term );
    if (postingLocation ==postings_.end()) { return nullptr; }
        return &postingLocation->second;
}


// const Chunk* CorpusIndex::find_chunk(
//     const std::vector<Chunk>&,
//     const std::string&) const noexcept {
//     // TODO: find the chunk identified by the requested chunk ID.
//     return nullptr;
// }


const Chunk* CorpusIndex::find_chunk( const std::vector<Chunk>& chunks, const std::string& chunk_id ) const noexcept {
    auto chunkLocation =chunk_by_id_.find( chunk_id );
        std::size_t requestedChunkIndex =chunkLocation->second;
    if ( chunkLocation ==chunk_by_id_.end() ) {  return nullptr; }
    if ( requestedChunkIndex >= chunks.size() ) { return nullptr; }
    return &chunks[ requestedChunkIndex ];
}



// std::size_t CorpusIndex::chunk_index(const std::string&) const {
//     // TODO: return the stored index of the requested chunk ID.
//     return 0;
// }

std::size_t CorpusIndex::chunk_index(const std::string& chunk_id) const {
    auto chunkLocation =chunk_by_id_.find( chunk_id );
    if ( chunkLocation== chunk_by_id_.end() ) {throw std::out_of_range("cant find chunk :(((");}
        return chunkLocation->second;
}

}  // namespace aiws


