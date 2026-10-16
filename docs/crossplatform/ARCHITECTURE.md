# Tài Liệu Kiến Trúc & Đặc Tả Kỹ Thuật VisKey (Dành Cho Developer)

> **Ghi chú về đường dẫn trong repo này:** mọi đường dẫn bên dưới (`core/…`, `macos/…`, `linux/…`, `tests/…`) nằm trong thư mục [`crossplatform/`](../../crossplatform/README.md), ví dụ `core/viskey_engine.hpp` → [`crossplatform/core/viskey_engine.hpp`](../../crossplatform/core/viskey_engine.hpp). Tài liệu này chỉ áp dụng cho nhánh `crossplatform/`, không phải app Swift ở thư mục gốc.
>
> Những điểm lệch so với mã hiện tại:
> - Số dòng ở §4.1 đã cũ: `english_collision_words_` được khởi tạo quanh dòng 448 của `viskey_engine.hpp`; `ENGLISH_COLLISION_WORDS` ở quanh dòng 93 của `viskey_engine.py`.
> - Trên macOS, `VisKeyController` hiện chỉ có menu `NSStatusItem`: chưa có cửa sổ `NSWindow` cấu hình, chưa lưu vào `NSUserDefaults` (tùy chọn mất khi thoát app).
> - `./test_cpp_engine` ở §4.1 là file nhị phân tự biên dịch từ `crossplatform/tests/test_cpp_engine.cpp` (xem [BUILD.md](BUILD.md) §2).

Tài liệu này mô tả chi tiết kiến trúc phần mềm, cấu trúc dữ liệu, máy trạng thái hữu hạn (FSM), thuật toán kiểm tra âm vị học tiếng Việt và cơ chế can thiệp sự kiện bàn phím cấp hệ điều hành của **VisKey** để bất kỳ lập trình viên C++, Objective-C++ hoặc Python nào cũng có thể đọc hiểu, bảo trì và mở rộng.

## 1. Kiến trúc tổng thể 3 tầng (3-Tier Architecture)

VisKey được thiết kế theo nguyên lý tách biệt hoàn toàn giữa **Thuật toán ngôn ngữ thuần túy (Core Engine)** và **Tầng giao tiếp hệ điều hành (OS Adapter & UI)**:

| Tầng kiến trúc | Thành phần file | Trách nhiệm kỹ thuật |
| :--- | :--- | :--- |
| **Tầng 1: Core Linguistic Engine (Độc lập nền tảng)** | `core/viskey_engine.hpp` (C++17)<br>`core/viskey_engine.py` (Python 3) | Nhận từng ký tự phím thô (`process_key`, `process_backspace`), duy trì bộ đệm âm tiết UTF-32, phân tích cấu trúc âm tiết (`initial`, `vowel`, `final`, `tone`), kiểm tra chính tả âm vị học, tự động khôi phục từ tiếng Anh, mở rộng Macro và trả về cấu trúc lệnh thay thế (`KeyProcessResult`). |
| **Tầng 2: OS Input Pipeline Adapter (Tích hợp hệ điều hành)** | `macos/VisKeyApp.mm` (`CGEventTap`)<br>`linux/ibus/ibus_viskey.py` (`IBus.Engine`) | Chặn phím vật lý từ bàn phím trước khi tới ứng dụng đích, lọc bỏ các phím tắt điều khiển (`Cmd`, `Ctrl`, `Alt`), chuyển đổi mã phím sang ký tự ASCII đưa vào Tầng 1, và thực thi lệnh xóa lùi (`Backspace` / `delete_surrounding_text`) + bơm chuỗi Unicode đầu ra. |
| **Tầng 3: Native UI & Persistence (Giao diện & Lưu trữ)** | `VisKeyController` (Cocoa `NSWindow` & `NSStatusItem`)<br>`viskey_settings.py` (Tkinter/GTK + JSON) | Cung cấp biểu tượng thanh trạng thái (`VI`/`EN`), cửa sổ cấu hình, trình quản lý từ gõ tắt (Macro), đồng bộ cấu hình xuống ổ cứng (`NSUserDefaults` trên macOS, `~/.config/viskey/config.json` trên Linux) và theo dõi thời gian thực cửa sổ ứng dụng đang active. |

