//
//  ConvertTool.h
//  OpenKey
//
//  Created by Tuyen on 9/4/19.
//  Copyright © 2019 Tuyen Mai. All rights reserved.
//
//  Modified by VisKey contributors.
//  SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef ConvertTool_h
#define ConvertTool_h

#include "DataType.h"
#include <string>
using namespace std;

struct vk_convert_options {
    bool toAllCaps = false;
    bool toAllNonCaps = false;
    bool toCapsFirstLetter = false;
    bool toCapsEachWord = false;
    bool removeMark = false;
    Uint8 fromCode = 0; //code table of the source text
    Uint8 toCode = 0; //code table of the result
};

string convertUtil(const vk_convert_options& opt, const string& sourceString);

#endif /* ConvertTool_h */
