# takina(A CLI options utility)
## Introduction
`takina`是一个方便CLI应用设置选项(options)而编写的函数库。<br>
该库是header-only的，因此只需要包含头文件即可使用，不需要连接库文件，也不需要build。<br>

## Usage
###  添加CLI的使用方式
```cpp
// For example
takina::AddUsage("./process-name [options]");
```

### 添加CLI的描述
```cpp
takina::AddDescription("...");
```
上述两个函数没必要按照顺序写，即`AddDescription()`->`AddUsage()`编写生成的help是一样的。

### 添加命令行选项的节(section)

我将分割命令行的标题称为`节`：
```
Section title:
-xxx, --xxxx  xxxx
// ...

Section title2:
// ...
```

```cpp
takina::AddSection("Section title1");
// set options
takina::AddSection("Section title2");
```
注意`AddSection()`是调用位置相关的(call-position-dependent)，因此只有之后设置的选项属于该节。


### 添加命令行选项

`takina`支持`长选项(long option)`和`短选项(short option)`，前者是**必须的(required)**，而后者是*可选(optional)*的。
表示无短选项用空字符串即可。
```cpp
takina::AddOption({"", "long-option", ...}, ...);
```
同时，选项参数支持以下格式：
* 无参(unary)
* 单参(single)
* 固定个数参数(fixed number)
* 多参(multiple)

而支持的参数类型有`字符串`，`整型`，`浮点数`。<br>
在`C++`中分别用`std::string`，`int`(我认为`int`够用了)，`double`表示。

其中，`help`是内置的长选项，help没有短选项，因为我认为`-h`留给别的选项更好。

#### 无参

无参仅表示该选项用户是否设置，因此用bool型变量表示。
```cpp
bool has_option;
takina::AddOption({"short-option", "long-option", "description"}, &has_option);
```

#### 单参:
只接受一个实参。
```cpp
std::string param;
// OR int param;
// OR double param;
takina::AddOption({"short-option", "long-option", "description"}, &param);
```

#### 多参
接受不定个数的实参。
如果数目固定，用下面这种参数格式。
```cpp
takina::MultiParams<std::string> str_params;
takina::AddOption({...}, &str_params);
```

#### 固定个数的参数
接受固定个数的实参。
```cpp
int params[2];
takina::AddOption({...}, params, 2);
```

### Parse
最后，调用`takina::Parse()`解析命令行参数，返回值表示解析是否成功，如果有错误，那么错误信息会写入第三参数中，比如单参选项实参个数超过1等。
```cpp
std::string err_msg;
const bool success = takina::Parse(argc, argv, &err_msg);
if (success) {
  // do something
} else {
  // do something
  // e.g. print the err_msg
}
```

## Example/Test
在项目根目录中有一个测试文件，可以编译并运行。
```shell
g++ -o takina_test takina_test.cc -I...
```

## Run
```shell
./takina_test --help
```

`help`信息是由该库生成的，其中短选项和长选项的输出行都进行了左对齐，因此较为整齐，结果如下：

```
Usage: ./takina_test [options]

The program is just a test

Options: 

Information:
     --version       Display the version information

Parameter set test:
-s , --string        Set string argument
-i , --int           Set int argument
-f , --float         Set floating-point number

Multiple Parameters set test:
-ms, --multi_string  Set multiple-string arguments
-mi, --multi_int     Set multiple-int arguments
-mf, --multi_float   Set multiple-floating-point-number arguments

Fixed parameters set test:
-fs, --fixed_string  Set Fixed-string arguments
-fi, --fixed_int     Set Fixed-int arguments
-ff, --fixed_float   Set Fixed-floating-point-number arguments
     --help          Display the help message
```