## 2. Phân tích chuyên sâu Tầng 1: Core Linguistic Engine (`VisKeyEngine`)

### 2.1. Tại sao dùng UTF-32 (`std::u32string`) trong bộ lõi C++17?
Trong bảng mã UTF-8, một chữ cái tiếng Việt có dấu (như `ắ`, `ệ`, `ự`, `đ`) chiếm từ **2 đến 3 bytes**, nhưng khi hiển thị trên màn hình soạn thảo của macOS hay Linux, mỗi chữ cái đó là **1 điểm mã Unicode (1 codepoint)** và chỉ cần đúng **1 nhịp phím `Backspace`** để xóa.
* Nếu tính toán độ dài chuỗi trên `std::string` (UTF-8), số lượng `Backspace` gửi đi sẽ bị thừa (gửi 3 nhịp xóa cho 1 chữ `ệ`), gây xóa nhầm các chữ phía trước.
* Vì vậy, `core/viskey_engine.hpp` cài đặt hai hàm chuyển đổi nội bộ không phụ thuộc thư viện ngoài (`utf8_to_u32` và `u32_to_utf8`) để toàn bộ các biến trạng thái `raw_keys_` và `chars_` luôn thao tác trên mảng điểm mã `char32_t` (`std::u32string`).

### 2.2. Cấu trúc dữ liệu trạng thái của một âm tiết
Mỗi khi người dùng gõ một từ, `VisKeyEngine` lưu trữ 5 biến trạng thái cốt lõi:
* `raw_keys_` (`std::u32string` / `List[str]`): Lưu nguyên vẹn từng phím vật lý người dùng đã gõ theo đúng thứ tự thời gian và đúng chữ hoa/thường (ví dụ khi gõ `Tieengs`, `raw_keys_` là `"Tieengs"`). Bộ đệm này là cơ sở để khôi phục nguyên trạng từ tiếng Anh (`window`, `software`) hoặc tra bảng Macro.
* `chars_` (`std::u32string` / `List[str]`): Lưu các chữ cái cơ sở của âm tiết sau khi đã biến đổi dấu mũ/móc (`ă, â, ê, ô, ơ, ư, đ`), nhưng **chưa gắn dấu thanh điệu** (ví dụ với `Tiếng`, `chars_` là `['T', 'i', 'ê', 'n', 'g']`).
* `tone_` (`int`, giá trị `0..5`): Lưu thanh điệu của toàn âm tiết (`0`: Ngang, `1`: Sắc, `2`: Huyền, `3`: Hỏi, `4`: Ngã, `5`: Nặng). Việc tách rời `tone_` khỏi `chars_` cho phép dấu thanh **tự động dịch chuyển sang nguyên âm khác** khi người dùng gõ thêm nguyên âm hoặc phụ âm cuối mà không cần xóa dấu cũ.
* `disabled_for_word_` (`bool`): Cờ đánh dấu từ hiện tại đã thoát khỏi chế độ biến đổi tiếng Việt (do người dùng gõ đúp phím để thoát dấu như `ss, ff, ww, ooo`, hoặc do bộ kiểm tra âm vị học phát hiện từ đang gõ là tiếng Anh).
* `has_backspaced_` (`bool`): Cờ ghi nhận người dùng vừa nhấn phím `Backspace` để sửa chữ trong từ hiện tại, giúp ngăn cơ chế Auto-Restore khôi phục nhầm một từ tiếng Việt đang xóa dở.

