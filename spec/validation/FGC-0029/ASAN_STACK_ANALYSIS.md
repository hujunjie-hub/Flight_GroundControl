# FGC-0029 R18 — ASan 报告三栈完整解析

本文件是对 R17/R18 捕获的三份 `attempting double-free` 报告的**逐帧重新解析**，取代 R17 中
"按最近导出符号猜函数"的做法。所有 RVA 均相对 `Qt6Core.dll` 映像基址
（`0x180000000`，报告中的 `Qt6Core.dll+0x180xxxxxx` 即 RVA）。

- 报告 1：`work/validation/fgc0029-r17/asan-run2/r11-i15.err`（GeoTagControllerTest，修复前）
- 报告 2：`work/validation/fgc0029-r17/asan-run2/r13-i10.err`（GeoTagControllerTest，修复前）
- 报告 3：`work/validation/fgc0029-r18/diff-run1/a-r5-i1.err`（**仅 `QDateTime::fromString`**，R18 新增）

## 1. 三份报告的公共事实

| 项 | 报告 1 | 报告 2 | 报告 3 |
|---|---|---|---|
| 出错线程 | T4 | T12 | T12 |
| 块大小 | 20 字节 | 20 字节 | 20 字节 |
| 分配线程 | T4 | T12 | T12 |
| 第一次释放线程 | **T14** | **T4** | **T2** |
| 第二次释放（报错）线程 | T4 | T12 | T12 |
| 触发程序 | `GeoTagControllerTest` → `ExifParser::readTime` | 同左 | `runModeA`（`QDateTime::fromString` 循环，`main.cpp:133`） |

三份报告**逐帧 RVA 完全相同**（只有线程号与地址不同），说明是同一缺陷、同一代码路径。
报告 3 证明该缺陷**不需要 libexif、不需要 GeoTagController**：一个只有
`QDateTime::fromString("2024:05:06 07:08:09", "yyyy:MM:dd HH:mm:ss")` 的程序即可复现。

## 2. 分配栈（Allocation）

`previously allocated by thread T4 / T12`：

| # | RVA | 最近导出符号（区间） | 偏移 | 置信度 |
|---|---|---|---|---|
| 0 | — | `malloc` (ASan interceptor) | — | 确定 |
| 1 | `0x1c236d` | `QArrayData::allocate2` | +0xFD | 高（区间宽 0x130，导出表精确界定） |
| 2 | `0x17f3cd` | `QString::QString(QChar const*, qsizetype)` | +0x9D | 高（区间宽 0x110） |
| 3 | `0x26c89e` | `QSystemLocale::query` 区间内的内部函数 F | +0x468E | 见 §5 |
| 4 | `0x2686a2` | `QSystemLocale::query` | +0x492 | 中（`query` 是 0x48C0 宽的巨型 switch） |
| 5 | `0x16d7b1` | `QLocale::system` | +0x3B1 | 中（`systemData()`/`updateSystemPrivate()` 被内联在此） |
| 6 | `0x1671ac` | `QDateTimeParser` TU（`locale()` 之后） | +0x4C | 中（`QDateTimeParser` 构造函数位置） |
| 7 | `0x167363` | 同上 | +0x203 | 中 |
| 8 | `0x1707a8` | `QLocale::toString` 之前 | +0x38 | 低（无导出覆盖的内部函数） |
| 9–18 | `0x2acc75 … 0x4ddf` | 无导出覆盖 | — | 低（仅用于链路定位） |
| 19 | — | `ExifParser::readTime`（`ExifParser.cc:29`） | — | 确定（有 PDB） |

## 3. 第一次释放栈（First free）

`freed by thread T14 / T4 / T2`：

| # | RVA | 符号 | 偏移 | 说明 |
|---|---|---|---|---|
| 0 | — | `free` (ASan interceptor) | — | — |
| 1 | `0x26c8f8` | 内部函数 F（与分配栈 #3 同一函数） | +0x46E8 | 紧跟 `call QArrayData::deallocate`（见 §5 反汇编） |
| 2 | `0x2686a2` | `QSystemLocale::query` | +0x492 | 调用 F |
| 3 | `0x16d7b1` | `QLocale::system` | +0x3B1 | — |
| 4–18 | 与分配栈 #6 起相同 | — | — | 同一链路 |

## 4. 第二次释放栈（Second free / 报错点）

`attempting double-free ... in thread T4 / T12`：

| # | RVA | 符号 | 偏移 | 说明 |
|---|---|---|---|---|
| 0 | — | `free` (ASan interceptor) | — | — |
| 1 | `0x111f12` | `QVariant::~QVariant` | +0x42 | 区间宽 0x80，导出表精确界定（见 §6 反汇编） |
| 2 | `0x16d874` | `QLocale::system` | +0x474 | 与分配栈 #5 同一函数、不同调用点 |
| 3–19 | 与分配栈 #6 起相同 | — | — | 同一链路 |

