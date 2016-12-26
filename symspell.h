#pragma once

#define USE_GOOGLE_DENSE_HASH_MAP
#define VECTOR_RESERVED_SIZE 2048
//#define ENABLE_TEST

#include <iostream>
#include <string>
#include <vector>
#include <regex>
#include <algorithm>
#include <math.h>
#include <algorithm>
#include <ctime>
#include <chrono>
#include <fstream>
#include <list>

#ifdef USE_GOOGLE_DENSE_HASH_MAP
#include <sparsehash/dense_hash_map>
#include <sparsehash/dense_hash_set>

#define set google::dense_hash_set
#define map google::dense_hash_map
#else
#include <map>
#include <set>
#endif

#define min(a, b)  (((a) < (b)) ? (a) : (b))
#define max(a, b)  (((a) > (b)) ? (a) : (b))

using namespace std;


class dictionaryItem {
public:
	vector<size_t> suggestions;
	size_t count = 0;

	dictionaryItem(size_t c)
	{
		count = c;
	}

	dictionaryItem()
	{
		count = 0;
	}
	~dictionaryItem()
	{
		suggestions.clear();
	}
};

class dictionaryItemContainer
{
public:
	dictionaryItemContainer(void) {
		dictValue = NULL;
	}

	enum type { NONE, ITEM, INTEGER };
	type itemType;
	size_t intValue;
	dictionaryItem* dictValue;
};

class suggestItem
{
public:
	string term;
	size_t distance = 0;
	size_t count;

	bool operator== (const suggestItem & item) const
	{
		return term.compare(item.term) == 0;
	}

	size_t HastCode() {
		return hash<string>()(term);
	}
};

class SymSpell {
public:
	size_t verbose = 0;

	SymSpell()
	{
		setlocale(LC_ALL, "");
#ifdef USE_GOOGLE_DENSE_HASH_MAP
		dictionary.set_empty_key(0);
#endif
	}

	void CreateDictionary(string corpus)
	{
		std::ifstream sr(corpus);

		if (!sr.good())
		{
			cout << "File not found: " << corpus;
			return;
		}

		cout << "Creating dictionary ..." << endl;

		long wordCount = 0;

		for (std::string line; std::getline(sr, line); ) {

			for (const string & key : parseWords(line))
			{
				if (CreateDictionaryEntry(key)) ++wordCount;
			}
		}

		sr.close();
	}

	bool CreateDictionaryEntry(string key)
	{
		bool result = false;
		dictionaryItemContainer value;

		auto dictionaryEnd = dictionary.end(); // for performance
		auto valueo = dictionary.find(getHastCode(key));
		if (valueo != dictionaryEnd)
		{
			value = valueo->second;

			if (valueo->second.itemType == dictionaryItemContainer::INTEGER)
			{
				value.itemType = dictionaryItemContainer::ITEM;
				value.dictValue = new dictionaryItem();
				value.dictValue->suggestions.push_back(valueo->second.intValue);
			}
			else
				value = valueo->second;

			if (value.dictValue->count < INT_MAX)
				++(value.dictValue->count);
		}
		else if (wordlist.size() < INT_MAX)
		{
			value.itemType = dictionaryItemContainer::ITEM;
			value.dictValue = new dictionaryItem();
			++(value.dictValue->count);
			string mapKey = key;
			dictionary.insert(pair<size_t, dictionaryItemContainer>(getHastCode(mapKey), value));
			dictionaryEnd = dictionary.end(); // for performance

			if (key.size() > maxlength)
				maxlength = key.size();
		}

		if (value.dictValue->count == 1)
		{
			wordlist.push_back(key);
			size_t keyint = wordlist.size() - 1;

			result = true;

			auto deleted = set<string>();
#ifdef USE_GOOGLE_DENSE_HASH_MAP
			deleted.set_empty_key("");
#endif

			Edits(key, 0, deleted);

			for (string del : deleted)
			{
				auto value2 = dictionary.find(getHastCode(del));
				if (value2 != dictionaryEnd)
				{
					if (value2->second.itemType == dictionaryItemContainer::INTEGER)
					{
						value2->second.itemType = dictionaryItemContainer::ITEM;
						value2->second.dictValue = new dictionaryItem();
						value2->second.dictValue->suggestions.push_back(value2->second.intValue);
						dictionary[getHastCode(del)].dictValue = value2->second.dictValue;

						if (std::find(value2->second.dictValue->suggestions.begin(), value2->second.dictValue->suggestions.end(), keyint) == value2->second.dictValue->suggestions.end())
							AddLowestDistance(value2->second.dictValue, key, keyint, del);
					}
					else if (std::find(value2->second.dictValue->suggestions.begin(), value2->second.dictValue->suggestions.end(), keyint) == value2->second.dictValue->suggestions.end())
						AddLowestDistance(value2->second.dictValue, key, keyint, del);
				}
				else
				{
					dictionaryItemContainer tmp;
					tmp.itemType = dictionaryItemContainer::INTEGER;
					tmp.intValue = keyint;

					string mapKey = del;
					dictionary.insert(pair<size_t, dictionaryItemContainer>(getHastCode(mapKey), tmp));
					dictionaryEnd = dictionary.end();
				}

			}
		}
		return result;
	}

