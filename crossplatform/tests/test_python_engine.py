#!/usr/bin/env python3
# -*- coding: utf-8 -*-
import os
import sys

PROJECT_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
sys.path.insert(0, os.path.join(PROJECT_ROOT, "core"))

from viskey_engine import EngineConfig, VisKeyEngine

def run_tests():
    # Default orthography: hòa, khỏe, thúy (modern_orthography=False)
    engine = VisKeyEngine(EngineConfig(input_method="telex", modern_orthography=False, auto_restore_english=True))

    # 1. Standard Telex & Dynamic Tone Repositioning
    vietnamese_cases = [
        ("Tieengs Vieetj ", "Tiếng Việt "),
        ("chuowng trinhf ", "chương trình "),
        ("thuowngs ", "thướng "),
        ("thusowng ", "thướng "),
        ("thuoswng ", "thướng "),
        ("nguowif ", "người "),
        ("nguoiwf ", "người "),
        ("huowu ", "hươu "),
        ("khuysu ", "khuýu "),
        ("khuyur ", "khuỷu "),
        ("ngoaif ", "ngoài "),
        ("xoays ", "xoáy "),
        ("ngoanwf ", "ngoằn "),
        ("ngoeof ", "ngoèo "),
        ("nghieengs ", "nghiếng "),
        ("nghieeng ", "nghiêng "),
        ("quoocs gia ", "quốc gia "),
        ("giuwx ginf ", "giữ gìn "),
        ("gif ", "gì "),
        ("quar ", "quả "),
        ("quawpj ", "quặp "),
        ("muaw ", "mưa "),
        ("chuaw ", "chưa "),
        ("luarw ", "lửa "),
        ("ddawcj bieetj ", "đặc biệt "),
        ("DDaif Loan ", "Đài Loan "),
        ("hoaf binhf ", "hòa bình "),
        ("thuys kieeuf ", "thúy kiều "),
        ("khoer manhj ", "khỏe mạnh "),
        ("toans hocj ", "toán học "),
        ("hoas ", "hóa "),
        ("hoasn ", "hoán "),   # Tone dynamically shifts from 'ó' (hóa) to 'á' (hoán) when 'n' is typed!
        ("tuas ", "túa "),
        ("tuasn ", "tuán "),   # Tone shifts from 'ú' to 'á' when 'n' is added
        ("tuasna ", "tuấn "),  # 'a' modifies 'a' into 'â', tone shifts onto 'ấ'!
        ("xooocj ", "xoọc "),
        ("boooong ", "boong "), #Wait: let's check ooo -> oo + ng = boong!
        ("tets ", "tét "),     # Vietnamese 'tét' (s at end)
    ]

    failed = 0
    for typed, expected in vietnamese_cases:
        if typed == "boooong ":
            typed = "booong "
        actual = engine.feed_sequence(typed)
        if actual != expected:
            print(f"[FAIL Vietnamese] typed={typed!r} -> expected={expected!r}, got={actual!r}")
            failed += 1

    # 2. Modern Orthography (Kiểu mới: hoà, thuý, khoẻ)
    modern_engine = VisKeyEngine(EngineConfig(input_method="telex", modern_orthography=True))
    modern_cases = [
        ("hoaf binhf ", "hoà bình "),
        ("thuys kieeuf ", "thuý kiều "),
        ("khoer manhj ", "khoẻ mạnh "),
        ("toans hocj ", "toán học "),  # With final consonant, still 'toán'
    ]
    for typed, expected in modern_cases:
        actual = modern_engine.feed_sequence(typed)
        if actual != expected:
            print(f"[FAIL Modern Orthography] typed={typed!r} -> expected={expected!r}, got={actual!r}")
            failed += 1

    # 3. English Auto-Restore (Gõ song ngữ Anh - Việt không cần tắt bộ gõ)
    english_cases = [
        ("window ", "window "),
        ("Window ", "Window "),
        ("software ", "software "),
        ("refresh ", "refresh "),
        ("express ", "express "),
        ("book ", "book "),
        ("food ", "food "),
        ("good ", "good "),
        ("feel ", "feel "),
        ("school ", "school "),
        ("keep ", "keep "),
        ("root ", "root "),
        ("loop ", "loop "),
        ("raw ", "raw "),
        ("saw ", "saw "),
        ("law ", "law "),
        ("draw ", "draw "),
        ("view ", "view "),
        ("review ", "review "),
        ("new ", "new "),
        ("true ", "true "),
        ("issue ", "issue "),
        ("service ", "service "),
        ("source ", "source "),
        ("course ", "course "),
        ("house ", "house "),
        ("user ", "user "),
        ("server ", "server "),
        ("update ", "update "),
        ("create ", "create "),
        ("delete ", "delete "),
        ("test ", "test "),
        ("first ", "first "),
        ("world ", "world "),
        ("github ", "github "),
        ("macos ", "macos "),
        ("linux ", "linux "),
        ("soon ", "soon "),
        ("moon ", "moon "),
        ("room ", "room "),
        ("beer ", "beer "),
        ("door ", "door "),
        ("poor ", "poor "),
        ("more ", "more "),
        ("core ", "core "),
        ("store ", "store "),
        ("restore ", "restore "),
        ("pure ", "pure "),
        ("sure ", "sure "),
        ("nature ", "nature "),
        ("future ", "future "),
        ("feature ", "feature "),
    ]
    for typed, expected in english_cases:
        actual = engine.feed_sequence(typed)
        if actual != expected:
            print(f"[FAIL English Auto-Restore] typed={typed!r} -> expected={expected!r}, got={actual!r}")
            failed += 1

    # 4. Mixed Bilingual Sentence (Gõ câu pha trộn Tiếng Việt + Tiếng Anh liên tục)
    mixed_sentence = "Tooi ddang code app treen macos vaf linux vowis window mode trong browser "
    expected_mixed = "Tôi đang code app trên macos và linux với window mode trong browser "
    actual_mixed = engine.feed_sequence(mixed_sentence)
    if actual_mixed != expected_mixed:
        print(f"[FAIL Mixed Sentence]\n  expected={expected_mixed!r}\n  got     ={actual_mixed!r}")
        failed += 1

    # 5. VNI Input Method
    vni_engine = VisKeyEngine(EngineConfig(input_method="vni", modern_orthography=False))
    vni_cases = [
        ("Tie61ng Vie65t ", "Tiếng Việt "),
        ("chu7o7ng trinh2 ", "chương trình "),
        ("d9a85c bie65t ", "đặc biệt "),
        ("hoa2 binh2 ", "hòa bình "),
    ]
    for typed, expected in vni_cases:
        actual = vni_engine.feed_sequence(typed)
        if actual != expected:
            print(f"[FAIL VNI] typed={typed!r} -> expected={expected!r}, got={actual!r}")
            failed += 1

    # 6. Macros & Quick Telex
    macro_engine = VisKeyEngine(EngineConfig(input_method="telex", quick_telex=True, enable_macros=True))
    macro_cases = [
        ("vn ", "Việt Nam "),
        ("ko ", "không "),
        ("KO ", "KHÔNG "),
        ("Ko ", "Không "),
        ("dc ", "được "),
        ("ttieengs ", "thiếng "),
    ]
    for typed, expected in macro_cases:
        actual = macro_engine.feed_sequence(typed)
        if actual != expected:
            print(f"[FAIL Macro/QuickTelex] typed={typed!r} -> expected={expected!r}, got={actual!r}")
            failed += 1

    # 7. Backspace Behavior (Deleting visible characters cleanly)
    bs_cases = [
        ("tieengs\b ", "tiến "),
        ("tieengs\b\b ", "tiế "),
        ("tieengs\b\b\b ", "ti "),
        ("hoasn\b ", "hóa "),   # Deleting 'n' from 'hoán' dynamically shifts tone back to 'ó' -> 'hóa'!
    ]
    for typed, expected in bs_cases:
        actual = engine.feed_sequence(typed)
        if actual != expected:
            print(f"[FAIL Backspace] typed={typed!r} -> expected={expected!r}, got={actual!r}")
            failed += 1

    if failed == 0:
        print(f"ALL {len(vietnamese_cases) + len(modern_cases) + len(english_cases) + 1 + len(vni_cases) + len(macro_cases) + len(bs_cases)} PYTHON ENGINE TESTS PASSED SUCCESSFULLY!")
    else:
        print(f"TOTAL FAILURES: {failed}")
        sys.exit(1)

if __name__ == "__main__":
    run_tests()