### 2.3. Thuật toán tách cấu trúc âm tiết (`split_syllable`)
Một âm tiết tiếng Việt chuẩn được cấu thành từ 3 phần: **Phụ âm đầu (`initial`)** + **Vần nguyên âm (`vowel`)** + **Phụ âm cuối (`final_c`)**. Hàm `split_syllable` quét qua `chars_` và xử lý hai trường hợp ngoại lệ quan trọng nhất của tiếng Việt:
1. **Cụm phụ âm đầu `qu`**: Chữ `u` đi sau `q` luôn thuộc về phụ âm đầu `qu`, không được tính là nguyên âm trong cụm vần (nhờ đó trong từ `quả` hay `quốc`, nguyên âm đầu tiên nhận dấu là `a` và `ô` chứ không phải `u`; và khi gõ `q-u-a-w`, phím `w` sẽ biến `a` thành `ă` trong `quặp` thay vì biến `ua` thành `ưa`).
2. **Cụm phụ âm đầu `gi`**:
   * Nếu sau `gi` có ít nhất một nguyên âm khác (như `gia`, `giữ`, `giường`, `giải`), cả `gi` đóng vai trò là phụ âm đầu và nguyên âm chính bắt đầu từ chữ thứ 3.
   * Ngược lại, nếu sau `gi` không có nguyên âm nào khác (như `gì`, `gìn`, `giếng` khi mới gõ `g-i`), thì chỉ có `g` là phụ âm đầu và `i` chính là nguyên âm hạt nhân để nhận dấu huyền/sắc (`gì`, `gìn`).

### 2.4. Thuật toán định vị dấu thanh động (`find_tone_target_index`)
Mỗi khi hàm `render_syllable()` được gọi để kết xuất chuỗi hiển thị, vị trí đặt dấu thanh (`tone_`) trên mảng chỉ số nguyên âm `vowel_indices` (với số lượng nguyên âm $k$) được tính toán động theo 4 mức ưu tiên:
1. **Ưu tiên 1 (Cặp nguyên âm đôi `ươ`)**: Nếu trong cụm nguyên âm có `ư` đi liền với `ơ` (như `thương`, `người`, `hươu`, `mượn`), dấu thanh luôn đặt vào chữ **`ơ`**.
2. **Ưu tiên 2 (Nguyên âm mang dấu phụ `ă, â, ê, ô, ơ, ư`)**: Nếu có bất kỳ nguyên âm nào mang dấu mũ hoặc dấu móc (như `tuần`, `tiếng`, `tưởng`, `ngoằn`, `quyền`), dấu thanh luôn đặt vào nguyên âm đó.
3. **Ưu tiên 3 (Cụm 2 nguyên âm không dấu mũ/móc, $k = 2$)**:
   * Nếu **có phụ âm cuối** (`final_c != ""`, âm tiết đóng như `toán`, `hoàng`, `luôn`): dấu thanh luôn đặt vào nguyên âm thứ 2 (`vowel_indices[1]`). Nhờ quy tắc này, khi gõ `h-o-s-a` màn hình hiện `hóa` (dấu trên `o`), nhưng ngay khi gõ thêm `n` (`h-o-s-a-n`), hàm tự động chuyển dấu sang `a` thành `hoán`.
   * Nếu **không có phụ âm cuối** (âm tiết mở) và cụm nguyên âm là `oa`, `oe`, `uy`:
     * Khi `modern_orthography = false` (Kiểu truyền thống phổ biến): đặt dấu ở nguyên âm thứ nhất (`hòa`, `khỏe`, `thúy`).
     * Khi `modern_orthography = true` (Kiểu mới): đặt dấu ở nguyên âm thứ hai (`hoà`, `khoẻ`, `thuý`).
   * Các cụm 2 nguyên âm mở còn lại (`ai, ao, au, ay, eo, ia, iu, oi, ua, ui, ưa, ưu`): đặt dấu ở nguyên âm thứ nhất (`mái`, `mùa`, `lửa`, `ngựa`).
4. **Ưu tiên 4 (Cụm 3 nguyên âm, $k = 3$ như `oai, oay, oeo, uya, uyu`)**: Dấu thanh luôn đặt ở nguyên âm giữa (`vowel_indices[1]`, ví dụ `ngoài`, `xoáy`, `ngoèo`, `khuỷu`).