	vector<suggestItem> Correct(string input)
	{
		vector<suggestItem> suggestions;
		
#ifdef ENABLE_TEST
        using namespace std::chrono;
        
        high_resolution_clock::time_point t1 = high_resolution_clock::now();
        
        for (size_t i = 0; i < 100000; ++i)
        {
            suggestions = Lookup(input, editDistanceMax);
        }
        
        high_resolution_clock::time_point t2 = high_resolution_clock::now();
        duration<double> time_span = duration_cast<duration<double>>(t2 - t1);
        
        std::cout << "It took me " << time_span.count() << " seconds.";
        std::cout << std::endl;
#endif
		suggestions = Lookup(input, editDistanceMax);
		return suggestions;

	}


private:
	size_t maxlength = 0;
	size_t editDistanceMax = 2;
	map<size_t, dictionaryItemContainer> dictionary;
	vector<string> wordlist;

	static size_t getHastCode(const string & term)
	{
		return hash<string>()(term);
	}

	vector<string> parseWords(string text) const
	{
		vector<string> returnData;

		std::transform(text.begin(), text.end(), text.begin(), ::tolower);
		std::regex word_regex("[^\\W\\d_]+");
		auto words_begin = std::sregex_iterator(text.begin(), text.end(), word_regex);
		auto words_end = std::sregex_iterator();

		for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
			std::smatch match = *i;
			returnData.push_back(match.str());
		}

