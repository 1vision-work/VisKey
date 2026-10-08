//
//  Engine.h
//  OpenKey
//
//  Created by Tuyen on 1/18/19.
//  Copyright © 2019 Tuyen Mai. All rights reserved.
//
//  Modified by VisKey contributors: all engine state is encapsulated in vk_engine
//  (no global variables); logic is unchanged.
//  SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef Engine_h
#define Engine_h

#include <list>
#include <locale>
#include <codecvt>
#include <map>
#include <string>
#include <vector>

#include "DataType.h"
#include "Vietnamese.h"
#include "Macro.h"
#include "SmartSwitchKey.h"
#include "ConvertTool.h"

#define IS_DEBUG 1

#ifndef LOBYTE
#define LOBYTE(data) (data & 0xFF)
#endif // !LOBYTE
#ifndef HIBYTE
#define HIBYTE(data) ((data>>8) & 0xFF)
#endif // !HIBYTE

#define GET_SWITCH_KEY(data) (data & 0xFF)
#define HAS_CONTROL(data) ((data & 0x100) ? 1 : 0)
#define HAS_OPTION(data) ((data & 0x200) ? 1 : 0)
#define HAS_COMMAND(data) ((data & 0x400) ? 1 : 0)
#define HAS_SHIFT(data) ((data & 0x800) ? 1 : 0)
#define GET_BOOL(data) (data ? 1 : 0)
#define HAS_BEEP(data) (data & 0x8000)
#define SET_SWITCH_KEY(data, key) data = (data & 0xFF) | key
#define SET_CONTROL_KEY(data, val) data|=val<<8;
#define SET_OPTION_KEY(data, val) data|=val<<9;
#define SET_COMMAND_KEY(data, val) data|=val<<10;


/**
 * Engine options. Same names and meaning as the OpenKey globals they replace.
 */
struct vk_options {
    /* 0: English, 1: Vietnamese */
    int vLanguage = 1;
    /* 0: Telex, 1: VNI, 2: Simple Telex 1, 3: Simple Telex 2 */
    int vInputType = 0;
    /* 0: No, 1: Yes */
    int vFreeMark = 0;
    /* 0: Unicode, 1: TCVN3 (ABC), 2: VNI-Windows, 3: Unicode compound, 4: Vietnamese locale CP1258 */
    int vCodeTable = 0;
    /* 0: No, 1: Yes */
    int vCheckSpelling = 1;
    /* 0: òa, úy; 1: oà, uý */
    int vUseModernOrthography = 0;
    /* 0: No, 1: Yes (cc=ch, gg=gi, kk=kh, nn=ng, qq=qu, pp=ph, tt=th, uu=ươ) */
    int vQuickTelex = 0;
    /* Work together with vCheckSpelling */
    int vRestoreIfWrongSpelling = 0;
    /* Macro on or off */
    int vUseMacro = 0;
    /* Still use macro if you are in english mode */
    int vUseMacroInEnglishMode = 0;
    /* Ex: define: btw -> by the way; type: `Btw` -> `By the way`; type: `BTW` -> `BY THE WAY` */
    int vAutoCapsMacro = 0;
    /* Auto write upper case character for first letter */
    int vUpperCaseFirstChar = 0;
    /* Allow write word with consonant Z, F, W, J */
    int vAllowConsonantZFWJ = 0;
    /* f -> ph: fanh -> phanh, j -> gi: jang -> giang, w -> qu: wen -> quen */
    int vQuickStartConsonant = 0;
    /* g -> ng: hag -> hang, h -> nh: vih -> vinh, k -> ch: bak -> bach */
    int vQuickEndConsonant = 0;
};

/**
 * One Vietnamese typing engine instance. Owns all of its state; instances are independent.
 * Not thread-safe: use one instance from one thread at a time.
 */
struct vk_engine : vk_options {
    /**
     * Data to send back to main program. Valid after vKeyHandleEvent / vEnglishMode.
     */
    vKeyHookState HookState = {};

    /* ---- typing state -------------------------------------------------------------- */
    /**
     * data structure of each element in TypingWord (Uint64)
     * first 2 byte is character code or key code.
     * bit 16: has caps or not
     * bit 17: has tone ^ or not
     * bit 18: has tone w or not
     * bit 19 - > 23: has mark or not (Sắc, huyền, hỏi, ngã, nặng)
     * bit 24: is standalone key? (w, [, ])
     * bit 25: is character code or keyboard code; 1: character code; 0: keycode
     */
    Uint32 TypingWord[MAX_BUFF] = {};
    Byte _index = 0;
    vector<Uint32> _longWordHelper; //save the word when _index >= MAX_BUFF
    list<vector<Uint32>> _typingStates; //Aug 28th, 2019: typing helper, save long state of Typing word, can go back and modify the word
    vector<Uint32> _typingStatesData;

    /**
     * Use for restore key if invalid word
     */
    Uint32 KeyStates[MAX_BUFF] = {};
    Byte _stateIndex = 0;