## 5. 分配点与第一次释放点的反汇编（决定性）

在 `QDateTime` 之外没有任何 Qt 私有符号，因此直接反汇编 `0x26c8xx` 区域（`dump-32020` 中
Qt6Core 基址 `0x7ffe5d250000`，文件 `rva-map/dis-freesite-region.txt`）：

```
mov  r9d, 0Bh                        ; wchar_t digits[11]  ← LOCALE_SNATIVEDIGITS 缓冲区
call qword ptr [...GetLocaleInfoW]   ; GetLocaleInfoW(lcid, LOCALE_SNATIVEDIGITS, digits, 11)
test eax, eax / je
mov  r8d, 1                          ; 长度 1
lea  rdx, [rsp+48h]                  ; digits
lea  rcx, [rsp+20h]
call Qt6Core!QString::QString        ; QString(digits, 1)  ← 1 字符 → 2*1+18 = 20 字节
mov  rax, [rsp+20h]
mov  rdx, [rdi+8]                    ; 旧 zero 的 d 指针
mov  [rdi+8], rax                    ; ← 把新数据写入共享对象成员（无锁、非原子）
...
lock xadd dword ptr [rdx], eax       ; 旧数据引用计数 -1
cmp  eax, 1 / jne
call Qt6Core!QArrayData::deallocate  ; ← 第一次释放（返回地址 0x26c8f8）
```

与 Qt 6.11.1 `qlocale_win.cpp` 的 `QSystemLocalePrivate::zeroDigit()` 完全对应：

```cpp
QVariant QSystemLocalePrivate::zeroDigit()
{
    if (zero.isEmpty()) {
        wchar_t digits[11];                                  // mov r9d, 0Bh
        if (getLocaleInfo(LOCALE_SNATIVEDIGITS, digits, 11)) {
            zero = QString::fromWCharArray(digits, 1);        // QString(digits,1) → 20 字节
        }
    }
    return nullIfEmpty(zero);
}
```

**结论（高置信度）**：被双重释放的 20 字节块是 Qt 进程级系统 locale 缓存
`QSystemLocalePrivate::zero`（1 个字符的 QString，即本地零数字）的数据区；
"第一次释放"就是 `zero = ...` 赋值时对旧值的释放。

## 6. 第二次释放点反汇编

`QVariant::~QVariant`（`0x111ed0`）在 `+0x42` 处对被持有对象调用释放：

```
mov  rax, [rcx+18h]      ; handler
mov  rcx, rax
and  rcx, ~3
mov  rax, [rcx+38h]      ; shared-payload 释放函数
test rax, rax / je
mov  rdx, rbx
call qword ptr [IAT]     ; ← 释放 QString 数据（返回地址 0x111f12）
```

即：第二次释放来自一个 **`QVariant` 临时对象析构**，它持有的 QString 与 `zero` 共享同一个
数据块。`QVariant` 是 `QLocale`/`qlocale_win` 中查询系统 locale 的返回类型
（`QSystemLocalePrivate::*()` 全部返回 `QVariant`）。

## 7. 仍未证明的部分（明确标注）

1. **谁在锁保护之外读写了 `zero`。** `systemData()` 中的 `systemDataMutex` 是真实存在的
   （见 `ROOT_CAUSE_ANALYSIS.md` §4），因此"`systemData()` 无锁修改共享 QString"这一 R17 说法
   **不成立，已撤回**。但 `QSystemLocale::query()` 还有其它调用方（如 `QLocaleData::zeroUcs()`
   在 `qlocale.cpp:1107` 直接调用 `systemLocale()->query(ZeroDigit)`，**不经过** `systemData()`），
   这些调用方是否就是本次竞态的另一端，尚未用实验证实。
2. **函数 F 的符号名**只能由反汇编 + 源码模式确认（§5），没有 Qt PDB 可交叉验证。
3. **为何两次释放都发生在同一函数、却没有被互斥量串行化**：无法从现有样本中直接观察锁状态
   （ASan 报告不含寄存器/锁状态），因此保留为未决问题。

因此本文件只主张：

> **已验证**：被双重释放的 20 字节对象是 Qt 系统 locale 缓存中的 1 字符 QString
> （`QSystemLocalePrivate::zero`），由 `zeroDigit()` 的赋值分配、由同一赋值释放一次、
> 再由另一个线程中的 `QVariant` 析构释放一次；两条 worker 线程都在
> `ExifParser::readTime → QDateTime::fromString` 链路内。
>
> **未验证**：Qt 内部具体的同步缺陷位置与责任字段。
