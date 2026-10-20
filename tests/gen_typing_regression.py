#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Generates tests/data/typing_regression.tsv (method<TAB>keys<TAB>expected).

Expected values come from typing RULES (the word itself, or the documented
escape/standalone behaviour), never from running the engine. The keystrokes
are derived from the Vietnamese text by a standalone Telex/VNI encoder below.

    python3 tests/gen_typing_regression.py          # rewrite the TSV
    python3 tests/gen_typing_regression.py --check  # fail if the TSV is stale
"""
import os
import sys
import unicodedata

HERE = os.path.dirname(os.path.abspath(__file__))
OUT_PATH = os.path.join(HERE, "data", "typing_regression.tsv")

# ---------------------------------------------------------------------------
# Corpus: common Vietnamese words / syllables
# ---------------------------------------------------------------------------
VIETNAMESE_WORDS = """
a á à ả ã ạ ă ắ ằ ẳ ẵ ặ â ấ ầ ẩ ẫ ậ e é è ẻ ẽ ẹ ê ế ề ể ễ ệ
i í ì ỉ ĩ ị o ó ò ỏ õ ọ ô ố ồ ổ ỗ ộ ơ ớ ờ ở ỡ ợ u ú ù ủ ũ ụ ư ứ ừ ử ữ ự
y ý ỳ ỷ ỹ ỵ
ba bá bà bả bã bạ ma má mà mả mã mạ la lá là lả lã lạ
ta tá tà tả tã tạ ca cá cà cả cạ
sẽ sĩ mĩ bõ rét cát lát lót lít hót xem tẽ bá dẽ nẽ lé mé
mút trút cót mót tết tét mét hét két sét
tôi bạn anh em chị ông bà cô chú bác cháu con cha mẹ vợ chồng
người ngày đêm sáng trưa chiều tối năm tháng tuần giờ phút giây
đi đến về ra vào lên xuống ở đứng ngồi nằm chạy đọc viết nói nghe nhìn thấy
ăn uống ngủ làm học chơi mua bán gửi nhận cho lấy đưa mang cầm
có không được là của và với cũng đã đang sẽ rồi chưa vẫn còn
này kia đó đây đấy nào gì sao thế vậy nhưng mà nếu thì vì nên
rất lắm quá hơn nhất nhiều ít mấy mọi những các một hai ba bốn
sáu bảy tám chín mười trăm nghìn triệu tỷ
tiếng việt nam hà nội huế sài gòn đà nẵng
nhà cửa đường phố xe máy điện thoại bàn ghế giường tủ sách vở bút
nước cơm phở bánh mì cà phê trà sữa thịt cá rau quả trứng muối đường
trời đất mưa gió nắng mây sông núi biển rừng cây hoa lá
đẹp xấu tốt tồi lớn nhỏ cao thấp dài ngắn mới cũ nóng lạnh
vui buồn yêu ghét thương nhớ quên mong muốn cần phải nên
hiểu biết nghĩ tin hỏi trả lời giúp đỡ chờ đợi gặp tìm
trường lớp thầy giáo sinh viên bài tập kiểm tra thi
công việc văn phòng máy tính phần mềm chương trình dữ liệu
tiền giá rẻ đắt chợ cửa hàng siêu thị
bệnh viện bác sĩ thuốc khỏe mạnh đau ốm
quốc gia dân tộc lịch sử văn hóa xã hội kinh tế chính trị
gia đình bố mẹ ông nội bà ngoại họ hàng
người việt tiếng anh nước ngoài
khuya khuyên khuyết quyển quyết quyền quý quỳnh
nguyễn huyền thuyền chuyện chuyển truyện tuyệt tuyết luyện
yêu yên yếu yến
uống uốn muốn cuộn cuốn chuông buồn luôn suốt thuộc
được nước bước trước người mười cười tười rượu hươu bướu
ươm ướt ướp ước
oa hoa hòa hóa hỏa khoa toán hoàn hoạt ngoài xoài khoai
xoáy ngoáy hoặc khoác ngoặc loắt
khỏe xòe lòe hòe
thúy thùy lũy quy tùy
thuế huế huệ quê quế
câu cầu đầu sâu trâu nâu lâu dâu
mây cây dây bây giây thấy đấy vậy
kêu nêu đều kều lều
kia mía chìa bia tía
múa lụa đũa chùa cua mua
mưa cửa giữa lửa chưa sữa
núi túi mùi bụi tủi
gửi ngửi
cứu hưu mưu cừu
tiêu hiểu nhiều chiều kiều điều
ngoằn ngoèo
giữ gìn giấy giờ giá giết giặt giỏi giúp gió giàu giường
gì ghi ghế ghét nghe nghĩ nghĩa nghiêng
kẻ kê kí kỹ kiếm kiểm kìa
gặp gắng gần gấp gậy
cách sách ếch lịch thích ích khách bạch
mạnh nhanh xanh lạnh cảnh tranh hành
bệnh lệnh kênh mênh
học đọc gốc bọc mộc cốc
thép đẹp lớp gặp tập kịp
mất đất bất chất nhất thật tất
đúng đứng chúng trong cũng không
xoong boong
đời mời chơi bơi rơi tơi
ơi ối ới ừ ừm ạ ấy ở ăn ốc ếch
"""

TITLE_WORDS = """
Việt Nam Hà Nội Huế Đà Nẵng Sài Gòn Nguyễn Trần Lê Phạm Hoàng Huỳnh Phan Võ Đặng Bùi Đỗ
Hồ Ngô Dương Lý Tiếng Người Đường Ước Ăn Ở Ơi Ừ Ánh Ông Ưu
"""

UPPER_WORDS = """
VIỆT NAM HÀ NỘI ĐƯỜNG NGƯỜI TIẾNG KHÔNG ĐƯỢC CHƯƠNG TRÌNH ĐẸP QUỐC GIA
TRƯỜNG HỌC THƯƠNG YÊU NƯỚC ĐI ĂN Ở Ư
"""

SENTENCES = [
    "Tôi yêu tiếng Việt",
    "Hôm nay trời đẹp quá",
    "Chúng ta cùng đi học nhé",
    "Bạn có khỏe không",
    "Cảm ơn bạn rất nhiều",
    "Xin chào các bạn",
    "Tôi đang làm việc ở văn phòng",
    "Mẹ tôi nấu cơm rất ngon",
    "Chiều nay mưa to lắm",
    "Anh ấy là bác sĩ",
    "Em sẽ về nhà sớm",
    "Họ đã đến Hà Nội rồi",
    "Cuộc sống thật tươi đẹp",
    "Người Việt Nam rất hiếu khách",
    "Đường phố đông người qua lại",
    "Mùa thu lá vàng rơi",
    "Tôi muốn uống một cốc cà phê sữa",
    "Con mèo nằm ngủ trên ghế",
    "Bố mẹ tôi sống ở quê",
    "Chương trình này rất hay",
    "Hãy giữ gìn sự trong sáng của tiếng Việt",
    "Chị ấy mua một chiếc xe máy mới",
    "Nước sông chảy rất mạnh",
    "Thời gian trôi qua nhanh quá",
    "Ông bà tôi đã già rồi",
    "Học sinh đang làm bài kiểm tra",
    "Bầu trời xanh ngắt không một gợn mây",
    "Quyển sách này của ai vậy",
    "Chúc mừng năm mới",
    "Tuyết rơi trắng xóa cả khu rừng",
    "Rượu ngon không say",
    "Hươu cao cổ ăn lá cây",
]

# Escapes: the doubled modifier key undoes the mark and leaves ONE literal key.
ESCAPE_CASES = [
    ("tess", "tes"), ("tesst", "test"), ("ww", "w"), ("uww", "uw"), ("ddd", "dd"),
    ("aaa", "aa"), ("eee", "ee"), ("ooo", "oo"), ("ass", "as"), ("aff", "af"),
    ("arr", "ar"), ("axx", "ax"), ("ajj", "aj"), ("aww", "aw"), ("oww", "ow"),
    ("tww", "tw"), ("WW", "W"), ("Ww", "W"), ("www", "ww"), ("Tess", "Tes"),
    ("TESS", "TES"), ("tooo", "too"), ("baff", "baf"), ("maxx", "max"), ("caff", "caf"),
    ("hoss", "hos"), ("lajj", "laj"), ("mee", "mê"), ("meee", "mee"), ("dddi", "ddi"),
    ("taaan", "taan"), ("tuww", "tuw"), ("chuoww", "chuow"), ("bawwn", "bawn"),
]

# A bare (modified) vowel with no initial and no final consonant stays Vietnamese.
STANDALONE_CASES = [
    ("w", "ư"), ("uw", "ư"), ("ow", "ơ"), ("aw", "ă"), ("aa", "â"), ("ee", "ê"),
    ("oo", "ô"), ("uow", "ươ"), ("uwow", "ươ"), ("W", "Ư"), ("UW", "Ư"), ("AW", "Ă"),
    ("Aa", "Â"), ("wf", "ừ"), ("ws", "ứ"), ("owf", "ờ"), ("aws", "ắ"), ("aaj", "ậ"),
    # a word that is only đ/Đ stays Vietnamese (UniKey); ddd still escapes to dd
    ("dd", "đ"), ("DD", "Đ"), ("Dd", "Đ"), ("DDD", "DD"), ("dd dd", "đ đ"),
]

# Same rules in VNI
VNI_EXTRA_CASES = [("d9", "đ"), ("D9", "Đ"), ("d99", "d9")]

# Backspace ("\\b" in the TSV) inside a word, including after an escape.
BACKSPACE_CASES = [
    ("tieengs\\b", "tiến"), ("tieengs\\b\\b", "tiế"), ("hoasn\\b", "hóa"),
    ("tesst\\b", "tes"), ("tess\\bt", "tet"), ("ww\\b", ""), ("ww\\bw", "ư"),
    ("ddd\\b", "d"), ("aaa\\ba", "aa"), ("windo\\bow", "window"),
]

# English words that are invalid Vietnamese: auto-restore keeps them as typed.
ENGLISH_WORDS = """
window windows software hardware refresh express update create delete service source
course house user server first world github macos linux google apple facebook
youtube twitter email phone mobile laptop desktop keyboard mouse screen monitor
printer network internet website browser chrome firefox safari office excel word
python java javascript script coding program function variable class object string
number integer float double boolean array table column index value return
import export module package library framework react angular vue node docker
kubernetes cloud amazon azure deploy build release version commit branch merge
pull push fetch clone rebase stash patch diff log status config setting option
hello world welcome thanks please sorry yes okay what where when which while
with without within about above after again against because before being below
between both during each from further have having into itself just more
other over same should some such than that their them then
they through under until very were will would your yours yourself
think thing things know knew known make made take took taken give gave given
work works worked play played player game games music movie video picture photo
travel hotel flight ticket airport train station market price money bank card
coffee water bread cheese chicken pizza burger salad fruit apple orange banana
happy angry funny lovely pretty smart strong weak young old new good great
morning evening night today tomorrow yesterday things kings music weekend monday tuesday friday
problem solution question answer example simple complex design develop review
""".split()

# English words whose doubled Telex keys (ss/ff/rr/xx/jj, ww/aa/ee/dd) would collapse to one
# key, plus common dev/tech words: with auto-restore they must come back unchanged at the space.
DOUBLE_KEY_ENGLISH = """
assistant assist assign assignment assume assert assets asset assess assessment possible impossible
mission session passed password passing passion missing missed discuss discussion essential necessary
professional access accessed essay lesson lessons message messages massive classic boss kiss miss less
mess toss loss pass mass bass guess glass grass cross across dress stress press express address process
success business happiness fitness wireless unless regardless effort effect effective offer office
official officer difference different differ suffer buffer staff stuff sniff cliff diff tariff sheriff
coffee toffee error errors mirror terror horror sorry carry marry worry hurry berry cherry current
correct correctly arrow narrow borrow tomorrow sorrow arrive arrange array barrel carrot parrot terrible
embarrass interrupt occurred preferred referred transferred ferry merry jerry hello
class ress async isset offset buff stdout todo json http https localhost docker kubectl npm git commit
merge rebase issue release
""".split()

# Collision words deliberately kept in the restore list (their Telex reading is
# not a common Vietnamese word, or an existing test pins them).
COLLISION_WORDS = """
test best post past soon moon room seen beer door poor more core store restore
pure sure nature future feature view new raw saw law draw true issue fix wax fox
next text west fast just pass miss boss lesson message possible error sorry
""".split()

# ---------------------------------------------------------------------------
# Standalone Telex / VNI encoder (independent of the engine)
# ---------------------------------------------------------------------------
TONE_MARKS = {"́": 1, "̀": 2, "̉": 3, "̃": 4, "̣": 5}
TELEX_TONE = {1: "s", 2: "f", 3: "r", 4: "x", 5: "j"}
VNI_TONE = {1: "1", 2: "2", 3: "3", 4: "4", 5: "5"}
VOWELS = set("aeiouy")


def decompose(ch):
    """-> (base ascii letter, modifier in {'', '^', '(', '+', 'd'}, tone 0..5)"""
    if ch in "đĐ":
        return ("d" if ch == "đ" else "D"), "d", 0
    nfd = unicodedata.normalize("NFD", ch)
    base, mod, tone = nfd[0], "", 0
    for m in nfd[1:]:
        if m == "̂":
            mod = "^"
        elif m == "̆":
            mod = "("
        elif m == "̛":
            mod = "+"
        elif m in TONE_MARKS:
            tone = TONE_MARKS[m]
        else:
            raise ValueError(f"unexpected mark in {ch!r}")
    return base, mod, tone


def encode(word, method, tone_after_vowel=False, uow=True):
    """Keystrokes for one Vietnamese word. Tone key goes at the end of the word,
    or (tone_after_vowel) right after the last vowel key, before the final consonant."""
    all_caps = word.isupper()
    parts = [decompose(c) for c in word]
    tone = max(t for _, _, t in parts)
    keys = []
    last_vowel_pos = -1
    i = 0
    while i < len(parts):
        base, mod, _ = parts[i]
        low = base.lower()
        if method == "telex" and uow and low == "u" and mod == "+" and i + 1 < len(parts) \
                and parts[i + 1][0].lower() == "o" and parts[i + 1][1] == "+":
            # ươ typed as 'uow'
            o_base = parts[i + 1][0]
            w = "W" if all_caps else "w"
            keys += [base, o_base, w]
            i += 2
            last_vowel_pos = len(keys)
            continue
        keys.append(base)
        if method == "telex":
            if mod == "^":
                keys.append(base if all_caps else low)
            elif mod in ("(", "+"):
                keys.append("W" if all_caps else "w")
            elif mod == "d":
                keys.append(base if all_caps else low)
            elif low == "o" and i + 1 < len(parts) and parts[i + 1][0].lower() == "o" and parts[i + 1][1] == "":
                # literal 'oo' (boong, xoong): o o o
                keys += [parts[i + 1][0], parts[i + 1][0]]
                i += 1
        else:
            if mod:
                keys.append({"^": "6", "(": "8", "+": "7", "d": "9"}[mod])
        if low in VOWELS:
            last_vowel_pos = len(keys)
        i += 1
    if tone:
        tk = (TELEX_TONE if method == "telex" else VNI_TONE)[tone]
        if all_caps and method == "telex":
            tk = tk.upper()
        pos = last_vowel_pos if tone_after_vowel else len(keys)
        keys.insert(pos, tk)
    return "".join(keys)


def has_final_consonant(word):
    return not unicodedata.normalize("NFD", word)[-1].lower() in VOWELS and word[-1] not in "đĐ"


def build_cases():
    cases = []
    seen = set()

    def add(method, keys, expected):
        if (method, keys) in seen:
            return
        seen.add((method, keys))
        cases.append((method, keys, expected))

    words = VIETNAMESE_WORDS.split() + TITLE_WORDS.split() + UPPER_WORDS.split()
    kept_collisions = set(COLLISION_WORDS)
    for w in words:
        add("telex", encode(w, "telex"), w)
        # 'tét' typed tone-first is 'test', which is deliberately restored to English
        if has_final_consonant(w) and encode(w, "telex", tone_after_vowel=True).lower() not in kept_collisions:
            add("telex", encode(w, "telex", tone_after_vowel=True), w)
        if "ươ" in w.lower():
            add("telex", encode(w, "telex", uow=False), w)
        add("vni", encode(w, "vni"), w)
    # Titlecase variants of lowercase words
    for w in VIETNAMESE_WORDS.split()[::3]:
        t = w[0].upper() + w[1:]
        add("telex", encode(t, "telex"), t)
        add("vni", encode(t, "vni"), t)

    for s in SENTENCES:
        add("telex", " ".join(encode(w, "telex") for w in s.split()), s)
        add("telex", " ".join(encode(w, "telex", tone_after_vowel=True) for w in s.split()), s)
        add("vni", " ".join(encode(w, "vni") for w in s.split()), s)

    for keys, expected in ESCAPE_CASES + STANDALONE_CASES + BACKSPACE_CASES:
        add("telex", keys, expected)

    for w in ENGLISH_WORDS + COLLISION_WORDS + DOUBLE_KEY_ENGLISH:
        add("telex", w, w)
        add("telex", w[0].upper() + w[1:], w[0].upper() + w[1:])
    for keys, expected in VNI_EXTRA_CASES:
        add("vni", keys, expected)
    # Without auto-restore, an English word that is not touched by Telex stays as typed,
    # and a Vietnamese reading stays Vietnamese.
    for keys, expected in [("hello", "hello"), ("sex", "sẽ"), ("test", "tét"), ("tess", "tes"),
                           ("ww", "w"), ("aw", "ă"), ("dd", "đ"), ("Tieengs", "Tiếng"), ("lesson", "leson")]:
        add("telex_norestore", keys, expected)
    return cases


def render(cases):
    lines = ["# Generated by tests/gen_typing_regression.py - do not edit by hand.",
             "# method\tkeys\texpected   (each case is typed followed by a space)"]
    lines += [f"{m}\t{k}\t{e}" for m, k, e in cases]
    return "\n".join(lines) + "\n"


def main():
    cases = build_cases()
    content = render(cases)
    if "--check" in sys.argv:
        with open(OUT_PATH, encoding="utf-8") as f:
            if f.read() != content:
                print(f"{OUT_PATH} is stale: run python3 tests/gen_typing_regression.py")
                sys.exit(1)
        print(f"{OUT_PATH} is up to date ({len(cases)} cases)")
        return
    os.makedirs(os.path.dirname(OUT_PATH), exist_ok=True)
    with open(OUT_PATH, "w", encoding="utf-8", newline="\n") as f:
        f.write(content)
    print(f"wrote {len(cases)} cases to {OUT_PATH}")


if __name__ == "__main__":
    main()
