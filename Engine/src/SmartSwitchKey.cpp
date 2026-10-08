//
//  SmartSwitchKey.cpp
//  OpenKey
//
//  Created by Tuyen on 8/13/19.
//  Copyright © 2019 Tuyen Mai. All rights reserved.
//
//  Modified by VisKey contributors.
//  SPDX-License-Identifier: GPL-3.0-or-later
//

#include "SmartSwitchKey.h"
#include <map>
#include <iostream>
#include <memory.h>

//main data, i use `map` because it has O(Log(n))

void SmartSwitchStore::load(const Byte* pData, const int& size) {
    _data.clear();
    if (pData == NULL) return;
    Uint16 count = 0;
    Uint32 cursor = 0;
    if (size >= 2) {
        memcpy(&count, pData + cursor, 2);
        cursor+=2;
    }
    Uint8 bundleIdSize;
    Uint8 value;
    for (int i = 0; i < count; i++) {
        bundleIdSize = pData[cursor++];
        string bundleId((char*)pData + cursor, bundleIdSize);
        cursor += bundleIdSize;
        value = pData[cursor++];
        _data[bundleId] = value;
    }
}

void SmartSwitchStore::getSaveData(vector<Byte>& outData) const {
    outData.clear();
    Uint16 count = (Uint16)_data.size();
    outData.push_back((Byte)count);
    outData.push_back((Byte)(count>>8));
    
    for (std::map<string, Int8>::const_iterator it = _data.begin(); it != _data.end(); ++it) {
        outData.push_back((Byte)it->first.length());
        for (int j = 0; j < it->first.length(); j++) {
            outData.push_back(it->first[j]);
        }
        outData.push_back(it->second);
    }
}

int SmartSwitchStore::getAppInputMethodStatus(const string& bundleId, const int& currentInputMethod) {
    if (_cacheKey.compare(bundleId) == 0) {
        return _cacheData;
    }
    if (_data.find(bundleId) != _data.end()) {
        _cacheKey = bundleId;
        _cacheData = _data[bundleId];
        return _cacheData;
    }
    _cacheKey = bundleId;
    _cacheData = currentInputMethod;
    _data[bundleId] = _cacheData;
    return -1;
}

void SmartSwitchStore::setAppInputMethodStatus(const string& bundleId, const int& language) {
    _data[bundleId] = language;
    _cacheKey = bundleId;
    _cacheData = language;
}
