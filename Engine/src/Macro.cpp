//
//  Macro.cpp
//  OpenKey
//
//  Created by Tuyen on 8/4/19.
//  Copyright © 2019 Tuyen Mai. All rights reserved.
//
//  Modified by VisKey contributors.
//  SPDX-License-Identifier: GPL-3.0-or-later
//

#include "Macro.h"
#include "Vietnamese.h"
#include "Engine.h"
#include <iostream>
#include <memory.h>
#include <fstream>

using namespace std;

void vk_engine::convert(const string& str, vector<Uint32>& outData) {
    outData.clear();
    wstring data = utf8ToWideString(str);
    Uint32 t = 0;
    int kSign = -1;
    int k = 0;
    for (int i = 0; i < data.size(); i++) {
        t = (Uint32)data[i];
        
        //find normal character fist
        if (_characterMap.find(t) != _characterMap.end()) {
            outData.push_back(_characterMap[t]);
            continue;
        }
        
        //find character which has tone/mark
        for (map<Uint32, vector<Uint16>>::iterator it = _codeTable[0].begin(); it != _codeTable[0].end(); ++it) {
            kSign = -1;
            k = 0;
            for (int j = 0; j < it->second.size(); j++) {
                if ((Uint16)t == it->second[j]) {
                    kSign = 0;
                    outData.push_back(_codeTable[vCodeTable][it->first][k] | CHAR_CODE_MASK);
                    break;
                }//end if
                k++;
            }
            if (kSign != -1)
                break;
        }
        if (kSign != -1)
            continue;
        
        //find other character
        outData.push_back(t | PURE_CHARACTER_MASK); //mark it as pure character
    }
}

/**
 * data structure:
 * byte 0 and 1: macro count
 *
 * byte n: macroText size (macroTextSize)
 * byte n + macroTextSize: macroText data
 *
 * byte m, m+1: macroContentSize
 * byte m+1 + macroContentSize: macroContent data
 *
 * ...
 * next macro
 */
void vk_engine::initMacroMap(const Byte* pData, const int& size) {
    _macroMap.clear();
    Uint16 macroCount = 0;
    Uint32 cursor = 0;
    if (size >= 2) {
        memcpy(&macroCount, pData + cursor, 2);
        cursor+=2;
    }
    Uint8 macroTextSize;
    Uint16 macroContentSize;
    for (int i = 0; i < macroCount; i++) {
        macroTextSize = pData[cursor++];
        string macroText((char*)pData + cursor, macroTextSize);
        cursor += macroTextSize;
        
        memcpy(&macroContentSize, pData + cursor, 2);
        cursor+=2;
        string macroContent((char*)pData + cursor, macroContentSize);
        cursor += macroContentSize;
        
        MacroData data;
        data.macroText = macroText;
        data.macroContent = macroContent;
        
        vector<Uint32> key;
        convert(macroText, key);
        convert(macroContent, data.macroContentCode);
        
        _macroMap[key] = data;
    }
}

void vk_engine::getMacroSaveData(vector<Byte>& outData) const {
    Uint16 totalMacro = (Uint16)_macroMap.size();
    outData.push_back((Byte)totalMacro);
    outData.push_back((Byte)(totalMacro>>8));
    
    for (std::map<vector<Uint32>, MacroData>::const_iterator it = _macroMap.begin(); it != _macroMap.end(); ++it) {
        outData.push_back((Byte)it->second.macroText.size());
        for (int j = 0; j < it->second.macroText.size(); j++) {
            outData.push_back(it->second.macroText[j]);
        }
        
        Uint16 macroContentSize = (Uint16)it->second.macroContent.size();
        outData.push_back((Byte)macroContentSize);
        outData.push_back(macroContentSize>>8);
        for (int j = 0; j < macroContentSize; j++) {
            outData.push_back(it->second.macroContent[j]);
        }
    }
}

bool vk_engine::modifyCaseUnicode(Uint32& code, const bool& isUpperCase) {
    Uint32 _charBuff;
    int _kMacro;
    _charBuff = code;
    if (!(code & CHAR_CODE_MASK)) { //for normal char
        code &= isUpperCase ? CAPS_MASK :  ~CAPS_MASK;
        return code != _charBuff;
    }
    
    //for unicode character
    for (map<Uint32, vector<Uint16>>::iterator it = _codeTable[vCodeTable].begin(); it != _codeTable[vCodeTable].end(); ++it) {
        for (_kMacro = 0; _kMacro < it->second.size(); _kMacro++) {
            if ((Uint16)code == it->second[_kMacro]) {
                if (_kMacro % 2 == 0 && !isUpperCase)
                    _kMacro++;
                else if (_kMacro % 2 != 0 && isUpperCase)
                    _kMacro--;
                code = _codeTable[vCodeTable][it->first][_kMacro] | CHAR_CODE_MASK;
                return code != _charBuff;;
            }//end if
        }
    }
    return false;
}

