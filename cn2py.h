/*
 * cn2py.h
 *
 *  Created on: Oct 31, 2018
 *      Author: qiyingwang
 */

#ifndef CN2PY_H_
#define CN2PY_H_

#include <string>
#include <vector>

namespace cn2py
{
    int load_dict(const std::string& path);

    typedef std::vector<std::string> StringArray;
    typedef std::vector<StringArray> PinyinResult;

    int cn2pinyin(const std::string& cnstr, PinyinResult& result);
}



#endif /* CN2PY_H_ */
