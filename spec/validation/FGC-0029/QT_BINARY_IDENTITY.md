# FGC-0029 — Qt 二进制身份（R19 基线资料，R20 沿用）

目的：确认 R17/R18 的 ASan 调用栈与 `ROOT_CAUSE_ANALYSIS.md` 引用的 Qt 源码是**同一版本**，
并且被分析的 DLL 是 Qt 官方二进制（而不是本机自行编译或第三方替换的版本）。

## 1. Kit 与源码

| 项 | 值 |
|---|---|
| Qt Kit | Qt 6.11.1 `msvc2022_64`，路径 `.qt/Qt/6.11.1/msvc2022_64` |
| 配置它的 CMake 缓存 | `Qt6_DIR = <repo>/.qt/Qt/6.11.1/msvc2022_64/lib/cmake/Qt6` |
| 源码对照版本 | `qtbase` tag **v6.11.1**（`raw.githubusercontent.com/qt/qtbase/v6.11.1/...`） |
| Qt 构建方式 | 官方在线安装器二进制（见 §2 数字签名） |

## 2. DLL 身份

| 文件 | 字节 | FileVersion | ProductVersion | 数字签名 | SHA256 |
|---|---:|---|---|---|---|
| `bin/Qt6Core.dll` | 10,480,440 | 6.11.1.0 | 6.11.1.0 | **Valid**，`CN=The QT Company Oy` | `FAE4778A42E93ADC82B831C879C886A05147E9CC26760808D21116BE5547259B` |
| `bin/Qt6Concurrent.dll` | 35,128 | 6.11.1.0 | 6.11.1.0 | **Valid**，`CN=The QT Company Oy` | `6B06B5AC67A4E08978EA87884974AF4323E5B38FACE713C191A97C0AB66E8D32` |
| `bin/Qt6Test.dll` | 382,776 | 6.11.1.0 | 6.11.1.0 | **Valid**，`CN=The QT Company Oy` | `830B8BBEE71C51B3F69EAFA77E089B0824AAA84821E18C568984CE900D8A4CBC` |

PE 头信息（`dumpbin /headers Qt6Core.dll`）：

```
machine (x64)
linker version 14.39
image version 6.11
subsystem version 6.00
```

`linker version 14.39` 对应 **MSVC 19.39 / Visual Studio 2022 17.9**。这是 Qt 官方构建所用
工具链的标识，**不能**据此推断本机 Agent 或本仓库使用的编译器版本——本仓库用
**MSVC 14.44.35207**（独立记录见下），二者不同属预期（官方二进制不会用我们的编译器）。

> R18 曾把 14.39 写成 "VS 17.10"，R19 更正为 **VS 17.9（MSVC 19.39）**。

## 3. 符号（PDB）情况

- Qt Kit 内**没有** `Qt6Core.pdb`（`Get-ChildItem -Recurse -Filter *.pdb` 无结果）。
  官方 Windows 二进制默认只带导出表；调试符号需另行安装 "Qt Debug Information Files"。
- 因此本轮的 Qt 栈解析采用：**导出表区间（`dumpbin /exports`）+ RVA + 反汇编 + v6.11.1 源码**
  三段交叉，而不是"最近导出符号即函数名"。
- QGroundControl 自身有 PDB：`build-win-asan-geotag/RelWithDebInfo/QGroundControl.pdb`
  （ASan 构建产物，报告里的 `ExifParser::readTime`、`GeoTagController::_parseExifForImage`
  等帧由它解析）。

## 4. 版本一致性判定

| 检查 | 结果 |
|---|---|
| DLL FileVersion / ProductVersion = 6.11.1.0 | ✅ 与源码 tag v6.11.1 一致 |
| 官方签名有效（The Qt Company Oy） | ✅ 非本机编译、未被替换 |
| 安装路径与 CMake 使用的 Qt 一致 | ✅ 同一个 `.qt/Qt/6.11.1/msvc2022_64` |
| 报告中出现的 RVA 与 v6.11.1 源码行为一致 | ✅ `QVariant::~QVariant`、`QArrayData::allocate2`、`QString(QChar*,n)`、`zeroDigit()` 的指令序列与源码一一对应（见 `ASAN_STACK_ANALYSIS.md` §5/§6） |
| Qt 私有 PDB 可用于更精确符号化 | ❌ 不可得（见 §3），故部分帧仍标为"内部函数，中/低置信度" |

结论：**被分析的 DLL 就是官方 Qt 6.11.1（msvc2022_64）**，与引用的 v6.11.1 源码版本一致；
栈解析精度受"无 Qt 私有 PDB"限制，已在分析中逐帧标注置信度。

## 5. 原始输出（R19 采集，未加工）

```text
PS> Get-Item .qt\Qt\6.11.1\msvc2022_64\bin\Qt6Core.dll | Select-Object FullName,Length,LastWriteTime
FullName : E:\Flight\Flight_GroundControl_test\Flight_GroundControl\.qt\Qt\6.11.1\msvc2022_64\bin\Qt6Core.dll
Length   : 10480440
LastWriteTime : 2026/5/8 0:09:34

PS> (Get-Item ...\Qt6Core.dll).VersionInfo | Select-Object FileVersion,ProductVersion,FileDescription,CompanyName
FileVersion     : 6.11.1.0
ProductVersion  : 6.11.1.0
FileDescription : C++ Application Development Framework
CompanyName     : The Qt Company Ltd.

PS> Get-FileHash ...\Qt6Core.dll -Algorithm SHA256
FAE4778A42E93ADC82B831C879C886A05147E9CC26760808D21116BE5547259B

PS> Get-AuthenticodeSignature ...\Qt6Core.dll
Status : Valid
Signer : CN=The QT Company Oy, O=The QT Company Oy, L=Espoo, S=Uusimaa, C=FI,
         SERIALNUMBER=2637805-2, OID.2.5.4.15=Private Organization, OID.1.3.6.1.4.1.311.60.2.1.3=FI

PS> dumpbin /headers ...\Qt6Core.dll | Select-String "linker version|machine|image version"
machine (x64)
linker version 14.39
image version 6.11
```

本机编译器（与 Qt DLL 无关，独立记录）：

```text
MSVC 14.44.35207（VS 2022 BuildTools），cl.exe 19.44.35229
```