    bool tempDisableKey = false;
    int capsElem = 0;
    int key = 0;
    int markElem = 0;
    bool isCorect = false;
    bool isChanged = false;
    Byte vowelCount = 0;
    Byte vowelStartIndex = 0;
    Byte vowelEndIndex = 0;
    Byte vowelWillSetMark = 0;
    int i = 0, ii = 0, iii = 0;
    int j = 0;
    int k = 0, kk = 0;
    int l = 0;
    bool isRestoredW = false;
    Uint16 keyForAEO = 0;
    bool isCheckedGrammar = false;
    bool _isCaps = false;
    int _spaceCount = 0; //add: July 30th, 2019
    bool _hasHandledMacro = false; //for macro flag August 9th, 2019
    Byte _upperCaseStatus = 0; //for Write upper case for the first letter; 2: will upper case
    bool _isCharKeyCode = false;
    vector<Uint32> _specialChar;
    bool _useSpellCheckingBefore = false;
    bool _hasHandleQuickConsonant = false;
    bool _willTempOffEngine = false;

    bool _spellingOK = false;
    bool _spellingFlag = false;
    bool _spellingVowelOK = false;
    Byte _spellingEndIndex = 0;

    /* ---- macro and smart switch ------------------------------------------------------ */
    map<vector<Uint32>, MacroData> _macroMap;
    SmartSwitchStore smartSwitch;

    /* ---- public entry points ------------------------------------------------------- */
    /**
     * Reset typing state and return the data pointer (&HookState).
     * Call after changing options.
     */
    void* vKeyInit();

    /**
     * MAIN entry point for each key
     * event: mouse or keyboard event
     * state: additional state for event
     * data: key code
     * capsStatus: 1: shift is pressing, 2: caps lock is on
     * otherControlKey: ctrl, option,... is pressing
     */
    void vKeyHandleEvent(const vKeyEvent& event,
                         const vKeyEventState& state,
                         const Uint16& data,
                         const Uint8& capsStatus=0,
                         const bool& otherControlKey=false);

    /**
     * do some task in english mode (use for macro)
     */
    void vEnglishMode(const vKeyEventState& state, const Uint16& data, const bool& isCaps, const bool& otherControlKey);

    /**
     * Convert engine character to real character
     */
    Uint32 getCharacterCode(const Uint32& data);

    /* ---- macro (Macro.cpp) --------------------------------------------------------- */
    /**
     * Load macro data from memory (see getMacroSaveData for the format)
     */
    void initMacroMap(const Byte* pData, const int& size);
    /**
     * convert all macro data to save on disk
     */
    void getMacroSaveData(vector<Byte>& outData) const;
    /**
     * Use to find full text by macro
     */
    bool findMacro(vector<Uint32>& key, vector<Uint32>& macroContentCode);
    bool hasMacro(const string& macroName);
    void getAllMacro(vector<vector<Uint32>>& keys, vector<string>& macroTexts, vector<string>& macroContents);
    bool addMacro(const string& macroText, const string& macroContent);
    bool deleteMacro(const string& macroText);
    /**
     * When table code changed, we have to call this function to reload all macroContentCode
     */
    void onTableCodeChange();
    void saveToFile(const string& path);
    void readFromFile(const string& path, const bool& append=true);

    /* ---- internals (Engine.cpp) ---------------------------------------------------- */
    bool isWordBreak(const vKeyEvent& event, const vKeyEventState& state, const Uint16& data);
    bool isMacroBreakCode(const int& data);
    void setKeyData(const Byte& index, const Uint16& keyCode, const bool& isCaps);
    void checkSpelling(const bool& forceCheckVowel=false);
    void checkGrammar(const int& deltaBackSpace);
    void insertKey(const Uint16& keyCode, const bool& isCaps, const bool& isCheckSpelling=true);
    void insertState(const Uint16& keyCode, const bool& isCaps);
    void saveWord();
    void saveWord(const Uint32& keyCode, const int& count);
    void saveSpecialChar();
    void restoreLastTypingState();
    void startNewSession();
    void checkCorrectVowel(vector<vector<Uint16>>& charset, int& i, int& k, const Uint16& markKey);
    void findAndCalculateVowel(const bool& forGrammar=false);
    void removeMark();
    bool canHasEndConsonant();
    void handleModernMark();
    void handleOldMark();
    void insertMark(const Uint32& markMask, const bool& canModifyFlag=true);
    void insertD(const Uint16& data, const bool& isCaps);
    void insertAOE(const Uint16& data, const bool& isCaps);
    void insertW(const Uint16& data, const bool& isCaps);
    void reverseLastStandaloneChar(const Uint32& keyCode, const bool& isCaps);
    void checkForStandaloneChar(const Uint16& data, const bool& isCaps, const Uint32& keyWillReverse);
    void upperCaseFirstCharacter();
    void handleMainKey(const Uint16& data, const bool& isCaps);
    void handleQuickTelex(const Uint16& data, const bool& isCaps);
    bool checkRestoreIfWrongSpelling(const int& handleCode);
    void vTempOffSpellChecking();
    void vSetCheckSpelling();
    void vTempOffEngine(const bool& off=true);
    bool checkQuickConsonant();

    /* ---- helpers (Macro.cpp) ------------------------------------------------------- */
    void convert(const string& str, vector<Uint32>& outData);
    bool modifyCaseUnicode(Uint32& code, const bool& isUpperCase=true);
};

/**
 * some utils function
 */
wstring utf8ToWideString(const string& str);
string wideStringToUtf8(const wstring& str);

#endif /* Engine_h */
