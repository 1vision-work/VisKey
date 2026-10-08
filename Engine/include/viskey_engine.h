/*
 * viskey_engine.h — C API of the VisKey typing engine.
 *
 * The engine is derived from OpenKey (https://github.com/tuyenvm/OpenKey) by Mai Vũ Tuyên.
 * Modified by VisKey contributors.
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * The engine knows nothing about the platform: it answers "how many characters to delete and
 * which characters to insert". All state lives in an opaque vk_engine; engines are independent.
 * An engine is NOT thread-safe: use one engine from one thread at a time.
 */
#ifndef VISKEY_ENGINE_H
#define VISKEY_ENGINE_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct vk_engine vk_engine;

/* ---- options ------------------------------------------------------------------------- */
typedef enum {
    VK_OPT_LANGUAGE = 0,               /* 0 English, 1 Vietnamese */
    VK_OPT_INPUT_TYPE,                 /* vk_input_type */
    VK_OPT_CODE_TABLE,                 /* vk_code_table */
    VK_OPT_FREE_MARK,                  /* 1: skip grammar checks of mark placement */
    VK_OPT_CHECK_SPELLING,
    VK_OPT_MODERN_ORTHOGRAPHY,         /* 0: òa, úy; 1: oà, uý */
    VK_OPT_QUICK_TELEX,                /* cc=ch gg=gi kk=kh nn=ng qq=qu pp=ph tt=th uu=ươ */
    VK_OPT_RESTORE_IF_WRONG_SPELLING,
    VK_OPT_USE_MACRO,
    VK_OPT_USE_MACRO_IN_ENGLISH_MODE,
    VK_OPT_AUTO_CAPS_MACRO,
    VK_OPT_UPPERCASE_FIRST_CHAR,
    VK_OPT_ALLOW_CONSONANT_ZFWJ,
    VK_OPT_QUICK_START_CONSONANT,      /* f->ph j->gi w->qu */
    VK_OPT_QUICK_END_CONSONANT         /* g->ng h->nh k->ch */
} vk_option;

typedef enum { VK_INPUT_TELEX = 0, VK_INPUT_VNI = 1, VK_INPUT_SIMPLE_TELEX_1 = 2, VK_INPUT_SIMPLE_TELEX_2 = 3 } vk_input_type;

typedef enum {
    VK_TABLE_UNICODE = 0,
    VK_TABLE_TCVN3 = 1,
    VK_TABLE_VNI_WINDOWS = 2,
    VK_TABLE_UNICODE_COMPOUND = 3,
    VK_TABLE_CP1258 = 4
} vk_code_table;

/* ---- events -------------------------------------------------------------------------- */
typedef enum {
    VK_EVENT_KEY_DOWN = 0,
    VK_EVENT_MOUSE_DOWN = 1            /* mouse click: ends the current word; keycode is ignored */
} vk_event_kind;

/* Modifier bits for vk_handle_key. */
enum {
    VK_MOD_SHIFT = 1u << 0,
    VK_MOD_CAPS_LOCK = 1u << 1,
    VK_MOD_OTHER = 1u << 2             /* Cmd, Ctrl, Option, Fn... (anything that is not a typing modifier) */
};

/* vk_result.code */
enum {
    VK_DO_NOTHING = 0,                 /* pass the key through unchanged */
    VK_WILL_PROCESS = 1,               /* delete backspace_count chars, insert chars[] */
    VK_BREAK_WORD = 2,
    VK_RESTORE = 3,                    /* like WILL_PROCESS, then re-insert the typed key itself */
    VK_REPLACE_MACRO = 4,              /* delete backspace_count chars, insert the macro content, then the typed key */
    VK_RESTORE_AND_START_NEW_SESSION = 5
};

#define VK_MAX_RESULT_CHARS 64

/*
 * Result of one key event.
 *
 * chars[] holds "engine words" in DISPLAY order (first word is shown first). OpenKey returns them
 * reversed; this API already reversed them. A word is not a plain code point: it is either a key code
 * with flags or a character code of the current code table. Convert each word with vk_word_decode().
 *
 * For VK_REPLACE_MACRO, chars[] holds the first VK_MAX_RESULT_CHARS words of the macro content and
 * macro_total is the full length; fetch the rest with vk_last_macro_words(). For all other codes
 * macro_total is 0.
 *
 * ext_code keeps the OpenKey meaning: 1 word break, 2 delete key, 3 normal key, 4 do not send an
 * empty character first, 0 other.
 */
