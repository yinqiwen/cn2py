/*
 * cn2py.cpp
 *
 *  Created on: Oct 29, 2018
 *      Author: qiyingwang
 */
#include <string>
#include <map>
#include <vector>
#include <set>
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <functional>
#include <cctype>
#include <stdint.h>
#include "cn2py.h"

namespace cn2py
{
    typedef std::map<int64_t, std::string> PyDict;
    typedef std::map<std::string, std::string> PhoneticSymbolDict;
    typedef std::map<std::string, StringArray> PhrasePinyinDict;
    static PyDict g_py_dict;
    static PhrasePinyinDict g_phrase_py_dict;
    static PhoneticSymbolDict g_phonetic_symbol_dict;

    static void init_phonetic_symbol_dict()
    {
        if (!g_phonetic_symbol_dict.empty())
        {
            return;
        }
        g_phonetic_symbol_dict["ā"] = "a1";
        g_phonetic_symbol_dict["á"] = "a2";
        g_phonetic_symbol_dict["ǎ"] = "a3";
        g_phonetic_symbol_dict["à"] = "a4";
        g_phonetic_symbol_dict["ē"] = "e1";
        g_phonetic_symbol_dict["é"] = "e2";
        g_phonetic_symbol_dict["ě"] = "e3";
        g_phonetic_symbol_dict["è"] = "e4";
        g_phonetic_symbol_dict["ō"] = "o1";
        g_phonetic_symbol_dict["ó"] = "o2";
        g_phonetic_symbol_dict["ǒ"] = "o3";
        g_phonetic_symbol_dict["ò"] = "o4";
        g_phonetic_symbol_dict["ī"] = "i1";
        g_phonetic_symbol_dict["í"] = "i2";
        g_phonetic_symbol_dict["ǐ"] = "i3";
        g_phonetic_symbol_dict["ì"] = "i4";
        g_phonetic_symbol_dict["ū"] = "u1";
        g_phonetic_symbol_dict["ú"] = "u2";
        g_phonetic_symbol_dict["ǔ"] = "u3";
        g_phonetic_symbol_dict["ù"] = "u4";
        g_phonetic_symbol_dict["ü"] = "v";
        g_phonetic_symbol_dict["ū"] = "v1";
        g_phonetic_symbol_dict["ú"] = "v2";
        g_phonetic_symbol_dict["ǔ"] = "v3";
        g_phonetic_symbol_dict["ù"] = "v4";
        g_phonetic_symbol_dict["ń"] = "n2";
        g_phonetic_symbol_dict["ň"] = "n3";
        g_phonetic_symbol_dict["ǹ"] = "n4";
        g_phonetic_symbol_dict[""] = "m2";
    }
// trim from start
    static inline std::string &ltrim(std::string &s)
    {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
        return s;
    }
// trim from end
    static inline std::string &rtrim(std::string &s)
    {
        s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
        return s;
    }

// trim from both ends
    static inline std::string &trim(std::string &s)
    {
        return ltrim(rtrim(s));
    }

    static void split(const std::string& str, char sep, std::vector<std::string>& vals)
    {
        std::string token;
        std::istringstream ss(str);
        while (std::getline(ss, token, sep))
        {
            vals.push_back(token);
        }
    }

    static std::string remove_phonetic_char(const std::string& str)
    {
        PhoneticSymbolDict::iterator found = g_phonetic_symbol_dict.find(str);
        if (found == g_phonetic_symbol_dict.end())
        {
            return str;
        }
        std::string new_str = found->second;
        return new_str.substr(0, 1);
    }

    static void remove_phonetic(const std::string& str, std::string& new_str)
    {
        for (size_t i = 0; i < str.size(); i++)
        {
            if (!(str[i] & 0x80))
            {
                new_str.append(1, str[i]);
            }
            else if ((uint8_t) str[i] <= 0xdf)
            {
                std::string utf8_char = str.substr(i, 2);
                new_str.append(remove_phonetic_char(utf8_char));
                i++;
            }
            else if ((uint8_t) str[i] <= 0xef)
            {
                std::string utf8_char = str.substr(i, 3);
                new_str.append(remove_phonetic_char(utf8_char));
                i += 2;
            }
            else if ((uint8_t) str[i] <= 0xf7)
            {
                std::string utf8_char = str.substr(i, 4);
                new_str.append(remove_phonetic_char(utf8_char));
                i += 3;
            }
        }
    }