		return returnData;
	}

	void AddLowestDistance(dictionaryItem * item, string suggestion, size_t suggestionint, string del)
	{
		if ((verbose < 2) && (item->suggestions.size() > 0) && (wordlist[item->suggestions[0]].size() - del.size() > suggestion.size() - del.size()))
			item->suggestions.clear();

		if ((verbose == 2) || (item->suggestions.size() == 0) || (wordlist[item->suggestions[0]].size() - del.size() >= suggestion.size() - del.size()))
			item->suggestions.push_back(suggestionint);
	}


	set<string> Edits(string word, size_t editDistance, set<string> & deletes)
	{
		++editDistance;
		if (word.size() > 1)
		{
			for (size_t i = 0; i < word.size(); ++i)
			{
				string wordClone = word;
				string del = wordClone.erase(i, 1);
				if (!deletes.count(del))
				{
					deletes.insert(del);

					if (editDistance < editDistanceMax)
						Edits(del, editDistance, deletes);
				}
			}
		}

		return deletes;
	}

	vector<suggestItem> Lookup(string input, size_t editDistanceMax)
	{
		if (input.size() - editDistanceMax > maxlength)
			return vector<suggestItem>();

		vector<string> candidates;
		candidates.reserve(VECTOR_RESERVED_SIZE);
		set<size_t> hashset1;
#ifdef USE_GOOGLE_DENSE_HASH_MAP
		hashset1.set_empty_key(0);
#endif

		vector<suggestItem> suggestions;
		set<size_t> hashset2;
#ifdef USE_GOOGLE_DENSE_HASH_MAP
		hashset2.set_empty_key(0);
#endif

		//object valueo;

		candidates.push_back(input);
		auto dictionaryEnd = dictionary.end();

		size_t candidatesIndexer = 0; // for performance
		while ((candidates.size() - candidatesIndexer) > 0)
		{
			string candidate = candidates[candidatesIndexer];
			size_t candidateSize = candidate.size(); // for performance
			++candidatesIndexer;

			if ((verbose < 2) && (suggestions.size() > 0) && (input.size() - candidateSize > suggestions[0].distance))
				goto sort;

			auto valueo = dictionary.find(getHastCode(candidate));

			//read candidate entry from dictionary
			if (valueo != dictionaryEnd)
			{
				if (valueo->second.itemType == dictionaryItemContainer::INTEGER)
				{
					valueo->second.itemType = dictionaryItemContainer::ITEM;
					valueo->second.dictValue = new dictionaryItem();
					valueo->second.dictValue->suggestions.push_back(valueo->second.intValue);
				}


				if ((valueo->second.dictValue->count > 0) && hashset2.insert(getHastCode(candidate)).second)
				{
					//add correct dictionary term term to suggestion list
					suggestItem si;
					si.term = move(candidate);
					si.count = valueo->second.dictValue->count;
					si.distance = input.size() - candidateSize;
					suggestions.push_back(si);
					//early termination
					if ((verbose < 2) && (input.size() - candidateSize == 0))
						goto sort;
				}

				for (size_t suggestionint : valueo->second.dictValue->suggestions)
				{
					//save some time
					//skipping double items early: different deletes of the input term can lead to the same suggestion
					//index2word
					string suggestion = wordlist[suggestionint];
					if (hashset2.insert(getHastCode(suggestion)).second)
					{
						size_t distance = 0;
						if (suggestion != input)
						{
							if (suggestion.size() == candidateSize) distance = input.size() - candidateSize;
							else if (input.size() == candidateSize) distance = suggestion.size() - candidateSize;
							else
							{
								size_t ii = 0;
								size_t jj = 0;
								while ((ii < suggestion.size()) && (ii < input.size()) && (suggestion[ii] == input[ii]))
									++ii;

								while ((jj < suggestion.size() - ii) && (jj < input.size() - ii) && (suggestion[suggestion.size() - jj - 1] == input[input.size() - jj - 1]))
									++jj;
								if ((ii > 0) || (jj > 0))
									distance = DamerauLevenshteinDistance(suggestion.substr(ii, suggestion.size() - ii - jj), input.substr(ii, input.size() - ii - jj));
								else
									distance = DamerauLevenshteinDistance(move(suggestion), move(input));

							}
						}

						if ((verbose < 2) && (suggestions.size() > 0) && (suggestions[0].distance > distance))
							suggestions.clear();

						if ((verbose < 2) && (suggestions.size() > 0) && (distance > suggestions[0].distance))
							continue;

						if (distance <= editDistanceMax)
						{
							auto value2 = dictionary.find(getHastCode(suggestion));

							if (value2 != dictionaryEnd)
							{
								suggestItem si;
								si.term = move(suggestion);
								si.count = value2->second.dictValue->count;
								si.distance = distance;
								suggestions.push_back(si);
							}
						}
					}
				}
			}


			if (input.size() - candidateSize < editDistanceMax)
			{
				if ((verbose < 2) && (suggestions.size() > 0) && (input.size() - candidateSize >= suggestions[0].distance))
					continue;

				for (size_t i = 0; i < candidateSize; ++i)
				{
					string wordClone = candidate;
					string && del = move(wordClone.erase(i, 1));
					if (hashset1.insert(getHastCode(del)).second)
						candidates.push_back(std::move(del));
				}
			}
		}//end while

		 //sort by ascending edit distance, then by descending word frequency
	sort:
		if (verbose < 2)
			sort(suggestions.begin(), suggestions.end(), Xgreater1());
		else
			sort(suggestions.begin(), suggestions.end(), Xgreater2());

		if ((verbose == 0) && (suggestions.size() > 1))
			return vector<suggestItem>(suggestions.begin(), suggestions.begin() + 1);
		else
			return suggestions;
	}

	struct Xgreater1
	{
		bool operator()(const suggestItem& lx, const suggestItem& rx) const {
			return lx.count > rx.count;
		}
	};

	struct Xgreater2
	{
		bool operator()(const suggestItem& lx, const suggestItem& rx) const {
			return 2 * (lx.distance - rx.distance) > (lx.count - rx.count);
		}
	};


	static size_t DamerauLevenshteinDistance(const std::string &&s1, const std::string &&s2)
	{
		const size_t m(s1.size());
		const size_t n(s2.size());

		if (m == 0) return n;
		if (n == 0) return m;

		size_t *costs = new size_t[n + 1];

		for (size_t k = 0; k <= n; ++k) costs[k] = k;

		size_t i = 0;
		auto s1End = s1.end();
		auto s2End = s2.end();
		for (std::string::const_iterator it1 = s1.begin(); it1 != s1End; ++it1, ++i)
		{
			costs[0] = i + 1;
			size_t corner = i;

			size_t j = 0;
			for (std::string::const_iterator it2 = s2.begin(); it2 != s2End; ++it2, ++j)
			{
				size_t upper = costs[j + 1];
				if (*it1 == *it2)
				{
					costs[j + 1] = corner;
				}
				else
				{
					size_t t(upper < corner ? upper : corner);
					costs[j + 1] = (costs[j] < t ? costs[j] : t) + 1;
				}

				corner = upper;
			}
		}

		size_t result = costs[n];
		delete[] costs;

		return result;
	}
};
