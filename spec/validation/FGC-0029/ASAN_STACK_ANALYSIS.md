# FGC-0029 R19 — ASan 报告三栈完整展开

本文件取代 R18 版本。所有帧表由脚本 `work/validation/fgc0029-r19/stack_report.py` 生成：
它解析 ASan 报告里的每一帧，用 **PE `.pdata`（异常目录）函数边界表**确定"该 RVA 属于哪个函数"，
只有当函数起点恰好等于某个导出符号时，才给出函数名；否则标注为
***(unnamed internal function)*** 并给出区间与偏移。**不再使用"最近导出符号"**。

- Qt6Core.dll：6.11.1.0，SHA256 `FAE4778A42E93ADC82B831C879C886A05147E9CC26760808D21116BE5547259B`
  （本文件的所有 RVA 都以该 DLL 为前提；DLL 未变，故 R17/R18 的 RVA 仍适用）。
- `.pdata` 函数数：**13642**；导出函数起点：**7290**。
- ASan 报告的 RVA 是"返回地址 − 1"（ASan 的符号化约定），因此帧偏移与实际 `call` 指令相差 1 字节。

## 0. 符号解析方法与置信度约定

| 符号解析方法 | 含义 | 置信度 |
|---|---|---|
| `export at function start` | `.pdata` 函数起点恰好是该导出符号 | **确定** |
| `no export at function start` | 函数无导出；区间由 `.pdata` 确定 | 函数**边界**确定，**名称未定** |
| 调用指令交叉验证 | 反汇编该函数并比对调用目标/参数与 Qt 源码 | 在表中单独注明（高/中高/低） |

已用调用指令确认的链路（详见 `QT_LOCALE_SOURCE_AUDIT.md` §6）：

```
QLocale::toString(qlonglong) [0x170770)
  → [0x167300)    内部函数（源码候选 QLocaleData::longLongToString）  置信度 中高
    → [0x167180)  内部函数（源码候选 QLocaleData::zeroDigit）          置信度 中高
      → [0x16D700) 内部函数（systemLocaleString，唯一具备 query→metaType→toString 序列）置信度 高
        → QSystemLocale::query(ZeroDigit) [0x268210)
          → [0x26C840) 内部函数（QSystemLocalePrivate::zeroDigit：GetLocaleInfo(…,11) → QString(buf,1) → 写成员 → 释放旧值）置信度 高
```

## 1. 五份报告（逐帧）

> 说明：`r11-i15`、`r13-i10` 来自 R17（`GeoTagControllerTest`）；
> `a-r5-i1` 来自 R18（仅 `QDateTime::fromString`）；`a-r12-i10`、`legacy-r10-i08` 来自 R19
> （B 臂与 C 臂，统一 16 进程 × 15 轮、1000 迭代/线程）。
# Function ranges from Qt6Core.dll: 13642 functions, 7290 exported starts

## `r11-i15.err`

- ==7660==ERROR: AddressSanitizer: attempting double-free on 0x12ad9c41ceb0 in thread T4:
- 0x12ad9c41ceb0 is located 0 bytes inside of 20-byte region [0x12ad9c41ceb0,0x12ad9c41cec4)

### allocation — previously allocated by thread T4 here:

| # | module | RVA | function range | function | symbol method |
|---:|---|---|---|---|---|
| 0 | (exe / CRT) | — | — | in malloc D:\a\_work\1\s\src\vctools\asan\llvm\compiler-rt\lib\asan\asan_malloc_win.cpp:134 | ASan symbolization |
| 1 | Qt6Core.dll | 0x001C236D | [0x001C2270,0x001C239C) +0xFD | `?allocate2@QArrayData@@SAPEAXPEAPEAU1@_JW4AllocationOption@1@@Z` | export at function start |
| 2 | Qt6Core.dll | 0x0017F3CD | [0x0017F330,0x0017F439) +0x9D | `??0QString@@QEAA@PEBVQChar@@_J@Z` | export at function start |
| 3 | Qt6Core.dll | 0x0026C89E | [0x0026C840,0x0026C94B) +0x5E | *(unnamed internal function)* | no export at function start |
| 4 | Qt6Core.dll | 0x002686A2 | [0x00268210,0x00268D18) +0x492 | `?query@QSystemLocale@@UEBA?AVQVariant@@W4QueryType@1@$$QEAV2@@Z` | export at function start |
| 5 | Qt6Core.dll | 0x0016D7B1 | [0x0016D700,0x0016D8FB) +0xB1 | *(unnamed internal function)* | no export at function start |
| 6 | Qt6Core.dll | 0x001671AC | [0x00167180,0x00167216) +0x2C | *(unnamed internal function)* | no export at function start |
| 7 | Qt6Core.dll | 0x00167363 | [0x00167300,0x0016745F) +0x63 | *(unnamed internal function)* | no export at function start |
| 8 | Qt6Core.dll | 0x001707A8 | [0x00170770,0x001707B2) +0x38 | `?toString@QLocale@@QEBA?AVQString@@_J@Z` | export at function start |
| 9 | Qt6Core.dll | 0x002ACC75 | [0x002ACC20,0x002AD5EF) +0x55 | *(unnamed internal function)* | no export at function start |
| 10 | Qt6Core.dll | 0x002B00BF | [0x002AFFE0,0x002B01AE) +0xDF | *(unnamed internal function)* | no export at function start |
| 11 | Qt6Core.dll | 0x002AE927 | [0x002ADBB0,0x002AEACB) +0xD77 | *(unnamed internal function)* | no export at function start |
| 12 | Qt6Core.dll | 0x002A8282 | [0x002A7F40,0x002A82EA) +0x342 | *(unnamed internal function)* | no export at function start |
| 13 | Qt6Core.dll | 0x002A45E4 | [0x002A4550,0x002A4F9A) +0x94 | *(unnamed internal function)* | no export at function start |
| 14 | Qt6Core.dll | 0x001B4A78 | [0x001B4990,0x001B4B85) +0xE8 | *(unnamed internal function)* | no export at function start |
| 15 | Qt6Core.dll | 0x001B564E | [0x001B5623,0x001B5704) +0x2B | *(unnamed internal function)* | no export at function start |
| 16 | Qt6Core.dll | 0x001B0B8D | [0x001B0A10,0x001B0BC8) +0x17D | *(unnamed internal function)* | no export at function start |
| 17 | Qt6Core.dll | 0x001B6EA4 | [0x001B6E20,0x001B7186) +0x84 | `?startOfDay@QDate@@QEBA?AVQDateTime@@AEBVQTimeZone@@@Z` | export at function start |
| 18 | Qt6Core.dll | 0x002B3239 | [0x002B31F0,0x002B3378) +0x49 | `?fromString@QDateTimeParser@@QEBA_NAEBVQString@@PEAVQDateTime@@H@Z` | export at function start |
| 19 | Qt6Core.dll | 0x001B348B | [0x001B33A0,0x001B350F) +0xEB | `?fromString@QDateTime@@SA?AV1@AEBVQString@@VQStringView@@HVQCalendar@@@Z` | export at function start |
| 20 | Qt6Core.dll | 0x001B3381 | [0x001B3330,0x001B339A) +0x51 | `?fromString@QDateTime@@SA?AV1@AEBVQString@@VQStringView@@H@Z` | export at function start |
| 21 | Qt6Core.dll | 0x00004DDF | [0x00004DB0,0x00004DE9) +0x2F | `?fromString@QDateTime@@SA?AV1@AEBVQString@@0H@Z` | export at function start |
| 22 | (exe / CRT) | — | — | in ExifParser::readTime(class QByteArray const &) | ASan symbolization |
| 23 | (exe / CRT) | — | — | in GeoTagController::_parseExifForImage(class QFileInfo const &) | ASan symbolization |
| 24 | (exe / CRT) | — | — | in GeoTagController::_startParseExif::__l2::<lambda_1>::operator() | ASan symbolization |
| 25 | (exe / CRT) | — | — | in std::invoke | ASan symbolization |
| 26 | (exe / CRT) | — | — | in QtConcurrent::MappedEachKernel<QList<QFileInfo>::const_iterator,`GeoTagController::_startParseExif'::`2'::<lambda_1> >::runIteration | ASan symbolization |
| 27 | (exe / CRT) | — | — | in QtConcurrent::MappedEachKernel<QList<QFileInfo>::const_iterator,`GeoTagController::_startParseExif'::`2'::<lambda_1> >::runIterations | ASan symbolization |
| 28 | (exe / CRT) | — | — | in QtConcurrent::IterateKernel<class QList<class QFileInfo>::const_iterator, struct GeoTagController::ExifResult>::forThreadFunction(void) | ASan symbolization |
| 29 | (exe / CRT) | — | — | in QtConcurrent::IterateKernel<class QList<class QFileInfo>::const_iterator, struct GeoTagController::ExifResult>::threadFunction(void) | ASan symbolization |

### first free — freed by thread T14 here:

