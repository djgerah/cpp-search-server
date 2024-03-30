#include <algorithm>
#include <cmath>
#include <iostream>
#include <numeric>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double EPSILON = 1e-6;

string ReadLine() 
{
    string line;
    getline(cin, line);
    
    return line;
}

int ReadLineWithNumber() 
{
    int result;
    cin >> result;
    ReadLine();
    
    return result;
}

vector<string> SplitIntoWords(const string& text) 
{    
    vector<string> words;
    string word;
    
    for (const char ch : text) 
    {
        if (ch == ' ') 
        {
            if (!word.empty()) 
            {
                words.push_back(word);
                word.clear();
            }
        } 
        else 
        {
            word += ch;
        }
    }
    if (!word.empty()) 
    {
        words.push_back(word);
    }
    return words;
}

struct Document 
{
    Document() = default;

    Document(int id_, double relevance_, int rating_) : id(id_), relevance(relevance_), rating(rating_) {}

    int id = 0;
    double relevance = 0.0;
    int rating = 0;
};

enum class DocumentStatus 
{
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED
};

class SearchServer 
{
public:

    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words)  
    {
        set<string> valid_words;
        
        for (const string& word : stop_words) 
        {
            if (word.empty() || !IsValidWord(word)) 
            {
                throw invalid_argument("Слово не должно содержать спец-символы!"s);  
            }

            valid_words.insert(word);
        }

    stop_words_ = valid_words;  
    }

    explicit SearchServer(const string& stop_words_text) : SearchServer(SplitIntoWords(stop_words_text)) {}

    void AddDocument(int document_id, const string& document, DocumentStatus status, const vector<int>& ratings) 
    {
        if (document_id < 0) 
        {
            throw invalid_argument("Попытка добавить документ с отрицательным ID!"s);
        }

        if (documents_.count(document_id)) 
        {
            throw invalid_argument("Попытка добавить документ c ID ранее добавленного документа!"s);
        }

        if (!IsValidWord(document)) 
        {
            throw invalid_argument("Наличие недопустимых символов в тексте добавляемого документа!"s);
        }

        const vector<string> words = SplitIntoWordsNoStop(document);
        const double inv_word_count = 1.0 / static_cast<double>(words.size());
        
        for (const string& word : words) 
        {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status});
      
        documents_ids_.push_back(document_id);
    }

    template <typename Predicate>
    vector<Document> FindTopDocuments(const string& raw_query,
                                      Predicate status) const 
    {
        const Query query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query, status);
        
        sort(matched_documents.begin(), matched_documents.end(),
             [](const Document& lhs, const Document& rhs) 
             {
                 if (abs(lhs.relevance - rhs.relevance) < EPSILON) 
              /* if (abs(lhs.relevance - rhs.relevance) 
                      < std::numeric_limits<double>::epsilon() */
                 {
                     return lhs.rating > rhs.rating;
                 } 
                     return lhs.relevance > rhs.relevance;
             }); 
        
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) 
        {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }

    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus status) const 
    {
        return FindTopDocuments(raw_query, [&status](int document_id, DocumentStatus new_status, int rating) 
                                                    {
                                                        return new_status == status;
                                                    }
                                );
    }

    vector<Document> FindTopDocuments(const string& raw_query) const 
    {
        return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
    }

    int GetDocumentCount() const 
    {
        return static_cast<int>(documents_.size());
    }

    int GetDocumentId(int index) const 
    {
        if ((index < 0) || (index > GetDocumentCount())) 
        {
            throw out_of_range("Индекс переданного документа выходит за пределы допустимого диапазона!");
        }
        return documents_ids_.at(index);
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query,
                                                        int document_id) const 
    {
        const Query query = ParseQuery(raw_query);
        vector<string> matched_words;
        
        for (const string& word : query.plus_words) 
        {
            if (word_to_document_freqs_.count(word) == 0) 
            {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) 
            {
                matched_words.push_back(word);
            }
        }
        
        for (const string& word : query.minus_words) 
        {
            if (word_to_document_freqs_.count(word) == 0) 
            {
                continue;
            }
            
            if (word_to_document_freqs_.at(word).count(document_id)) 
            {
                matched_words.clear();
                break;
            }
        }
        return {matched_words, documents_.at(document_id).status};
    }

