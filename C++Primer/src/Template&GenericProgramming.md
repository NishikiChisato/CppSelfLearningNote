# Template & Generic Programming

- [Template \& Generic Programming](#template--generic-programming)
  - [Definint template](#definint-template)


## Definint template

模板的定义以关键字 `template` 开始，后面的是模板参数列表 `template parameter list`，该列表用尖括号括起来，并用逗号分隔一个或多个模板参数 `template parameter`

类似于函数的形参，形参列表仅仅指明若干类型的**未初始化**的局部变量，形参的值需要由实际传入的实参决定。在这里 `template parameter` 表示类模板或函数模板定义中用到的**类型或者值**，在实际使用模板时我们会显式或隐式地指定模板实参 `template arguement`，将其绑定到模板参数上

对于函数模板，编译器通常用函数实参来推断 `template arguement`；而对于类模板，编译器无法进行推断，需要显示地指定其 `template arguement`

编译器用推断出的 `template parameter` 来实例化 `instantiate` 一个模板实例 `instantiation`

模板参数 `template parameter` 分为类型参数 `type parameter` 和非类型参数 `nontype parameter`。前者是在模板参数列表中无法确定的类型或值，后者是在模板参数列表中可以确定的参数或值

> 需要说明的是，模板参数 `template parameter` 既可以表示类型 `type`，也可以表示值 `value`

<details><summary>以下是一个具体的例子：</summary>

```cpp
//type parameter
template<typename T>
int compare(const T& v1, const T& v2)
{
    return v1 < v2;
}

//nontype parameter
template<unsigned N, unsigned M>
int compare(const char (&v1)[N], const char (&v2)[M])
{
    cout << N << " " << M <<endl;
    return strcmp(v1, v2);
}

int main(int argc, char* argv[], char* envp[])
{
    //implicit
    cout << compare(10, 20) << endl;
    //explicit
    cout << compare<9, 6>("Kamiyama", "Shiki") << endl;
    //following are both implicit
    cout << compare<>("Kamiyama", "Shiki") << endl;
    cout << compare("Kamiyama", "Shiki") << endl;

    string s1 = "Nishiki", s2 = "Chisato";
    //implicit
    cout << compare(s1, s2) << endl;
    cout << compare<>(s1, s2) << endl;
    //explicit
    cout << compare<string>(s1, s2) << endl;
    return 0;
}
```

输出如下：

```bash
1
9 6
-8
9 6
-8
9 6
-8
0
0
0
```

在这里，编译器自动推导出非类型模板参数 `N` 和 `M` 的值，编译器也可以推断出类型模板参数 `T` 的类型

需要说明的是，对于第一个函数，只会返回 `0` 或 `1`，而第二个则可以返回任意整数

</details>

如果如下加入 `inline` 或者 `constexpr`，那么只能放在**模板参数列表之后，返回类型之前**

```cpp
template <typename T> inline T min(const T&, cont T&)
```

