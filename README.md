# takina(A CLI options utility)
## Introduction
`takina`是一个方便CLI应用设置选项(options)而编写的函数库。<br>

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

效果可以看最下面的help信息的输出。


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
* 用户自定义参数类型(user-defined)

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
takina::AddOption({"short-option", "long-option", "description", "PARAMETER_NAME"}, &param);
```
除此之外，也支持给参数指定名称来表示参数的意义或语义（比如单参/多参）

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

#### 用户自定义参数
`takina`通过用户注册的回调解析并设置选项的参数。至于`参数个数`，`合理性`等均由用户控制，不由库本身负责。

回调签名(signature of callback)：
```cpp
/* \param arg argument of the current option
 * \return 
 *  true -- success
 */
typedef std::function<bool(char const *arg)> OptionFunction;
```
如果传递的实参按用户回调的逻辑不合理，可以返回`false`表示解析错误。
为了方便检测选项的实参合理性， 允许由用户指定接受的实参个数（`AddOption()`的第三参数），默认为1，最大为`MAX_OPTION_ARGS_NUM`，表示无论多少实参都接受。

### 位置无关的非选项实参（position-independent non-option arguments）
`非选项实参`是指不应视作选项的实参，而`位置无关`是指无论它出现在哪都应该被视作非选项实参。e.g.
```shell
ssh -p 6000 xxxx@xxxx
# 与下面的命令等价
ssh xxxx@xxxx -p 6000
```
这里`-p`的可接受实参个数为1个，因此多出来的被视为非选项实参。

相关函数：
```cpp
// NOTICE: position省略了
takina::EnableIndependentNonOptionArgument(bool);
takina::GetNonOptionArguments()
```

### Parse
最后，调用`takina::Parse()`解析命令行参数，返回值表示解析是否成功，如果有错误，那么错误信息会写入第三参数中，比如单参选项实参个数超过1等。
```cpp
std::string err_msg;
const bool success = takina::Parse(argc, argv, &err_msg);
if (success) {
  takina::Teardown(); // free all the resources used for parse the cmd args
  // do something
} else {
  // do something
  // e.g. print the err_msg
}
```
如果解析成功，可以调用`takina::Teardown()`释放用于解析命令行参数的资源，因为不再需要了。

## Example/Test
在项目根目录中有一个测试文件，可以编译并运行。
```shell
g++ -o takina_test takina_test.cc takina.cc
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
     --version                     Display the version information

Parameter set test:
-s , --string STR                  Set string argument
-i , --int INT                     Set int argument
-f , --float FLOAT_NUMBER          Set floating-point number

Multiple Parameters set test:
-ms, --multi_string STRINGS        Set multiple-string arguments
-mi, --multi_int INTS              Set multiple-int arguments
-mf, --multi_float FLOAT_NUMBERS   Set multiple-floating-point-number arguments

Fixed parameters set test:
-fs, --fixed_string                Set Fixed-string arguments
-fi, --fixed_int                   Set Fixed-int arguments
-ff, --fixed_float                 Set Fixed-floating-point-number arguments
     --help                        Display the help message
```
