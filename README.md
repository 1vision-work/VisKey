# VisKey

Bộ gõ tiếng Việt native cho macOS — gõ đúng ở mọi nơi.

> Trạng thái: đang phát triển (M0). Chưa có bản phát hành.

- License: GPL-3.0-or-later
- Based on OpenKey by Mai Vũ Tuyên — https://github.com/tuyenvm/OpenKey
- By 1VISION

Spec dự án: [CLAUDE.md](CLAUDE.md). Quyết định kỹ thuật: [docs/decisions.md](docs/decisions.md).

## Engine (C++17, chạy được trên Linux và macOS)

```sh
cmake -S Engine -B build/engine && cmake --build build/engine -j
ctest --test-dir build/engine --output-on-failure
```

Xem [Engine/tests/README.md](Engine/tests/README.md) về snapshot so với engine OpenKey gốc.