| # | module | RVA | function range | function | symbol method |
|---:|---|---|---|---|---|
| 0 | (exe / CRT) | — | — | in free D:\a\_work\1\s\src\vctools\asan\llvm\compiler-rt\lib\asan\asan_malloc_win.cpp:125 | ASan symbolization |
| 1 | Qt6Core.dll | 0x0026C8F8 | [0x0026C840,0x0026C94B) +0xB8 | *(unnamed internal function)* | no export at function start |
| 2 | Qt6Core.dll | 0x002686A2 | [0x00268210,0x00268D18) +0x492 | `?query@QSystemLocale@@UEBA?AVQVariant@@W4QueryType@1@$$QEAV2@@Z` | export at function start |
| 3 | Qt6Core.dll | 0x0016D7B1 | [0x0016D700,0x0016D8FB) +0xB1 | *(unnamed internal function)* | no export at function start |
| 4 | Qt6Core.dll | 0x001671AC | [0x00167180,0x00167216) +0x2C | *(unnamed internal function)* | no export at function start |
| 5 | Qt6Core.dll | 0x00167363 | [0x00167300,0x0016745F) +0x63 | *(unnamed internal function)* | no export at function start |
| 6 | Qt6Core.dll | 0x001707A8 | [0x00170770,0x001707B2) +0x38 | `?toString@QLocale@@QEBA?AVQString@@_J@Z` | export at function start |
| 7 | Qt6Core.dll | 0x002ACC75 | [0x002ACC20,0x002AD5EF) +0x55 | *(unnamed internal function)* | no export at function start |
| 8 | Qt6Core.dll | 0x002B00BF | [0x002AFFE0,0x002B01AE) +0xDF | *(unnamed internal function)* | no export at function start |
| 9 | Qt6Core.dll | 0x002AE927 | [0x002ADBB0,0x002AEACB) +0xD77 | *(unnamed internal function)* | no export at function start |
| 10 | Qt6Core.dll | 0x002A8282 | [0x002A7F40,0x002A82EA) +0x342 | *(unnamed internal function)* | no export at function start |
| 11 | Qt6Core.dll | 0x002A45E4 | [0x002A4550,0x002A4F9A) +0x94 | *(unnamed internal function)* | no export at function start |
| 12 | Qt6Core.dll | 0x001B4A78 | [0x001B4990,0x001B4B85) +0xE8 | *(unnamed internal function)* | no export at function start |
| 13 | Qt6Core.dll | 0x001B564E | [0x001B5623,0x001B5704) +0x2B | *(unnamed internal function)* | no export at function start |
| 14 | Qt6Core.dll | 0x001B0B8D | [0x001B0A10,0x001B0BC8) +0x17D | *(unnamed internal function)* | no export at function start |
| 15 | Qt6Core.dll | 0x001B6EA4 | [0x001B6E20,0x001B7186) +0x84 | `?startOfDay@QDate@@QEBA?AVQDateTime@@AEBVQTimeZone@@@Z` | export at function start |
| 16 | Qt6Core.dll | 0x002B3239 | [0x002B31F0,0x002B3378) +0x49 | `?fromString@QDateTimeParser@@QEBA_NAEBVQString@@PEAVQDateTime@@H@Z` | export at function start |
| 17 | Qt6Core.dll | 0x001B348B | [0x001B33A0,0x001B350F) +0xEB | `?fromString@QDateTime@@SA?AV1@AEBVQString@@VQStringView@@HVQCalendar@@@Z` | export at function start |
| 18 | Qt6Core.dll | 0x001B3381 | [0x001B3330,0x001B339A) +0x51 | `?fromString@QDateTime@@SA?AV1@AEBVQString@@VQStringView@@H@Z` | export at function start |
| 19 | Qt6Core.dll | 0x00004DDF | [0x00004DB0,0x00004DE9) +0x2F | `?fromString@QDateTime@@SA?AV1@AEBVQString@@0H@Z` | export at function start |
| 20 | (exe / CRT) | — | — | in ExifParser::readTime(class QByteArray const &) | ASan symbolization |
| 21 | (exe / CRT) | — | — | in GeoTagController::_parseExifForImage(class QFileInfo const &) | ASan symbolization |
| 22 | (exe / CRT) | — | — | in GeoTagController::_startParseExif::__l2::<lambda_1>::operator() | ASan symbolization |
| 23 | (exe / CRT) | — | — | in std::invoke | ASan symbolization |
| 24 | (exe / CRT) | — | — | in QtConcurrent::MappedEachKernel<QList<QFileInfo>::const_iterator,`GeoTagController::_startParseExif'::`2'::<lambda_1> >::runIteration | ASan symbolization |
| 25 | (exe / CRT) | — | — | in QtConcurrent::MappedEachKernel<QList<QFileInfo>::const_iterator,`GeoTagController::_startParseExif'::`2'::<lambda_1> >::runIterations | ASan symbolization |
| 26 | (exe / CRT) | — | — | in QtConcurrent::IterateKernel<class QList<class QFileInfo>::const_iterator, struct GeoTagController::ExifResult>::forThreadFunction(void) | ASan symbolization |
| 27 | (exe / CRT) | — | — | in QtConcurrent::IterateKernel<class QList<class QFileInfo>::const_iterator, struct GeoTagController::ExifResult>::threadFunction(void) | ASan symbolization |
| 28 | Qt6Concurrent.dll | 0x0000202F | — | — | module+offset |
| 29 | Qt6Core.dll | 0x001A8B6B | [0x001A8AC0,0x001A916F) +0xAB | *(unnamed internal function)* | no export at function start |

### second free — ==7660==ERROR: AddressSanitizer: attempting double-free on 0x12ad9c41ceb0 in thread T4:

| # | module | RVA | function range | function | symbol method |
|---:|---|---|---|---|---|
| 0 | (exe / CRT) | — | — | in free D:\a\_work\1\s\src\vctools\asan\llvm\compiler-rt\lib\asan\asan_malloc_win.cpp:125 | ASan symbolization |
| 1 | Qt6Core.dll | 0x00111F12 | [0x00111ED0,0x00111F42) +0x42 | `??1QVariant@@QEAA@XZ` | export at function start |
| 2 | Qt6Core.dll | 0x0016D874 | [0x0016D700,0x0016D8FB) +0x174 | *(unnamed internal function)* | no export at function start |
| 3 | Qt6Core.dll | 0x001671AC | [0x00167180,0x00167216) +0x2C | *(unnamed internal function)* | no export at function start |
| 4 | Qt6Core.dll | 0x00167363 | [0x00167300,0x0016745F) +0x63 | *(unnamed internal function)* | no export at function start |
| 5 | Qt6Core.dll | 0x001707A8 | [0x00170770,0x001707B2) +0x38 | `?toString@QLocale@@QEBA?AVQString@@_J@Z` | export at function start |
| 6 | Qt6Core.dll | 0x002ACC75 | [0x002ACC20,0x002AD5EF) +0x55 | *(unnamed internal function)* | no export at function start |
| 7 | Qt6Core.dll | 0x002B00BF | [0x002AFFE0,0x002B01AE) +0xDF | *(unnamed internal function)* | no export at function start |
| 8 | Qt6Core.dll | 0x002AE927 | [0x002ADBB0,0x002AEACB) +0xD77 | *(unnamed internal function)* | no export at function start |
| 9 | Qt6Core.dll | 0x002A8282 | [0x002A7F40,0x002A82EA) +0x342 | *(unnamed internal function)* | no export at function start |
| 10 | Qt6Core.dll | 0x002A45E4 | [0x002A4550,0x002A4F9A) +0x94 | *(unnamed internal function)* | no export at function start |
| 11 | Qt6Core.dll | 0x001B4A78 | [0x001B4990,0x001B4B85) +0xE8 | *(unnamed internal function)* | no export at function start |
| 12 | Qt6Core.dll | 0x001B564E | [0x001B5623,0x001B5704) +0x2B | *(unnamed internal function)* | no export at function start |
| 13 | Qt6Core.dll | 0x001B0B8D | [0x001B0A10,0x001B0BC8) +0x17D | *(unnamed internal function)* | no export at function start |
| 14 | Qt6Core.dll | 0x001B6EA4 | [0x001B6E20,0x001B7186) +0x84 | `?startOfDay@QDate@@QEBA?AVQDateTime@@AEBVQTimeZone@@@Z` | export at function start |
| 15 | Qt6Core.dll | 0x002B3239 | [0x002B31F0,0x002B3378) +0x49 | `?fromString@QDateTimeParser@@QEBA_NAEBVQString@@PEAVQDateTime@@H@Z` | export at function start |
| 16 | Qt6Core.dll | 0x001B348B | [0x001B33A0,0x001B350F) +0xEB | `?fromString@QDateTime@@SA?AV1@AEBVQString@@VQStringView@@HVQCalendar@@@Z` | export at function start |
| 17 | Qt6Core.dll | 0x001B3381 | [0x001B3330,0x001B339A) +0x51 | `?fromString@QDateTime@@SA?AV1@AEBVQString@@VQStringView@@H@Z` | export at function start |
| 18 | Qt6Core.dll | 0x00004DDF | [0x00004DB0,0x00004DE9) +0x2F | `?fromString@QDateTime@@SA?AV1@AEBVQString@@0H@Z` | export at function start |
| 19 | (exe / CRT) | — | — | in ExifParser::readTime(class QByteArray const &) | ASan symbolization |
| 20 | (exe / CRT) | — | — | in GeoTagController::_parseExifForImage(class QFileInfo const &) | ASan symbolization |
| 21 | (exe / CRT) | — | — | in GeoTagController::_startParseExif::__l2::<lambda_1>::operator() | ASan symbolization |
| 22 | (exe / CRT) | — | — | in std::invoke | ASan symbolization |
| 23 | (exe / CRT) | — | — | in QtConcurrent::MappedEachKernel<QList<QFileInfo>::const_iterator,`GeoTagController::_startParseExif'::`2'::<lambda_1> >::runIteration | ASan symbolization |
| 24 | (exe / CRT) | — | — | in QtConcurrent::MappedEachKernel<QList<QFileInfo>::const_iterator,`GeoTagController::_startParseExif'::`2'::<lambda_1> >::runIterations | ASan symbolization |
| 25 | (exe / CRT) | — | — | in QtConcurrent::IterateKernel<class QList<class QFileInfo>::const_iterator, struct GeoTagController::ExifResult>::forThreadFunction(void) | ASan symbolization |
| 26 | (exe / CRT) | — | — | in QtConcurrent::IterateKernel<class QList<class QFileInfo>::const_iterator, struct GeoTagController::ExifResult>::threadFunction(void) | ASan symbolization |
| 27 | Qt6Concurrent.dll | 0x0000202F | — | — | module+offset |
| 28 | Qt6Core.dll | 0x001A8B6B | [0x001A8AC0,0x001A916F) +0xAB | *(unnamed internal function)* | no export at function start |
| 29 | Qt6Core.dll | 0x0026D851 | [0x0026D580,0x0026D9E3) +0x2D1 | *(unnamed internal function)* | no export at function start |
| 30 | (exe / CRT) | — | — | in asan_thread_start D:\a\_work\1\s\src\vctools\asan\llvm\compiler-rt\lib\asan\asan_win.cpp:170 | ASan symbolization |
| 31 | (exe / CRT) | — | — | (C:\WINDOWS\System32\KERNEL32.DLL+0x18002dda6) | ASan symbolization |
| 32 | ntdll.dll | 0x000C9C5B | — | — | module+offset |

## `r13-i10.err`

- ==3956==ERROR: AddressSanitizer: attempting double-free on 0x11d4518e56b0 in thread T12:
- 0x11d4518e56b0 is located 0 bytes inside of 20-byte region [0x11d4518e56b0,0x11d4518e56c4)

### allocation — previously allocated by thread T12 here:

