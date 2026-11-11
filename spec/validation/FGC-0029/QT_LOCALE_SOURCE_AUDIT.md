# FGC-0029 R19 — Qt 6.11.1 Windows Locale 实现审计

审计对象：`qtbase` **v6.11.1**（本地稀疏克隆 `E:\Flight\Flight_GroundControl_test\work\bench\qtbase`，
commit `59c81a3c2247b821b9b84b4eb8d939b77e07e276`），文件：

- `src/corelib/text/qlocale_win.cpp`（Windows 后端）
- `src/corelib/text/qlocale.cpp`（QLocale / QLocaleData / systemData）
- `src/corelib/text/qlocale_p.h`（`QSystemLocale`、`QueryType`）
- `src/corelib/time/qdatetimeparser.cpp`（解析器侧调用）

运行时二进制：`Qt6Core.dll` 6.11.1.0，SHA256
`FAE4778A42E93ADC82B831C879C886A05147E9CC26760808D21116BE5547259B`（身份见
`QT_BINARY_IDENTITY.md`）。函数边界由 PE `.pdata` 表推导（`pe_funcs.py`，13642 个函数），
**不采用"最近导出符号"**。

---

## 1. `QSystemLocalePrivate` 字段与生命周期

`QSystemLocalePrivate` 定义在 `qlocale_win.cpp`，是文件内类型；实例由
`Q_GLOBAL_STATIC(QSystemLocalePrivate, systemLocalePrivate)` 提供（进程内单例）。

| 字段 | 类型 | 写入函数 | 读取函数 | 保护 | 生命周期 |
|---|---|---|---|---|---|
| `lcid` | `LCID` | 构造函数 `QSystemLocalePrivate()`；`update()` | 所有 `getLocaleInfo()` 调用 | **无** | 单例构造 → 进程退出 |
| `zero` | `QString` | `zeroDigit()`（`zero = QString::fromWCharArray(digits, 1)`）；`update()`（`zero.resize(0)`） | `zeroDigit()` 的 `zero.isEmpty()` 与 `nullIfEmpty(zero)`；`substituteDigits()`（`zero.size()/zero.at()`）；`yearFix()`（`zero != u'0'`）；`correctDigits()` 间接 | **无** | 首次查询时分配；`update()` 或重新赋值时释放 |
| `substitutionType` | `SubstitutionType`（enum） | `substitution()`（`SUnknown → SContext/SAlways/SNever`）；`update()`（置回 `SUnknown`） | `substitution()` | **无** | 单例构造（`SUnknown`）→ 进程退出 |
| `sizes` | `QLocaleData::GroupSizes` | `groupingSizes()`（`sizes.least/higher`） | `groupingSizes()` | **无** | 同上 |

关键源码（`qlocale_win.cpp`）：

```cpp
QVariant QSystemLocalePrivate::zeroDigit()
{
    if (zero.isEmpty()) {                                  // (1) 读共享 zero
        wchar_t digits[11];
        if (getLocaleInfo(LOCALE_SNATIVEDIGITS, digits, 11)) {
            zero = QString::fromWCharArray(digits, 1);      // (2) 写共享 zero；旧值在此释放
        }
    }
    return nullIfEmpty(zero);                               // (3) 读共享 zero（构造 QVariant）
}

void QSystemLocalePrivate::update()
{
    lcid = getDefaultWinId();
    substitutionType = SUnknown;
    zero.resize(0);                                         // (4) 释放共享 zero 的数据
}
```

- **`update()` 不重置 `sizes`**（第 3.2 节第 9 问的答案：不重置）。
- `substituteDigits()` 会**读** `zero`（`zero.size()`、`zero.at(0)`），且先调用
  `zeroDigit()` 以确保已初始化 —— 即它也参与同一次读-改-写。
- `correctDigits()` 在 `substitution() == SAlways` 时调用 `substituteDigits()`。

## 2. `QSystemLocale::query()` 与 `QueryType`

`qlocale_p.h:144-193` 定义 `enum QueryType`，本机 Qt 6.11.1 的序号：