typedef struct {
    uint8_t  code;
    uint8_t  backspace_count;
    uint8_t  char_count;
    uint32_t chars[VK_MAX_RESULT_CHARS];
    uint8_t  ext_code;
    uint16_t macro_total;
} vk_result;

/* ---- lifecycle ----------------------------------------------------------------------- */
vk_engine* vk_create(void);
void       vk_destroy(vk_engine*);

/* Options may be changed at any time; the typing state is reset. */
void       vk_set_option(vk_engine*, vk_option key, int32_t value);
int32_t    vk_get_option(const vk_engine*, vk_option key);

/* ---- typing -------------------------------------------------------------------------- */
vk_result  vk_handle_key(vk_engine*, uint16_t keycode, uint32_t modifiers, vk_event_kind kind);

/* Start a new word: mouse click, app switch, Esc... */
void       vk_new_session(vk_engine*);

/* Temporary switches driven by modifier keys (Ctrl: spelling, Cmd/Alt: whole engine). */
void       vk_temp_off_spelling(vk_engine*);
void       vk_restore_spelling(vk_engine*);
void       vk_temp_off_engine(vk_engine*, int off);

/* Words of the macro content of the last VK_REPLACE_MACRO result, starting at offset. Returns count copied. */
size_t     vk_last_macro_words(const vk_engine*, size_t offset, uint32_t* out, size_t cap);

/*
 * Encode one engine word with the current code table, like OpenKey's SendNewCharString.
 * Writes 1 or 2 UTF-16 units (VNI/TCVN3/CP1258: low and high byte; compound: base and combining mark)
 * to out and returns the count; 0 if the word has no character.
 */
size_t     vk_word_decode(const vk_engine*, uint32_t word, uint16_t out[2]);

/* Character produced by a plain key (keycode, with 0x10000 set for Shift/Caps); 0 if none. */
uint16_t   vk_keycode_to_char(uint32_t keycode_with_caps);

/* ---- macro --------------------------------------------------------------------------- */
/* Blob format: see Engine/src/Macro.cpp (initMacroMap). Replaces the whole macro table. */
void       vk_macro_load(vk_engine*, const uint8_t* blob, size_t len);
/* Writes the blob to out (if cap is large enough) and returns the size needed. */
size_t     vk_macro_save(const vk_engine*, uint8_t* out, size_t cap);
int        vk_macro_add(vk_engine*, const char* text_utf8, const char* content_utf8);
int        vk_macro_delete(vk_engine*, const char* text_utf8);
/* Re-encode macro contents after a code table change. vk_set_option(VK_OPT_CODE_TABLE) calls it. */
void       vk_macro_reload(vk_engine*);

/* ---- smart switch -------------------------------------------------------------------- */
/* Returns -1 if the app was unknown (and records `current`), else its language (0 or 1). */
int        vk_smart_switch_get(vk_engine*, const char* bundle_id, int current);
void       vk_smart_switch_set(vk_engine*, const char* bundle_id, int language);

/* ---- convert tool -------------------------------------------------------------------- */
enum {
    VK_CONVERT_ALL_CAPS = 1u << 0,
    VK_CONVERT_ALL_LOWER = 1u << 1,
    VK_CONVERT_CAPS_FIRST_LETTER = 1u << 2,
    VK_CONVERT_CAPS_EACH_WORD = 1u << 3,
    VK_CONVERT_REMOVE_MARK = 1u << 4
};

/*
 * Convert UTF-8 text between code tables. `from` is the table the input bytes are in
 * (single-byte tables are given as UTF-8 encoded code points 0-255). Writes a NUL-terminated
 * UTF-8 result to out (truncated if cap is too small) and returns the full result length
 * excluding the NUL, so a return value >= cap means truncation.
 */
size_t     vk_convert_ex(vk_engine*, const char* utf8, size_t len, vk_code_table from, vk_code_table to,
                         uint32_t flags, char* out, size_t cap);
size_t     vk_convert(vk_engine*, const char* utf8, size_t len, vk_code_table from, vk_code_table to,
                      char* out, size_t cap);

#ifdef __cplusplus
}
#endif

#endif /* VISKEY_ENGINE_H */
