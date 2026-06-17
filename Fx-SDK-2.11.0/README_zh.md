# fxSDK

🌏 [English](README.md) | **简体中文**

*Translated by 陈湛明 (Chén Zhànmíng) on 2024-03-26*

*下文中的“程序”指的不是 BASIC 程序，而是带有图标且可以在主菜单打开的 add-in。*

*Planète Casio 上的一个帖子：[fxSDK 是另外一套开发计算器程序的工具链（法语帖子）](https://www.planet-casio.com/Fr/forums/topic13164-last-fxsdk-un-sdk-alternatif-pour-ecrire-des-add-ins.html)*

fxSDK 是一套开发卡西欧图形计算器里的程序的工具，里面提供了命令行工具、构建系统以及一个跨平台编译器，用来开发程序以及库。这套工具需要和 [gint 内核](/Lephenixnoir/gint) 一起使用，因为这个内核提供的运行时非常之厉害。

这个仓库只提供了 SDK 的命令行工具。如果想要构建程序，需要一些其他的组件，比如[跨平台编译器](/Lephenixnoir/sh-elf-gcc)还有 [gint 内核](/Lephenixnoir/gint)。更多信息请移步至[如何安装]()。

fxSDK 以 [MIT 协议](LICENSE)发布。

## 支持的机型及其兼容性

**计算器**

fxSDK，或者说 gint，支持卡西欧 fx-9860G 系列几乎所有的型号，包括：

* ![](https://www.planet-casio.com/images/icones/calc/g85.png)（部分支持）基于SH3 的 fx-9860G、fx-9860G SD、以及类似运行 1.xx 版本操作系统的型号，还有 fx-9860G SDK 里的模拟器。
* ![](https://www.planet-casio.com/images/icones/calc/g95.png) 基于 SH3 的 fx-9750G、fx-9860G II、fx-9860G II SD，以及类似运行 2.xx 版本操作系统的型号。
* ![](https://www.planet-casio.com/images/icones/calc/g75+.png) 所有基于 SH4 且运行操作系统 2.xx 的型号，包括 fx-9750G II 与 SH4 的 fx-9860G II。
* ![](https://www.planet-casio.com/images/icones/calc/g35+e2.png) fx-9750G III 与 fx-9860G III。

也支持卡西欧 fx-CG 系列的计算器：

* ![](https://www.planet-casio.com/images/icones/calc/cg20.png) 经典款 fx-CG 10/20。
* ![](https://www.planet-casio.com/images/icones/calc/g90+e.png) fx-CG 50。

**操作系统**

* Linux 有官方支持。只要装好了依赖，任何的发行版都可以运行。
* macOS 由于缺少测试，需要进行一些小小的调整才可以运行。
* Windows 因为有 WSL（Linux 子系统），所以也有官方支持。然而 Windows 本身并不受支持，但如果您想的话可以尝试贡献。

**编程语言**

* C 语言：支持最新版 GCC，目前是 C11 或者 C2X。标准库 [fxlibc](https://gitea.planet-casio.com/Vhex-Kernel-Core/fxlibc/) 是我们自行构建的，大致遵循 C99。使用其他的标准库也是可以的。
* C++：支持最新版 GCC，目前是 C++20 或者 C++23。标准库 [libstdc++](https://gcc.gnu.org/onlinedocs/libstdc++/) 也是最新的。
* 汇编语言：卡西欧计算器的处理器为 SuperH 架构，其中数 SH4AL-DSP 最多。binutils 里面提供的汇编器可供使用。
* 还想要其他的语言？只要能编译到 C 就行，比如 [fxtran for Fortran](https://www.planet-casio.com/Fr/forums/topic17064-1-fxtran-codez-en-fortran-pour-votre-casio.html)。其他的 GCC 支持的前端语言也可以，比如 [the D compiler builds](https://www.planet-casio.com/Fr/forums/topic17037-last-omegafail-dlang-et-gint.html)。对于那些基于 LLVM 的语言（比如 Rust）不在考虑范围内，因为 LLVM 没有 SuperH 后端。

**构建系统**

fxSDK 主要使用 CMake 来构建程序和库。有个老的 Makefile 模板仍然可以用，而且命令行工具仍有官方支持，但是在更新的时候就得手动修改您自己的构建系统了。

## 如何安装

这里说明的安装方法不仅仅适用这个仓库，还包括 fxSDK 里的所有东西。

**方法一：使用 GiteaPC（新手推荐，虽然现在不叫这个了）**

[GiteaPC](https://gitea.planet-casio.com/Lephenixnoir/GiteaPC) 是一个类似包管理器一样的东西，专门用来下载、构建及安装来自 Planète Casio 仓库的东西。整个过程是自动的，所以推荐在安装 fxSDK 的时候使用。

**方法二：在 Arch Linux 发行版里使用 AUR**

[Dark Storm](https://www.planet-casio.com/Fr/compte/voir_profil.php?membre=Dark%20Storm) 为 Arch Linux 维护了一个叫 [MiddleArch](https://www.planet-casio.com/Fr/forums/topic16790-1-middlearch-un-depot-communautaire.html) 的仓库，包含了最新版本的 fxSDK。

**方法三：手动构建（如果您是专家）**

您可以照着 README 文件里的说明来手动构建各个工具。请先看看 GiteaPC 的教程，好知道需要以什么顺序安装哪些东西。在此提醒，要装的东西真的很多（SDK、跨平台编译器、libm、libc、libstdc++、内核、用户库等等），安装和更新都会很花时间。

## 命令行工具

当你用 fxSDK 开发程序的时候，你主要会使用命令行工具和 fxSDK 的构建系统。先来看看命令行工具吧。当你不传任何的参数直接调用工具的时候会显示帮助。例如 `fxsdk` 或 `fxgxa`。

*曾经在这里有一个叫 `fxos` 的工具，现在被移到[它自己的仓库](/Lephenixnoir/fxos)里了。*

使用 `fxsdk` 来**管理项目**

你可以用 `fxsdk new` 来创建一个新项目，并指定名字。本例为“MyAddin”：

```sh
fxsdk new MyAddin
```

然后进入这个文件夹：

```bash
cd MyAddin
```

进入新创建的文件夹之后，就可以用 `fxsdk build-*` 来构建程序：

```bash
# 构建 fx-9860G 系列计算器的程序（后缀 .g1a）：
fxsdk build-fx
# 构建 fx-CG 系列计算器的程序（后缀 .g3a）：
fxsdk build-cg
```

然后你就可以把这个程序用你喜欢的方式发到计算器上了。但是这里有些更简单的方法：

* `fxsdk send-fx` 会使用 [p7](/cake/p7utils) 来发送 g1a 程序，就像 FA-124 和 xfer9860 那样。除了 fx-975G0G III 和 fx-9860G III，所有 fx-9860G 系列的型号都可以用。
* `fxsdk send-cg` 会使用 UDisks2 来发送 g3a 程序，其实就相当与您在文件资源管理器里面操作一样。下面具体有写 fxlink 做了什么。

`fxsdk path` 会告诉你 SDK 里面的一些重要文件（主要是跨平台编译器）的位置。

使用 `fxgxa` 来**生成 G1A 和 G3A 文件**

`fxgxa` 是一个多功能的 g1a 与 g3a 文件编辑器，可以创建、编辑与导出程序的头部。实际上，在运行 `fxsdk build-*` 这个命令的时候，您也间接调用了 `fxgxa`。所以这个只有在您想要查看已经编译好的程序的内容的时候才会用得到。

这可以用来导出 PNG 格式的图标、执行校验和、修复损坏的头部以及导出程序的其他信息。主要是这些命令：

* `fxgxa --g1a|--g3a`：生成 g1a 或 g3a 文件
* `fxgxa -e`：编辑 g1a 或 g3a 文件
* `fxgxa -d`：提取元数据、校验和，以及图标
* `fxgxa -r`：修复破损文件的控制字节与校验和
* `fxgxa -x`：以 PNG 格式提取图标

`fxgxa` 有另一个名字 `fxg1a`。这是为了兼容 2.7 版本以前的 fxSDK，因为在那时还不能生成 g3a 程序。

使用 `fxconv` 来**转换素材格式**

`fxconv` 是一个素材转换器。可以把照片、字体和其他常见的素材变成可以直接在程序中使用的数据结构，包括 gint 图像、gint 字体、[libimg](/Lephenixnoir/libimg) 图像，以及二进制数据。

其他项目可以扩展它的功能来生成自定义的素材，比如地图、对话框、GUI 文本。对 `fxconv` 的扩展需要在对应的项目里面用 Python 来实现。

`fxconv` 稍微和构建系统集成在一起。通常您只需要在 `CMakeLists.txt` 里面生命有哪些素材，把参数填进 `fxconv-metadata.txt`，然后构建系统自己会把剩下的事情做好。

> 未完待续：应该更详细地解释一下怎么使用 fxconv。

使用 `fxlink` 来**进行 USB 通信**

`fxlink` 是一个 USB 通信工具，可以用来往 gint 的 USB 驱动或者计算器里面发送文件。但是这个工具目前还不成熟，不过已经有两个很有用的功能了。

注意：`fxlink` 在 Windows 的 WSL 里面不能用，参见[这个 bug](https://github.com/Microsoft/WSL/issues/2195)。

第一个功能是使用 libusb 与程序进行交流，可以让程序把文字、照片和截图实时发到电脑上。

第二个功能是使用 UDisks2 来往 fx-CG 和 G-III 系列计算器上发文件（像一个 USB 磁盘一样）。`fxlink` 可以把计算器挂在到电脑上，然后传送文件，最后再移除，全程不需要 root 权限。

## 代码提示

[如何在 Visual Studio Code 中获得代码提示](VSCode_zh.md)

## 构建系统

**使用 CMake**

自从 fxSDK 2.3，官方的构建系统就是 CMake 了。当创建一个新的项目的时候，会生成一个 `CMakeLists.txt`。这个文件和正常的 CMake 有点偏差，在[这篇帖子（法语帖子）](https://www.planet-casio.com/Fr/forums/topic16647-1-tutoriel-compiler-des-add-ins-avec-cmake-fxsdk.html)里解释了。下面也一并解释了它们有什么区别。

因为我们需要使用跨平台编译器，所以 `cmake` 没办法弄明白我们要干什么，所以需要传一些额外的参数。`fxsdk build-*` 这个命令会帮你传好正确的参数的。

fxSDK 提供了[许多模块](fxsdk/cmake)：

* [`FX9860G.cmake`](fxsdk/cmake/FX9860G.cmake) 和 [`FXCG50.cmake`](fxsdk/cmake/FXCG50.cmake) 会在运行 `fxsdk build-fx` 和 `fxsdk build-cg` 的时候加载。不直接调用 `cmake` 是有原因的，就是我们会定义一些形似 `FXSDK_*` 的变量，它们指定了有关编译目标的一些信息，且都存在上面的两个文件里面。
* [`Fxconv.cmake`](fxsdk/cmake/Fxconv.cmake) 提供了一些使用 fxconv 的一些函数。`fxconv_declare_assets(... WITH_METADATA)` 会把一些文件标记为需要转换的素材。然后 `fxconv_declare_converters(...)` 定义了这个项目需要用哪些自定义的 Python 模块来转换自定义素材。
* [`GenerateG1A.cmake`](fxsdk/cmake/GenerateG1A.cmake) 和 [`GenerateG3A.cmake`](fxsdk/cmake/GenerateG3A.cmake) 帮您调用 `fxgxa` 来生成 g1a/g3a 程序。默认的 `CMakeLists.txt` 说明了它们如何使用.
* [`FindSimpleLibrary.cmake`](fxsdk/cmake/FindSimpleLibrary.cmake) 和 [`GitVersionNumber.cmake`](fxsdk/cmake/GitVersionNumber.cmake) 是一些小工具与库。看看 [Lephenixnoir/Template-gint-library](https://gitea.planet-casio.com/Lephenixnoir/Template-gint-library)，里面讲了如何用 fxSDK 开发一个库。

**只使用 Makefile**

原来的 Makefile 仍然可以用。可以用 `fxsdk new` 的 `--makefile` 选项来创建一个基于 Makefile 的项目。但是由于很少人用，所以可能会过时，而且在更新的时候需要您自己来维护。只有在您对 make 很熟悉的情况下才建议使用。

## 手动构建

下面是手动安装 fxSDK 的步骤。一开始，您应该先安装本仓库。如果您之前已经安装过 fxSDK 或者跨平台编译器之类的，为了避免冲突，您应该先把它们删掉之后再来安装。

这个仓库的依赖:

* CMake
* libpng ≥ 1.6
* Python ≥ 3.7（3.6 有可能也可以）
* Python 3 的 Pillow 库
* libusb 1.0
* UDisks2 库（在构建工具链的时候如果没用就不用）

当您在运行 `configure.sh` 的时候，您需要确保您有权限写入安装路径，个人建议使用 `$HOME/.local`。
需要注意，跨平台编译器**必须**被安装在 `fxsdk path sysroot` 这个命令说的路径下面。

* 通过 `-DCMAKE_INSTALL_PREFIX=...` 来改变安装文件夹；
* 当您在使用 WSL 子系统的时候，或者您没有 UDisks2 的时候，通过 `-DFXLINK_DISABLE_UDISKS2=1` 来禁用 `fxlink` 对于 UDisks2 的支持。

```bash
% cmake -B build [OPTIONS...]
% make -C build
% make -C build install
```

然后您就可以去安装跨平台编译器了。如果您不清楚安装的顺序，看看 [GiteaPC README](https://gitea.planet-casio.com/Lephenixnoir/GiteaPC) 或者 各个仓库里的 `giteapc.make`，里面写明了依赖项。