| # | module | RVA | function range | function | symbol method |
|---:|---|---|---|---|---|
| 0 | (exe / CRT) | — | — | in malloc D:\a\_work\1\s\src\vctools\asan\llvm\compiler-rt\lib\asan\asan_malloc_win.cpp:134 | ASan symbolization |
| 1 | Qt6Core.dll | 0x001C236D | [0x001C2270,0x001C239C) +0xFD | `?allocate2@QArrayData@@SAPEAXPEAPEAU1@_JW4AllocationOption@1@@Z` | export at function start |
| 2 | Qt6Core.dll | 0x0017F3CD | [0x0017F330,0x0017F439) +0x9D | `??0QString@@QEAA@PEBVQChar@@_J@Z` | export at function start |
| 3 | Qt6Core.dll | 0x0026C89E | [0x0026C840,0x0026C94B) +0x5E | *(unnamed internal function)* | no export at function start |
| 4 | Qt6Core.dll | 0x002686A2 | [0x00268210,0x00268D18) +0x492 | `?query@QSystemLocale@@UEBA?AVQVariant@@W4QueryType@1@$$QEAV2@@Z` | export at function start |
| 5 | Qt6Core.dll | 0x0016D7B1 | [0x0016D700,0x0016D8FB) +0xB1 | *(unnamed internal function)* | no export at function start |
| 6 | Qt6Core.dll | 0x001671AC | [0x00167180,0x00167216) +0x2C | *(unnamed internal function)* | no export at function start |
| 7 | Qt6Core.dll | 0x00167363 | [0x00167300,0x0016745F) +0x63 | *(unnamed internal function)* | no export at function start |
| 8 | Qt6Core.dll | 0x001707A8 | [0x00170770,0x001707B2) +0x38 | `?toString@QLocale@@QEBA?AVQString@@_J@Z` | export at function start |
| 9 | Qt6Core.dll | 0x002ACC75 | [0x002ACC20,0x002AD5EF) +0x55 | *(unnamed internal function)* | no export at function start |
| 10 | Qt6Core.dll | 0x002B00BF | [0x002AFFE0,0x002B01AE) +0xDF | *(unnamed internal function)* | no export at function start |
| 11 | Qt6Core.dll | 0x002AE927 | [0x002ADBB0,0x002AEACB) +0xD77 | *(unnamed internal function)* | no export at function start |
| 12 | Qt6Core.dll | 0x002A8282 | [0x002A7F40,0x002A82EA) +0x342 | *(unnamed internal function)* | no export at function start |
| 13 | Qt6Core.dll | 0x002A45E4 | [0x002A4550,0x002A4F9A) +0x94 | *(unnamed internal function)* | no export at function start |
| 14 | Qt6Core.dll | 0x001B4A78 | [0x001B4990,0x001B4B85) +0xE8 | *(unnamed internal function)* | no export at function start |
| 15 | Qt6Core.dll | 0x001B564E | [0x001B5623,0x001B5704) +0x2B | *(unnamed internal function)* | no export at function start |
| 16 | Qt6Core.dll | 0x001B0B8D | [0x001B0A10,0x001B0BC8) +0x17D | *(unnamed internal function)* | no export at function start |
| 17 | Qt6Core.dll | 0x001B6EA4 | [0x001B6E20,0x001B7186) +0x84 | `?startOfDay@QDate@@QEBA?AVQDateTime@@AEBVQTimeZone@@@Z` | export at function start |
| 18 | Qt6Core.dll | 0x002B3239 | [0x002B31F0,0x002B3378) +0x49 | `?fromString@QDateTimeParser@@QEBA_NAEBVQString@@PEAVQDateTime@@H@Z` | export at function start |
| 19 | Qt6Core.dll | 0x001B348B | [0x001B33A0,0x001B350F) +0xEB | `?fromString@QDateTime@@SA?AV1@AEBVQString@@VQStringView@@HVQCalendar@@@Z` | export at function start |
| 20 | Qt6Core.dll | 0x001B3381 | [0x001B3330,0x001B339A) +0x51 | `?fromString@QDateTime@@SA?AV1@AEBVQString@@VQStringView@@H@Z` | export at function start |
| 21 | Qt6Core.dll | 0x00004DDF | [0x00004DB0,0x00004DE9) +0x2F | `?fromString@QDateTime@@SA?AV1@AEBVQString@@0H@Z` | export at function start |
| 22 | (exe / CRT) | — | — | in ExifParser::readTime(class QByteArray const &) | ASan symbolization |
| 23 | (exe / CRT) | — | — | in GeoTagController::_parseExifForImage(class QFileInfo const &) | ASan symbolization |
| 24 | (exe / CRT) | — | — | in GeoTagController::_startParseExif::__l2::<lambda_1>::operator() | ASan symbolization |
| 25 | (exe / CRT) | — | — | in std::invoke | ASan symbolization |
| 26 | (exe / CRT) | — | — | in QtConcurrent::MappedEachKernel<QList<QFileInfo>::const_iterator,`GeoTagController::_startParseExif'::`2'::<lambda_1> >::runIteration | ASan symbolization |
| 27 | (exe / CRT) | — | — | in QtConcurrent::MappedEachKernel<QList<QFileInfo>::const_iterator,`GeoTagController::_startParseExif'::`2'::<lambda_1> >::runIterations | ASan symbolization |
| 28 | (exe / CRT) | — | — | in QtConcurrent::IterateKernel<class QList<class QFileInfo>::const_iterator, struct GeoTagController::ExifResult>::forThreadFunction(void) | ASan symbolization |
| 29 | (exe / CRT) | — | — | in QtConcurrent::IterateKernel<class QList<class QFileInfo>::const_iterator, struct GeoTagController::ExifResult>::threadFunction(void) | ASan symbolization |

### first free — freed by thread T4 here:

| # | module | RVA | function range | function | symbol method |
|---:|---|---|---|---|---|
| 0 | (exe / CRT) | — | — | in free D:\a\_work\1\s\src\vctools\asan\llvm\compiler-rt\lib\asan\asan_malloc_win.cpp:125 | ASan symbolization |
| 1 | Qt6Core.dll | 0x0026C8F8 | [0x0026C840,0x0026C94B) +0xB8 | *(unnamed internal function)* | no export at function start |
| 2 | Qt6Core.dll | 0x002686A2 | [0x00268210,0x00268D18) +0x492 | `?query@QSystemLocale@@UEBA?AVQVariant@@W4QueryType@1@$$QEAV2@@Z` | export at function start |
| 3 | Qt6Core.dll | 0x0016D7B1 | [0x0016D700,0x0016D8FB) +0xB1 | *(unnamed internal function)* | no export at function start |
| 4 | Qt6Core.dll | 0x001671AC | [0x00167180,0x00167216) +0x2C | *(unnamed internal function)* | no export at function start |
| 5 | Qt6Core.dll | 0x00167363 | [0x00167300,0x0016745F) +0x63 | *(unnamed internal function)* | no export at function start |
| 6 | Qt6Core.dll | 0x001707A8 | [0x00170770,0x001707B2) +0x38 | `?toString@QLocale@@QEBA?AVQString@@_J@Z` | export at function start |
| 7 | Qt6Core.dll | 0x002ACC75 | [0x002ACC20,0x002AD5EF) +0x55 | *(unnamed internal function)* | no export at function start |
| 8 | Qt6Core.dll | 0x002B00BF | [0x002AFFE0,0x002B01AE) +0xDF | *(unnamed internal function)* | no export at function start |
| 9 | Qt6Core.dll | 0x002AE927 | [0x002ADBB0,0x002AEACB) +0xD77 | *(unnamed internal function)* | no export at function start |
| 10 | Qt6Core.dll | 0x002A8282 | [0x002A7F40,0x002A82EA) +0x342 | *(unnamed internal function)* | no export at function start |
| 11 | Qt6Core.dll | 0x002A45E4 | [0x002A4550,0x002A4F9A) +0x94 | *(unnamed internal function)* | no export at function start |
| 12 | Qt6Core.dll | 0x001B4A78 | [0x001B4990,0x001B4B85) +0xE8 | *(unnamed internal function)* | no export at function start |
| 13 | Qt6Core.dll | 0x001B564E | [0x001B5623,0x001B5704) +0x2B | *(unnamed internal function)* | no export at function start |
| 14 | Qt6Core.dll | 0x001B0B8D | [0x001B0A10,0x001B0BC8) +0x17D | *(unnamed internal function)* | no export at function start |
| 15 | Qt6Core.dll | 0x001B6EA4 | [0x001B6E20,0x001B7186) +0x84 | `?startOfDay@QDate@@QEBA?AVQDateTime@@AEBVQTimeZone@@@Z` | export at function start |
| 16 | Qt6Core.dll | 0x002B3239 | [0x002B31F0,0x002B3378) +0x49 | `?fromString@QDateTimeParser@@QEBA_NAEBVQString@@PEAVQDateTime@@H@Z` | export at function start |
| 17 | Qt6Core.dll | 0x001B348B | [0x001B33A0,0x001B350F) +0xEB | `?fromString@QDateTime@@SA?AV1@AEBVQString@@VQStringView@@HVQCalendar@@@Z` | export at function start |
| 18 | Qt6Core.dll | 0x001B3381 | [0x001B3330,0x001B339A) +0x51 | `?fromString@QDateTime@@SA?AV1@AEBVQString@@VQStringView@@H@Z` | export at function start |
| 19 | Qt6Core.dll | 0x00004DDF | [0x00004DB0,0x00004DE9) +0x2F | `?fromString@QDateTime@@SA?AV1@AEBVQString@@0H@Z` | export at function start |
| 20 | (exe / CRT) | — | — | in ExifParser::readTime(class QByteArray const &) | ASan symbolization |
| 21 | (exe / CRT) | — | — | in GeoTagController::_parseExifForImage(class QFileInfo const &) | ASan symbolization |
| 22 | (exe / CRT) | — | — | in GeoTagController::_startParseExif::__l2::<lambda_1>::operator() | ASan symbolization |
| 23 | (exe / CRT) | — | — | in std::invoke | ASan symbolization |
| 24 | (exe / CRT) | — | — | in QtConcurrent::MappedEachKernel<QList<QFileInfo>::const_iterator,`GeoTagController::_startParseExif'::`2'::<lambda_1> >::runIteration | ASan symbolization |
| 25 | (exe / CRT) | — | — | in QtConcurrent::MappedEachKernel<QList<QFileInfo>::const_iterator,`GeoTagController::_startParseExif'::`2'::<lambda_1> >::runIterations | ASan symbolization |
| 26 | (exe / CRT) | — | — | in QtConcurrent::IterateKernel<class QList<class QFileInfo>::const_iterator, struct GeoTagController::ExifResult>::forThreadFunction(void) | ASan symbolization |
| 27 | (exe / CRT) | — | — | in QtConcurrent::IterateKernel<class QList<class QFileInfo>::const_iterator, struct GeoTagController::ExifResult>::threadFunction(void) | ASan symbolization |
| 28 | Qt6Concurrent.dll | 0x0000202F | — | — | module+offset |
| 29 | Qt6Core.dll | 0x001A8B6B | [0x001A8AC0,0x001A916F) +0xAB | *(unnamed internal function)* | no export at function start |

### second free — ==3956==ERROR: AddressSanitizer: attempting double-free on 0x11d4518e56b0 in thread T12:

