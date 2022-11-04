#include <libfts/search.hpp>

#include <fmt/format.h>

#include <algorithm>
#include <cmath>

namespace libfts {

static ScoreTable sort_by_score(
    const std::map<DocId, double> &score_table, IndexAccessor &index) {
    ScoreTable sorted_score_table;
    for (const auto &[document_id, score] : score_table) {
        sorted_score_table.push_back(
            {document_id, score, index.get_document_by_id(document_id)});
    }
    std::sort(
        sorted_score_table.begin(),
        sorted_score_table.end(),
        [](const auto &lhs, const auto &rhs) {
            return lhs.score_ != rhs.score_
                ? lhs.score_ > rhs.score_
                : lhs.document_id_ < rhs.document_id_;
        });
    return sorted_score_table;
}

std::string get_string_search_result(const ScoreTable &score_table) {
    std::string result = "\tid\tscore\ttext\n";
    for (const auto &[id, score, text] : score_table) {
        result += fmt::format("\t{}\t{}\t{}\n", id, score, text);
    }
    return result;
}

ScoreTable search(
    const std::string &query,
    const ParserConfiguration &config,
    IndexAccessor &index) {
    auto parsed_query = parse(query, config);
    std::map<DocId, double> score;
    const auto total_doc_count =
        static_cast<double>(index.get_document_count());
    for (const auto &word : parsed_query) {
        for (const auto &term : word.ngrams_) {
            auto docs = index.get_documents_by_term(term);
            auto doc_freq = static_cast<double>(docs.size());
            for (const auto &identifier : docs) {
                auto term_freq = static_cast<double>(
                    index.get_term_positions_in_document(term, identifier)
                        .size());
                score[identifier] +=
                    term_freq * log(total_doc_count / doc_freq);
            }
        }
    }
    return sort_by_score(score, index);
}

} // namespace libfts