| 值 | 名称 | 是否触碰共享可变状态 | 触碰哪个 |
|---:|---|---|---|
| 0 | `LanguageId` | 否（构造临时 QLocaleId） | — |
| 1 | `TerritoryId` | 否 | — |
| 2 | `DecimalPoint` | 否（每次 `GetLocaleInfo`） | — |
| 3 | `Grouping` | **是** | `sizes` |
| 4 | `GroupSeparator` | 否 | — |
| **5** | **`ZeroDigit`** | **是** | **`zero`、`substitutionType`（经 `substituteDigits`）** |
| 6 | `NegativeSign` | 否 | — |
| … | Date/Time 格式、名称类 | 否（每次查询 OS） | — |
| 28/29 | `AMText`/`PMText` | 否 | — |
| 39 | `LocaleChanged` | **是** | `lcid`、`substitutionType`、`zero`（`update()`） |

`QSystemLocale::query()` 的对应分支（`qlocale_win.cpp` 尾部）：

```cpp
case Grouping:      return d->groupingSizes();   // 写 sizes（无锁）
case ZeroDigit:     return d->zeroDigit();       // 读写 zero（无锁）
case LocaleChanged: d->update(); break;          // 重置 zero（调用方持锁时才有锁）
```

## 3. ZeroDigit 的三条查询路径（必须分开记录）

| 路径 | 源码位置 | 调用点 | 是否持 `systemDataMutex` |
|---|---|---|---|
| **P1 格式化（本报告 ASan 栈实际命中）** | `qlocale.cpp:2251 calculateFiller` → `1095 QLocaleData::zeroDigit()` → `1067 localeString()` → `1048 systemLocaleString()` → `1055 systemLocale()->query(type)` | `QLocale::toString(qlonglong, int, char32_t)` 的补零分支（`qlocale.cpp:2295-2312`） | **否** |
| **P2 解析（`NumericData`）** | `qlocale.cpp:4391 NumericData::NumericData` → `4412-4429` 的 lambda `getString(ZeroDigit)` → `sys->query(query)` | `NumericTokenizer`/`stringToUnsLongLong` 等数值解析 | **否** |
| **P3 直接查询** | `qlocale.cpp:1100 QLocaleData::zeroUcs()` → `1104 systemLocale()->query(ZeroDigit)` | `QLocaleData::longLongToString` 等数字转字符串/解析 | **否** |

三条路径都**不获取** `systemDataMutex`：该互斥量只出现在 `systemData()` 内部
（`qlocale.cpp:919-937`），见 §4。

## 4. `systemDataMutex` 的保护范围（A/B/C）

```cpp
static const QLocaleData *systemData(qsizetype *sysIndex = nullptr)
{
    /* This modifies (cross-thread) global state, so is mutex-protected. */
    {
        Q_CONSTINIT static QLocaleId sysId;
        bool updated = false;
        Q_CONSTINIT static QBasicMutex systemDataMutex;
        systemDataMutex.lock();
        if (systemLocaleData.m_language_id == 0) { updateSystemPrivate(); updated = true; }
        if (sysIndex && (updated || *sysIndex < 0)) { ... }
        systemDataMutex.unlock();
    }
    return &systemLocaleData;
}
```

| 编号 | 路径 | 是否在该锁内 | 说明 |
|---|---|---|---|
| **A** | `systemData()` → `updateSystemPrivate()` | **是** | 仅当 `systemLocaleData.m_language_id == 0`（首次/失效后）执行 |
| **B** | `QLocaleData::zeroDigit()` / `zeroUcs()` / `NumericData` → `query(ZeroDigit)` | **否** | 三条路径见 §3 |
| **C** | `query(LocaleChanged)` → `update()` | **是**（唯一调用方是 A 中的 `updateSystemPrivate()`） | 因此 `update()` 只会在持锁状态下运行 |

**结论**：写方 `update()`（持锁）与读-写方 `zeroDigit()`（无锁）**没有共同的同步机制**；
而 `zeroDigit()` 自身在两个线程间也**完全没有**互斥。`systemDataMutex` 不能保护 `zero`。

> 注意用词：这是"`zero` 字段在无锁情况下被读写"的**源码事实**，不是"已证明该无锁读写就是
> 本次双重释放的根因"——后者见 `ROOT_CAUSE_ANALYSIS.md` 的证据分级。