| # | module | RVA | function range | function | symbol method |
|---:|---|---|---|---|---|
| 0 | (exe / CRT) | — | — | in free D:\a\_work\1\s\src\vctools\asan\llvm\compiler-rt\lib\asan\asan_malloc_win.cpp:125 | ASan symbolization |
| 1 | Qt6Core.dll | 0x00111F12 | [0x00111ED0,0x00111F42) +0x42 | `??1QVariant@@QEAA@XZ` | export at function start |
| 2 | Qt6Core.dll | 0x0016D874 | [0x0016D700,0x0016D8FB) +0x174 | *(unnamed internal function)* | no export at function start |
| 3 | Qt6Core.dll | 0x001671AC | [0x00167180,0x00167216) +0x2C | *(unnamed internal function)* | no export at function start |
| 4 | Qt6Core.dll | 0x00167363 | [0x00167300,0x0016745F) +0x63 | *(unnamed internal function)* | no export at function start |
| 5 | Qt6Core.dll | 0x001707A8 | [0x00170770,0x001707B2) +0x38 | `?toString@QLocale@@QEBA?AVQString@@_J@Z` | export at function start |
| 6 | Qt6Core.dll | 0x002ACC75 | [0x002ACC20,0x002AD5EF) +0x55 | *(unnamed internal function)* | no export at function start |
| 7 | Qt6Core.dll | 0x002B00BF | [0x002AFFE0,0x002B01AE) +0xDF | *(unnamed internal function)* | no export at function start |
| 8 | Qt6Core.dll | 0x002AE927 | [0x002ADBB0,0x002AEACB) +0xD77 | *(unnamed internal function)* | no export at function start |
| 9 | Qt6Core.dll | 0x002A8282 | [0x002A7F40,0x002A82EA) +0x342 | *(unnamed internal function)* | no export at function start |
| 10 | Qt6Core.dll | 0x002A45E4 | [0x002A4550,0x002A4F9A) +0x94 | *(unnamed internal function)* | no export at function start |
| 11 | Qt6Core.dll | 0x001B4A78 | [0x001B4990,0x001B4B85) +0xE8 | *(unnamed internal function)* | no export at function start |
| 12 | Qt6Core.dll | 0x001B564E | [0x001B5623,0x001B5704) +0x2B | *(unnamed internal function)* | no export at function start |
| 13 | Qt6Core.dll | 0x001B0B8D | [0x001B0A10,0x001B0BC8) +0x17D | *(unnamed internal function)* | no export at function start |
| 14 | Qt6Core.dll | 0x001B6EA4 | [0x001B6E20,0x001B7186) +0x84 | `?startOfDay@QDate@@QEBA?AVQDateTime@@AEBVQTimeZone@@@Z` | export at function start |
| 15 | Qt6Core.dll | 0x002B3239 | [0x002B31F0,0x002B3378) +0x49 | `?fromString@QDateTimeParser@@QEBA_NAEBVQString@@PEAVQDateTime@@H@Z` | export at function start |
| 16 | Qt6Core.dll | 0x001B348B | [0x001B33A0,0x001B350F) +0xEB | `?fromString@QDateTime@@SA?AV1@AEBVQString@@VQStringView@@HVQCalendar@@@Z` | export at function start |
| 17 | Qt6Core.dll | 0x001B3381 | [0x001B3330,0x001B339A) +0x51 | `?fromString@QDateTime@@SA?AV1@AEBVQString@@VQStringView@@H@Z` | export at function start |
| 18 | Qt6Core.dll | 0x00004DDF | [0x00004DB0,0x00004DE9) +0x2F | `?fromString@QDateTime@@SA?AV1@AEBVQString@@0H@Z` | export at function start |
| 19 | (exe / CRT) | — | — | in ExifParser::readTime(class QByteArray const &) | ASan symbolization |
| 20 | (exe / CRT) | — | — | in GeoTagController::_parseExifForImage(class QFileInfo const &) | ASan symbolization |
| 21 | (exe / CRT) | — | — | in GeoTagController::_startParseExif::__l2::<lambda_1>::operator() | ASan symbolization |
| 22 | (exe / CRT) | — | — | in std::invoke | ASan symbolization |
| 23 | (exe / CRT) | — | — | in QtConcurrent::MappedEachKernel<QList<QFileInfo>::const_iterator,`GeoTagController::_startParseExif'::`2'::<lambda_1> >::runIteration | ASan symbolization |
| 24 | (exe / CRT) | — | — | in QtConcurrent::MappedEachKernel<QList<QFileInfo>::const_iterator,`GeoTagController::_startParseExif'::`2'::<lambda_1> >::runIterations | ASan symbolization |
| 25 | (exe / CRT) | — | — | in QtConcurrent::IterateKernel<class QList<class QFileInfo>::const_iterator, struct GeoTagController::ExifResult>::forThreadFunction(void) | ASan symbolization |
| 26 | (exe / CRT) | — | — | in QtConcurrent::IterateKernel<class QList<class QFileInfo>::const_iterator, struct GeoTagController::ExifResult>::threadFunction(void) | ASan symbolization |
| 27 | Qt6Concurrent.dll | 0x0000202F | — | — | module+offset |
| 28 | Qt6Core.dll | 0x001A8B6B | [0x001A8AC0,0x001A916F) +0xAB | *(unnamed internal function)* | no export at function start |
| 29 | Qt6Core.dll | 0x0026D851 | [0x0026D580,0x0026D9E3) +0x2D1 | *(unnamed internal function)* | no export at function start |
| 30 | (exe / CRT) | — | — | in asan_thread_start D:\a\_work\1\s\src\vctools\asan\llvm\compiler-rt\lib\asan\asan_win.cpp:170 | ASan symbolization |
| 31 | (exe / CRT) | — | — | (C:\WINDOWS\System32\KERNEL32.DLL+0x18002dda6) | ASan symbolization |
| 32 | ntdll.dll | 0x000C9C5B | — | — | module+offset |

## `a-r5-i1.err`

- ==26552==ERROR: AddressSanitizer: attempting double-free on 0x12106faa6220 in thread T12:
- 0x12106faa6220 is located 0 bytes inside of 20-byte region [0x12106faa6220,0x12106faa6234)

### allocation — previously allocated by thread T12 here:

| # | module | RVA | function range | function | symbol method |
|---:|---|---|---|---|---|
| 0 | (exe / CRT) | — | — | in malloc D:\a\_work\1\s\src\vctools\asan\llvm\compiler-rt\lib\asan\asan_malloc_win.cpp:134 | ASan symbolization |
| 1 | Qt6Core.dll | 0x001C236D | [0x001C2270,0x001C239C) +0xFD | `?allocate2@QArrayData@@SAPEAXPEAPEAU1@_JW4AllocationOption@1@@Z` | export at function start |
| 2 | Qt6Core.dll | 0x0017F3CD | [0x0017F330,0x0017F439) +0x9D | `??0QString@@QEAA@PEBVQChar@@_J@Z` | export at function start |
| 3 | Qt6Core.dll | 0x0026C89E | [0x0026C840,0x0026C94B) +0x5E | *(unnamed internal function)* | no export at function start |
| 4 | Qt6Core.dll | 0x002686A2 | [0x00268210,0x00268D18) +0x492 | `?query@QSystemLocale@@UEBA?AVQVariant@@W4QueryType@1@$$QEAV2@@Z` | export at function start |
| 5 | Qt6Core.dll | 0x0016D7B1 | [0x0016D700,0x0016D8FB) +0xB1 | *(unnamed internal function)* | no export at function start |
| 6 | Qt6Core.dll | 0x001671AC | [0x00167180,0x00167216) +0x2C | *(unnamed internal function)* | no export at function start |
| 7 | Qt6Core.dll | 0x00167363 | [0x00167300,0x0016745F) +0x63 | *(unnamed internal function)* | no export at function start |
| 8 | Qt6Core.dll | 0x001707A8 | [0x00170770,0x001707B2) +0x38 | `?toString@QLocale@@QEBA?AVQString@@_J@Z` | export at function start |
| 9 | Qt6Core.dll | 0x002ACC75 | [0x002ACC20,0x002AD5EF) +0x55 | *(unnamed internal function)* | no export at function start |
| 10 | Qt6Core.dll | 0x002B00BF | [0x002AFFE0,0x002B01AE) +0xDF | *(unnamed internal function)* | no export at function start |
| 11 | Qt6Core.dll | 0x002AE927 | [0x002ADBB0,0x002AEACB) +0xD77 | *(unnamed internal function)* | no export at function start |
| 12 | Qt6Core.dll | 0x002A8282 | [0x002A7F40,0x002A82EA) +0x342 | *(unnamed internal function)* | no export at function start |
| 13 | Qt6Core.dll | 0x002A45E4 | [0x002A4550,0x002A4F9A) +0x94 | *(unnamed internal function)* | no export at function start |
| 14 | Qt6Core.dll | 0x001B4A78 | [0x001B4990,0x001B4B85) +0xE8 | *(unnamed internal function)* | no export at function start |
| 15 | Qt6Core.dll | 0x001B564E | [0x001B5623,0x001B5704) +0x2B | *(unnamed internal function)* | no export at function start |
| 16 | Qt6Core.dll | 0x001B0B8D | [0x001B0A10,0x001B0BC8) +0x17D | *(unnamed internal function)* | no export at function start |
| 17 | Qt6Core.dll | 0x001B6EA4 | [0x001B6E20,0x001B7186) +0x84 | `?startOfDay@QDate@@QEBA?AVQDateTime@@AEBVQTimeZone@@@Z` | export at function start |
| 18 | Qt6Core.dll | 0x002B3239 | [0x002B31F0,0x002B3378) +0x49 | `?fromString@QDateTimeParser@@QEBA_NAEBVQString@@PEAVQDateTime@@H@Z` | export at function start |
| 19 | Qt6Core.dll | 0x001B348B | [0x001B33A0,0x001B350F) +0xEB | `?fromString@QDateTime@@SA?AV1@AEBVQString@@VQStringView@@HVQCalendar@@@Z` | export at function start |
| 20 | Qt6Core.dll | 0x001B3381 | [0x001B3330,0x001B339A) +0x51 | `?fromString@QDateTime@@SA?AV1@AEBVQString@@VQStringView@@H@Z` | export at function start |
| 21 | Qt6Core.dll | 0x00004DDF | [0x00004DB0,0x00004DE9) +0x2F | `?fromString@QDateTime@@SA?AV1@AEBVQString@@0H@Z` | export at function start |
| 22 | (exe / CRT) | — | — | in ``anonymous namespace'::runModeA'::`2'::<lambda_1>::operator() | ASan symbolization |
| 23 | (exe / CRT) | — | — | in std::invoke | ASan symbolization |
| 24 | (exe / CRT) | — | — | in std::_Func_impl_no_alloc<``anonymous namespace'::runModeA'::`2'::<lambda_1>,void,int>::_Do_call | ASan symbolization |
| 25 | (exe / CRT) | — | — | in std::_Func_class<void,int>::operator() | ASan symbolization |
| 26 | (exe / CRT) | — | — | in ``anonymous namespace'::runThreads'::`4'::<lambda_1>::operator() | ASan symbolization |
| 27 | (exe / CRT) | — | — | in std::invoke | ASan symbolization |
| 28 | (exe / CRT) | — | — | in std::thread::_Invoke<std::tuple<``anonymous namespace'::runThreads'::`4'::<lambda_1> >,0> | ASan symbolization |
| 29 | ucrtbase.dll | 0x0002CD2F | — | — | module+offset |
| 30 | (exe / CRT) | — | — | in asan_thread_start D:\a\_work\1\s\src\vctools\asan\llvm\compiler-rt\lib\asan\asan_win.cpp:170 | ASan symbolization |

### first free — freed by thread T2 here:

