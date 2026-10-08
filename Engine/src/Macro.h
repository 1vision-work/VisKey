//
//  Macro.h
//  OpenKey
//
//  Created by Tuyen on 8/4/19.
//  Copyright © 2019 Tuyen Mai. All rights reserved.
//
//  Modified by VisKey contributors: macro table lives inside vk_engine (see Engine.h).
//  SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef Macro_h
#define Macro_h

#include <vector>
#include <map>
#include <string>
#include "DataType.h"

using namespace std;

struct MacroData {
    string macroText; //ex: "ms"
    string macroContent; //ex: "millisecond"
    vector<Uint32> macroContentCode; //converted of macroContent
};

#endif /* Macro_h */