    int str2utf8codes(const std::string& str, std::vector<int64_t>& codes)
    {
        for (size_t i = 0; i < str.size(); i++)
        {
            if (!(str[i] & 0x80))
            { // 0xxxxxxx
              // 7bit, total 7bit
              //rp.rune = (uint8_t) (str[0]) & 0x7f;
              //rp.len = 1;
                int64_t code = (uint8_t) (str[i]) & 0x7f;
                codes.push_back(code);
            }
            else if ((uint8_t) str[i] <= 0xdf)
            {
                if ((i + 1) < str.size())
                {
                    // 110xxxxxx
                    // 5bit, total 5bit
                    int64_t code = (uint8_t) (str[i]) & 0x1f;
                    // 6bit, total 11bit
                    code <<= 6;
                    code |= (uint8_t) (str[i + 1]) & 0x3f;
                    codes.push_back(code);
                    i++;
                }
                else
                {
                    printf("Invalid utf8 string 1\n");
                    return -1;
                }
            }
            else if ((uint8_t) str[i] <= 0xef)
            {

                if ((i + 2) < str.size())
                {
                    // 1110xxxxxx
                    // 4bit, total 4bit
                    int64_t code = (uint8_t) (str[i]) & 0x0f;

                    // 6bit, total 10bit
                    code <<= 6;
                    code |= (uint8_t) (str[i + 1]) & 0x3f;

                    // 6bit, total 16bit
                    code <<= 6;
                    code |= (uint8_t) (str[i + 2]) & 0x3f;
                    codes.push_back(code);
                    i += 2;
                }
                else
                {
                    printf("Invalid utf8 string 2\n");
                    return -1;
                }

            }
            else if ((uint8_t) str[i] <= 0xf7)
            {
                if ((i + 3) < str.size())
                {
                    // 11110xxxx
                    // 3bit, total 3bit
                    int64_t code = (uint8_t) (str[i]) & 0x07;

                    // 6bit, total 9bit
                    code <<= 6;
                    code |= (uint8_t) (str[i + 1]) & 0x3f;

                    // 6bit, total 15bit
                    code <<= 6;
                    code |= (uint8_t) (str[i + 2]) & 0x3f;

                    // 6bit, total 21bit
                    code <<= 6;
                    code |= (uint8_t) (str[i + 3]) & 0x3f;
                    codes.push_back(code);
                    i += 3;
                }
                else
                {
                    printf("Invalid utf8 string 3\n");
                    return -1;
                }

            }
            else
            {
                printf("Invalid utf8 string 4\n");
                return -1;
            }
        }
        return 0;
    }

    int cn2pinyin(const std::string& cnstr, PinyinResult& result)
    {
        if (g_py_dict.empty())
        {
            return -1;
        }
        typedef std::set<std::string> StringSet;
        std::vector<int64_t> codes;
        str2utf8codes(cnstr, codes);
        for (size_t i = 0; i < codes.size(); i++)
        {
            StringArray pinyins;
            PyDict::iterator found = g_py_dict.find(codes[i]);
            if (found != g_py_dict.end())
            {
                const std::string& py = found->second;
                std::vector<std::string> vals;
                split(py, ',', vals);
                for (const std::string& val : vals)
                {
                    std::string new_str;
                    remove_phonetic(val, new_str);
                    pinyins.push_back(new_str);
                }
            }
            else
            {
                pinyins.push_back(std::string(1, (char) codes[i]));
            }
            StringSet tmp;
            StringArray uniq_pinyins;
            for(size_t i = 0; i < pinyins.size(); i++)
            {
                if(!tmp.insert(pinyins[i]).second)
                {
                    continue;
                }
                uniq_pinyins.push_back(pinyins[i]);
            }

            if (result.size() > 0)
            {
                size_t prev_size = result.size();
                for (size_t i = 0; i < uniq_pinyins.size() - 1; i++)
                {
                    //copy
                    result.insert(result.end(), result.begin(), result.begin() + prev_size);
                }
                //printf("###Result resize to %zu from %zu\n", result.size(), prev_size);
                size_t offset = 0;
                for (const std::string& v : uniq_pinyins)
                {
                    for (size_t i = 0; i < prev_size; i++)
                    {
                        result[offset + i].push_back(v);
                    }
                    offset += prev_size;
                }
            }
            else
            {
                for (const std::string& v : uniq_pinyins)
                {
                    result.push_back(StringArray(1, v));
                }
            }
        }
        return 0;
    }