| # | module | RVA | function range | function | symbol method |
|---:|---|---|---|---|---|
| 0 | (exe / CRT) | — | — | in free D:\a\_work\1\s\src\vctools\asan\llvm\compiler-rt\lib\asan\asan_malloc_win.cpp:125 | ASan symbolization |
| 1 | Qt6Core.dll | 0x0026C8F8 | [0x0026C840,0x0026C94B) +0xB8 | *(unnamed internal function)* | no export at function start |
| 2 | Qt6Core.dll | 0x002686A2 | [0x00268210,0x00268D18) +0x492 | `?query@QSystemLocale@@UEBA?AVQVariant@@W4QueryType@1@$$QEAV2@@Z` | export at function start |
| 3 | Qt6Core.dll | 0x0016D7B1 | [0x0016D700,0x0016D8FB) +0xB1 | *(unnamed internal function)* | no export at function start |
| 4 | Qt6Core.dll | 0x001671AC | [0x00167180,0x00167216) +0x2C | *(unnamed internal function)* | no export at function start |
| 5 | Qt6Core.dll | 0x00167363 | [0x00167300,0x0016745F) +0x63 | *(unnamed internal function)* | no export at function start |
| 6 | Qt6Core.dll | 0x001707A8 | [0x00170770,0x001707B2) +0x38 | `?toString@QLocale@@QEBA?AVQString@@_J@Z` | export at function start |
| 7 | Qt6Core.dll | 0x002ACC75 | [0x002ACC20,0x002AD5EF) +0x55 | *(unnamed internal function)* | no export at function start |
| 8 | Qt6Core.dll | 0x002B00BF | [0x002AFFE0,0x002B01AE) +0xDF | *(unnamed internal function)* | no export at function start |
| 9 | Qt6Core.dll | 0x002AE927 | [0x002ADBB0,0x002AEACB) +0xD77 | *(unnamed internal function)* | no export at function start |
| 10 | Qt6Core.dll | 0x002A8282 | [0x002A7F40,0x002A82EA) +0x342 | *(unnamed internal function)* | no export at function start |
| 11 | Qt6Core.dll | 0x002A45E4 | [0x002A4550,0x002A4F9A) +0x94 | *(unnamed internal function)* | no export at function start |
| 12 | Qt6Core.dll | 0x001B4A78 | [0x001B4990,0x001B4B85) +0xE8 | *(unnamed internal function)* | no export at function start |
| 13 | Qt6Core.dll | 0x001B564E | [0x001B5623,0x001B5704) +0x2B | *(unnamed internal function)* | no export at function start |
| 14 | Qt6Core.dll | 0x001B0B8D | [0x001B0A10,0x001B0BC8) +0x17D | *(unnamed internal function)* | no export at function start |
| 15 | Qt6Core.dll | 0x001B6EA4 | [0x001B6E20,0x001B7186) +0x84 | `?startOfDay@QDate@@QEBA?AVQDateTime@@AEBVQTimeZone@@@Z` | export at function start |
| 16 | Qt6Core.dll | 0x002B3239 | [0x002B31F0,0x002B3378) +0x49 | `?fromString@QDateTimeParser@@QEBA_NAEBVQString@@PEAVQDateTime@@H@Z` | export at function start |
| 17 | Qt6Core.dll | 0x001B348B | [0x001B33A0,0x001B350F) +0xEB | `?fromString@QDateTime@@SA?AV1@AEBVQString@@VQStringView@@HVQCalendar@@@Z` | export at function start |
| 18 | Qt6Core.dll | 0x001B3381 | [0x001B3330,0x001B339A) +0x51 | `?fromString@QDateTime@@SA?AV1@AEBVQString@@VQStringView@@H@Z` | export at function start |
| 19 | Qt6Core.dll | 0x00004DDF | [0x00004DB0,0x00004DE9) +0x2F | `?fromString@QDateTime@@SA?AV1@AEBVQString@@0H@Z` | export at function start |
| 20 | (exe / CRT) | — | — | in ``anonymous namespace'::runModeA'::`2'::<lambda_1>::operator() | ASan symbolization |
| 21 | (exe / CRT) | — | — | in std::invoke | ASan symbolization |
| 22 | (exe / CRT) | — | — | in std::_Func_impl_no_alloc<``anonymous namespace'::runModeA'::`2'::<lambda_1>,void,int>::_Do_call | ASan symbolization |
| 23 | (exe / CRT) | — | — | in std::_Func_class<void,int>::operator() | ASan symbolization |
| 24 | (exe / CRT) | — | — | in ``anonymous namespace'::runThreads'::`4'::<lambda_1>::operator() | ASan symbolization |
| 25 | (exe / CRT) | — | — | in std::invoke | ASan symbolization |
| 26 | (exe / CRT) | — | — | in std::thread::_Invoke<std::tuple<``anonymous namespace'::runThreads'::`4'::<lambda_1> >,0> | ASan symbolization |
| 27 | ucrtbase.dll | 0x0002CD2F | — | — | module+offset |
| 28 | (exe / CRT) | — | — | in asan_thread_start D:\a\_work\1\s\src\vctools\asan\llvm\compiler-rt\lib\asan\asan_win.cpp:170 | ASan symbolization |
| 29 | (exe / CRT) | — | — | (C:\WINDOWS\System32\KERNEL32.DLL+0x18002dda6) | ASan symbolization |
| 30 | ntdll.dll | 0x000C9C5B | — | — | module+offset |

### second free — ==26552==ERROR: AddressSanitizer: attempting double-free on 0x12106faa6220 in thread T12:

| # | module | RVA | function range | function | symbol method |
|---:|---|---|---|---|---|
| 0 | (exe / CRT) | — | — | in free D:\a\_work\1\s\src\vctools\asan\llvm\compiler-rt\lib\asan\asan_malloc_win.cpp:125 | ASan symbolization |
| 1 | Qt6Core.dll | 0x00111F12 | [0x00111ED0,0x00111F42) +0x42 | `??1QVariant@@QEAA@XZ` | export at function start |
| 2 | Qt6Core.dll | 0x0016D874 | [0x0016D700,0x0016D8FB) +0x174 | *(unnamed internal function)* | no export at function start |
| 3 | Qt6Core.dll | 0x001671AC | [0x00167180,0x00167216) +0x2C | *(unnamed internal function)* | no export at function start |
| 4 | Qt6Core.dll | 0x00167363 | [0x00167300,0x0016745F) +0x63 | *(unnamed internal function)* | no export at function start |
| 5 | Qt6Core.dll | 0x001707A8 | [0x00170770,0x001707B2) +0x38 | `?toString@QLocale@@QEBA?AVQString@@_J@Z` | export at function start |
| 6 | Qt6Core.dll | 0x002ACC75 | [0x002ACC20,0x002AD5EF) +0x55 | *(unnamed internal function)* | no export at function start |
| 7 | Qt6Core.dll | 0x002B00BF | [0x002AFFE0,0x002B01AE) +0xDF | *(unnamed internal function)* | no export at function start |
| 8 | Qt6Core.dll | 0x002AE927 | [0x002ADBB0,0x002AEACB) +0xD77 | *(unnamed internal function)* | no export at function start |
| 9 | Qt6Core.dll | 0x002A8282 | [0x002A7F40,0x002A82EA) +0x342 | *(unnamed internal function)* | no export at function start |
| 10 | Qt6Core.dll | 0x002A45E4 | [0x002A4550,0x002A4F9A) +0x94 | *(unnamed internal function)* | no export at function start |
| 11 | Qt6Core.dll | 0x001B4A78 | [0x001B4990,0x001B4B85) +0xE8 | *(unnamed internal function)* | no export at function start |
| 12 | Qt6Core.dll | 0x001B564E | [0x001B5623,0x001B5704) +0x2B | *(unnamed internal function)* | no export at function start |
| 13 | Qt6Core.dll | 0x001B0B8D | [0x001B0A10,0x001B0BC8) +0x17D | *(unnamed internal function)* | no export at function start |
| 14 | Qt6Core.dll | 0x001B6EA4 | [0x001B6E20,0x001B7186) +0x84 | `?startOfDay@QDate@@QEBA?AVQDateTime@@AEBVQTimeZone@@@Z` | export at function start |
| 15 | Qt6Core.dll | 0x002B3239 | [0x002B31F0,0x002B3378) +0x49 | `?fromString@QDateTimeParser@@QEBA_NAEBVQString@@PEAVQDateTime@@H@Z` | export at function start |
| 16 | Qt6Core.dll | 0x001B348B | [0x001B33A0,0x001B350F) +0xEB | `?fromString@QDateTime@@SA?AV1@AEBVQString@@VQStringView@@HVQCalendar@@@Z` | export at function start |
| 17 | Qt6Core.dll | 0x001B3381 | [0x001B3330,0x001B339A) +0x51 | `?fromString@QDateTime@@SA?AV1@AEBVQString@@VQStringView@@H@Z` | export at function start |
| 18 | Qt6Core.dll | 0x00004DDF | [0x00004DB0,0x00004DE9) +0x2F | `?fromString@QDateTime@@SA?AV1@AEBVQString@@0H@Z` | export at function start |
| 19 | (exe / CRT) | — | — | in ``anonymous namespace'::runModeA'::`2'::<lambda_1>::operator() | ASan symbolization |
| 20 | (exe / CRT) | — | — | in std::invoke | ASan symbolization |
| 21 | (exe / CRT) | — | — | in std::_Func_impl_no_alloc<``anonymous namespace'::runModeA'::`2'::<lambda_1>,void,int>::_Do_call | ASan symbolization |
| 22 | (exe / CRT) | — | — | in std::_Func_class<void,int>::operator() | ASan symbolization |
| 23 | (exe / CRT) | — | — | in ``anonymous namespace'::runThreads'::`4'::<lambda_1>::operator() | ASan symbolization |
| 24 | (exe / CRT) | — | — | in std::invoke | ASan symbolization |
| 25 | (exe / CRT) | — | — | in std::thread::_Invoke<std::tuple<``anonymous namespace'::runThreads'::`4'::<lambda_1> >,0> | ASan symbolization |
| 26 | ucrtbase.dll | 0x0002CD2F | — | — | module+offset |
| 27 | (exe / CRT) | — | — | in asan_thread_start D:\a\_work\1\s\src\vctools\asan\llvm\compiler-rt\lib\asan\asan_win.cpp:170 | ASan symbolization |
| 28 | (exe / CRT) | — | — | (C:\WINDOWS\System32\KERNEL32.DLL+0x18002dda6) | ASan symbolization |
| 29 | ntdll.dll | 0x000C9C5B | — | — | module+offset |

## `a-r12-i10.err`

- ==25752==ERROR: AddressSanitizer: attempting double-free on 0x11d2eeea9220 in thread T11:
- 0x11d2eeea9220 is located 0 bytes inside of 20-byte region [0x11d2eeea9220,0x11d2eeea9234)

### allocation — previously allocated by thread T11 here:

