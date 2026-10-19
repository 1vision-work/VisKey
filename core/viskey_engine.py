#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
VisKey Core Engine (Telex & VNI Vietnamese Input Method Engine)
Addresses the core issues of existing Vietnamese IMEs on macOS and Linux:
1. Free-order diacritics & Dynamic Tone Repositioning (Kiểu cũ: hòa, thúy & Kiểu mới: hoà, thuý)
2. Dual-language Vietnamese Phonotactics Validator & Auto-Restore for English words
3. Double-key undo/escape (ss, ff, rr, xx, jj, ww, aa, ee, oo, dd)
4. Smart compound horn handling (uo + w -> ươ, uoi + w -> ươi, ua + w -> ưa vs qua + w -> quă)
5. Quick Telex & Case-Preserving Macro expansion
6. Autocomplete / Spotlight / Omnibox replacement metadata
"""

from dataclasses import dataclass, field
from typing import Dict, List, Optional, Tuple

# Tone indices:
# 0: Ngang, 1: Sắc (s/1), 2: Huyền (f/2), 3: Hỏi (r/3), 4: Ngã (x/4), 5: Nặng (j/5)
VOWEL_TABLE: Dict[str, Tuple[str, str, str, str, str, str]] = {
    "a": ("a", "á", "à", "ả", "ã", "ạ"),
    "ă": ("ă", "ắ", "ằ", "ẳ", "ẵ", "ặ"),
    "â": ("â", "ấ", "ầ", "ẩ", "ẫ", "ậ"),
    "e": ("e", "é", "è", "ẻ", "ẽ", "ẹ"),
    "ê": ("ê", "ế", "ề", "ể", "ễ", "ệ"),
    "i": ("i", "í", "ì", "ỉ", "ĩ", "ị"),
    "o": ("o", "ó", "ò", "ỏ", "õ", "ọ"),
    "ô": ("ô", "ố", "ồ", "ổ", "ỗ", "ộ"),
    "ơ": ("ơ", "ớ", "ờ", "ở", "ỡ", "ợ"),
    "u": ("u", "ú", "ù", "ủ", "ũ", "ụ"),
    "ư": ("ư", "ứ", "ừ", "ử", "ữ", "ự"),
    "y": ("y", "ý", "ỳ", "ỷ", "ỹ", "ỵ"),
    "A": ("A", "Á", "À", "Ả", "Ã", "Ạ"),
    "Ă": ("Ă", "Ắ", "Ằ", "Ẳ", "Ẵ", "Ặ"),
    "Â": ("Â", "Ấ", "Ầ", "Ẩ", "Ẫ", "Ậ"),
    "E": ("E", "É", "È", "Ẻ", "Ẽ", "Ẹ"),
    "Ê": ("Ê", "Ế", "Ề", "Ể", "Ễ", "Ệ"),
    "I": ("I", "Í", "Ì", "Ỉ", "Ĩ", "Ị"),
    "O": ("O", "Ó", "Ò", "Ỏ", "Õ", "Ọ"),
    "Ô": ("Ô", "Ố", "Ồ", "Ổ", "Ỗ", "Ộ"),
    "Ơ": ("Ơ", "Ớ", "Ờ", "Ở", "Ỡ", "Ợ"),
    "U": ("U", "Ú", "Ù", "Ủ", "Ũ", "Ụ"),
    "Ư": ("Ư", "Ứ", "Ừ", "Ử", "Ữ", "Ự"),
    "Y": ("Y", "Ý", "Ỳ", "Ỷ", "Ỹ", "Ỵ"),
}

BASE_VOWELS_LOWER = {"a", "ă", "â", "e", "ê", "i", "o", "ô", "ơ", "u", "ư", "y"}
MODIFIED_VOWELS_LOWER = {"ă", "â", "ê", "ô", "ơ", "ư"}

VALID_INITIALS = {
    "", "b", "c", "ch", "d", "đ", "g", "gh", "gi", "h", "k", "kh",
    "l", "m", "n", "ng", "ngh", "nh", "p", "ph", "qu", "r", "s",
    "t", "th", "tr", "v", "x"
}

VALID_INITIALS_TYPING = VALID_INITIALS | {"q"}

VALID_FINALS = {"", "c", "ch", "m", "n", "ng", "nh", "p", "t"}
STOP_FINALS = {"c", "ch", "p", "t"}

# Valid vowel nuclei in committed Vietnamese words
VALID_VOWEL_NUCLEI_COMMITTED = {
    # Single vowels
    "a", "ă", "â", "e", "ê", "i", "o", "ô", "ơ", "u", "ư", "y",
    # Diphthongs
    "ai", "ao", "au", "âu", "ay", "ây",
    "eo", "êu",
    "ia", "iê", "iu",
    "oa", "oă", "oe", "oi", "ôi", "ơi", "oo",
    "ua", "uă", "uâ", "ưa", "uê", "ui", "ưi", "uô", "ươ", "uơ", "ưu", "uy",
    "ya", "yê",
    # Triphthongs
    "iêu", "oai", "oao", "oay", "oeo", "uây", "uôi", "ươi", "ươu", "uya", "uyê", "uyu", "yêu"
}

# Intermediate vowel clusters valid while actively typing a syllable
VALID_VOWEL_NUCLEI_TYPING = VALID_VOWEL_NUCLEI_COMMITTED | {
    "ie", "uo", "ưo", "ye", "uye", "uoi", "ưoi", "uou", "ưou", "ieu", "yeu", "uay"
}

# Vowel clusters valid right after applying a circumflex (^)
VALID_CIRCUMFLEX_NUCLEI = {
    "â", "ê", "ô",
    "âu", "ây", "êu", "iê", "ôi", "uâ", "uê", "uô", "yê",
    "iêu", "uây", "uôi", "uyê", "yêu"
}

# Open-syllable forbidden vowels: cannot stand at the end of a committed word without a final consonant
FORBIDDEN_OPEN_NUCLEI = {
    "ă", "â", "iê", "yê", "uô", "ươ", "oă", "uă", "uâ", "uyê"
}

# Common English words whose Telex sequences otherwise resemble valid Vietnamese syllables
ENGLISH_COLLISION_WORDS = {
    "soon", "moon", "room", "noon", "spoon", "boom", "zoom", "loom", "bloom", "gloom", "broom", "groom",
    "seen", "been", "teen", "green", "screen", "queen", "keen", "between",
    "seem", "deem", "teem",
    "beer", "deer", "peer", "cheer", "steer", "sheer", "queer", "veer", "jeer", "sneer", "career", "engineer", "volunteer", "pioneer",
    "door", "poor", "floor", "moor", "boor",
    "tour", "four", "pour", "your", "hour", "sour", "flour", "scour", "devour",
    "user", "uses", "used", "use", "fuse", "muse", "abuse", "excuse", "refuse", "confuse", "accuse", "amuse",
    "true", "blue", "clue", "glue", "due", "sue", "cue", "hue", "issue", "tissue", "value", "statue", "virtue", "rescue", "continue", "avenue", "revenue",
    "more", "core", "bore", "sore", "tore", "wore", "shore", "score", "store", "ignore", "explore", "restore", "adore", "before", "therefore",
    "pure", "sure", "cure", "lure", "endure", "insure", "ensure", "assure", "secure", "obscure", "procure", "mature", "nature", "future", "feature", "creature", "culture", "structure", "picture", "lecture", "architecture", "literature", "temperature", "signature", "furniture", "adventure", "departure", "capture", "manufacture", "measure", "pleasure", "treasure", "pressure", "exposure", "closure", "enclosure", "disclosure", "leisure", "seizure", "failure", "figure", "procedure", "brochure",
    "fire", "wire", "hire", "tire", "entire", "require", "acquire", "inquire", "inspire", "desire", "admire", "retire", "expire", "empire",
    "care", "dare", "rare", "ware", "share", "spare", "square", "stare", "aware", "compare", "prepare", "declare", "software", "hardware", "firmware", "malware", "freeware", "middleware",
    "raw", "saw", "law", "jaw", "paw", "claw", "flaw", "draw", "straw", "thaw",
    "new", "few", "knew", "flew", "grew", "crew", "drew", "screw", "threw", "chew", "stew", "brew", "view", "review", "interview", "preview", "overview",
    "box", "fox", "tax", "wax", "max", "fax", "fix", "mix", "six", "sex", "text", "next",
    "test", "best", "rest", "west", "list", "cost", "lost", "most", "post", "host", "fast", "last", "past", "cast", "just", "must", "dust", "trust",
    "auto", "audio", "video", "radio", "studio", "portfolio", "scenario", "ratio", "mode", "code", "node", "home", "some", "come", "done", "none", "gone", "tone", "zone", "bone", "cone", "line", "fine", "mine", "nine", "pine", "wine", "time", "lime", "dime", "mime", "name", "same", "game", "fame", "came", "tame", "lame", "frame", "blame", "flame", "shame",
}


@dataclass
class EngineConfig:
    input_method: str = "telex"          # "telex" or "vni"
    modern_orthography: bool = False     # False: hòa, khỏe, thúy (phổ biến); True: hoà, khoẻ, thuý (kiểu mới)
    auto_restore_english: bool = True    # Automatically restore invalid Vietnamese words to raw English
    quick_telex: bool = False            # cc=ch, gg=gi, kk=kh, nn=ng, qq=qu, pp=ph, tt=th
    enable_macros: bool = True           # Expand shorthand macros on word boundary
    fix_autocomplete: bool = True        # Emit autocomplete-safe replacement metadata for macOS/browsers
    macros: Dict[str, str] = field(default_factory=lambda: {
        "vn": "Việt Nam",
        "ko": "không",
        "dc": "được",
        "uh": "ừ",
        "ntn": "như thế nào",
        "bt": "bình thường",
        "mn": "mọi người",
    })


@dataclass
class KeyProcessResult:
    handled: bool                # True if the engine modified or consumed the keystroke
    backspaces: int              # Number of characters to delete before inserting `commit_text`
    commit_text: str             # Replacement string to send to the application
    current_word: str            # Current displayed buffer for the active word
    raw_word: str                # Raw unmodified keystrokes for the active word
    autocomplete_fix_needed: bool = False  # True if browser/Spotlight autocomplete fix should trigger


class VisKeyEngine:
    def __init__(self, config: Optional[EngineConfig] = None):
        self.config = config or EngineConfig()
        self.reset()

    def reset(self) -> None:
        self.raw_keys: List[str] = []
        self.chars: List[str] = []       # Base characters (vowels without tone, 'đ'/'d', consonants)
        self.tone: int = 0               # 0..5
        self.disabled_for_word: bool = False
        self.has_backspaced: bool = False
        self.last_modifier_key: Optional[str] = None

    def get_displayed_word(self) -> str:
        if self.disabled_for_word:
            return "".join(self.raw_keys)
        return self._render_syllable()

    def _split_syllable(self) -> Tuple[str, str, str, List[int]]:
        """
        Splits `self.chars` into (initial_lower, vowel_lower, final_lower, vowel_indices).
        Correctly treats 'qu' and 'gi' (when followed by another vowel) as initial consonants.
        """
        n = len(self.chars)
        if n == 0:
            return "", "", "", []

        lower_chars = [c.lower() for c in self.chars]
        i = 0

        if n >= 2 and lower_chars[0] == "q" and lower_chars[1] == "u":
            i = 2
        elif n >= 2 and lower_chars[0] == "g" and lower_chars[1] == "i":
            if n >= 3 and lower_chars[2] in BASE_VOWELS_LOWER:
                i = 2
            else:
                i = 1
        else:
            while i < n and lower_chars[i] not in BASE_VOWELS_LOWER:
                i += 1

        initial = "".join(lower_chars[:i])

        vowel_indices: List[int] = []
        while i < n and lower_chars[i] in BASE_VOWELS_LOWER:
            vowel_indices.append(i)
            i += 1

        vowel = "".join(lower_chars[idx] for idx in vowel_indices)
        final = "".join(lower_chars[i:])
        return initial, vowel, final, vowel_indices

    def _find_tone_target_index(self, initial: str, vowel: str, final: str, vowel_indices: List[int]) -> int:
        """
        Determines which index in `self.chars` receives the tone mark (`self.tone`).
        Supports both Standard/Traditional (hòa, khỏe, thúy) and Modern (hoà, khoẻ, thuý) orthography.
        """
        k = len(vowel_indices)
        if k == 0:
            return -1
        if k == 1:
            return vowel_indices[0]

        # 1. If 'ươ' or 'ưo' is present in the vowel cluster, tone always goes on 'ơ'/'o'
        for pos in range(k - 1):
            c1 = self.chars[vowel_indices[pos]].lower()
            c2 = self.chars[vowel_indices[pos + 1]].lower()
            if c1 == "ư" and c2 in ("ơ", "o"):
                return vowel_indices[pos + 1]

        # 2. If any modified vowel (ă, â, ê, ô, ơ, ư) exists, tone goes on it
        for idx in vowel_indices:
            if self.chars[idx].lower() in MODIFIED_VOWELS_LOWER:
                return idx

        # 3. Unmodified 2-vowel clusters
        if k == 2:
            if final != "":
                # Closed syllable (e.g. toán, hoàng, tuấn): tone on 2nd vowel
                return vowel_indices[1]
            # Open syllable: check oa, oe, uy
            if vowel in ("oa", "oe", "uy"):
                # modern_orthography=True -> hoà, khoẻ, thuý (2nd vowel)
                # modern_orthography=False -> hòa, khỏe, thúy (1st vowel)
                return vowel_indices[1] if self.config.modern_orthography else vowel_indices[0]
            return vowel_indices[0]

        # 4. Unmodified 3-vowel clusters (oai, oay, uya, khuỷu, ngoèo): tone on middle vowel
        return vowel_indices[1]

    def _render_syllable(self) -> str:
        if not self.chars:
            return ""
        if self.tone == 0:
            return "".join(self.chars)

        initial, vowel, final, vowel_indices = self._split_syllable()
        target_idx = self._find_tone_target_index(initial, vowel, final, vowel_indices)
        if target_idx < 0:
            return "".join(self.chars)

        out = list(self.chars)
        base_ch = out[target_idx]
        if base_ch in VOWEL_TABLE:
            out[target_idx] = VOWEL_TABLE[base_ch][self.tone]
        return "".join(out)

    def is_valid_vietnamese(self, committed: bool = False) -> bool:
        """
        Validates whether the current syllable conforms to Vietnamese phonotactics.
        """
        if not self.chars:
            return True

        raw_str = "".join(self.raw_keys).lower()
        if committed and raw_str in ENGLISH_COLLISION_WORDS:
            return False

        lower_chars = [c.lower() for c in self.chars]
        for c in lower_chars:
            if c in ("f", "j", "w", "z"):
                return False

        initial, vowel, final, vowel_indices = self._split_syllable()

        valid_init_set = VALID_INITIALS if committed else VALID_INITIALS_TYPING
        if initial not in valid_init_set:
            return False

        if vowel == "":
            if final != "":
                return False
            return not committed

        # Reject words with a second vowel cluster after the final consonant (e.g. 'update', 'service', 'mode')
        last_vowel_idx = vowel_indices[-1]
        for idx in range(last_vowel_idx + 1, len(lower_chars)):
            if lower_chars[idx] in BASE_VOWELS_LOWER:
                return False

        # Orthographic distribution rules for c/k/g/gh/ng/ngh
        first_v = vowel[0]
        if initial == "c" and first_v in ("i", "e", "ê", "y"):
            return False
        if initial in ("gh", "ngh") and first_v not in ("i", "e", "ê"):
            return False
        if initial == "k" and first_v not in ("i", "e", "ê", "y"):
            return False

        # Check final consonant
        if final != "":
            if final not in VALID_FINALS:
                return False

        # Check vowel cluster validity
        valid_nuclei = VALID_VOWEL_NUCLEI_COMMITTED if committed else VALID_VOWEL_NUCLEI_TYPING
        if vowel not in valid_nuclei:
            return False

        # Specific vowel + final consonant restrictions
        if vowel == "oo":
            allowed_oo_finals = ("c", "ng") if committed else ("", "c", "n", "ng")
            if final not in allowed_oo_finals:
                return False

        # Stop finals (c, ch, p, t) can ONLY take Sắc (1) or Nặng (5)
        if final in STOP_FINALS:
            if self.tone in (2, 3, 4):
                return False
            if committed and self.tone == 0:
                return False

        # Committed words cannot end with open short/diphthong vowels: ă, â, iê, yê, uô, ươ, oă, uă, uâ, uyê
        if committed and final == "" and vowel in FORBIDDEN_OPEN_NUCLEI:
            return False

        return True

    def _apply_case_pattern(self, source: str, target: str) -> str:
        if source.isupper():
            return target.upper()
        if source and source[0].isupper():
            return target[0].upper() + target[1:]
        return target

    def process_backspace(self) -> KeyProcessResult:
        """
        Deletes the last visible character on screen, keeping syllable tone on remaining vowels if valid.
        """
        if not self.raw_keys and not self.chars:
            return KeyProcessResult(False, 0, "", "", "")
        self.has_backspaced = True

        old_display = self.get_displayed_word()
        if not old_display:
            self.reset()
            return KeyProcessResult(False, 0, "", "", "")

        if self.disabled_for_word:
            self.raw_keys.pop()
            if self.chars:
                self.chars.pop()
            new_display = "".join(self.raw_keys)
            if not self.raw_keys:
                self.reset()
            return KeyProcessResult(False, 0, "", new_display, new_display)

        # In Vietnamese mode, remove the last character from `self.chars`
        initial, vowel, final, vowel_indices = self._split_syllable()
        target_idx = self._find_tone_target_index(initial, vowel, final, vowel_indices)
        removed_idx = len(self.chars) - 1
        self.chars.pop()
        if self.raw_keys:
            self.raw_keys.pop()

        if not self.chars:
            self.reset()
            return KeyProcessResult(False, 0, "", "", "")

        # If the vowel carrying the tone was deleted, or no vowels remain, clear the tone
        _, new_vowel, _, new_vowel_indices = self._split_syllable()
        if len(new_vowel_indices) == 0 or removed_idx == target_idx:
            self.tone = 0

        new_display = self.get_displayed_word()
        # Synchronize raw_keys with current simplified state
        if len(old_display) == len(new_display) + 1 and old_display[:-1] == new_display:
            return KeyProcessResult(False, 0, "", new_display, "".join(self.raw_keys))

        return KeyProcessResult(
            handled=True,
            backspaces=len(old_display),
            commit_text=new_display,
            current_word=new_display,
            raw_word="".join(self.raw_keys),
            autocomplete_fix_needed=self.config.fix_autocomplete,
        )

    def _process_char_internal(self, ch: str) -> bool:
        self.raw_keys.append(ch)
        if self.disabled_for_word:
            self.chars.append(ch)
            return False

        ch_lower = ch.lower()
        im = self.config.input_method

        # 1. Quick Telex (cc=ch, gg=gi, kk=kh, nn=ng, qq=qu, pp=ph, tt=th) at start of word
        if im == "telex" and self.config.quick_telex and len(self.chars) == 1:
            prev_lower = self.chars[0].lower()
            quick_map = {
                "c": "h", "g": "i", "k": "h", "n": "g", "q": "u", "p": "h", "t": "h"
            }
            if ch_lower == prev_lower and ch_lower in quick_map:
                added = quick_map[ch_lower]
                self.chars.append(added.upper() if self.chars[0].isupper() and ch.isupper() else added)
                return True

        # 2. Handle 'd' + 'd' (Telex) or '9' (VNI) -> 'đ'
        if (im == "telex" and ch_lower == "d") or (im == "vni" and ch == "9"):
            if len(self.chars) >= 1 and self.chars[0].lower() in ("d", "đ"):
                if im == "vni" or len(self.chars) == 1 or self.last_modifier_key != "d":
                    if self.chars[0].lower() == "d":
                        self.chars[0] = "Đ" if self.chars[0].isupper() else "đ"
                        self.last_modifier_key = ch_lower
                        return True
                    elif self.chars[0].lower() == "đ" and self.last_modifier_key == ch_lower:
                        self.chars[0] = "D" if self.chars[0].isupper() else "d"
                        self.chars.append(ch)
                        self.disabled_for_word = True
                        return True

        # 3. Handle Tone marks:
        tone_map_telex = {"s": 1, "f": 2, "r": 3, "x": 4, "j": 5, "z": 0}
        tone_map_vni = {"1": 1, "2": 2, "3": 3, "4": 4, "5": 5, "0": 0}
        tone_map = tone_map_telex if im == "telex" else tone_map_vni

        if ch_lower in tone_map:
            initial, vowel, final, vowel_indices = self._split_syllable()
            if len(vowel_indices) > 0:
                new_tone = tone_map[ch_lower]
                if new_tone == 0:
                    if self.tone != 0:
                        self.tone = 0
                        self.last_modifier_key = ch_lower
                        return True
                else:
                    if self.tone == new_tone and self.last_modifier_key == ch_lower:
                        self.tone = 0
                        self.chars.append(ch)
                        self.disabled_for_word = True
                        return True
                    self.tone = new_tone
                    self.last_modifier_key = ch_lower
                    if self.config.auto_restore_english and not self.is_valid_vietnamese(committed=False):
                        self.disabled_for_word = True
                    return True

        # 4. Handle Vowel Modifiers (Circumflex ^, Breve ă, Horn ơ/ư)
        initial, vowel, final, vowel_indices = self._split_syllable()

        # 4a. Telex circumflex (aa->â, ee->ê, oo->ô) or VNI '6'
        if (im == "telex" and ch_lower in ("a", "e", "o")) or (im == "vni" and ch == "6"):
            target_bases = (ch_lower,) if im == "telex" else ("a", "e", "o")
            hat_map = {"a": "â", "A": "Â", "e": "ê", "E": "Ê", "o": "ô", "O": "Ô"}
            unhat_map = {"â": "a", "Â": "A", "ê": "e", "Ê": "E", "ô": "o", "Ô": "O"}

            # Double-tap undo (e.g. 'aaa' -> 'aa', 'ooo' -> 'oo', 'eee' -> 'ee')
            for idx in reversed(vowel_indices):
                cur_v = self.chars[idx]
                if cur_v in unhat_map and self.last_modifier_key == ch_lower:
                    self.chars[idx] = unhat_map[cur_v]
                    self.chars.append(ch)
                    # Special case: 'xoọc', 'boong' have valid double 'oo' in Vietnamese!
                    _, check_v, _, _ = self._split_syllable()
                    if check_v != "oo":
                        self.disabled_for_word = True
                    self.last_modifier_key = None
                    return True

            for idx in reversed(vowel_indices):
                cur_v = self.chars[idx]
                if cur_v.lower() in target_bases and cur_v in hat_map:
                    # Simulate applying circumflex and verify the resulting vowel cluster is valid
                    old_v = self.chars[idx]
                    self.chars[idx] = hat_map[cur_v]
                    _, new_vowel, _, _ = self._split_syllable()
                    if new_vowel in VALID_CIRCUMFLEX_NUCLEI:
                        self.last_modifier_key = ch_lower
                        if self.config.auto_restore_english and not self.is_valid_vietnamese(committed=False):
                            self.disabled_for_word = True
                        return True
                    else:
                        # Revert if invalid (e.g. 'oe' + 'o' should be 'oeo' as in 'ngoèo', not 'ôe'!)
                        self.chars[idx] = old_v

        # 4b. Telex 'w' (ă, ơ, ư, ươ) or VNI '7' (ơ, ư, ươ) and '8' (ă)
        if (im == "telex" and ch_lower == "w") or (im == "vni" and ch in ("7", "8")):
            if im == "telex" and self.last_modifier_key == "w":
                for idx in reversed(vowel_indices):
                    unw_map = {"ă": "a", "Ă": "A", "ơ": "o", "Ơ": "O", "ư": "u", "Ư": "U"}
                    if self.chars[idx] in unw_map:
                        self.chars[idx] = unw_map[self.chars[idx]]
                if len(self.raw_keys) >= 2 and self.raw_keys[-2].lower() == "w" and len(self.chars) == 1 and self.chars[0].lower() == "ư":
                    self.chars[0] = self.raw_keys[-2]
                    self.disabled_for_word = True
                    return True
                self.chars.append(ch)
                self.disabled_for_word = True
                return True

            if len(vowel_indices) > 0:
                if im == "telex" or ch == "7":
                    # Case 1: 'uo' / 'uô' / 'ưo' + w/7 -> 'ươ'
                    for pos in range(len(vowel_indices) - 1):
                        i1, i2 = vowel_indices[pos], vowel_indices[pos + 1]
                        v1, v2 = self.chars[i1].lower(), self.chars[i2].lower()
                        if v1 in ("u", "ư") and v2 in ("o", "ô", "ơ"):
                            self.chars[i1] = "Ư" if self.chars[i1].isupper() else "ư"
                            self.chars[i2] = "Ơ" if self.chars[i2].isupper() else "ơ"
                            self.last_modifier_key = ch_lower
                            return True

                    # Case 2: 'ua' + w/7 -> 'ưa' (when initial is NOT 'qu', e.g. mưa, chưa, lửa)
                    if vowel == "ua" and initial != "qu":
                        i1 = vowel_indices[0]
                        self.chars[i1] = "Ư" if self.chars[i1].isupper() else "ư"
                        self.last_modifier_key = ch_lower
                        return True

                # Case 3: Single vowel transformation
                for idx in reversed(vowel_indices):
                    v_ch = self.chars[idx]
                    v_low = v_ch.lower()
                    if v_low in ("a", "ă", "â") and (im == "telex" or ch == "8"):
                        self.chars[idx] = "Ă" if v_ch.isupper() else "ă"
                        self.last_modifier_key = ch_lower
                        if self.config.auto_restore_english and not self.is_valid_vietnamese(committed=False):
                            self.disabled_for_word = True
                        return True
                    if v_low in ("o", "ô", "ơ") and (im == "telex" or ch == "7"):
                        self.chars[idx] = "Ơ" if v_ch.isupper() else "ơ"
                        self.last_modifier_key = ch_lower
                        if idx > vowel_indices[0]:
                            prev_idx = vowel_indices[vowel_indices.index(idx) - 1]
                            if self.chars[prev_idx].lower() == "u" and final != "":
                                self.chars[prev_idx] = "Ư" if self.chars[prev_idx].isupper() else "ư"
                        if self.config.auto_restore_english and not self.is_valid_vietnamese(committed=False):
                            self.disabled_for_word = True
                        return True
                    if v_low in ("u", "ư") and (im == "telex" or ch == "7"):
                        self.chars[idx] = "Ư" if v_ch.isupper() else "ư"
                        self.last_modifier_key = ch_lower
                        if self.config.auto_restore_english and not self.is_valid_vietnamese(committed=False):
                            self.disabled_for_word = True
                        return True

            # Standalone 'w' in Telex at start of syllable -> 'ư'
            if im == "telex" and len(vowel_indices) == 0:
                initial_str = "".join(c.lower() for c in self.chars)
                if initial_str in VALID_INITIALS and initial_str != "q":
                    self.chars.append("Ư" if ch.isupper() else "ư")
                    self.last_modifier_key = "w"
                    return True

        # 5. Normal character append
        self.chars.append(ch)
        self.last_modifier_key = None

        # 6. Auto-convert 'uơ' or 'ưo' + final consonant/semivowel -> 'ươ'
        initial, vowel, final, vowel_indices = self._split_syllable()
        if (vowel in ("uơ", "ưo") and final != "") or vowel in ("uơi", "ưoi", "uơu", "ưou"):
            i1, i2 = vowel_indices[0], vowel_indices[1]
            self.chars[i1] = "Ư" if self.chars[i1].isupper() else "ư"
            self.chars[i2] = "Ơ" if self.chars[i2].isupper() else "ơ"

        # 7. Check Phonotactics validity during typing
        if self.config.auto_restore_english and not self.is_valid_vietnamese(committed=False):
            self.disabled_for_word = True

        return False

    def process_key(self, ch: str) -> KeyProcessResult:
        if not ch or len(ch) != 1:
            return KeyProcessResult(False, 0, "", self.get_displayed_word(), "".join(self.raw_keys))

        # Word boundary (Space, Enter, Tab, Punctuation)
        if not (ch.isalpha() or (self.config.input_method == "vni" and ch.isdigit() and len(self.raw_keys) > 0)):
            old_display = self.get_displayed_word()
            raw_word = "".join(self.raw_keys)

            # 1. Check Macro expansion
            if self.config.enable_macros and raw_word:
                raw_lower = raw_word.lower()
                if raw_lower in self.config.macros:
                    expanded = self._apply_case_pattern(raw_word, self.config.macros[raw_lower])
                    self.reset()
                    return KeyProcessResult(
                        handled=True,
                        backspaces=len(old_display),
                        commit_text=expanded + ch,
                        current_word="",
                        raw_word="",
                        autocomplete_fix_needed=self.config.fix_autocomplete,
                    )

            # 2. Check Word-Boundary Auto-Restore for English words
            if (
                self.config.auto_restore_english
                and not self.disabled_for_word
                and not self.has_backspaced
                and raw_word
                and old_display != raw_word
                and not self.is_valid_vietnamese(committed=True)
            ):
                self.reset()
                return KeyProcessResult(
                    handled=True,
                    backspaces=len(old_display),
                    commit_text=raw_word + ch,
                    current_word="",
                    raw_word="",
                    autocomplete_fix_needed=self.config.fix_autocomplete,
                )

            self.reset()
            return KeyProcessResult(False, 0, "", "", "")

        old_display = self.get_displayed_word()
        self._process_char_internal(ch)
        new_display = self.get_displayed_word()
        raw_word = "".join(self.raw_keys)

        if new_display == old_display + ch:
            return KeyProcessResult(False, 0, ch, new_display, raw_word, False)

        common_len = 0
        min_len = min(len(old_display), len(new_display))
        while common_len < min_len and old_display[common_len] == new_display[common_len]:
            common_len += 1

        backspaces = len(old_display) - common_len
        commit_text = new_display[common_len:]

        return KeyProcessResult(
            handled=True,
            backspaces=backspaces,
            commit_text=commit_text,
            current_word=new_display,
            raw_word=raw_word,
            autocomplete_fix_needed=(self.config.fix_autocomplete and common_len == 0 and len(old_display) == 1),
        )

    def feed_sequence(self, text: str) -> str:
        self.reset()
        screen_buffer: List[str] = []
        for ch in text:
            if ch == "\b":
                res = self.process_backspace()
                if res.handled:
                    for _ in range(res.backspaces):
                        if screen_buffer:
                            screen_buffer.pop()
                    screen_buffer.extend(list(res.commit_text))
                else:
                    if screen_buffer:
                        screen_buffer.pop()
            else:
                res = self.process_key(ch)
                if res.handled:
                    for _ in range(res.backspaces):
                        if screen_buffer:
                            screen_buffer.pop()
                    screen_buffer.extend(list(res.commit_text))
                else:
                    screen_buffer.append(ch)
        return "".join(screen_buffer)
