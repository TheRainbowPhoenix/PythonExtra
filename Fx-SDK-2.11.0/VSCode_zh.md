# 如何在 Visual Studio Code 中获得代码提示

虽然目前您没有办法在电脑上运行程序，更不用说 debug 了，但是您仍然可以享受全方位的代码提示！这已经比卡西欧在 2007 年推出的那个 IDE 好多了。

## 下载

虽然这个软件很火，但有些人可能真的没听说过，先去 [Visual Studio Code 官网](https://code.visualstudio.com) 下载吧！

## 安装插件

首先安装来自微软的 [C/C++ 插件](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools)。或者直接在“扩展”中搜索 `C/C++` 也可以。

## 选择计算器型号

我们知道，彩屏计算器可以显示彩色（废话），但是单色屏的计算器只能显示黑白，所以您在写程序的时候，能用的颜色取决于您选择的计算器型号。所以在开始之前，得选择一下：

1. 打开命令面板 (CTRL/CMD + Shift + P)
2. 输入 `C/C++: Select a Configuration...`，您不需要打得跟这个完全一样，因为 VSCode 会自动补全。如果是我的话，会输入 `c sel a conf`。
3. 按下回车，然后选择您想要的计算器型号：fx-9860G 或者 fx-CG50。

打开 `src/main.c` 文件，输入以下代码，看看代码提示是否有正常工作。

```c
#include <gint/display.h>
#include <gint/keyboard.h>

int main(void)
{
    // 用白色来清空屏幕
	dclear(C_WHITE);

    // 用黑色在左上角写上 Hello world!
	dtext(1, 1, C_BLACK, "Hello world!");

    // 将缓冲区的内容更新到屏幕上
    dupdate();

    // 在检测到下一次按键之前卡在这里。
    // 因为在卡西欧计算器上，程序结束之后将直接返回菜单，
    // 所以只有卡在这里不让它结束，才能看到程序运行结果。
	getkey();

    return 1;
}
```

## 对不同的型号分别考虑

如果您希望在不同的型号中做一些不同的事，需要使用以下两个宏来判断。

* `FX9860G` 黑白屏
* `FXCG50` 彩屏

譬如我想要在不同的型号上显示不同的文字：

```c
#include <gint/display.h>
#include <gint/keyboard.h>

int main(void)
{
	dclear(C_WHITE);

	dtext(1, 1, C_BLACK,
        // 如果是黑白屏
		#ifdef FX9860G
			"9860G: Hello world!"
		#endif
        // 如果是彩屏
		#ifdef FXCG50
			"CG50: Hello world!"
		#endif
	);

	dupdate();

	getkey();

    return 1;
}
```

这样，在黑白屏上就会显示 `9860G: Hello world!`，而在彩屏上就会显示 `CG50: Hello world!`。您也许还发现了，这两个字符串有一个的颜色比其他的淡一点，这代表了这个字符串不会在您刚才选择的计算器型号上显示。刚刚讲到了您可以在两种型号之间切换，所以在切换的时候您也应该可以看到两个字符串的颜色深浅发生了变化。

同时，在彩屏和黑白屏上能使用的颜色也是不一样的。感兴趣的话，去看看 `gint/display.h` 里面的定义吧！

## 救命！我看不到代码提示！

先检查一下在 `~/.local/bin` 这个文件夹下有没有 `sh-elf-gcc` 这个跨平台编译器。如果没有，您需要重新安装 fxSDK。

## 结语

好啦！您已经学会了如何用这个牛逼轰轰的内核来开发计算器程序了，期待您的作品！