| # | module | RVA | function range | function | symbol method |
|---:|---|---|---|---|---|
| 0 | (exe / CRT) | — | — | in malloc D:\a\_work\1\s\src\vctools\asan\llvm\compiler-rt\lib\asan\asan_malloc_win.cpp:134 | ASan symbolization |
| 1 | Qt6Core.dll | 0x001C236D | [0x001C2270,0x001C239C) +0xFD | `?allocate2@QArrayData@@SAPEAXPEAPEAU1@_JW4AllocationOption@1@@Z` | export at function start |
| 2 | Qt6Core.dll | 0x0017F3CD | [0x0017F330,0x0017F439) +0x9D | `??0QString@@QEAA@PEBVQChar@@_J@Z` | export at function start |
| 3 | Qt6Core.dll | 0x0026C89E | [0x0026C840,0x0026C94B) +0x5E | *(unnamed internal function)* | no export at function start |
| 4 | Qt6Core.dll | 0x002686A2 | [0x00268210,0x00268D18) +0x492 | `?query@QSystemLocale@@UEBA?AVQVariant@@W4QueryType@1@$$QEAV2@@Z` | export at function start |
| 5 | Qt6Core.dll | 0x0016D7B1 | [0x0016D700,0x0016D8FB) +0xB1 | *(unnamed internal function)* | no export at function start |
| 6 | Qt6Core.dll | 0x001671AC | [0x00167180,0x00167216) +0x2C | *(unnamed internal function)* | no export at function start |
| 7 | Qt6Core.dll | 0x00167363 | [0x00167300,0x0016745F) +0x63 | *(unnamed internal function)* | no export at function start |
| 8 | Qt6Core.dll | 0x001707A8 | [0x00170770,0x001707B2) +0x38 | `?toString@QLocale@@QEBA?AVQString@@_J@Z` | export at function start |
| 9 | Qt6Core.dll | 0x002ACC75 | [0x002ACC20,0x002AD5EF) +0x55 | *(unnamed internal function)* | no export at function start |
| 10 | Qt6Core.dll | 0x002B00BF | [0x002AFFE0,0x002B01AE) +0xDF | *(unnamed internal function)* | no export at function start |
| 11 | Qt6Core.dll | 0x002AE927 | [0x002ADBB0,0x002AEACB) +0xD77 | *(unnamed internal function)* | no export at function start |
| 12 | Qt6Core.dll | 0x002A8282 | [0x002A7F40,0x002A82EA) +0x342 | *(unnamed internal function)* | no export at function start |
| 13 | Qt6Core.dll | 0x002A45E4 | [0x002A4550,0x002A4F9A) +0x94 | *(unnamed internal function)* | no export at function start |
| 14 | Qt6Core.dll | 0x001B4A78 | [0x001B4990,0x001B4B85) +0xE8 | *(unnamed internal function)* | no export at function start |
| 15 | Qt6Core.dll | 0x001B564E | [0x001B5623,0x001B5704) +0x2B | *(unnamed internal function)* | no export at function start |
| 16 | Qt6Core.dll | 0x001B0B8D | [0x001B0A10,0x001B0BC8) +0x17D | *(unnamed internal function)* | no export at function start |
| 17 | Qt6Core.dll | 0x001B6EA4 | [0x001B6E20,0x001B7186) +0x84 | `?startOfDay@QDate@@QEBA?AVQDateTime@@AEBVQTimeZone@@@Z` | export at function start |
| 18 | Qt6Core.dll | 0x002B3239 | [0x002B31F0,0x002B3378) +0x49 | `?fromString@QDateTimeParser@@QEBA_NAEBVQString@@PEAVQDateTime@@H@Z` | export at function start |
| 19 | Qt6Core.dll | 0x001B348B | [0x001B33A0,0x001B350F) +0xEB | `?fromString@QDateTime@@SA?AV1@AEBVQString@@VQStringView@@HVQCalendar@@@Z` | export at function start |
| 20 | Qt6Core.dll | 0x001B3381 | [0x001B3330,0x001B339A) +0x51 | `?fromString@QDateTime@@SA?AV1@AEBVQString@@VQStringView@@H@Z` | export at function start |
| 21 | Qt6Core.dll | 0x00004DDF | [0x00004DB0,0x00004DE9) +0x2F | `?fromString@QDateTime@@SA?AV1@AEBVQString@@0H@Z` | export at function start |
| 22 | (exe / CRT) | — | — | in ``anonymous namespace'::runModeA'::`2'::<lambda_1>::operator() | ASan symbolization |
| 23 | (exe / CRT) | — | — | in std::invoke | ASan symbolization |
| 24 | (exe / CRT) | — | — | in std::_Func_impl_no_alloc<``anonymous namespace'::runModeA'::`2'::<lambda_1>,void,int>::_Do_call | ASan symbolization |
| 25 | (exe / CRT) | — | — | in std::_Func_class<void,int>::operator() | ASan symbolization |
| 26 | (exe / CRT) | — | — | in ``anonymous namespace'::runThreads'::`4'::<lambda_1>::operator() | ASan symbolization |
| 27 | (exe / CRT) | — | — | in std::invoke | ASan symbolization |
| 28 | (exe / CRT) | — | — | in std::thread::_Invoke<std::tuple<``anonymous namespace'::runThreads'::`4'::<lambda_1> >,0> | ASan symbolization |
| 29 | ucrtbase.dll | 0x0002CD2F | — | — | module+offset |
| 30 | (exe / CRT) | — | — | in asan_thread_start D:\a\_work\1\s\src\vctools\asan\llvm\compiler-rt\lib\asan\asan_win.cpp:170 | ASan symbolization |

### first free — freed by thread T10 here:

| # | module | RVA | function range | function | symbol method |
|---:|---|---|---|---|---|
| 0 | (exe / CRT) | — | — | in free D:\a\_work\1\s\src\vctools\asan\llvm\compiler-rt\lib\asan\asan_malloc_win.cpp:125 | ASan symbolization |
| 1 | Qt6Core.dll | 0x0026C8F8 | [0x0026C840,0x0026C94B) +0xB8 | *(unnamed internal function)* | no export at function start |
| 2 | Qt6Core.dll | 0x002686A2 | [0x00268210,0x00268D18) +0x492 | `?query@QSystemLocale@@UEBA?AVQVariant@@W4QueryType@1@$$QEAV2@@Z` | export at function start |
| 3 | Qt6Core.dll | 0x0016D7B1 | [0x0016D700,0x0016D8FB) +0xB1 | *(unnamed internal function)* | no export at function start |
| 4 | Qt6Core.dll | 0x001671AC | [0x00167180,0x00167216) +0x2C | *(unnamed internal function)* | no export at function start |
| 5 | Qt6Core.dll | 0x00167363 | [0x00167300,0x0016745F) +0x63 | *(unnamed internal function)* | no export at function start |
| 6 | Qt6Core.dll | 0x001707A8 | [0x00170770,0x001707B2) +0x38 | `?toString@QLocale@@QEBA?AVQString@@_J@Z` | export at function start |
| 7 | Qt6Core.dll | 0x002ACC75 | [0x002ACC20,0x002AD5EF) +0x55 | *(unnamed internal function)* | no export at function start |
| 8 | Qt6Core.dll | 0x002B00BF | [0x002AFFE0,0x002B01AE) +0xDF | *(unnamed internal function)* | no export at function start |
| 9 | Qt6Core.dll | 0x002AE927 | [0x002ADBB0,0x002AEACB) +0xD77 | *(unnamed internal function)* | no export at function start |
| 10 | Qt6Core.dll | 0x002A8282 | [0x002A7F40,0x002A82EA) +0x342 | *(unnamed internal function)* | no export at function start |
| 11 | Qt6Core.dll | 0x002A45E4 | [0x002A4550,0x002A4F9A) +0x94 | *(unnamed internal function)* | no export at function start |
| 12 | Qt6Core.dll | 0x001B4A78 | [0x001B4990,0x001B4B85) +0xE8 | *(unnamed internal function)* | no export at function start |
| 13 | Qt6Core.dll | 0x001B564E | [0x001B5623,0x001B5704) +0x2B | *(unnamed internal function)* | no export at function start |
| 14 | Qt6Core.dll | 0x001B0B8D | [0x001B0A10,0x001B0BC8) +0x17D | *(unnamed internal function)* | no export at function start |
| 15 | Qt6Core.dll | 0x001B6EA4 | [0x001B6E20,0x001B7186) +0x84 | `?startOfDay@QDate@@QEBA?AVQDateTime@@AEBVQTimeZone@@@Z` | export at function start |
| 16 | Qt6Core.dll | 0x002B3239 | [0x002B31F0,0x002B3378) +0x49 | `?fromString@QDateTimeParser@@QEBA_NAEBVQString@@PEAVQDateTime@@H@Z` | export at function start |
| 17 | Qt6Core.dll | 0x001B348B | [0x001B33A0,0x001B350F) +0xEB | `?fromString@QDateTime@@SA?AV1@AEBVQString@@VQStringView@@HVQCalendar@@@Z` | export at function start |
| 18 | Qt6Core.dll | 0x001B3381 | [0x001B3330,0x001B339A) +0x51 | `?fromString@QDateTime@@SA?AV1@AEBVQString@@VQStringView@@H@Z` | export at function start |
| 19 | Qt6Core.dll | 0x00004DDF | [0x00004DB0,0x00004DE9) +0x2F | `?fromString@QDateTime@@SA?AV1@AEBVQString@@0H@Z` | export at function start |
| 20 | (exe / CRT) | — | — | in ``anonymous namespace'::runModeA'::`2'::<lambda_1>::operator() | ASan symbolization |
| 21 | (exe / CRT) | — | — | in std::invoke | ASan symbolization |
| 22 | (exe / CRT) | — | — | in std::_Func_impl_no_alloc<``anonymous namespace'::runModeA'::`2'::<lambda_1>,void,int>::_Do_call | ASan symbolization |
| 23 | (exe / CRT) | — | — | in std::_Func_class<void,int>::operator() | ASan symbolization |
| 24 | (exe / CRT) | — | — | in ``anonymous namespace'::runThreads'::`4'::<lambda_1>::operator() | ASan symbolization |
| 25 | (exe / CRT) | — | — | in std::invoke | ASan symbolization |
| 26 | (exe / CRT) | — | — | in std::thread::_Invoke<std::tuple<``anonymous namespace'::runThreads'::`4'::<lambda_1> >,0> | ASan symbolization |
| 27 | ucrtbase.dll | 0x0002CD2F | — | — | module+offset |
| 28 | (exe / CRT) | — | — | in asan_thread_start D:\a\_work\1\s\src\vctools\asan\llvm\compiler-rt\lib\asan\asan_win.cpp:170 | ASan symbolization |
| 29 | (exe / CRT) | — | — | (C:\WINDOWS\System32\KERNEL32.DLL+0x18002dda6) | ASan symbolization |
| 30 | ntdll.dll | 0x000C9C5B | — | — | module+offset |

### second free — ==25752==ERROR: AddressSanitizer: attempting double-free on 0x11d2eeea9220 in thread T11:

