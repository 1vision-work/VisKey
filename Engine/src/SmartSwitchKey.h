//
//  SmartSwitchKey.h
//  OpenKey
//
//  Created by Tuyen on 8/9/19.
//  Copyright © 2019 Tuyen Mai. All rights reserved.
//
//  Modified by VisKey contributors: per-engine store instead of global state.
//  SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef SmartSwitchKey_h
#define SmartSwitchKey_h

#include "DataType.h"
#include <map>
#include <string>
#include <vector>

using namespace std;

/**
 * Remembers the language (0: English, 1: Vietnamese) of each application (bundle id).
 */
class SmartSwitchStore {
public:
    /**
     * Load saved data (see getSaveData for the format); pData may be NULL to clear the store.
     */
    void load(const Byte* pData, const int& size);

    /**
     * convert all data to save on disk
     */
    void getSaveData(vector<Byte>& outData) const;

    /**
     * find and get language input method, if don't has set @currentInputMethod value for this app
     * return:
     * -1: don't have this bundleId
     * 0: English
     * 1: Vietnamese
     */
    int getAppInputMethodStatus(const string& bundleId, const int& currentInputMethod);

    /**
     * Set default language for this @bundleId
     */
    void setAppInputMethodStatus(const string& bundleId, const int& language);

private:
    map<string, Int8> _data;
    string _cacheKey = ""; //use cache for faster
    Int8 _cacheData = 0; //use cache for faster
};

#endif /* SmartSwitchKey_h */