bool vk_engine::findMacro(vector<Uint32>& key, vector<Uint32>& macroContentCode) {
    int c = 0;
    bool _macroFlag = false;
    Uint16 _kChar = 0;
    for (c = 0; c < key.size(); c++) {
        key[c] = getCharacterCode(key[c]);
    }
    if (_macroMap.find(key) != _macroMap.end()) {
        macroContentCode.clear();
        MacroData data = _macroMap[key];
        macroContentCode = data.macroContentCode;
        return true;
    }
    if (vAutoCapsMacro) {
        _macroFlag = false;
        if (key.size() > 1 && modifyCaseUnicode(key[1], false)) {
            _macroFlag = true;
            for (c = 2; c < key.size(); c++) {
                modifyCaseUnicode(key[c], false);
            }
        }
        
        if (key.size() > 0 && modifyCaseUnicode(key[0], false)) {
            if (_macroMap.find(key) != _macroMap.end()) {
                macroContentCode.clear();
                MacroData data = _macroMap[key];
                macroContentCode = data.macroContentCode;
                for (c = 0; c < macroContentCode.size(); c++) {
                    if (c == 0 || _macroFlag) {
                        _kChar = keyCodeToCharacter(macroContentCode[c]);
                        if (_kChar != 0) {
                            _kChar = toupper(_kChar);
                            macroContentCode[c] = _characterMap[_kChar];
                            continue;
                        }
                        if (macroContentCode[c] & CHAR_CODE_MASK) {
                            modifyCaseUnicode(macroContentCode[c]);
                        }
                    }
                }
                return true;
            }
        }
    }
    return false;
}

bool vk_engine::hasMacro(const string& macroName) {
    vector<Uint32> key;
    convert(macroName, key);
    return (_macroMap.find(key) != _macroMap.end());
}

void vk_engine::getAllMacro(vector<vector<Uint32>>& keys, vector<string>& macroTexts, vector<string>& macroContents) {
    keys.clear();
    macroTexts.clear();
    macroContents.clear();
    for (std::map<vector<Uint32>, MacroData>::iterator it = _macroMap.begin(); it != _macroMap.end(); ++it) {
        keys.push_back(it->first);
        macroTexts.push_back(it->second.macroText);
        macroContents.push_back(it->second.macroContent);
    }
}

bool vk_engine::addMacro(const string& macroText, const string& macroContent) {
    vector<Uint32> key;
    convert(macroText, key);
    if (_macroMap.find(key) == _macroMap.end()) { //add new macro
        MacroData data;
        data.macroText = macroText;
        data.macroContent = macroContent;
        convert(macroContent, data.macroContentCode);
        _macroMap[key] = data;
    } else { //edit this macro
        _macroMap[key].macroContent = macroContent;
        convert(macroContent, _macroMap[key].macroContentCode);
    }
    return true;
}

bool vk_engine::deleteMacro(const string& macroText) {
    vector<Uint32> key;
    convert(macroText, key);
    if (_macroMap.find(key) != _macroMap.end()) {
        _macroMap.erase(key);
        return true;
    }
    return false;
}

void vk_engine::onTableCodeChange() {
    for (std::map<vector<Uint32>, MacroData>::iterator it = _macroMap.begin(); it != _macroMap.end(); ++it) {
        convert(it->second.macroContent, it->second.macroContentCode);
    }
}

void vk_engine::saveToFile(const string& path) {
    ofstream myfile;
    myfile.open(path.c_str());
    myfile << ";Compatible OpenKey Macro Data file for UniKey*** version=1 ***\n";
    for (std::map<vector<Uint32>, MacroData>::iterator it = _macroMap.begin(); it != _macroMap.end(); ++it) {
        myfile <<it->second.macroText << ":" << it->second.macroContent<<"\n";
    }
    myfile.close();
}

void vk_engine::readFromFile(const string& path, const bool& append) {
    ifstream myfile(path.c_str());
    string line;
    int k = 0;
    size_t pos = 0;
    string name, content;
    if (myfile.is_open()) {
        if (!append) {
            _macroMap.clear();
        }
        while (getline (myfile,line) ) {
            k++;
            if (k == 1) continue;
            pos = line.find(":");
            if (string::npos != pos) {
                name = line.substr(0, pos);
                content = line.substr(pos + 1, line.length() - pos - 1);
				while (name.compare("") == 0 && content.compare("") != 0) {
					pos = content.find(":");
					if (string::npos != pos) {
						name += ":";
						name += content.substr(0, pos);
						content = content.substr(pos + 1, line.length() - pos - 1);
					} else {
						break;
					}
				}

                if (name.compare("") != 0 && !hasMacro(name)) {
                    addMacro(name, content);
                }
            }
        }
        myfile.close();
    }
}