## 5. 初始化与析构竞争

```cpp
Q_CONSTINIT static QSystemLocale *_systemLocale = nullptr;   // qlocale.cpp:87
Q_CONSTINIT static QLocaleData systemLocaleData = {};        // qlocale.cpp:88

QSystemLocale::QSystemLocale() : next(_systemLocale) {       // 无锁压栈
    _systemLocale = this;
    systemLocaleData.m_language_id = 0;                      // 强制刷新
}
QSystemLocale::~QSystemLocale() { ... }                       // 无锁出栈

static const QSystemLocale *systemLocale()
{
    if (_systemLocale) return _systemLocale;
    static QSystemLocale globalInstance;                     // magic static（线程安全初始化）
    return &globalInstance;
}
```

- `_systemLocale` 是**非原子的全局指针**，`QSystemLocale` 的构造/析构对它做无锁读写。
  Qt 自己的注释说明："there should not be diverse threads creating and destroying
  QSystemLocale instances concurrently, so no attempt is made at thread-safety"。
- `globalInstance` 的构造本身受 C++ magic static 保护，**但**构造函数会把
  `systemLocaleData.m_language_id` 置 0 —— 这是对 `systemLocaleData` 的写。
- `QSystemLocalePrivate` 由 `Q_GLOBAL_STATIC` 提供：首次 `systemLocalePrivate()` 调用时在锁内
  构造，进程退出时销毁；其析构不会与业务线程并发（正常情况下）。

## 6. ASan 栈 → 源码映射（基于 `.pdata` 函数边界 + 调用指令）

被测栈（三份报告一致）自内向外：

| 帧 | RVA | `.pdata` 函数区间 | 起始处是否有导出 | 判定（置信度） |
|---|---|---|---|---|
| 0 | — | `ucrtbase!free`（ASan 拦截） | — | 确定 |
| 1 | `0x111F12` | `[0x111ED0,0x111F42)` | `??1QVariant@@QEAA@XZ` | **`QVariant::~QVariant`**（确定） |
| 2 | `0x16D874` | `[0x16D700,0x16D8FB)` | 否 | 内部函数，见下（高） |
| 3 | `0x1671AC` | `[0x167180,0x167216)` | 否 | 内部函数，见下（中高） |
| 4 | `0x167363` | `[0x167300,0x16745F)` | 否 | 内部函数，见下（中高） |
| 5 | `0x1707A8` | `[0x170770,0x1707B2)` | `?toString@QLocale@@QEBA?AVQString@@_J@Z` | **`QLocale::toString(qlonglong)`**（确定） |
| 6–9 | `0x2ACC75` … `0x2A45E4` | 各为独立内部函数 | 否 | **未解析内部函数**（仅保留 RVA/区间） |
| 10 | `0x1B4A78` | `[0x1B4990,0x1B4B85)` | 否 | 未解析内部函数 |
| … | `0x1B564E`…`0x1B348B` | — | `?fromString@QDateTime@@SA…` 等 | 解析器入口（部分确定） |
| 末 | `0x4DDF` | `[0x4DB0,0x4DE9)` | `?fromString@QDateTime@@SA?AV1@AEBVQString@@0H@Z` | **`QDateTime::fromString(QString, QString, int)`**（确定） |

调用指令核验（对 `dump-32020`，Qt6Core 基址 `0x7FFE5D250000`）：

```
0x5D3C0788..7A4  QLocale::toString(qlonglong):  r9d=-1, [rsp+28h]=-1, [rsp+20h]=0Ah
0x5D3C07A4       call 0x5D3B7300        ; 内部函数 [0x167300) —— 实参 (data, number, -1, 10, flags)
0x5D3B735F       call 0x5D3B7180        ; [0x167300) → [0x167180)
0x5D3B71A8       call 0x5D3BD700        ; [0x167180) → [0x16D700)
0x5D3BD7AC..B1   内部函数 [0x16D700) 调用 [IAT 0x3F1DB8]，紧随其后调用 QVariant::~QVariant/metaType/toString
0x5D4B869E       query+0x48E → call 0x5D4BC840   ; QSystemLocale::query → [0x26C840)
0x5D4BC89A       [0x26C840) → call QString::QString(QChar const*, qsizetype)   ← ASan 分配点 (0x26C89E)
0x5D4BC8F4       [0x26C840) → call QArrayData::deallocate                     ← ASan 第一次释放 (0x26C8F8)
0x5D4BC8AC       [0x26C840) → mov [rdi+8], rax                                 ← 写共享成员的 QString
```