private:

    struct Query 
    {
        set<string> plus_words;
        set<string> minus_words;
    };

    struct DocumentData 
    {
        int rating;
        DocumentStatus status;
    };

    set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;
    map<int, DocumentData> documents_;
    vector<int> documents_ids_;

    bool IsStopWord(const string& word) const 
    {
        return stop_words_.count(word) > 0;
    }

    static bool IsValidWord(const string& word) 
    {
        // Слово не должно содержать спец-символов
        return none_of(word.begin(), word.end(), [](char c) 
        {
            return c >= '\0' && c < ' ';
        });
    }

    vector<string> SplitIntoWordsNoStop(const string& text) const 
    {
        vector<string> words;
        
        for (const string& word : SplitIntoWords(text)) 
        {
            if (!IsStopWord(word)) 
            {
                words.push_back(word);
            }
        }
        return words;
    }

    static int ComputeAverageRating(const vector<int>& ratings) 
        {
            if (ratings.empty()) 
            {
                return 0;
            }
        
            return accumulate(ratings.begin(), ratings.end(), 0) 
                    / static_cast<int>(ratings.size());
        }

    struct QueryWord 
    {
        string data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(string text) const 
    {
        QueryWord result;

        bool is_minus = false;

        if (text[0] == '-') 
        {
            is_minus = true;
            text = text.substr(1);
        }

        if(!IsValidWord(text))
        {
            throw invalid_argument ("В словах поискового запроса есть недопустимые символы!"s);
        }

        if (text[0] == '-')
        {
            throw invalid_argument("Наличие более чем одного минуса перед словами, которых не должно быть в искомых документах!"s);
        }

        if (text.empty()) 
        {
            throw invalid_argument("Отсутствие текста после символа «минус» в поисковом запросе!"s);
        }

        return { text, is_minus, IsStopWord(text) };
    }

    Query ParseQuery(const string& text) const 
    {
        Query query;
        for (const string& word : SplitIntoWords(text)) 
        {
            const QueryWord query_word = ParseQueryWord(word);
            if (!query_word.is_stop) 
            {
                if (query_word.is_minus) 
                {
                    query.minus_words.insert(query_word.data);
                } 
                else 
                {
                    query.plus_words.insert(query_word.data);
                }
            }
        }
        return query;
    }

    double ComputeWordInverseDocumentFreq(const string& word) const 
        {
            return log(GetDocumentCount() * 1.0 
                       / word_to_document_freqs_.at(word).size());
        }

    template <typename Predicate>
    vector<Document> FindAllDocuments(const Query& query, Predicate status) const 
    {
        map<int, double> document_to_relevance;
        vector<Document> matched_documents;
        
        for (const string& word : query.plus_words) 
        {
            if (word_to_document_freqs_.count(word) == 0) 
            {
                continue;
            }
            
            const double inverse_document_freq 
                            = ComputeWordInverseDocumentFreq(word);
            
            for (const auto &[document_id, term_freq] : 
                              word_to_document_freqs_.at(word)) 
            {
                if (status(document_id, documents_.at(document_id).status, 
                           documents_.at(document_id).rating)) 
                {
                    document_to_relevance[document_id] 
                        += term_freq * inverse_document_freq;
                }
            }
        }

        for (const string& word : query.minus_words) 
        {
            if (word_to_document_freqs_.count(word) == 0) 
            {
                continue;
            }
                for (const auto &[document_id, term_freq] : 
                                  word_to_document_freqs_.at(word)) 
                {
                    document_to_relevance.erase(document_id);
                }
        }
        
        for (const auto &[document_id, relevance] : document_to_relevance) 
        {
            matched_documents.push_back({ document_id, relevance, 
                                         documents_.at(document_id).rating });
        }
        return matched_documents;
    }
};

void PrintDocument(const Document& document) 
{
    cout << "{ "s
        << "document_id = "s << document.id << ", "s
        << "relevance = "s << document.relevance << ", "s
        << "rating = "s << document.rating << " }"s << endl;
}

int main() 
{
    SearchServer search_server("и в на"s);

    try {
        search_server.AddDocument(1, "пушистый кот пушистый хвост"s, 
                                  DocumentStatus::ACTUAL, { 7, 2, 7 });
    } catch (const invalid_argument& e) {
        cout << "Ошибка добавления документа: "s << e.what() << endl;
    }
    
    try {
        search_server.AddDocument(1, "пушистый пёс и модный ошейник"s, 
                                  DocumentStatus::ACTUAL, { 1, 2 });
    } catch (const invalid_argument& e) {
        cout << "Ошибка добавления документа: "s << e.what() << endl;
    }
    
    try {
        search_server.AddDocument(-1, "пушистый пёс и модный ошейник"s, 
                                  DocumentStatus::ACTUAL, { 1, 2 });
    } catch (const invalid_argument& e) {
        cout << "Ошибка добавления документа: "s << e.what() << endl;
    }
    
    try {
        search_server.AddDocument(3, "большой пёс скво\x12рец евгений"s, 
                                  DocumentStatus::ACTUAL, { 1, 3, 2 });
    } catch (const invalid_argument& e) {
        cout << "Ошибка добавления документа: "s << e.what() << endl;
    }
    
    try {
        search_server.AddDocument(4, "большой пёс скворец евгений"s, 
                                  DocumentStatus::ACTUAL, { 1, 1, 1 });
    } catch (const invalid_argument& e) {
        cout << "Ошибка добавления документа: "s << e.what() << endl;
    }
    
    try {
        search_server.FindTopDocuments("--пушистый"s); 
    } catch (const invalid_argument& e) {
        cout << "Ошибка в поисковом запросе: "s << e.what() << endl;
    }
    
    try {
        search_server.MatchDocument("пушистый -"s, 1); 
    } catch (const invalid_argument& e) {
        cout << "Ошибка в поисковом запросе: "s << e.what() << endl;
    }
    
    try {
        search_server.GetDocumentId(100); 
    } catch (const out_of_range& e) {
        cout << "Ошибка в поисковом запросе: "s << e.what() << endl;
    }
    
    return 0;
}