    int words2piniyin(const StringArray& words, PinyinResult& result)
    {
        if (g_py_dict.empty())
        {
            return -1;
        }
        for(const std::string& word:words)
        {
            PhrasePinyinDict::iterator found = g_phrase_py_dict.find(word);
            if(found != g_phrase_py_dict.end())
            {
                StringArray pys = found->second;
                for(std::string& py:pys)
                {
                    std::string new_py;
                    remove_phonetic(py, new_py);
                    py = new_py;
                }
                if(result.empty())
                {
                    result.resize(1);
                }
                for(StringArray& prev:result)
                {
                    prev.insert(prev.end(),pys.begin(), pys.end());
                }
            }else
            {
                PinyinResult r;
                cn2pinyin(word, r);
                if(result.empty())
                {
                    result = r;
                }
                else
                {
                    size_t prev_size = result.size();
                    for (size_t i = 0; i < r.size() - 1; i++)
                    {
                        //copy
                        result.insert(result.end(), result.begin(), result.begin() + prev_size);
                    }
                    size_t offset = 0;
                    for (const StringArray& v : r)
                    {
                        for (size_t i = 0; i < prev_size; i++)
                        {
                            result[offset + i].insert(result[offset + i].end(), v.begin(), v.end());
                        }
                        offset += prev_size;
                    }
                }
            }
        }
        return 0;
    }

    int load_dict(const std::string& path)
    {
        init_phonetic_symbol_dict();
        std::ifstream infile(path);
        std::string line;
        while (std::getline(infile, line))
        {
            std::string new_line = trim(line);
            if (new_line[0] == '#')
            {
                continue;
            }
            std::vector<std::string> vals, val2;
            split(new_line, ':', vals);
            if (vals.size() < 2)
            {
                printf("Invalid line:%s\n", new_line.c_str());
                continue;
            }
            std::string utf8_code = trim(vals[0]);
            long long utf8_code_int;
            std::stringstream ss;
            ss << std::hex << utf8_code.substr(2);
            ss >> utf8_code_int;
            split(vals[1], '#', val2);
            std::string py = trim(val2[0]);
            std::string cn = trim(val2[1]);
            g_py_dict[utf8_code_int] = py;
            //printf("###%lld  %s %s  %s\n", utf8_code_int,utf8_code.c_str(), py.c_str(), cn.c_str());
        }
        return 0;
    }

    int load_phase_dict(const std::string& path)
    {
        std::ifstream infile(path);
        std::string line;
        while (std::getline(infile, line))
        {
            std::string new_line = trim(line);
            if (new_line[0] == '#')
            {
                continue;
            }
            std::vector<std::string> vals, val2;
            split(new_line, ':', vals);
            if (vals.size() < 2)
            {
                printf("Invalid line:%s\n", new_line.c_str());
                continue;
            }
            std::string cnstr = trim(vals[0]);
            StringArray py;
            split(trim(vals[1]),' ', py);
            g_phrase_py_dict[cnstr] = py;
            //printf("###%lld  %s %s  %s\n", utf8_code_int,utf8_code.c_str(), py.c_str(), cn.c_str());
        }
        return 0;
    }
}

//int main()
//{
//    cn2py::load_dict("pinyin.txt");
//    cn2py::load_phase_dict("pinyin_phrase.txt");
//    while (std::cin)
//    {
//        std::string line;
//        std::getline(std::cin, line);
//        line = cn2py::trim(line);
//        cn2py::PinyinResult rs;
//        std::vector<std::string> words;
//        cn2py::split(line, ' ', words);
//        cn2py::words2piniyin(words, rs);
//        for (const cn2py::StringArray& ss : rs)
//        {
//            for(const std::string& v:ss)
//            {
//                std::cout  << v  << " ";
//            }
//            std::cout  << std::endl;
//        }
//
//    };
//    return 0;
//}

