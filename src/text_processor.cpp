#include "aiws/text_processor.hpp"

#include <algorithm>

namespace aiws {

namespace {

// M1 tokens are built from ASCII letters and digits only.
bool isALetterInASCII(char c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}
bool isADigitInASCII(char c) { return c >= '0' && c <= '9'; }
bool isTokenChar(char c) { return isALetterInASCII(c) || isADigitInASCII(c); }
char lowercaseASCII(char c) {
    if (c >= 'A' && c <= 'Z') {return static_cast<char>(c - 'A' + 'a'); }
    return c;
}

 //--Scan the gap between tokens for a paragraph break//
 //--M1 treats a paragraph boundary as two line endings separated only by spaces or tabs
//
bool containsParagraphBoundary(const std::string& text, std::size_t begin, std::size_t end) {
    bool sawLineEnding = false;
    for (std::size_t ind = begin; ind < end; ++ind) {
        if (text[ind] == '\r' && ind + 1 < end 
            && text[ind + 1] == '\n') {
            if (sawLineEnding) { return true; }
            sawLineEnding = true;
            ++ind; //--skip the '\n' in a CRLF pair //
        }
        else if (text[ind] == '\n') {
            if (sawLineEnding) { return true; }
            sawLineEnding = true;
        }
        //--nonspace sep = blank-line pattern interrupted//
        else if (text[ind] != ' ' && text[ind] != '\t') {
            sawLineEnding = false;
        }
    }    return false;
}

} // namespace


// std::vector<TokenInfo> TextProcessor::tokenize(const std::string&) {
//     // TODO: produce normalized tokens with source and paragraph information.
//     return {};
// }
// token = "hello"
// begin = 0
// end = 5
// paragraph = 0
// token = "world"
// begin = 7
// end = 12
// paragraph = 0
std::vector<TokenInfo> TextProcessor::tokenize(const std::string& text)
 {
    std::size_t separatorStart = 0;
    std::vector<TokenInfo> tokens;
    std::size_t currentParagraph = 0;
    std::size_t cursor = 0;
    while (cursor < text.size()) {
        //--past separators until  next token starts//
        while (cursor < text.size() && !isTokenChar(text[cursor])) { ++cursor; }

        //--iff the gap between tokens includes a blank line  next token starts a new paragraph.
        if (!tokens.empty() && containsParagraphBoundary(text, separatorStart, cursor)) {
            ++currentParagraph;
        }
        if (cursor >= text.size()) {break; }

        std::size_t tokenStart = cursor;
        std::string token;
        while (cursor < text.size() && isTokenChar(text[cursor])) {
            token += lowercaseASCII(text[cursor]);
            ++cursor;
        }
        std::size_t tokenEnd = cursor;
        tokens.push_back({
            token,
            tokenStart,
            tokenEnd,
            currentParagraph
        });
        separatorStart = cursor;
        //-- 
    }
    //--paragrpah count
    return tokens;
}


// std::vector<std::string> TextProcessor::terms(const std::string&) {
//     // TODO: return the normalized terms represented by the input text.
//     return {};
// }

std::vector<std::string> TextProcessor::terms(const std::string& text) {
    std::vector<TokenInfo> tokenInfo = tokenize(text);
    std::vector<std::string> normalizedTerms;
    normalizedTerms.reserve(tokenInfo.size());
    for (const TokenInfo& token : tokenInfo) {
        normalizedTerms.push_back(token.token);
    }
    //-- 
    return normalizedTerms;
}


// std::string TextProcessor::normalize(const std::string&) {
//     // TODO: return the normalized form of the input text.
//     return {};
// }

// std::string TextProcessor::normalize(const std::string& text) {

//     return join(tokenize(text), 0, tokenize(text).size());
// }

std::string TextProcessor::normalize(const std::string& text) {
    std::vector<TokenInfo> tokens = tokenize(text);
    //-- recheck//
    return join(tokens, 0, tokens.size());
}


// std::string TextProcessor::join(const std::vector<TokenInfo>&,
//                                 std::size_t,
//                                 std::size_t) {
//     // TODO: join the requested token range into normalized text.
//     return {};
// }

std::string TextProcessor::join(const std::vector<TokenInfo>& tokens, std::size_t begin, std::size_t end) {
    if (begin >= tokens.size() || begin >= end) {
        return  "";
    }
    std::string joinedText;
    end = std::min(end, tokens.size());
    for (std::size_t ind = begin; ind < end; ++ind) {
        if (!joinedText.empty()) {
            joinedText += ' ';
        }
        joinedText += tokens[ind].token;
    }
    //--simple loop?//
    return joinedText;
}

// std::string TextProcessor::join(const std::vector<std::string>&,
//                                 std::size_t,
//                                 std::size_t) {
//     // TODO: join the requested term range into normalized text.
//     return {};
// }
std::string TextProcessor::join(const std::vector<std::string>& tokens, std::size_t begin, std::size_t end) {
    if (begin >= tokens.size() || begin >= end) {
        return  "";
    }
    std::string joinedText;
    end = std::min(end, tokens.size());
    for (std::size_t ind = begin; ind < end; ++ind) {
        if (!joinedText.empty()) {
            joinedText += ' ';
        }
        joinedText += tokens[ind];
    }
    //-- strings instead of TokenInfo //
    return joinedText;
}



} // namespace aiws







