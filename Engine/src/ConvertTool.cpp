//
//  ConvertTool.cpp
//  OpenKey
//
//  Created by Tuyen on 9/4/19.
//  Copyright © 2019 Tuyen Mai. All rights reserved.
//
//  Modified by VisKey contributors.
//  SPDX-License-Identifier: GPL-3.0-or-later
//
#include <algorithm>
#include <locale>
#include <codecvt>
#include "ConvertTool.h"
#include "Engine.h"
#include <iostream>
#include <memory.h>

static const vector<Uint8> _breakCode = {'.', '?', '!'};

static bool findKeyCode(const Uint32& charCode, const Uint8& code, int& j, int& k) {
    //find character which has tone/mark
    for (map<Uint32, vector<Uint16>>::iterator it = _codeTable[code].begin(); it != _codeTable[code].end(); ++it) {
        for (int z = 0; z < it->second.size(); z++) {
            if (charCode == it->second[z]) {
                j = it->first;
                k = z;
                return true;
            }//end if
        }
    }
    return false;
}

static Uint16 getUnicodeCompoundMarkIndex(const Uint16& mark) {
    for (int i = 0; i < 5; i++) {
        if (mark == _unicodeCompoundMark[i]) {
            return ((i + 1) << 13);
        }
    }
    return 0;
}

string convertUtil(const vk_convert_options& opt, const string& sourceString) {
    wstring data = utf8ToWideString(sourceString);
    Uint16 t = 0, target;
    int j, k, p;
    vector<wchar_t> _temp;
    bool hasBreak = false;
    bool shouldUpperCase = false;
    if (opt.toCapsFirstLetter || opt.toCapsEachWord)
        shouldUpperCase = true;
    if (opt.toAllNonCaps)
        shouldUpperCase = false;
    
    for (int i = 0; i < data.size(); i++) {
        p = 0;
        //find char with tone/mark
        if (i < data.size() - 1) {
            switch (opt.fromCode) {
                case 2: //VNI
                case 4: //1258
                    t = (Uint16)data[i] | (data[i+1] << 8);
                    p = 1;
                    break;
                case 3:{ //Unicode Compound
                    target = getUnicodeCompoundMarkIndex(data[i+1]);
                    if (target > 0){
                        t = (Uint16)data[i] | target;
                        p = 1;
                    } else {
                        t = (Uint16)data[i];
                    }
                    break;
                }
                default:
                    t = (Uint16)data[i];
                    break;
            }
            
            if (findKeyCode(t, opt.fromCode, j, k)) {
                i += p;
                target = _codeTable[opt.toCode][j][k];
                if ((opt.toAllCaps || shouldUpperCase) && k % 2 != 0) {
                    target = _codeTable[opt.toCode][j][k-1];
                } else if ((opt.toAllNonCaps || !shouldUpperCase) && k % 2 == 0) {
                    target = _codeTable[opt.toCode][j][k+1];
                }
                
                //remove mark/tone
                if (opt.removeMark) {
                    target = keyCodeToCharacter((Uint8)j);
                    if (opt.toAllCaps) {
                        target = towupper(target);
                    } else if (opt.toAllNonCaps) {
                        target = towlower(target);
                    }
                }
                
                if (opt.toCode == 0 || opt.toCode == 1) { //Unicode
                    _temp.push_back(target);
                } else if (opt.toCode == 2 || opt.toCode == 4) { //VNI, VN Locale 1258
                    if (HIBYTE(target) > 32) {
                        _temp.push_back((Uint8)target);
                        _temp.push_back(target>>8);
                    } else {
                        _temp.push_back((Uint8)target);
                    }
                } else if (opt.toCode == 3) { //Unicode Compound
                    if ((target >> 13) > 0) {
                        _temp.push_back(target & 0x1FFF);
                        _temp.push_back(_unicodeCompoundMark[(target>>13) - 1]);
                    } else {
                        _temp.push_back(target);
                    }
                }
                shouldUpperCase = false;
                hasBreak = false;
                continue;
            }
        }
        
        //find primary keycode first
        t = (Uint16)data[i];
        if (findKeyCode(t, opt.fromCode, j, k)) {
            target = _codeTable[opt.toCode][j][k];
            if ((opt.toAllCaps || shouldUpperCase) && k % 2 != 0) {
                target = _codeTable[opt.toCode][j][k-1];
            } else if ((opt.toAllNonCaps || !shouldUpperCase) && k % 2 == 0) {
                target = _codeTable[opt.toCode][j][k+1];
            }
            
            //remove mark/tone
            if (opt.removeMark) {
                target = keyCodeToCharacter((Uint8)j);
                if (opt.toAllCaps) {
                    target = towupper(target);
                } else if (opt.toAllNonCaps){
                    target = towlower(target);
                }
            }
            
            _temp.push_back(target);
            shouldUpperCase = false;
            hasBreak = false;
            continue;
        }
        
        //if dont find => normal char
        if (opt.toAllCaps || shouldUpperCase)
            _temp.push_back(towupper(data[i]));
        else if (opt.toAllNonCaps || !shouldUpperCase)
            _temp.push_back(towlower(data[i]));
        else
            _temp.push_back(data[i]);
        
        if (t == '\n' || (hasBreak && t == ' ')) {
            if (opt.toCapsFirstLetter || opt.toCapsEachWord)
                shouldUpperCase = true;
        } else if (t == ' ' && opt.toCapsEachWord) {
            shouldUpperCase = true;
        } else if (std::find(_breakCode.begin(), _breakCode.end(), t) != _breakCode.end()) {
            hasBreak = true;
        } else {
            shouldUpperCase = false;
            hasBreak = false;
        }
    }
    _temp.push_back(0);
    wstring str(_temp.begin(), _temp.end());
    return wideStringToUtf8(str);
}