| # | module | RVA | function range | function | symbol method |
|---:|---|---|---|---|---|
| 0 | (exe / CRT) | — | — | in free D:\a\_work\1\s\src\vctools\asan\llvm\compiler-rt\lib\asan\asan_malloc_win.cpp:125 | ASan symbolization |
| 1 | Qt6Core.dll | 0x00111F12 | [0x00111ED0,0x00111F42) +0x42 | `??1QVariant@@QEAA@XZ` | export at function start |
| 2 | Qt6Core.dll | 0x0016D874 | [0x0016D700,0x0016D8FB) +0x174 | *(unnamed internal function)* | no export at function start |
| 3 | Qt6Core.dll | 0x001671AC | [0x00167180,0x00167216) +0x2C | *(unnamed internal function)* | no export at function start |
| 4 | Qt6Core.dll | 0x00167363 | [0x00167300,0x0016745F) +0x63 | *(unnamed internal function)* | no export at function start |
| 5 | Qt6Core.dll | 0x001707A8 | [0x00170770,0x001707B2) +0x38 | `?toString@QLocale@@QEBA?AVQString@@_J@Z` | export at function start |
| 6 | Qt6Core.dll | 0x002ACC75 | [0x002ACC20,0x002AD5EF) +0x55 | *(unnamed internal function)* | no export at function start |
| 7 | Qt6Core.dll | 0x002B00BF | [0x002AFFE0,0x002B01AE) +0xDF | *(unnamed internal function)* | no export at function start |
| 8 | Qt6Core.dll | 0x002AE927 | [0x002ADBB0,0x002AEACB) +0xD77 | *(unnamed internal function)* | no export at function start |
| 9 | Qt6Core.dll | 0x002A8282 | [0x002A7F40,0x002A82EA) +0x342 | *(unnamed internal function)* | no export at function start |
| 10 | Qt6Core.dll | 0x002A45E4 | [0x002A4550,0x002A4F9A) +0x94 | *(unnamed internal function)* | no export at function start |
| 11 | Qt6Core.dll | 0x001B4A78 | [0x001B4990,0x001B4B85) +0xE8 | *(unnamed internal function)* | no export at function start |
| 12 | Qt6Core.dll | 0x001B564E | [0x001B5623,0x001B5704) +0x2B | *(unnamed internal function)* | no export at function start |
| 13 | Qt6Core.dll | 0x001B0B8D | [0x001B0A10,0x001B0BC8) +0x17D | *(unnamed internal function)* | no export at function start |
| 14 | Qt6Core.dll | 0x001B6EA4 | [0x001B6E20,0x001B7186) +0x84 | `?startOfDay@QDate@@QEBA?AVQDateTime@@AEBVQTimeZone@@@Z` | export at function start |
| 15 | Qt6Core.dll | 0x002B3239 | [0x002B31F0,0x002B3378) +0x49 | `?fromString@QDateTimeParser@@QEBA_NAEBVQString@@PEAVQDateTime@@H@Z` | export at function start |
| 16 | Qt6Core.dll | 0x001B348B | [0x001B33A0,0x001B350F) +0xEB | `?fromString@QDateTime@@SA?AV1@AEBVQString@@VQStringView@@HVQCalendar@@@Z` | export at function start |
| 17 | Qt6Core.dll | 0x001B3381 | [0x001B3330,0x001B339A) +0x51 | `?fromString@QDateTime@@SA?AV1@AEBVQString@@VQStringView@@H@Z` | export at function start |
| 18 | Qt6Core.dll | 0x00004DDF | [0x00004DB0,0x00004DE9) +0x2F | `?fromString@QDateTime@@SA?AV1@AEBVQString@@0H@Z` | export at function start |
| 19 | (exe / CRT) | — | — | in ``anonymous namespace'::runModeA'::`2'::<lambda_1>::operator() | ASan symbolization |
| 20 | (exe / CRT) | — | — | in std::invoke | ASan symbolization |
| 21 | (exe / CRT) | — | — | in std::_Func_impl_no_alloc<``anonymous namespace'::runModeA'::`2'::<lambda_1>,void,int>::_Do_call | ASan symbolization |
| 22 | (exe / CRT) | — | — | in std::_Func_class<void,int>::operator() | ASan symbolization |
| 23 | (exe / CRT) | — | — | in ``anonymous namespace'::runThreads'::`4'::<lambda_1>::operator() | ASan symbolization |
| 24 | (exe / CRT) | — | — | in std::invoke | ASan symbolization |
| 25 | (exe / CRT) | — | — | in std::thread::_Invoke<std::tuple<``anonymous namespace'::runThreads'::`4'::<lambda_1> >,0> | ASan symbolization |
| 26 | ucrtbase.dll | 0x0002CD2F | — | — | module+offset |
| 27 | (exe / CRT) | — | — | in asan_thread_start D:\a\_work\1\s\src\vctools\asan\llvm\compiler-rt\lib\asan\asan_win.cpp:170 | ASan symbolization |
| 28 | (exe / CRT) | — | — | (C:\WINDOWS\System32\KERNEL32.DLL+0x18002dda6) | ASan symbolization |
| 29 | ntdll.dll | 0x000C9C5B | — | — | module+offset |

## `legacy-r10-i08.err`

- ==24476==ERROR: AddressSanitizer: attempting double-free on 0x12d0ac6a7fc0 in thread T9:
- 0x12d0ac6a7fc0 is located 0 bytes inside of 20-byte region [0x12d0ac6a7fc0,0x12d0ac6a7fd4)

### allocation — previously allocated by thread T9 here:

| # | module | RVA | function range | function | symbol method |
|---:|---|---|---|---|---|
| 0 | (exe / CRT) | — | — | in malloc D:\a\_work\1\s\src\vctools\asan\llvm\compiler-rt\lib\asan\asan_malloc_win.cpp:134 | ASan symbolization |
| 1 | Qt6Core.dll | 0x001C236D | [0x001C2270,0x001C239C) +0xFD | `?allocate2@QArrayData@@SAPEAXPEAPEAU1@_JW4AllocationOption@1@@Z` | export at function start |
| 2 | Qt6Core.dll | 0x0017F3CD | [0x0017F330,0x0017F439) +0x9D | `??0QString@@QEAA@PEBVQChar@@_J@Z` | export at function start |
| 3 | Qt6Core.dll | 0x0026C89E | [0x0026C840,0x0026C94B) +0x5E | *(unnamed internal function)* | no export at function start |
| 4 | Qt6Core.dll | 0x002686A2 | [0x00268210,0x00268D18) +0x492 | `?query@QSystemLocale@@UEBA?AVQVariant@@W4QueryType@1@$$QEAV2@@Z` | export at function start |
| 5 | Qt6Core.dll | 0x0016D7B1 | [0x0016D700,0x0016D8FB) +0xB1 | *(unnamed internal function)* | no export at function start |
| 6 | Qt6Core.dll | 0x001671AC | [0x00167180,0x00167216) +0x2C | *(unnamed internal function)* | no export at function start |
| 7 | Qt6Core.dll | 0x00167363 | [0x00167300,0x0016745F) +0x63 | *(unnamed internal function)* | no export at function start |
| 8 | Qt6Core.dll | 0x001707A8 | [0x00170770,0x001707B2) +0x38 | `?toString@QLocale@@QEBA?AVQString@@_J@Z` | export at function start |
| 9 | Qt6Core.dll | 0x002ACC75 | [0x002ACC20,0x002AD5EF) +0x55 | *(unnamed internal function)* | no export at function start |
| 10 | Qt6Core.dll | 0x002B00BF | [0x002AFFE0,0x002B01AE) +0xDF | *(unnamed internal function)* | no export at function start |
| 11 | Qt6Core.dll | 0x002AE927 | [0x002ADBB0,0x002AEACB) +0xD77 | *(unnamed internal function)* | no export at function start |
| 12 | Qt6Core.dll | 0x002A8282 | [0x002A7F40,0x002A82EA) +0x342 | *(unnamed internal function)* | no export at function start |
| 13 | Qt6Core.dll | 0x002A45E4 | [0x002A4550,0x002A4F9A) +0x94 | *(unnamed internal function)* | no export at function start |
| 14 | Qt6Core.dll | 0x001B4A78 | [0x001B4990,0x001B4B85) +0xE8 | *(unnamed internal function)* | no export at function start |
| 15 | Qt6Core.dll | 0x001B564E | [0x001B5623,0x001B5704) +0x2B | *(unnamed internal function)* | no export at function start |
| 16 | Qt6Core.dll | 0x001B0B8D | [0x001B0A10,0x001B0BC8) +0x17D | *(unnamed internal function)* | no export at function start |
| 17 | Qt6Core.dll | 0x001B6EA4 | [0x001B6E20,0x001B7186) +0x84 | `?startOfDay@QDate@@QEBA?AVQDateTime@@AEBVQTimeZone@@@Z` | export at function start |
| 18 | Qt6Core.dll | 0x002B3239 | [0x002B31F0,0x002B3378) +0x49 | `?fromString@QDateTimeParser@@QEBA_NAEBVQString@@PEAVQDateTime@@H@Z` | export at function start |
| 19 | Qt6Core.dll | 0x001B348B | [0x001B33A0,0x001B350F) +0xEB | `?fromString@QDateTime@@SA?AV1@AEBVQString@@VQStringView@@HVQCalendar@@@Z` | export at function start |
| 20 | Qt6Core.dll | 0x001B3381 | [0x001B3330,0x001B339A) +0x51 | `?fromString@QDateTime@@SA?AV1@AEBVQString@@VQStringView@@H@Z` | export at function start |
| 21 | Qt6Core.dll | 0x00004DDF | [0x00004DB0,0x00004DE9) +0x2F | `?fromString@QDateTime@@SA?AV1@AEBVQString@@0H@Z` | export at function start |
| 22 | (exe / CRT) | — | — | in ExifParserLegacy::readTime(class QByteArray const &) | ASan symbolization |
| 23 | (exe / CRT) | — | — | in ``anonymous namespace'::runModeLegacy'::`2'::<lambda_1>::operator() | ASan symbolization |
| 24 | (exe / CRT) | — | — | in std::invoke | ASan symbolization |
| 25 | (exe / CRT) | — | — | in std::_Func_impl_no_alloc<``anonymous namespace'::runModeLegacy'::`2'::<lambda_1>,void,int>::_Do_call | ASan symbolization |
| 26 | (exe / CRT) | — | — | in std::_Func_class<void,int>::operator() | ASan symbolization |
| 27 | (exe / CRT) | — | — | in ``anonymous namespace'::runThreads'::`4'::<lambda_1>::operator() | ASan symbolization |
| 28 | (exe / CRT) | — | — | in std::invoke | ASan symbolization |
| 29 | (exe / CRT) | — | — | in std::thread::_Invoke<std::tuple<``anonymous namespace'::runThreads'::`4'::<lambda_1> >,0> | ASan symbolization |
| 30 | ucrtbase.dll | 0x0002CD2F | — | — | module+offset |

### first free — freed by thread T8 here:

| # | module | RVA | function range | function | symbol method |
|---:|---|---|---|---|---|
| 0 | (exe / CRT) | — | — | in free D:\a\_work\1\s\src\vctools\asan\llvm\compiler-rt\lib\asan\asan_malloc_win.cpp:125 | ASan symbolization |
| 1 | Qt6Core.dll | 0x0026C8F8 | [0x0026C840,0x0026C94B) +0xB8 | *(unnamed internal function)* | no export at function start |
| 2 | Qt6Core.dll | 0x002686A2 | [0x00268210,0x00268D18) +0x492 | `?query@QSystemLocale@@UEBA?AVQVariant@@W4QueryType@1@$$QEAV2@@Z` | export at function start |
| 3 | Qt6Core.dll | 0x0016D7B1 | [0x0016D700,0x0016D8FB) +0xB1 | *(unnamed internal function)* | no export at function start |
| 4 | Qt6Core.dll | 0x001671AC | [0x00167180,0x00167216) +0x2C | *(unnamed internal function)* | no export at function start |
| 5 | Qt6Core.dll | 0x00167363 | [0x00167300,0x0016745F) +0x63 | *(unnamed internal function)* | no export at function start |
| 6 | Qt6Core.dll | 0x001707A8 | [0x00170770,0x001707B2) +0x38 | `?toString@QLocale@@QEBA?AVQString@@_J@Z` | export at function start |
| 7 | Qt6Core.dll | 0x002ACC75 | [0x002ACC20,0x002AD5EF) +0x55 | *(unnamed internal function)* | no export at function start |
| 8 | Qt6Core.dll | 0x002B00BF | [0x002AFFE0,0x002B01AE) +0xDF | *(unnamed internal function)* | no export at function start |
| 9 | Qt6Core.dll | 0x002AE927 | [0x002ADBB0,0x002AEACB) +0xD77 | *(unnamed internal function)* | no export at function start |
| 10 | Qt6Core.dll | 0x002A8282 | [0x002A7F40,0x002A82EA) +0x342 | *(unnamed internal function)* | no export at function start |
| 11 | Qt6Core.dll | 0x002A45E4 | [0x002A4550,0x002A4F9A) +0x94 | *(unnamed internal function)* | no export at function start |
| 12 | Qt6Core.dll | 0x001B4A78 | [0x001B4990,0x001B4B85) +0xE8 | *(unnamed internal function)* | no export at function start |
| 13 | Qt6Core.dll | 0x001B564E | [0x001B5623,0x001B5704) +0x2B | *(unnamed internal function)* | no export at function start |
| 14 | Qt6Core.dll | 0x001B0B8D | [0x001B0A10,0x001B0BC8) +0x17D | *(unnamed internal function)* | no export at function start |
| 15 | Qt6Core.dll | 0x001B6EA4 | [0x001B6E20,0x001B7186) +0x84 | `?startOfDay@QDate@@QEBA?AVQDateTime@@AEBVQTimeZone@@@Z` | export at function start |
| 16 | Qt6Core.dll | 0x002B3239 | [0x002B31F0,0x002B3378) +0x49 | `?fromString@QDateTimeParser@@QEBA_NAEBVQString@@PEAVQDateTime@@H@Z` | export at function start |
| 17 | Qt6Core.dll | 0x001B348B | [0x001B33A0,0x001B350F) +0xEB | `?fromString@QDateTime@@SA?AV1@AEBVQString@@VQStringView@@HVQCalendar@@@Z` | export at function start |
| 18 | Qt6Core.dll | 0x001B3381 | [0x001B3330,0x001B339A) +0x51 | `?fromString@QDateTime@@SA?AV1@AEBVQString@@VQStringView@@H@Z` | export at function start |
| 19 | Qt6Core.dll | 0x00004DDF | [0x00004DB0,0x00004DE9) +0x2F | `?fromString@QDateTime@@SA?AV1@AEBVQString@@0H@Z` | export at function start |
| 20 | (exe / CRT) | — | — | in ExifParserLegacy::readTime(class QByteArray const &) | ASan symbolization |
| 21 | (exe / CRT) | — | — | in ``anonymous namespace'::runModeLegacy'::`2'::<lambda_1>::operator() | ASan symbolization |
| 22 | (exe / CRT) | — | — | in std::invoke | ASan symbolization |
| 23 | (exe / CRT) | — | — | in std::_Func_impl_no_alloc<``anonymous namespace'::runModeLegacy'::`2'::<lambda_1>,void,int>::_Do_call | ASan symbolization |
| 24 | (exe / CRT) | — | — | in std::_Func_class<void,int>::operator() | ASan symbolization |
| 25 | (exe / CRT) | — | — | in ``anonymous namespace'::runThreads'::`4'::<lambda_1>::operator() | ASan symbolization |
| 26 | (exe / CRT) | — | — | in std::invoke | ASan symbolization |
| 27 | (exe / CRT) | — | — | in std::thread::_Invoke<std::tuple<``anonymous namespace'::runThreads'::`4'::<lambda_1> >,0> | ASan symbolization |
| 28 | ucrtbase.dll | 0x0002CD2F | — | — | module+offset |
| 29 | (exe / CRT) | — | — | in asan_thread_start D:\a\_work\1\s\src\vctools\asan\llvm\compiler-rt\lib\asan\asan_win.cpp:170 | ASan symbolization |
| 30 | (exe / CRT) | — | — | (C:\WINDOWS\System32\KERNEL32.DLL+0x18002dda6) | ASan symbolization |