源码对应（按调用序列唯一匹配）：

| 内部函数 | 源码候选 | 依据 | 置信度 |
|---|---|---|---|
| `[0x26C840,0x26C94B)` | `QSystemLocalePrivate::zeroDigit()`（`qlocale_win.cpp`） | `GetLocaleInfoW(..., 11)` + `QString(buf,1)` + 写成员 + 释放旧值 + 返回 `QVariant` | **高** |
| `[0x16D700,0x16D8FB)` | `systemLocaleString()`（`qlocale.cpp:1048`） | 唯一同时具备 `query()` → `QVariant::metaType()` → `QVariant::toString()` 序列的函数 | **高** |
| `[0x167180,0x167216)` | `QLocaleData::zeroDigit()`（`qlocale.cpp:1095`） | 调用 `systemLocaleString()` 后以 `QString::fromRawData` 构造回退值 | 中高 |
| `[0x167300,0x16745F)` | `QLocaleData::longLongToString(...)` | 由 `toString(qlonglong)` 以 `(number, -1, 10, flags)` 调用，内部再取 zero digit | 中高 |
| `[0x2ACC20,0x2AD5EF)` 等 | **未解析** | 无导出、无可唯一匹配的调用序列 | 低 |

**结论**：ASan 捕获的分配与第一次释放都发生在 `QSystemLocalePrivate::zeroDigit()`（内部函数
`[0x26C840,0x26C94B)`），该函数经 `QSystemLocale::query(ZeroDigit)` 由
`QLocale::toString(qlonglong)` 的补零/数字本地化路径调用 —— **这条路径不持 `systemDataMutex`**。

## 7. 运行时 QueryType 捕获（R19 新增）

方法（不修改 Qt DLL）：诊断程序派生 `QSystemLocale`（`qlocale_p.h` 中 `query()` 是
public virtual，构造函数会把实例安装为进程级 system locale），在 `query()` 中计数后转调基类。
这属于 Qt 平台插件/测试使用的同一机制，语义等价。

| 代码路径 | 捕获到的 QueryType 计数（1000 次/线程 × 2 线程，单进程） | 结论 |
|---|---|---|
| A `QLocale::system()` | 见 `work/validation/fgc0029-r19/spy/spy-<mode>.log` | 见下方小结 |
| B `QDateTime::fromString()` | 同上 | 同上 |
| C `ExifParserLegacy::readTime` | 同上 | 同上 |
| D `ExifParser::readTime`（修复后） | 同上 | 同上 |

（具体计数与结论在本轮运行后填入 `ASAN_STACK_ANALYSIS.md` §7 与
`ROOT_CAUSE_ANALYSIS.md` §5；若捕获失败则标记为"仅静态推导"。）

## 8. 结论与仍开放的问题

**源码确认**：

1. `zero`、`substitutionType`、`sizes` 三个字段都在无锁情况下被 `query()` 路径读写。
2. `systemDataMutex` 只覆盖 `systemData()` 的刷新路径（A）与 `update()`（C）；
   `ZeroDigit` 的三条查询路径（B）**都不持锁**。
3. `update()` 会释放 `zero` 的数据并重置 `substitutionType`，且**不重置** `sizes`。
4. `_systemLocale` 是普通全局指针，`QSystemLocale` 的构造/析构无同步（Qt 注释已声明不支持）。

**仍开放**：

- 两个线程进入 `zeroDigit()` 的**精确交错**（谁先读 `zero.isEmpty()`、谁先赋值、谁先释放旧值）
  没有运行时证据；ASan 只给出"分配/首释/再释"三处快照。
- 因此"首次破坏引用计数的操作"仍属推断；严格表述见 `ROOT_CAUSE_ANALYSIS.md` §3。