### 2.5. Thuật toán Kiểm tra Âm vị học & Tự động khôi phục từ tiếng Anh (`is_valid_vietnamese`)
Để người dùng có thể gõ xen kẽ tiếng Anh và tiếng Việt liên tục mà không cần tắt bộ gõ, hàm `is_valid_vietnamese(bool committed)` kiểm tra tính hợp lệ ở **2 thời điểm**:
* **Kiểm tra tức thời khi đang gõ (`committed = false`)**:
  1. Từ chứa chữ cái ngoại lai không thuộc tiếng Việt (`f, j, w, z`) sau bước phân tích.
  2. Phụ âm đầu không nằm trong tập 27 phụ âm đầu tiếng Việt (loại ngay các cụm phụ âm đầu tiếng Anh như `bl, br, cl, cr, dr, fl, fr, gl, gr, pl, pr, sc, sk, sl, sm, sn, sp, st, sw, tw, wr, sh, wh`).
  3. Xuất hiện cụm nguyên âm thứ hai nằm sau phụ âm cuối (từ đa âm tiết của tiếng Anh như `update`, `service`, `create`, `restore`).
  4. Vi phạm quy tắc chính tả phân bố phụ âm đầu: `c` đứng trước `i, e, ê, y` (loại ngay `center`, `service`, `source`, `voice`, `price`); `gh` hoặc `ngh` đứng trước nguyên âm khác `i, e, ê`; `k` đứng trước `a, o, u`.
  5. Phụ âm cuối không nằm trong 8 phụ âm cuối tiếng Việt (`c, ch, m, n, ng, nh, p, t`) — loại ngay các từ kết thúc bằng `b, d, f, g, h, k, l, r, s, v, w, x, z` như `book`, `food`, `good`, `feel`, `school`, `window`, `view`, `refresh`, `express`.
  6. Vi phạm **luật thanh điệu của phụ âm tắc cuối (`c, ch, p, t`)**: Trong tiếng Việt, âm tiết đóng bằng `c, ch, p, t` chỉ có thể mang thanh **Sắc (`1`)** hoặc **Nặng (`5`)**, tuyệt đối không bao giờ mang thanh Huyền (`2`), Hỏi (`3`) hay Ngã (`4`). Quy tắc này lập tức khôi phục mọi từ tiếng Anh có đuôi `-ft` (`soft`, `left`, `shift`), `-rt` (`part`, `start`, `short`, `port`) và `-xt` (`text`, `next`).
* **Kiểm tra hoàn thiện khi chốt từ (`committed = true`, khi nhấn Space hoặc dấu câu)**:
  1. **Luật cấm thanh Ngang (`tone_ == 0`) trên phụ âm tắc cuối (`c, ch, p, t`)**: Mọi từ tiếng Việt kết thúc bằng `c, ch, p, t` bắt buộc phải có dấu Sắc hoặc Nặng (`mát/mạt`, `tốt/tột`, `học/hốc`, `lập/lấp`). Do đó, các từ tiếng Anh có `oo` hoặc `ee` đi với `p, t, c` mà không có dấu thanh như `keep`, `deep`, `feet`, `meet`, `root`, `boot`, `loop` sẽ được phát hiện và khôi phục về nguyên gốc tiếng Anh ngay khi nhấn phím Cách (`Space`).
  2. **Luật cấm nguyên âm ngắn và nguyên âm đôi mở (`ă, â, iê, yê, uô, ươ, oă, uă, uâ, uyê`)**: Trong tiếng Việt, các nguyên âm này không bao giờ đứng cuối một từ mà không có phụ âm cuối hoặc bán nguyên âm đi kèm. Nhờ đó, khi gõ các từ tiếng Anh như `raw` (tạm biến thành `ră`), `saw` (`să`), `law` (`lă`), `draw` (`dră`), ngay khi nhấn `Space`, VisKey nhận ra `ă` không có phụ âm cuối và tự động khôi phục thành `raw `, `saw `, `law `, `draw `.
  3. **Bảng từ vựng giao thoa (`english_collision_words_`)**: Xử lý các từ tiếng Anh phổ biến mà chuỗi phím Telex vô tình trùng khít với một âm tiết tiếng Việt hợp lệ (như `soon` -> `sôn`, `moon` -> `môn`, `room` -> `rôm`, `beer` -> `bể`, `user`, `true`, `issue`, `test`, `mode`, `code`). Lưu ý với từ `test` (`t-e-s-t`, chữ `s` gõ trước `t`) sẽ khôi phục thành `test`, trong khi gõ tiếng Việt `tets` (`s` ở cuối) vẫn ra chữ `tét` bình thường.