### second free — ==24476==ERROR: AddressSanitizer: attempting double-free on 0x12d0ac6a7fc0 in thread T9:

| # | module | RVA | function range | function | symbol method |
|---:|---|---|---|---|---|
| 0 | (exe / CRT) | — | — | in free D:\a\_work\1\s\src\vctools\asan\llvm\compiler-rt\lib\asan\asan_malloc_win.cpp:125 | ASan symbolization |
| 1 | Qt6Core.dll | 0x00111F12 | [0x00111ED0,0x00111F42) +0x42 | `??1QVariant@@QEAA@XZ` | export at function start |
| 2 | Qt6Core.dll | 0x0016D874 | [0x0016D700,0x0016D8FB) +0x174 | *(unnamed internal function)* | no export at function start |
| 3 | Qt6Core.dll | 0x001671AC | [0x00167180,0x00167216) +0x2C | *(unnamed internal function)* | no export at function start |
| 4 | Qt6Core.dll | 0x00167363 | [0x00167300,0x0016745F) +0x63 | *(unnamed internal function)* | no export at function start |
| 5 | Qt6Core.dll | 0x001707A8 | [0x00170770,0x001707B2) +0x38 | `?toString@QLocale@@QEBA?AVQString@@_J@Z` | export at function start |
| 6 | Qt6Core.dll | 0x002ACC75 | [0x002ACC20,0x002AD5EF) +0x55 | *(unnamed internal function)* | no export at function start |
| 7 | Qt6Core.dll | 0x002B00BF | [0x002AFFE0,0x002B01AE) +0xDF | *(unnamed internal function)* | no export at function start |
| 8 | Qt6Core.dll | 0x002AE927 | [0x002ADBB0,0x002AEACB) +0xD77 | *(unnamed internal function)* | no export at function start |
| 9 | Qt6Core.dll | 0x002A8282 | [0x002A7F40,0x002A82EA) +0x342 | *(unnamed internal function)* | no export at function start |
| 10 | Qt6Core.dll | 0x002A45E4 | [0x002A4550,0x002A4F9A) +0x94 | *(unnamed internal function)* | no export at function start |
| 11 | Qt6Core.dll | 0x001B4A78 | [0x001B4990,0x001B4B85) +0xE8 | *(unnamed internal function)* | no export at function start |
| 12 | Qt6Core.dll | 0x001B564E | [0x001B5623,0x001B5704) +0x2B | *(unnamed internal function)* | no export at function start |
| 13 | Qt6Core.dll | 0x001B0B8D | [0x001B0A10,0x001B0BC8) +0x17D | *(unnamed internal function)* | no export at function start |
| 14 | Qt6Core.dll | 0x001B6EA4 | [0x001B6E20,0x001B7186) +0x84 | `?startOfDay@QDate@@QEBA?AVQDateTime@@AEBVQTimeZone@@@Z` | export at function start |
| 15 | Qt6Core.dll | 0x002B3239 | [0x002B31F0,0x002B3378) +0x49 | `?fromString@QDateTimeParser@@QEBA_NAEBVQString@@PEAVQDateTime@@H@Z` | export at function start |
| 16 | Qt6Core.dll | 0x001B348B | [0x001B33A0,0x001B350F) +0xEB | `?fromString@QDateTime@@SA?AV1@AEBVQString@@VQStringView@@HVQCalendar@@@Z` | export at function start |
| 17 | Qt6Core.dll | 0x001B3381 | [0x001B3330,0x001B339A) +0x51 | `?fromString@QDateTime@@SA?AV1@AEBVQString@@VQStringView@@H@Z` | export at function start |
| 18 | Qt6Core.dll | 0x00004DDF | [0x00004DB0,0x00004DE9) +0x2F | `?fromString@QDateTime@@SA?AV1@AEBVQString@@0H@Z` | export at function start |
| 19 | (exe / CRT) | — | — | in ExifParserLegacy::readTime(class QByteArray const &) | ASan symbolization |
| 20 | (exe / CRT) | — | — | in ``anonymous namespace'::runModeLegacy'::`2'::<lambda_1>::operator() | ASan symbolization |
| 21 | (exe / CRT) | — | — | in std::invoke | ASan symbolization |
| 22 | (exe / CRT) | — | — | in std::_Func_impl_no_alloc<``anonymous namespace'::runModeLegacy'::`2'::<lambda_1>,void,int>::_Do_call | ASan symbolization |
| 23 | (exe / CRT) | — | — | in std::_Func_class<void,int>::operator() | ASan symbolization |
| 24 | (exe / CRT) | — | — | in ``anonymous namespace'::runThreads'::`4'::<lambda_1>::operator() | ASan symbolization |
| 25 | (exe / CRT) | — | — | in std::invoke | ASan symbolization |
| 26 | (exe / CRT) | — | — | in std::thread::_Invoke<std::tuple<``anonymous namespace'::runThreads'::`4'::<lambda_1> >,0> | ASan symbolization |
| 27 | ucrtbase.dll | 0x0002CD2F | — | — | module+offset |
| 28 | (exe / CRT) | — | — | in asan_thread_start D:\a\_work\1\s\src\vctools\asan\llvm\compiler-rt\lib\asan\asan_win.cpp:170 | ASan symbolization |
| 29 | (exe / CRT) | — | — | (C:\WINDOWS\System32\KERNEL32.DLL+0x18002dda6) | ASan symbolization |
| 30 | ntdll.dll | 0x000C9C5B | — | — | module+offset |


## 2. 运行时 QueryType 捕获（R19 新增，直接证据）

方法：诊断程序派生 `QSystemLocale`（`qlocale_p.h` 中 `query()` 为 public virtual，构造函数会把
实例安装为进程级 system locale），在 `query()` 中按 `QueryType` 计数后转调基类。**不修改
Qt6Core.dll、不替换 Qt 版本**。源码：`tools/validation/fgc0029-asan-probe/spy_systemlocale.cpp`。
每臂 2 线程 × 1000 次迭代。

| 臂 | 内容 | LanguageId | TerritoryId | ScriptId | LocaleChanged | Grouping | GroupSeparator | **ZeroDigit** |
|---|---|---:|---:|---:|---:|---:|---:|---:|
| A | `QLocale::system()` | 1 | 1 | 1 | 1 | 0 | 0 | **0** |
| B | `QDateTime::fromString()` | 1 | 1 | 1 | 1 | 16016 | 16016 | **40040** |
| C | `ExifParserLegacy::readTime`（8117cfe 原文） | 1 | 1 | 1 | 1 | 16016 | 16016 | **40040** |
| D | `ExifParser::readTime`（修复后） | 0 | 0 | 0 | 0 | 0 | 0 | **0** |

原始日志：`work/validation/fgc0029-r19/spy/spy-<mode>.log`。

**结论（运行时直接证据）**：

1. `QLocale::system()` 本身**不查询** `ZeroDigit` —— 直接解释了 A 臂 0/240 的阴性结果，
   并**彻底排除** R17 的"`QLocale::system()` 是竞态入口"假设。
2. `QDateTime::fromString()` 每次调用触发 **≈20 次 `ZeroDigit` 查询**（40040/2000）与 ≈8 次
   `Grouping`/`GroupSeparator` 查询 —— 与栈中 `QLocale::toString(qlonglong)` →
   `longLongToString` → `zeroDigit` 的**数字本地化/补零**路径一致。
3. 修复后的 `ExifParser::readTime()` 对系统 locale **零查询** —— 触发条件被完全移除。

限制：该捕获是"调用计数"，不包含锁状态，也**不能**证明两线程的精确交错；插桩本身可能改变时序，
因此本实验只用于确认"哪条 QueryType 被哪条路径查询"，不用于复现率比较。

## 3. 与 R18 的差异（修正）

| 项 | R18 写法 | R19 修正 |
|---|---|---|
| `0x16D7B1` | `QLocale::system+0x3B1` | **未导出内部函数** `[0x16D700,0x16D8FB)`（`systemLocaleString` 候选，高置信度） |
| `0x26C89E` / `0x26C8F8` | `QSystemLocale::query+0x468E/+0x46E8` | **未导出内部函数** `[0x26C840,0x26C94B)`（`zeroDigit` 候选，高置信度）；`query` 的真实区间是 `[0x268210,0x268D18)` |
| `0x2686A2` | `query+0x492` | 正确（`query` 导出起点 +0x492） |
| `0x111F12` | `QVariant::~QVariant+0x42` | 正确（导出 `??1QVariant@@QEAA@XZ`，区间 `[0x111ED0,0x111F42)`） |
| 归因 | "根因位于 `QLocale::system()` 的进程级共享状态" | **不是** `QLocale::system`；是 `zeroDigit()` 赋值路径，经 `query(ZeroDigit)` 从数字格式化调用进入 |