### 2.6. Tối ưu hóa số lượng Backspace bằng Tiền tố chung dài nhất (`common_len`)
Khi âm tiết chuyển từ trạng thái hiển thị cũ (`old_display`) sang trạng thái mới (`new_display`), thay vì xóa toàn bộ từ rồi chèn lại từ đầu, `process_key` tìm độ dài tiền tố chung dài nhất `common_len`:
```cpp
size_t common_len = 0;
size_t min_len = std::min(old_display.size(), new_display.size());
while (common_len < min_len && old_display[common_len] == new_display[common_len]) {
    common_len++;
}
int backspaces = static_cast<int>(old_display.size() - common_len);
std::string commit_utf8 = u32_to_utf8(new_display.substr(common_len));
```
Ví dụ: Khi màn hình đang có chữ `chương` (6 ký tự) và người dùng nhấn phím `f` (dấu huyền) để thành `chường`:
* `old_display` = `"chương"`, `new_display` = `"chường"`.
* Tiền tố chung `common_len` là 3 ký tự (`"chư"`).
* Engine chỉ cần gửi **3 nhịp `Backspace`** (xóa `"ơng"`) và chèn **`"ờng"`**, giữ nguyên 3 chữ đầu `"chư"` đứng yên trên màn hình.

## 3. Phân tích chuyên sâu Tầng 2: Cơ chế Bắt phím Hệ điều hành

### 3.1. Trên macOS (`macos/VisKeyApp.mm`)
1. **Vòng đời sự kiện `CGEventTap`**:
   Ứng dụng đăng ký một hook mức HID (`kCGHIDEventTap`, `kCGHeadInsertEventTap`) lắng nghe `kCGEventKeyDown`, `kCGEventFlagsChanged` và các sự kiện click chuột.
2. **Đánh dấu sự kiện nhân tạo (`kVisKeyEventMagic = 0x564B5821`)**:
   Khi `sendReplacementEvents()` phát các sự kiện `Backspace` và chuỗi Unicode thay thế vào hệ thống, chính `VisKeyEventCallback` cũng sẽ nhận lại các sự kiện đó. Để tránh vòng lặp vô hạn, mỗi `CGEventRef` do VisKey tạo ra đều được gắn nhãn `0x564B5821` vào trường `kCGEventSourceUserData`. Khi callback thấy nhãn này, nó cho sự kiện đi thẳng tới ứng dụng đích (`return event;`).
3. **Giải thuật sửa lỗi Autocomplete trên Spotlight / Chrome / Safari / Arc (`U+202F` Injection)**:
   * **Bài toán**: Khi gõ chữ đầu tiên của từ (ví dụ chữ `t` trong `tiếng`) vào thanh địa chỉ trình duyệt hoặc Spotlight, ứng dụng lập tức chèn phần gợi ý được bôi đen ngay sau con trỏ: `t[inhte.vn]`. Nếu tiếp theo người dùng gõ `a` + `s` (`tá`), bộ gõ gửi 1 phím `Backspace` với mục đích xóa chữ `t` rồi chèn chữ `á`. Nhưng vì `[inhte.vn]` đang bị bôi đen, phím `Backspace` đó chỉ xóa mất phần bôi đen `[inhte.vn]` mà **chữ `t` vẫn còn nguyên**, dẫn đến kết quả trên màn hình bị nhân đôi thành `ttá`!
   * **Giải pháp**: Khi `needAutocompleteFix` bật (`common_len == 0 && old_display.size() == 1`), trước khi gửi chuỗi `Backspace` chính thức, `sendReplacementEvents` gửi 1 ký tự `U+202F` (Narrow No-Break Space) rồi gửi 1 phím `Backspace`. Ký tự `U+202F` sẽ ghi đè lên vùng bôi đen `[inhte.vn]`, nhịp `Backspace` đầu tiên xóa ký tự `U+202F` đó, và nhịp `Backspace` tiếp theo sẽ xóa chính xác chữ `t` đầu tiên!

### 3.2. Trên Linux (`linux/ibus/ibus_viskey.py`)
Trên Linux, hàm `do_process_key_event(keyval, keycode, state)` trong lớp `VisKeyIBusEngine` hỗ trợ 2 cơ chế xuất chữ:
1. **Chế độ Không gạch chân (`self.no_underline_mode = True`)**:
   * Khi `res.handled == False`, trả về `False` để ứng dụng nhận phím bình thường.
   * Khi `res.handled == True`, gọi `_send_direct_replacement(res.backspaces, res.commit_text)`:
     * Nếu ứng dụng đích báo cáo hỗ trợ `IBus.Capabilite.SURROUNDING_TEXT`, engine gọi `self.delete_surrounding_text(-backspaces, backspaces)` để xóa trực tiếp các ký tự phía trước con trỏ trong bộ đệm của ứng dụng.
     * Nếu ứng dụng không hỗ trợ Surrounding Text, engine phát sự kiện phím `IBus.KEY_BackSpace` qua `self.forward_key_event()`, sau đó gọi `self.commit_text()` để chèn chuỗi Unicode mới mà không bật vùng gạch chân `preedit`.
2. **Chế độ Wayland-Safe Preedit (`self.no_underline_mode = False`)**:
   * Dùng cho các ứng dụng chạy trên Wayland nghiêm ngặt chặn `forward_key_event`.
   * Để khắc phục lỗi rớt chữ khi click chuột hoặc chuyển cửa sổ của `ibus-unikey` / `fcitx5-unikey`, các hàm `do_focus_out()`, `do_reset()` và nhánh bắt phím điều khiển đều gọi `self._commit_preedit_if_any()` nhằm chốt ngay từ đang gõ xuống văn bản trước khi xóa bộ đệm.

## 4. Hướng dẫn Mở rộng (How to Extend) dành cho Developer

### 4.1. Thêm quy tắc gõ mới hoặc từ tiếng Anh ngoại lệ
* Để thêm các từ tiếng Anh có cấu trúc trùng với âm tiết tiếng Việt vào danh sách tự động khôi phục, bạn chỉ cần thêm từ đó (viết thường) vào tập hợp `english_collision_words_` trong `core/viskey_engine.hpp` (dòng 314) và `ENGLISH_COLLISION_WORDS` trong `core/viskey_engine.py` (dòng 89).
* Sau khi sửa, chạy `./test_cpp_engine` và `python3 tests/test_python_engine.py` để kiểm tra hồi quy (Regression Test).

### 4.2. Tích hợp nhân C++17 `VisKeyEngine` vào Fcitx5 trên Linux
Vì `core/viskey_engine.hpp` là một file Header-only C++17 độc lập không phụ thuộc nền tảng, nếu bạn muốn viết thêm một Addon C++ gốc cho **Fcitx5** (`fcitx5-viskey`), bạn chỉ cần `#include "viskey_engine.hpp"` vào lớp kế thừa `fcitx::InputMethodEngineV2` và gọi `engine_.process_key(ch)` trong hàm `keyEvent(const fcitx::InputMethodEntry&, fcitx::KeyEvent& keyEvent)` theo đúng giao diện `KeyProcessResult` giống hệt như trong `macos/VisKeyApp.mm`.
