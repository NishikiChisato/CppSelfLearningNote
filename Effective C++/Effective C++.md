# 1.尽量以 const, enum, inline 替换 #define



当我写出

```cpp
#define SIZE = 1.638;
```

记号 SIZE会在预编译阶段就被预处理器给替换掉，**这使得SIZE这个名称不会被编译器看到，进而导致SIZE不会进入记号表(symbol table)**。当我用此常量而获得一个错误的编译信息时，这个错误信息会提到 1.638 而不会提到 SIZE 。如果我将 SIZE 写在一个头文件内，这便会导致追踪该变量会浪费时间

因此，我们可以使用以下代码来进行替换

```cpp
const double Size = 1.638;
```

作为一个常量，它一定会被编译器看见进而进入记号表内



在用常量替换 #define 时，我们需要注意两种情况。

* 第一个是定义常量指针。

当我们定义常量指针并将其放入头文件内时，我们需要将该指针声明为 const 。一般来说，这种常量指针会指向一个字符常量，即

```cpp
const char* const authorName = "Mellina";
```

这里我们需要使用两个 const ，当然我们也可以用 string 来代替，即

```cpp
const string authorName = "Mellina";
```

* 第二个是 class 内部的常量

当我需要将常量的作用域限定于 class 内时，我需要对成员变量使用 const ，与此同时，既然是常量，那么它就**只能有一个实体**，因此我还需要用 static 来进行修饰。

对于非 const 修饰的 static 成员变量而言，**必须在 class 内部声明， 在 class 外部赋值**，即声明式需要写在 class 内，定义式需要写在 class 外。

但是，当该成员变量被 const 修饰时，我们可以**直接在 class 内部对其赋值**，即

```cpp
class Test
{
public:
	const static int size = 10;
	int arr[size];
};
```

对于 class 专属常量又被 static 修饰且为整数类型（int, char, bool），只要不取出它们的地址，那么我便可以声明便使用它而无需提供定义式。但如果我取出它的地址或者编译器要求看到其定义式，那么我们需要另外提供其定义式：

```cpp
const int Test::size;
```

将该式子放入一个**实现文件**当中即可（这里不需要赋值，因为在声明的时候已经有初值了）

当然，如果编译器不支持上述语法（这种语法也只对整型常量进行），我们按照 static 修饰成员变量的方式赋值即可

对于 #define 言，它并不支持任何封装，即它不能用于定义 class 专属常量



如果编译器不允许上述做法，我们还可以通过 enum 来进行实现，即

```cpp
class Test
{
public:
	enum
    {
        size = 5    
    }
	int arr[size];
};
```

使用 enum ，将 size 变成一个表示 5 的记号。值得注意的是，我们**取一个 enum 类型的变量的地址是不合法的，同样地，取一个 #define 地地址也是不合法的**



#define 的另一种用法是定义宏，其优点是可以避免进行函数调用进而实现更快速的运算。当然，宏会出现一些意料之外的问题（哪怕我加了括号也一样），这个时候我们可以**使用 inline 来对宏进行替换**，它们的本质都是对代码块的替换

```cpp
#define FUN(X, Y) f((X) > (Y) ? (X) : (Y));//f为一个函数
```

该代码有如下问题

```cpp
int a = 5, b = 0;
FUN(++a, b);     //a 会自增两次
FUN(++a, b + 10);//a 只会自增一次
```

自然，我们可以用 inline 加上 template 来进行替换

```cpp
templete <typename T>
inline void FUN(const T& x, const T& y)
{
    f(x > y ? x : y);
}
```



# 2.尽可以使用 const



如果 const 出现在星号的左边， 表示该指针所指向的物是常量，即**不能通过该指针对这个值进行修改，但指针的指向可以被修改**；如果出现在星号右边，说明该指针是常量，即**该指针的指向不允许被修改，但该指针指向的值可以被指针修改**



在STL中，iterator本质上是个 T* 的指针，当我用 const 对其进行修饰时，说明该指针是一个常量，不能随便修改，即对应于 T * const。如果我希望STL给我模拟一个指向常量的指针，即对应于 T const *，那么我应该使用 const_iterator

```cpp
vector<int>vec;
const vector<int>::iterator it = vec.begin();
*it = 10;//Allow
//it++;//Error

vector<int>::const_iterator cIt = vec.begin();
//cIt = 10;//Error
cIt++;//Allow
```



令函数返回一个常量值（返回值被 const 修饰），这样做可以避免相关错误而造成的意外。

我们考虑一个自定义的数据类型 Real ，当我们对 * 进行重载运算符时，我们可以写成

```cpp
const Real operator* (const Real& e1, const Real& e2);
```

参数部分的 const 我们不做讨论，我们将重点放在返回值上。在返回值那里加上 const 是为了避免因手误写成

```cpp
(a * b) = c;
```

而导致程序出错（说不定真的会有天才这么写吧）。对于函数的形参，**除非是该函数会修改这个值，否则都应该用 const 修饰**



## const 成员函数

对于 class 的成员函数，我们可以通过其常量性对其进行重载，即同一个重载函数对应于 const 对象版本和 non-const 对象版本

当然，我们需要知道：

* 常对象（被 const 修饰的对象）只能调用常函数（class 中被 const 修饰的函数）
* 常函数不能修改 class 当中任何 non-const 的成员，当然 const 成员可以被其修改

基于这两点，我们看下面的代码

```cpp
class Test
{
public:
	char& operator[](std::size_t pos)
	{
		return str[pos];
	}
	const char& operator[](std::size_t pos) const
	{
		return str[pos];//我们不允许常对象修改 class 内部的成员变量，因此在返回值那里要加上 const
	}
	Test(const std::string& str)
	{
		this->str = str;
	}
private:
	std::string str;
};
```



对于 non-const 对象而言，我们调用 [] 时用的是 non-const 成员函数，这也是通常情况；对于 const 对象而言，我们调用 [] 时用的是 const 成员函数，这里便可以于上面结合起来了。

在我们写函数的时候，如果形参不会对输入值进行改变，那么我们要加上 const 。而形参可以是 Test 类型的数据，如果在形参部分加上了 const ，那么就说明该形参是一个常对象，它不能调用 non-const 成员函数。

因此，**当成员函数不会改变 class 中的任何一个成员（包括 const 和 non-const）时，我们写重载函数时需要写两个**



我们刚刚所讨论的全部都是基于 const 对象与 const 成员函数不能修改 class 内部的成员变量这一点的，如果我 class 的内部当中的部分数据被 const 对象与 const 成员函数所修改是允许的，即我希望 const 成员函数能够修改部分 class 当中的成员变量。

为了应对这一需求，**我们可以在变量前面加上 mutable 来允许该变量可以被 const 所修饰的成员函数来修改**

我们看如下代码

```cpp
class CTest
{
public:
	char& operator[](std::size_t pos)
	{
		return str[pos];
	}
	const char& operator[](std::size_t pos) const
	{
		return str[pos];
	}
	std::size_t length() const
	{
		if (!lengthIsValid)
		{
			Length = std::strlen(str);
			lengthIsValid = true;
		}
		return Length;
	}
private:
	char* str;
	mutable std::size_t Length;//最近一次计算的长度
	mutable bool lengthIsValid;//目前长度是否有效
};
```



对于 length 函数，它并不会改变 str 的值，仅仅只是想获取其长度。但于此同时我们需要对该长度是否有效进行检验，这就必定会导致需要修改其成员变量，而 mutable 则很好地解决了这个问题。

事实上，这个地方我们还有一个很严重的问题，就是我们的重载函数写了两个版本，而这两个版本的函数体是完全一样的！！！作为一个优雅的程序员，你怎么可能允许这种事情发生，当然关于这个问题，我们也有其解决办法。嘛，今天就先到这里，明天我们再看。



首先，我们回顾一下C风格地转型动作

(T) expression     //将 expression 转型为 T

函数类型是这样的：

T(expression)     //将 expression 转型为T

C++提供四种新式类型转换

* const_cast<T>( expression )
* dynamic_cast<T>( expression )
* reinterpret_cast<T>( expression )
* static_cast<T>( expression )

它们的作用为

1. const_cast 通常用于将对象的常量性移除，即将 const 类型的对象转换为对应 non-const 类型的对象
2. dynamic_cast 主要用于执行“安全向下转型“(safe downcasting) ，也就是用来决定某对象是否归属继承体系中的某个类型
3. reinterpret_cast 意图执行低级转型，实际动作（及结果）可能取决于编译器，因此它是不可移植的。例如将一个 pointer to int 转型为 int。
4. static_cast 用来强迫隐式转换，例如将 nont-const 对象转换为 const 对象，或将 int 转为 double 等待



知道了这些之后，我们再来看那个代码复用的问题。我们有两个一样的函数，如果我能在一个函数中调用另一个函数，那么我就可以完美解决代码复用这个问题，那么现在问题的关键就变成我该在哪个函数中调用另一个函数。

如果我们考虑在调用 const 成员函数时，让传入的参数发生类型转换进而得以调用另一个函数，那么这就会导致违背 const 成员函数自身的承诺——不会改变对象的逻辑状态。

说人话就是 const 成员函数不允许传入的对象有任何的更改，但我想通过 const 函数调用 non-const 函数就必定需要发生转换，这里就产生了矛盾，因此我们采用这种——当我调用 non-const 函数时，对其发生类型转换进而调用 const 函数。实际代码为：

```cpp
char& operator[](std::size_t pos)
{
    return const_cast<char&>(static_cast<const CTest&>(*this)[pos]);
}
const char& operator[](std::size_t pos) const
{
    return str[pos];
}
```

这里类型转换发生了两次。**第一次是将该对象从 non-const 转换为 const** ，这一步实施完成后才能调用 const 成员函数。然后 const 成员函数返回的是 const char& ，我们需要将该类型转换成 char& ，**这里便是第二次转换**

有一点需要值得注意的是第一次转换的类型是 const CTest&。这里我的理解是：this 指针本身指向调用该成员函数的对象， *this 则是该对象的值，这个值的类型是 CTest& （它的类型应该是 CTest，但这里在C语言里面，C++引入了左值引用和右值引用的概念后，这里应当为 CTest&），我们需要在它的基础是加上 const ，于是便有了 const CTest&。如果不加这个引用那么它将不会更改 this 所指向的对象的值



# 3.确认对象被使用前已先被初始化



对于无任何成员的内置数据类型，  **我们需要在其使用前就手动将其初始化**。对于自定义数据类型，其初始化则需要靠构造函数，即**确保每一个构造函数都能将对象的每一个成员初始化**

考虑下面这个例子：

```cpp
class Test
{
public:
	Test(const string& e1, const vector<int>& e2, const int& e3)
	{
		str = e1;
		vec = e2;
		num = e3;
	}
private:
	string str;
	vector<int> vec;
	int num;
};
```

C++规定，**对象的成员变量的初始化动作发生在进入构造函数之前**。也就是说在 Test 构造函数内，str 、 vec 、 num 都不是被初始化而是被赋值，它们的初始化**发生在调用构造函数之前**，即这些成员变量的 *default* 构造函数发生时机**早于**该对象的构造函数发生时机。但这一点对于内置数据类型（这里的 int）并不有效，即**对于内置数据类型就是在对象的构造函数内初始化的**

我们考察上面这一过程，发现调用该对象的构造函数时发生了：调用该成员变量的 *default* 构造函数和对该成员变量进行赋值。那么 *default* 构造函数所做的工作就被浪费了，想要避免这种情况发生，我们可以使用初始化列表

将上述构造函数改为

```cpp
Test(const string& e1, const vector<int>& e2, const int& e3) :
	str(e1),
	vec(e2),
	num(e3)
{ }
```

采用初始化列表之后，成员变量便会调用 *copy* 构造函数来进行初始化。

当然，在该对象的 *default* 构造函数内，我们需要 *default* 构造一个成员变量，我们可以这么写：

```cpp
Test() :
	str(),
	vec(),
	num(0)
{ }
```

总结一下就是**最后使用初始化列表，并且要列出所有的成员变量**

当然，如果变量很多，或者存在多重继承的情况下，可以把那些”初始化和赋值看起来是一样“的变量封装起来（通常是private权限），供构造函数调用。这种做法适用于该成员变量需要**从文件或者数据库中读入的情况**



## 初始化次序

对于一个 class 当中的成员变量而言，它们的初始化是有顺序的，它们必定是按照**从上到下**的顺序来进行初始化，尽管在初始化列表那里可能写的不一样（但这并没有关系）

这样的好处在于，如果存在一个数组，其大小需要用到该 class 中其他的成员变量，我们只需要将该成员变量**先于**该数组初始化即可，并且这里的**先于**是可控的（毕竟只需要放在数组定义的前面就行）

在此，我们来考虑一个更深层次的问题——如果我写了多个源文件，这些源文件当中的变量需要相互初始化，我该如何确定它们的初始化次序？说的专业一点就是：不同编译单元内定义的 non-local static 对象的初始化次序

我们先看 static 对象。所谓 static 对象，其生命周期是从被构造那一刻起一直到程序结束为止。很显然，这包括 global 对象、定义在 namespace 作用域下的对象、在 class 内、在函数内、以及在 file 作用域内被 static 修饰的对象。程序结束时 static 对象会被自动销毁，在 main 函数的结尾会自动调用它们的析构代码

这里有一个分类是：**如果是在函数内定义的 static 对象，称为 local static 对象**（相对于函数而言是 local 的），**其余的都是 non-local static 对象**

所谓编译单元是指产出单一目标文件（ obj 文件，一个源文件会对应一个 obj 文件）的那些源码。

明白了这些概念之后上面那个问题可以更加清晰地表述为：如果某个编译单元内的 non-local static 对象使用了另外一个编译单元内的 non-local static 对象，但它所使用的这个对象可能并没有在它前面完成初始化。这是因为C++对”定义于不同编译单元内的 non-local static 对象“的初始化次序并无规定。（C++居然对这个没有规定，我直接 虎躯一震.jpg 加 黑人问号.jpg ）

我们用具体实现来分析这个问题

目前我们有一个 class ，并且用 extern 声明了一个变量供其他源文件使用

```cpp
class Fun
{
public:
	size_t Cal() const;//其中一个成员函数
};

extern Fun tfs;
```

其他源文件使用了外部变量 tfs 。你也可以用这个 class 来创建对象

```cpp
class Dir
{
public:
	Dir()
	{
		size_t tem = tfs.Cal();//使用 tfs 来创建对象
	}
};

Dir temp;
```

这个时候就会出现上面所说的问题。那么问题是我们要怎么解决呢？

我们将每个 non-local static 对象都搬到自己的专属函数里面去，在此函数内将该变量声明为 static ，并且该函数返回该变量的 reference。也就是说，我们用一个函数来对 non-local static 对象进行封装，将其变为 local static 对象

具体实现为：

```cpp
class Fun
{
public:
	size_t Cal() const;//其中一个成员函数
};
//核心在这里
inline Fun& tfs()
{
	static Fun tmp;
	return tmp;
}

class Dir
{
public:
	Dir()
	{
		size_t tem = tfs().Cal();//这里发生了变化
	}
};

inline Dir& temp()//直接使用 temp 函数来创建对象，当然这是在其他源文件需要使用 temp 对象的情况下才需要这么做
{
	static Dir tmp;
	return tmp;
}
```

**这种封装在频繁调用的情况下可以写成 inline 的形式**



# 4.了解C++默默编写并调用了哪些函数



当我写下一个 class 时，编译器会自动帮我创建 *default* 构造函数、*copy* 构造函数、 *copy assignment* 操作符和一个析构函数，这些函数都是 public 和 inline 的。当然，这些函数**只有在被调用时才会创建**

```cpp
class Empty
{
    Empty() {}
    Empty(const Empty& e1) {}
    ~Empty() {}
    Empty& operator=(const Empty& e1) {}
}
```

在这里我们需要注意的是赋值操作符，这个函数有的时候编译器并不会帮你创建

编译器自动创建的 *copy assignment* 操作符是逐字节赋值（这当然会引发深拷贝的问题，但我们暂且不提这一点），也就是说它会将新对象的成员变量全部**赋值**为原对象的成员变量（这里是赋值，不是初始化）。

那么这里的问题就是如果成员变量当中存在 referencd 或者 const 类型，该对象已经完成初始化了（也就是前面的二者已经有值了），这个时候如果调用编译器创建的 *copy assignment* 操作符，就会导致新的对象的 reference 或者 const 成员的值发生改变，这当然是不允许的，因此编译器会禁止这种行为。这就需要我们手动写 *copy assignment* 操作符了。



# 5.若不想使用编译器自动生成的函数，就该明确拒绝



假设有一个类，我们不希望它被拷贝，即我们希望 `p2 = p1` 与 `p2(p1)` 这两种操作会被编译器禁止。一个最容易想到的方法就是不写 *copy* 构造函数与 *copy assignment* 操作符。

但问题是就算我们不写，编译器也会在需要的时候自动帮我们创建。当然，要解决这个问题，我们也有办法：

```cpp
class Test
{
public:
	//...
private:
	Test(const Test&);
	Test& operator=(const Test&);
};
```



我们可以将它的 *copy* 构造函数和 *copy assignment* 操作符**声明**为 *private* 权限，我们仅仅**只是声明，不予实现**。这样便完成了不让这个类进行拷贝的操作。但这里也有一个问题是： *member* 函数与 *firend* 函数依旧可以访问这两个 *copy* 函数，为了解决这个问题，我们可以采取继承的方式（关于继承方式的区别，其实我自己目前也不明白，往后看一看说不定就懂了）

```cpp
class Uncopyable
{
protected:
	Uncopyable();
	~Uncopyable();
private:
	Uncopyable(const Uncopyable&);//将这两个函数声明为 private
	Uncopyable& operator= (const Uncopyable&);
};

class Test: private Uncopyable//用 private 继承，这样可以使得派生类只能调用构造和析构函数
{						      //不知道用 public 继承行不行
public:
	//...

};
```



当 *derived class* 试图调用 *copy* 构造函数时，由于 *derived class* 并没有该函数，因此编译器会自动创建。由于 *derived class* 采取 private 继承自 *base class* ，因此编译器自己创建的 *copy* 构造函数中会**试图**调用 *base class* 的 *copy* 构造函数。

由于 *base class* 的 *copy* 构造函数**只有声明，没有定义**，因此编译器不会**自动创建 *base class* 的 *copy* 构造函数**。由于该 *copy* 构造函数并无实现，因此编译器会报错。



# 6.为多态基类声明 virtual 析构函数



我们来看一个例子：有许多方法可以记录时间，我们来设计一个 TimeKeeper base class 和一些 derived class 来表示不同的计时方法

```cpp
class TimeKeeper
{
public:
	TimeKeeper();
	~TimeKeeper();
};

class AtomicClock : public TimeKeeper {  };
class WtaerClock :public TimeKeeper {  };
```



然后，我们定义一个**基类指针**（当然要 new 一个），使得其可以指向所有的 *derived class* ，这么做合情合理

```cpp
TimeKeeper* pt = new AtomicClock();//这里只要是 derived class 就可以
// use pt for ...
delete pt;
```

这个地方，pt 指向 *derived class* 对象并且该指针通过被 *base class* 指针删除，并且 *base class* 还有个 non-virtual 析构函数，这会导致非常刺激的事情发生。（嘛，说人话就是 base 指针指向的 derived 对象在 delete 的时候，如果基类存在 non-virtual 析构函数就会出事）

那么具体是什么事情呢？ delete 掉 *base class* 指针的时候只会释放掉 *base class* 当中的成员变量而不会释放 *derived class* 的成员变量，即**它只会执行 *base class* 的析构函数而不会执行 *derived class* 的析构函数**（这就是局部销毁，做事情只做一半，给你玩明白了属于是）

要解决这个问题也十分简单，只需要在 *base class* 的析构函数前面加上 *virtual* 就行，即

```cpp
class TimeKeeper
{
public:
	TimeKeeper();
	virtual ~TimeKeeper();
};
```

这样做之后，在 delete 掉指针的时候就会析构掉 *base class* 和 *derived class* ，当然析构顺序是先 *derived* 后 *base*

实际上**对于 *base class* 而言， virtual 不仅仅可以用在析构函数处，也可以用在其 *member* 函数处**。此时 *base class* 当中的这个函数可以是空实现，而其不同的 *derived class* 当中写不同的该函数的实现。

也就是说，**任何 *base class* 只要带有 virtual 函数就必须要有一个 virtual 析构函数**，这是因为**如果一个 class 不含 virtual ，那么也就表面该函数不会被用做一个 *base class* **

STL容器和 string 类当中的析构函数是 non-virtual 的，也就是说，不可以将**STL容器和 string 类作为 *base class* 来使用** 

当然，我们也可以让 *base class* 带有一个 pure virtual 析构函数，这将使其变成 abstract class ——不能用这个 class 来实例化对象。具体操作如下

```cpp
class TimeKeeper
{
public:
	TimeKeeper();
	virtual ~TimeKeeper() = 0;
};
```

当然，对于 pure virtual 析构函数，我们还需要额外为其**在类外**提供一份定义

```cpp
TimeKeeper::~RimeKeeper()
{
    ;//空实现就行
}
```



# 7.别让异常逃离析构函数



假设有一个场景——我有一个含有10个对象的 vector 数组，在我函数结束的时候程序会自动**依次**执行每个对象的析构函数。如果第一个对象在执行析构的时候抛出了一个异常，紧接着后面一个对象同样也抛出了一个异常。而在两个异常同时存在的情况下，会导致程序提早结束或者导致不明确的行为。也就是说，C++不喜欢析构函数吐出异常。

我们来考虑一个例子，假设我使用一个 class 来负责数据库的连接

```cpp
class DBConnecton
{
public:
    static DBConnection creat();//创建一个对象
    
    void close();//关闭连接
}
```



为确保在以后每次的调用当中不忘记调用 close ，我们可以使用一个 class 来管理 DBConnection 资源，并在其析构函数中执行 close 

```cpp
class DBCon
{
public:
    ~DBCon()
    {
        db.close();
    }
private:
    DBConnection db;
}
```

如果在析构函数中成功调用 close ，那么一切都很完美。但如果该调用导致了异常，而 DBCon 的析构函数将此异常抛出，也就是**允许此异常离开析构函数**，那就会造成问题。为避免这个问题，我们可以：

* 如果抛出异常那么就结束程序，可以用 abort 来实现

```cpp
~DBCon()
{
    try{
        db.close;
    }
    catch(...)
    {
        //记录下对 close 的调用失败
        std::abort();
    }
}
```

这种方法是——只要一出现异常，则立刻结束程序

* 吞下 close 引发的异常

```cpp
~DBCon()
{
    try{
        db.close();
    }
    catch(...)
    {
        //对异常进行记录
    }
}
```

这相当于 catch 当中只记录该异常而不做处理，美其名曰：吞下

上面的这些操作，都是对发生异常之后的补救措施，并且都是在析构函数的地方进行补救（也就是说只有才**程序结束的时候**我才知道发生了异常，并不能确定异常出现在哪里）。实际上，我们是需要一种能够对"导致 close 发生异常"的情况进行处理的办法才行

那么，我们可以从新设计这个接口，我们可以提供一个手动关闭的接口，这样子只要用户调用了这个函数就能够知道问题出在哪里，就算用户忘记调用，也有析构函数兜底（只不过会发生上面的情况而言，不过总体而言要好一点），代码如下

```cpp
class DBCon
{
public:
    void close()
    {
        db.close();
        closed = true;
    }
    ~DBCon()
    {
        if(closed == false)
        {
        	try{
                db.close();
            }
            catch(...)
            {
                //强制结束或者吞下异常
            }
        }
    }
private:
    DBConnection db;
    closed;
}
```

也就是说，如果用户需要对某个操作函数在运行期间抛出的异常做出反应，那么 class 应该提供一个普通函数来执行该操作，而非在析构函数中

  

# 8.确认你的 public 继承塑膜出 is-a 关系



在 C++ 当中， public inheritance （公开继承）意味着 **is-a （是一种或是一个）**的关系。

如果令 class D (Derived) 以 public 方式继承 class B (Base) ，那么就意味着：**每一个类型为 class D 的对象都是一个类型为 class B 的对象，而反过来则不成立**。也就是说， **B 的概念比 D 的概念更一般化， D 的概念比 B 更特殊化**，B 能够做到的事情， D 一样可以做到。**每一个 D 对象都是一个 B 对象，反之则不成立**。

由于是 public 继承，即 *derived class* 会拥有 *base class* 的所有属性与方法，而 *derived class* 则可以拥有额外的属性与方法。因此我们说： *base class* 能做到的事情， *derived class* 一定也能做到，***derived class* 的范围大于 *base class* 的范围**。

C++中的 public 继承一定遵循上面的原则，举个例子：

```cpp
class Person {};
class Student: public Person {};
```

根据常识，人能做到的事情，学生一定能做到，而学生能做到的事情，人则不一定能做到。因此，**任何函数期望获得一个类型为 Person 的实参（形参的类型为 Person ），也都愿意接受一个类型为 Student 的对象**。这也就说明了为什么 ***base class* 的指针可以指向 *derived class* 对象而 *derived class* 的指针则不能指向一个 *base class* 的对象**

再次重申，C++坚持—— B 能做到的事情， D 一定可以做到，反之则错误。但有的时候直觉会带来意想不到的错误，我们考虑下面这个例子：企鹅（penguin）是一种鸟，这是事实。鸟可以飞，这也是事实。当我们用C++描述这层关系时，哦吼，好玩的事情来了。

```cpp
class Bird
{
    virtual void fly() = 0;//鸟可以飞
}
class Penguin: public Bird//企鹅是鸟
{
    
}
```

按照上面的说法，我们就会发现，企鹅居然能飞？？？好吧，这实际上是语言不严谨导致的问题。当我们说鸟能飞的时候，并不是说所有的鸟都会飞，而是部分鸟会飞。因此，实际的继承关系应该如下

```cpp
class Bird
{
    //不声明 fly 函数
}
class FlyBird: public Bird
{
    virtual void fly() = 0;//会飞的鸟
}
class Penguin: public Bird
{
    //不声明 fly 函数
}
```

当然，还有一种做法是：承认企鹅会飞，但在运行阶段报错，具体如下

```cpp
void error(const string& s);//定义与别处
class Penguin: public Bird
{
    virtual void fly()
    {
        error("Penguin can't fly!!!");
    }
}
```

二者的差异在哪里？我们从错误被侦测出的时间点来看，前者的错误在编译阶段就会被发现，而后者则需要到运行阶段才行。显然，我们**应当采用前者**。

然后我们再看一个例子，按照我们基础几何学的知识，所有的正方形一定是矩形，而反过来则不是。也就是说，正方形（ Square ）应当以 public 方式来继承矩形（ Rectangle ）。然后，我们考虑下面这段代码

```cpp
class Rectangle
{
    virtual void setwight(int wight);
    virtual void setheight(int height);
    virtual int wight() const;//返回当前值
    virtual int height() const;//用 virtual 使得 derived class 进行重写
};
void Bigger(Rectangle& r)
{
    int oldwight = r.wight();
    r.setwight(oldwight + 10);
}
```

 Bigger 只会改变 Rectangle 的宽度，显然这对于矩形而言是正确的。然后我们让 Square 以 public 方式继承

```cpp
class Square: public Rectangle {};
```

然后，非常好玩的事情发生了。Bigger 函数可以接受一个 Square 对象，也就是说 Square 对象可以只被改变宽度而不被改变高度，但这对于正方形而言显然是不对的。

关于这个问题的解决，可以有很多的办法。在这里我只是想说：有的时候直觉跟 public 继承真不太一样



# 9.绝不在构造和析构过程中调用 virtual 函数



假设我们有一个继承体系用来，塑膜股市交易。而每当我们创建一个交易对象时，都需要进行一次记录

```cpp
class Transaction //base class
{
public:
	Transaction();
	virtual void logTransaction() const = 0;//因不同的类而进行不同的记录，因此要 virtual
};

Transaction::Transaction()
{
    //...
	logTransaction();
}

class BuyTransaction: public Transaction //derived class
{
public:
	virtual void logTransaction() const;
};
```

最直接的想法是：每构造一个 *derived class* 对象都要执行一遍 *base class* 的构造函数，因此我将 logTransaction 放在 *base class* 的构造函数内就行。

现在，当我们用 BuyTransaction 来创建一个对象时，我们来考察发生了什么事情。

首先， BuyTransaction 的构造函数会被调用，而由于它是 *derived class* ，**因此其 *base class* 的构造函数的执行会先于 *derived class* 的构造函数的执行（ *derived class* 当中的 *base class* 成分会在 *derived class* 自身成分被构造前先构造妥当）**。所以会先执行 Transaction 的构造函数。其构造函数的最后一行是一个纯虚函数 logTransaction 。当程序执行到这一行时，好玩的来了。**此时调用的 logTransaction 是 *base class* 当中的版本而不是 *derived class* 当中的版本**。也就是说，***base class* 构造期间的 virtual 函数是不会下降到 *derived class* 的阶层的**。

在这里你肯定会问：为什么 virtual 函数不会下降到 *derived class* 的阶层呢？这里的原因是：由于 *base class* 的构造函数的执行先于 *derived class* 的构造函数的执行，因此此时 *derived class* 的成员变量仍处在为初始化阶段，如果这个时候下降到 *derived class* 的阶层，就会导致“要求使用对象内未初始化的成分”。要知道，这在C++当中可是禁止的。

对于上述解释，其实还有一个更根本的理由：**在 *derived class* 对象的 *base class* 构造期间，该对象的类型是 *base class* 而不是 *derived class* **。要知道，对于C++而言，如果对象为初始化，最安全的做法就是视它们不存在。即**对象在 *derived class* 的构造函数开始执行前不会成为一个 *derived class* 对象**。 

在构造函数期间， ***base class* 的构造函数的执行会先于 *derived class* 的构造函数的执行**，而析构则刚好相反，即 ***derived class* 的析构函数的执行会先于 *base class* 的析构函数的执行**。而上述的法则对于析构函数同样有效。在 *derived class* 的析构函数执行完毕后， *derived class* 的成员变量则会处于未定义的状态，此时C++视它们不存在。**因此在进入 *base class* 析构函数后，该对象会成为一个 *base class* 对象**。 

在上面这个例子中，由于在构造与析构函数中直接调用了 pure virtual 函数，因此一些编译器会发出警报。我们来考虑下面这一种情景：当 Transaction 有多个构造函数，并且它们有很多相同的部分，那么想当然的我会把它们都放在一个 init 函数里面（这里也包括 logTransaction ），即我们会写出如下代码：

```cpp
class Transaction //base class
{
public:
	Transaction()
    {
        init();
    }
	virtual void logTransaction() const = 0;//因不同的类而进行不同的记录，因此要 virtual
private:
    void init()
    {
        //...
        logTransaction();
    }
};
```

 对于这段代码，如果 logTransaction 依旧是 pure virtual 函数，那么编译器仍会报警。但如果这是一个 non-pure virtual 函数，并且它还带有一份具体的实现代码，那么编译器就不会报警。这么做的代价是：你压根不知道为什么在创建 *derived class* 对象时会调用错误版本的 logTransaction 。

该问题的唯一解决办法是：**确保构造函数和析构函数都没有调用 virtual 函数，而它们所调用的所以函数也同样要适用这条约束**。

但目前我们的问题是：只要有 Transaction 继承体系的对象被创建，就要有相应版本的 logTransaction 函数被调用。

通过上述分析，我们可以得出： *derived class* 对象的构造一定会执行 *base class* 的构造函数。再此基础上，如果 *base class* 的构造函数内调用 pure virtual 函数，那么编译器会报错，如果调用 non-pure virtual 函数，那么会导致该对象在构造时调用了错误版本的 logTransaction 函数。

那么这里的做法可以是：在 *base class* 中将 logTransaction 函数改为 non-virtual 函数，在 *base class* 的构造函数中调用该 non-virtual 函数并将需要记录的信息传递过来（这里的代码看51页）。

# 10.避免遮掩继承而来的名称



我们考虑如下这段代码：

```cpp
int x;           //global 变量
void fun()
{
    double x = 0;//local 变量
    cin >> x;
}
```

这里 cin 的对象是 local 变量而不是 global 变量，这是因为**内层作用域的名称会遮掩外层作用域的名称**。

当编译器处于 fun 的作用域下并遇到 x 时，它会先在 local 作用域下寻找 x ，如果找不到再去 global 作用域下去找 x 。找到了直接使用就行。这时你会说：这两个 x 的类型不一样啊。

嘛，C++的名称遮掩规则不跟你搞那些花里胡哨的，它要做的只有一件事：遮掩名称。它不会关注这个名称的类型或者这是一个变量名称还是一个函数名称，这些统统不管。

也就是说，在这里 double x 遮掩了 int x。

现在，我们在此基础上导入继承，来看看会发生什么

```cpp
class Base
{
private:
	int x;
public:
	virtual void mf1() = 0;
	virtual void mf2();
	void mf3();
};

class Derived : public Base
{
public:
	virtual void mf1();
	void mf4();
};
```

这里面的函数有 pure virtual 、 virtual 、 non-virtual 函数，但这些都不重要，C++只看名称。

我们假定 mf4 当中的实现为：

```cpp
void Derived::mf4()
{
    mf2();
}
```

当编译器看到 mf2 时，**它会先在 local 作用域下进行查找其声明**，这里显然没有 mf2 的声明。然后编译器**会去其外围作用域下查找**，这里就到了 Derived 的作用域。依旧没有找到，编译器会**继续向外围拓展**，到 Base 作用域下查找，这时找到 mf2 的声明式，于是便在此停止。如果 Base 里面还是没有，那么编译器**只能去 global 作用域下查找**，如果还是找不到就只能报错。

也就是说，**编译器对于名称声明式的查找是由内向外的**，现在 local ，然后再逐渐向外围拓展。

到此，我们就说清楚了C++对于名称的查找原则了，我们在原来的基础上加上重载

```cpp
class Base
{
private:
	int x;
public:
	virtual void mf1() = 0;
    virtual void mf1(int); //这里
	virtual void mf2();
	void mf3();
    void mf3(double); //这里
};

class Derived : public Base
{
public:
	virtual void mf1();
	void mf4();
    void mf3(); //这里
};
```

同样地，对于重载，遮掩规则依旧适用。对于一个子类的对象，它只能调用 **Derived 作用域内的 mf1 和 mf3，它不能调用 Base 作用域下的 mf1 和 mf3** 。

这种简单粗暴的遮掩法则会违背了 pubic 继承中的 is-a 关系（基类能做到的，派生类也一定要能够做到，而遮掩法则会忽视掉基类当中同名的函数或者变量）。想要解决这个问题，我们只需要在 *derived class* 当中加上 using 声明，即：

```cpp
class Derived : public Base
{
public:
    using Base::mf1;
    using Base::mf3;
	virtual void mf1();
	void mf4();
    void mf3();
};
```

那么这样子，我便可以在 Berived 作用域下找到 Base 当中的 mf1 和 mf2 ，其重载函数也能正常调用。

**这里的 using 是放在 public 作用域下的**，至于原因？还是派生类必须全部包含基类的所有成员

using 在这里会有一个问题是：**它会使得 *base class* 当中同名的变量与函数在 *derived class* 当中均可见**。如果我们只希望继承部分的同名函数，那么 using 便不能再使用了。

关于这个问题，我们可以用一个转接函数并**配合 private 继承**来解决。由于在 *derived class* 内部是可以通过 *base class* 的类名来访问其成员函数的，因此我只需要写一些函数（这些函数与我希望继承 *base class* 当中的函数重名），在这些函数内部调用 *base class* 当中对应的函数即可。

```cpp
class Base
{
private:
	int x = 10;
public:
	virtual void mf1() = 0;
	void mf1(int x)
	{
		cout << "fun1" << endl;
	}
};

class Derived : private Base //用 private 继承
{
public:
	virtual void mf1()
	{
		Base::mf1(10); //用基类类名来调用
	}
};
```

为什么不用 public 继承？

还是那个问题，这会违背 is-a 的原则，即派生类必须全部包含基类的所有成员。但我们这里是部分继承，显然不满足这一点。



# 11.区分接口继承和实现继承



public 继承，实际上分为函数接口继承和函数实现继承。

有的时候，我们希望 *derived class* 只**继承成员函数的接口（只继承该函数的声明，对于其定义则需要 *derived class* 自己实现）**；有时我们又希望 *derived class* 同时**继承函数接口和一份默认实现**；有时我们希望 *derived class* 同时**继承函数接口和一份强制实现**

我们来看一个例子：

```cpp
class Shape
{
public:
	virtual void draw() const = 0;
	virtual void error(string& str);
	int ID() const;
};

class Rectangle : public Shape
{

};

class Ellipse : public Shape
{

};
```



* 成员函数的接口总会被继承

函数的接口指的是该函数的声明。成员函数的接口被继承的意思是 ***derived class* 当中有一份函数声明，该声明来自 *base class* **。

* 声明一个 pure virtual 函数的目的是让 *derived class* 只继承函数接口

当 *base class* 存在 pure virtual 函数时， *derived class* 需要重写该 pure virtual 函数，即 ***derived class* 需要自己提供一份该函数的定义**。

上面的 draw 对应这种情况。

pure virtual 函数的声明式对其 *derived class* 的设计者说：**你必须提供一份该函数（draw）的实现，但我并不干涉你如何实现**。

一般来说， pure virtual 函数是不会有具体实现的，但**你如果写了也没关系**，但调用它们的唯一途径是要指明作用域

```cpp
Rectangle r;
r.Shape::draw(); //像这样
```

* 声明一个 impure virtual 函数的目的是让 *derived class* 继承该函数的接口和默认实现

**对于 impure virtual 函数，我们需要在 *base class* 中提供一份实现**

上面的 error 对应这种情况。

impure virtual 函数的声明式对其 *derived class* 的设计者说：**你必须提供一份该函数（error）的实现，如果你不想写，也可以使用 *base class* 的默认版本**。

* 声明一个 non-virtual 函数的目的是为了令 *derived class* 继承函数的接口和一份强制性实现

由于 *derived class* 并不能复写 non-virtual 函数，因此 *derived class* 会继承一份强制性实现



# 12.通过复合塑膜出 has-a 或“根据某物实现出”



复合是类型当中的一种关系，当某种类型的对象含有其它类型的对象时就属于这种关系

```cpp
class Address {  };
class PhoneNumber {  };
class Person
{
public:
	//...
private:
	string Name;
	Address add;
	PhoneNumber phone;
};
```

这里 Person 含有 string 、Address 、PhoneNumber 这些类型，因此 Person 跟前面这些类型属于复合关系

我们之前说过，public 继承带有 is-a（是一种）的意义。而复合则带有两种意义。复合意味着 has-a（有一个）或者 “根据某物实现出” 的意义。

程序当中的对象（用类实例化的，如自己定义的一些类）属于应用域，其余对象（组成前者的成分，如缓冲区、查找树）属于实现域。

**当复合发生在应用域的对象之间时，表现为 has-a（有一个）的关系，当复合发生在实现域的对象之间时，表现为 “根据某物实现出” 的关系**

我们看上面的例子。我们会说 Person has-a（有一个）Name，has-a（有一个） add，has-a（有一个）phone。我们不会说 Person is-a（是一种）Name。

也就是说，has-a 关系可以理解为组成这个类的成分，当然这个成分是其他类。

这一层是应用域，实现域会更偏向底层。我们通常会说：stack 的底层实现默认是 deque，你也可以把它改为 vector 或者 list 。这里我们说的是 “实现” ，也就是说这一层就属于实现域。

同样，这里是两个类的复合，因此属于 “根据某物实现出”



# 13.明智而审慎地使用 private 继承



我们设计一个类时，它的底层实现我们会把它设置成 private 属性。同样的道理，我们用 private 继承的时候，*base class* 当中的 public 与 protected 属性的成员会在 *derived class* 当中变成 private 。这正好符合作为另一种类的底层实现，即 private 继承是另一种的 "根据某物实现出"。

private 继承纯粹是一种实现技术，D private 继承 B ，意思是 D 对象根据 B 对象的实现而得，再没有其他含义了。

这里我们只讨论 private 继承的意义，其具体的应用场景参考原书 190 页



# 14. 在 operator= 中处理自我赋值



自我赋值很少会被刻意写出来，但它被写出来的时候通常很隐蔽，如

`a[i] = a[j]`

这种就是隐藏的自我赋值。我们来考虑一个继承体系当中的 operator= 的例子

```cpp
class Base { };
class Test : public Base
{
public:
	Test& operator=(const Test& rhs)
	{
		delete pb;//删除原先的指针，避免内存泄漏
		pb = new Base(*rhs.pb);
		return *this;
	}
private:
	Base* pb;
};
```



如果出现了自我赋值，即 `w = w` 这种情况。在 operator= 函数中，会先将 pb 给 delete 掉，那么这样就相当于是把 w 自己的 pb 给 delete 掉了。也就是说，执行完这个函数之后 w 当中的 pb 是未定义的（能不能执行完都不好说）

因此这么设计是有问题的，问题处在 rhs 与 *this 相同的时候依旧会执行该函数。

那么我们可以改为如下：

```cpp
Test& operator=(const Test& rhs)
{
    if(&rhs == this)
        return *this;
	delete pb;//删除原先的指针，避免内存泄漏
	pb = new Base(*rhs.pb);
	return *this;
}
```

在判断条件那块，**我们用指针来判断，不要用指针所指向的值来判断**。因为前者只需要判断指针的指向是否相同，而后者需要重载 == 运算符才行

当然，我们还有一种办法：

```cpp
Test& operator=(const Test& rhs)
{
    Base* tmp = pb;//记住原先 pb 所指向的位置
    pb = new Base(*rhs.pb);
    delete tmp;
    return *this;
}
```

 

# 15.复制对象时勿忘其每一个成分



如果我们希望当对象每执行一次 *copy* 构造或者 *copy assignment* 操作符时就进行一次记录，那么我们肯定不能用编译器给我们提供的函数，我们应当自己实现，即：

```cpp
void logCall(const string& FunName)
{
	cout << FunName << endl;
}

class base
{
public:
	base(const base& rhs);
	base& operator=(const base& rhs);
private:
	string name;
};

base::base(const base& rhs)
{
	logCall("base copy Constructor");// 每调用一次就调用 logCall
	name = rhs.name;
}

base& base::operator=(const base& rhs)
{
	logCall("base copy assignment operator");
	name = rhs.name;
}
```

用自己声明的函数来替代编译器生成的函数可能会导致的一个问题是：当我的代码必然出错时编译器并不会提醒我

上面这一切看似非常美好，但如果我们新添加一个变量，即：

```cpp
class Date
{
private:
	int data;
};

class base
{
public:
	base(const base& rhs);
	base& operator=(const base& rhs);
private:
	string name;
	Date date;// 这里
};

base::base(const base& rhs)
{
	logCall("base copy Constructor");
	name = rhs.name;
}

base& base::operator=(const base& rhs)
{
	logCall("base copy assignment operator");
	name = rhs.name;
}
```

这个时候，我们并没有改变 *copy* 构造函数和 *copy assignment* 操作符函数的定义。此时就会发生**局部赋值**，即 `name` 会被初始化而 `date` 并不会被初始化。当然，**编译器并不会报错**。

也就是说，**当我增加一个成员变量时，我需要修改所有的构造函数！！！**

如果这一切发生在继承，那么将会更要命。我们考虑下面的代码（**这里我们以初代的 base 为例**）：

```cpp
class Derived : public base
{
public:
	Derived();
	Derived(const Derived& rhs);
	Derived& operator=(const Derived& rhs);
private:
	int ths;
};

Derived::Derived() //default
	:ths(0)
{

}

Derived::Derived(const Derived& rhs)
	: ths(rhs.ths)
{

}

Derived& Derived::operator=(const Derived& rhs)
{
	this->ths = rhs.ths;
	return *this;
}
```

看上去， *derived class* 的 *copy* 构造函数和 *copy assignment* 操作符对 *derived class* 进行了拷贝。但是，**这仅仅只对属于 *derived class* 的成员变量进行了拷贝，对属于 *base class* 的成员变量并不会进行拷贝**。

 由于 *derived class* 的 *copy* 构造函数和 *copy assignment* 操作符函数在初始化时并没有提到 *base class* 所对应的函数，因此在调用 *derived class* 的函数时，**会默认调用 *base class* 的 *default* 构造函数**。这也正是问题的所在。

 因此，为避免这个问题。当我们调用 *derived class* 的构造函数时，需要将 *base class* 的部分也带上，即：

```cpp
Derived::Derived(const Derived& rhs)
	: ths(rhs.ths), base(rhs) //直接把 rhs 传进去就行
{

}

Derived& Derived::operator=(const Derived& rhs)
{
	this->ths = rhs.ths;
	base::base(rhs);
	return *this;
}
```

**这里直接把 *derived class* 对象作为参数传给 *base clas*s 的 *copy* 构造函数或者 *copy assignment* 操作符函数即可**。



# 16.以对象管理资源



我们来考虑一个类：

```cpp
class JK
{
public:
	JK()
	{
		cout << "Default" << endl;
	}
	~JK()
	{
		cout << "~Constructor" << endl;
	}
	void Pirnt()
	{
		cout << "233" << endl;
	}
};
```

通常，我们会用一个 *`Factory Function`* 来获取一个对象，即：

```cpp
JK* createJK() // Factury Function 
{
	JK* ptr = new JK();
	return ptr;
}
```

假设我们在某个函数当中需要用到该对象，我们可以这么写：

```cpp
void fun()
{
	JK* ptr1 = createJK();
	//...
	delete ptr1;
}
```

**我们总会在 return 前就将该指针析构掉**，但问题是，当代码进入日常维护时， "..." 部分的代码可能会有所更改，比如新添加一个 `return` 什么的。不管加了什么，如果导致**「当控制流离开这个函数或者区块时资源没有被释放」**，就会出现危险。

也就是说，在这里我们需要一种**「能够在程序执行完函数或代码块就自动析构掉资源」**的工具。十分幸运的是，C++提供了智能指针这个工具能够实现这一点。

```cpp
void fun()
{
	auto_ptr<JK>ptr1(createJK()); //这玩意是一个类模板，需要通过初始化来调用其内部函数
	//...
}
```

这里体现了「以对象管理资源」的两个特点：

* 获得资源（`new` 结束后）后立即放入管理对象（`auto_ptr`）
* 该管理对象运用析构函数使得该对象得以被释放

这里有一个问题是：`auto_ptr` 会在程序结束时自动调用析构函数来销毁掉对象，但如果多个 `auto_ptr` 指向同一个对象的话，你的对象就会被删除多次。「未定义行为」的列车将一去不复返。

为了解决这个问题， `auto_ptr` 不允许两个指针指向同一块内存地址，即**试图通过 *copy* 构造函数或 *copy assignment* 操作符复制它们，它们将会变为 NULL ，而复制所得的指针将用于该资源的唯一所有权** 。

```cpp
auto_ptr<JK> ptr1(createJK());
auto_ptr<JK> ptr2(ptr1);// ptr2 指向对象, ptr1 为 NULL
ptr1 = ptr2;// ptr1 指向对象, ptr2 为 NULL
```

因为这种特性，这会导致**受 auto_ptr 所指的资源不会有一个以上的 auto_ptr 同时指向它**。因为此特性，STL 当中的容器容不得 `auto_ptr` 。

`auto_ptr` 的替代方案是「引用计数型智能指针（RCSP）」。RCSP 指针可以持续跟踪共有多少个对象同时指向该资源，并在无人指向该资源时自动删除。但 RCSP 无法打破环状引用，即两个已经没有被使用的对象互指。

TR1 当中的 `tr1::shared_ptr` 就是一个 RCSP ，即上面的 fun 也可以这么写：

```cpp
void fun()
{
	tr1::shared_ptr<JK>ptr1(createJK());
}
```

该指针允许相互赋值，即：

```cpp
tr1::shared_ptr<JK>ptr1(createJK());

tr1::shared_ptr<JK> ptr2(ptr1);//ptr1 与 ptr2 均指向同一对象
```

显然，`tr1::shared_ptr` 的这种特性可以被用于 STL 或者任何「auto_ptr 的非传统赋值不适用」的情况下。

`auto_ptr` 和 `tr1::shared_ptr` **两者在其析构函数内执行的是 `delete` 动作而不是 `delete []` 的动作**，**这意味着在二者身上分配 `array` 是一种不合适的行为**。

看到这里你会想，为什么没有针对 C++ 动态数组而设计的智能指针呢？

啊，实际上 vector 和 string 可以完美替代掉动态数组这么个东西，因此 C++ 没有！！！（这个理由是我见过最牛逼的）



# 17.在资源管理类中小心 *copy* 行为



我们说，获取资源后立刻放入管理对象，这句话的意思实际是：当我用 `new` 取得了一块空间后，需要用一个指针来指向这块空间的首地址。在这里面，前者是**获取资源**，后者是**将资源放入管理对象**，管理对象就是这个指针。

在 C++ 当中引入了对象的概念后，我们将「以对象管理资源」称为「资源取得时便是初始化时机（ *Resource Acquisition Is Initialization* ; RAII ）」。我们在获取资源时就给 RAII 对象赋值，在控制流离开函数段或代码段时 RAII 对象自动将该资源销毁掉。这一切看似非常美好，直到 *copy* 行为的出现。

试想，如果我对 RAII 对象采取 *copy* 行为，即会有两个一样的对象来管理同一个资源。那么在析构的时候就会将该资源销毁掉两次，这里就出事了！！！

因此，关于这个问题的解决，我们提出以下几个办法：

* 禁止复制

就像我们之前所讨论的一样，用 private 继承一个 Uncopyable 的类来禁止调用 *copy* 构造函数或 *copy assignment* 操作符

* 对底层资源采取「引用计数法」

这里其实我也不太懂，参考 P68。一个例子是 `tr1::shared_ptr` 。

* 复制底部资源

**采取深拷贝的方式**，使得两个 RAII 对象所指向的资源**是相同的但不是同一个**。

* 转移资源所有权

这里参考 `auto_ptr` 的做法。当 *copy* 发生时，原指针会指向 NULL ，后面的指针会指向该资源。



# 18.在资源管理类中提供对原始资源的访问



假设我们有一个 *factory* 函数用以提供 `Test` 类型的资源，在获取资源后我们立刻将其放入到智能指针当中，即：

```cpp
tr1::shared_ptr<Test>ptr(createTest());
```

现有一个函数，其声明为 `void fun(Test*)` 。它接受一个 `Test*` 的指针，但我们并不能直接将 `ptr` 传给它，因为 `ptr` 的类型为 `tr1::shared_ptr<Test>` 。

实际上，在智能指针当中提供了对原始资源的访问方法。在 `tr1::shared_ptr` 和 `auto_ptr` 当中，都提供了 `get` 方法来用于显式转换，即该函数会返回**智能指针内部的原始指针（的复件）**。

对于我们自己写的资源管理的类，同样可以用这种方法。

上面的这是显示转换，还有隐式转换的方法

直接写**转换函数**即可，通过转换函数可以隐式转换

（我觉得）这里最好加上 `explicit` ，将隐式转换变为显式强制类型转换



# 19.成对使用 new 和 delete 时要采取相同形式



当使用 `new` 时（通过 `new` 动态生成一个或多个对象），会有两件事情发生。第一，内存会被分配出来（**通过名为 `operator new` 的函数**）。第二针对此内存**会有一个或多个构造函数被调用**。

当使用 `delete` 时，也会有两件事情发生。第一，针对此内存**会有一个或多个析构函数被调用**。第二，内存会被回收（**通过名为 `operator delete` 的函数**）。

当我用 `new []` 来生成一个数组时，其底层实现可能会是：数组前面留有一个空间用来存储对象的数量，之后才是对象数组。因此在使用 `new` 时，必须用 `delete` ；在使用 `new []` 时，必须使用 `delete[]` 。

实际上，由于有 `string` 和 `vector` 的存在，用 `new` 来生成数组的需求可以几乎降为零。



# 20.以独立语句将 new 对象置入智能指针



我们来考虑一个问题：

```cpp
int Priority();//这是函数声明，我们暂且不管定义
void fun(str::tr1::shared_ptr<Test> ptr, int priority)//同样是函数声明，第一个参数为智能指针，第二个为一个整型
```

还记得我们前面所讨论的吗？以对象来管理资源，当对象一生成就把它交给智能指针。因此我们很容易**在函数调用时**会写出以下代码：

```cpp
fun(new Test, Priority());//前面用 new 来生成一个对象，后面是调用另一个函数
```

但庆新的是，上面的代码并不会通过编译。原因在于 `tr1::shared_ptr` 的构造函数是一个 `explict` 的，它无法将一个 `Test*` 类型的对象转换成 `tr1::shared_ptr<Test>` 类型的对象。因此编译器会报错（你应该感到高兴）。

经过思考，你不难会**在函数调用阶段**写出：

```cpp
fun(tr1::shared_ptr<Test>(new Test), Priority());
```

（相信我，聪明的你一定会这么想）但这么做是有问题的。我们来仔细分析一下：

编译器在调用函数前，会核算即将被传递的各个实参。这里面第二个没什么好说的，就是一个函数的调用，得到返回值就行。

重点是第一个。`tr1::shared_ptr<Test>(new Test)` 由两部分组成：

* 调用 `new Test` 来生成 `Test` 对象
* 调用 `tr1::shared_ptr` 的**构造函数**

于是，在调用函数 `fun` 之前，编译器会先做三件事：

1. 调用 `Priority` 函数
2. 调用 `new` 来生成对象
3. 调用 `tr1::Shared_ptr<Test>` 的构造函数

我们能确定的是，调用 `new` 一定先于调用 `tr1::shared_ptr` 的构造函数。因为前者的结果要作为后者的参数。

**但是，对于调用 `Priority` 的次序则没有规定**。这个函数的调用，**可以在第一个，可以是第二个，可以是第三个**，这一切取决于编译器。

如果，`Priority` 是在第二个调用的，也就是说**这时 `Test` 对象已经生成好了**。如果在 `Priority` 内部抛出异常导致程序终止，**那么已经生成的对象就会导致内存泄漏**。这是因为**资源被创建**与**资源被放进管理对象**这两件事并不是在同一时间内完成的。

要解决这个问题也很简单，只需要在**单独语句内对智能指针进行赋值即可**，即：

```cpp
tr1::shared_ptr<Test>ptr(new Test);
fun(ptr, Priority());
```





# 21.宁以 Pass-by-reference-to-const 替换掉 Pass-by-value



对于自定义类型而言，*pass by value* 的成本往往高于 *pass by reference* ，具体我们看下面这个例子：
```cpp
class Person
{
public:
	Person();
	Person(const string& rhs, const string& chs);
	~Person();
private:
	string name;
	string adress;
};
class Student : public Person
{
public:
	Student();
	Student(const string& rhs, const string& chs);
	~Student();
private:
	string schoolname;
	string schooladress;
};
```

假设我们有一个函数，其声明为 `void fun(Student s)` ，当我写出：

```cpp
Student stu;
fun(stu);
```

该函数的形参是 `s` ，当该函数被调用时，**`Student` 的 *copy* 构造函数会被调用，它会以 `stu` 为蓝本复制一份给形参 `s` **，相应地，该函数结束时 `s` 会被销毁。因此总的调用成本是：一次 `Student` 的 *copy* 构造函数调用和一次 `Student` 的析构函数调用。

嘛从宏观上看是这样，但我们要知道，`Student` 是继承自 `Person` ，并且 *derived class* 当中有两个 `string` 对象，*base class* 当中也有两个 `string` 对象。

所以，总的调用应该是：一对 `Student` 的 *copy* 构造函数调用和析构函数调用，一对 `Person` 的 *copy* 构造函数调用和析构函数调用以及四个 `string` 对象的 *copy* 构造函数调用和析构函数调用。总共调用了 6 个构造函数和析构函数。

当然，我们也有办法能够回避掉这些函数的调用，那就是用 `pass by reference` 。当然，你会选择 `pass by value` 的一个很重要的原因可能是：你不想该函数改变我原本对象的值。当然，这一点也可以得到解决，就是 `pass by reference to const` ，即：

```cpp
void fun(const Student& s);
```

这样便不会有那么多次的构造函数调用和析构函数调用。

除此之外，`pass by reference to const` 也可以很好地避免**对象切割**的问题。还是上面那个例子，但我们加两行代码：

```cpp
class Person
{
public:
	Person();
	Person(const string& rhs, const string& chs);
	virtual ~Person();//bass class 一定要写 virtual
    
    vertual dosomething();//虚函数，用于提供接口
private:
	string name;
	string adress;
};
class Student : public Person
{
public:
	Student();
	Student(const string& rhs, const string& chs);
	~Student();
    
    dosomething();
private:
	string schoolname;
	string schooladress;
};
```

假定我们有一个函数，它**接受一个 *bass class* 对象为参数（并且是 `pass by value`）**，但我传入一个 *derived class* 对象，即：

```cpp
void fun(Person p);
Student stu;
fun(stu);
```

当一个 ***derived class* 对象**以**值传递**的方式传递给一个**接受 *bass class* 对象**的函数时，***base class* 的 *copy* 构造函数会被调用**（还记得吗，*base class* 形参也愿意接受一个 *derived class* 的实参），这将会使得**「 *derived class*  当中那些区别于 *base class* 的那些特质」**全部被切割掉。

我们具体说明一下：

`fun` 函数的形参是一个 `Person` 类型，当传入 *derived class* 对象时，会调用 *base class* 的 *copy* 构造函数，最终得到的是一个 ***base class* 对象**而不是 *derived class* 对象。也就是说，当我试图在此函数中调用 *derived class* 当中虚函数的接口时，**实际调用的是 *base class* 当中该函数的定义而不是 *derived class* 当中该函数的定义**。这才是关键！！！

也就是说，这里的 *base class* 是不能提供 pure virtual 函数的，因为这个函数在 *base class* 当中并没有定义，就算你给一份缺省定义也没有，因为 pure virtual 函数不能实例化对象。

所以，必须把 `fun(Person p)` 改为 `fun(const Person& p)` 才行，**这样传进来的是什么类型，那么 `p` 就表示什么类型**。



# 22.将成员变量声明为 private



从封装的角度看类，它只有两种属性：封装与不封装

封装对应的是 privited 权限下的成员，不封装对应的是 public 权限下的成员

这里想要理解很简单，对于 privited 权限下的成员，类外无法直接访问。对于 public 权限下的成员，类外可以直接访问。

那么对于 protected 权限呢？

这玩意虽然类外不能访问，这里看似和 privited 一样，实际上并不是的

protected 权限下的成员**对于以 public 继承的形式， *base class* 当中 protected 成员在 *derived class* 当前全部变为 public 成员**。因此我们也认为：**protected 权限没有封装性**



# 23.若所有参数皆需类型转换，请为此采用 non-member 函数



我们知道，友元函数会破坏封装性（毕竟非成员函数可以直接对类当中的 privited 权限的成员进行访问）

一般来说，友元函数用的时候多半是需要用全局函数重载运算符的时候

这一条款的建议是，**最好是通过公共的接口对类当中的 privited 成员进行访问而不要通过友元函数进行访问**。



# 24.尽可能延后变量定义式的出现时间



当我定义了一个变量而其类型带有构造和析构函数。那么程序的控制流在到达它与离开代码块时都会**产生构造与析构的成本**。就算这个变量不会被使用，这些成本也会产生。

你可能会说：我定义了一个变量就一定会使用啊，怎么可能会不用呢？

我们来考虑一个场景，我在函数的开头定义了一个 `string` 变量，然后**在使用该函数前抛出了异常**，这导致控制流离开该函数，这便造成了「没有使用该变量但产生了构造与析构成本」。

因此我们需要「尽可能延后该定义出现的时间」。这里的意思**不是**直到使用该变量的前一刻才给与定义，而是**尝试延后这份定义直到避免出现无意义的 *default* 构造函数行为**。

```cpp
string str;
str = "233";//这种并不合适，应该是下面

string str("233");//这一可以避免出现默认构造函数的成本
```

这个问题到这里我们就已经是完美解决了，还有一个问题是：如果出现循环怎么办？

考虑下面两个例子：

```cpp
string s1;
for(int i = 0; i < n; i++)
{
    s1 = function(i);//取决于 i 的值
}

for(int i = 0; i < n; i++)
{
    string s2(function(i));
}
```

这里的话我们稍微分析一下就行：

对于第一个，它有 1 个构造函数、 1 个析构函数、 n 次赋值运算

对于第二个，它有 n 个构造函数、 n 个析构函数

在这里，我们选择第一种做法的仅有这两个理由：

* 赋值成本低于构造加析构的成本
* 在循环外定义的变量不会对后面的代码造成冲突



# 25.尽量少做转型动作



关于 C++ 新式转型，前面其实已经提过了，在这里我们复习一下：

* `const_cast<T> ( expression )`
  * 通常被用作将对象的常量性移除，即将 const 变量改为 non-const 变量
* `dynamic_cast<T> ( expression )`
  * 用来决定某对象是否**归属**继承体系中的某个**类型**。这个东西我也不太懂，只知道最好不要用，运行太慢了
* `reinterpret_cast<T> ( expression )`
  * 实际结果取决于编译器，不可移植
* `static_cast<T> ( expression )`
  * 强迫隐式转换，可以将 non-const 转为 const ，或者将 int 转 double 。它也可以用来执行上述转换的反向转换。当然 const 到 non-const 只能 `const_cast` 才能做到

关于转型，我们来看一个非常有意思的例子

```cpp
class TP
{
  explicit TP(int x);  
};

void fun(const TP& t);

fun(TP(10));//这没有任何问题，属于函数风格的转型
fun(static_cast<YP>(15));//这也没有问题，属于C++风格的转型
```

两种做法都没错，我觉得很有意思，就把它们写了下来。还有一个有意思的：

很多人会相信，转型其实什么也没做，只不过是让编译器以另一个视角看待同一块内存空间当中的内容。啊啊啊，实际上不是这样的。

任何一个类型转换，不管是显示的还是隐式的，都会真的令编译器产生相关的代码。

我们来简单的考虑一个例子：我们说，基类指针可以指向派生类对象，派生类指针也可以指向派生类对象，那么这两个指针的地址一样吗？

显然不可能啊，派生类的内容是多余基类的。在构造函数阶段是基类的部分先被构造，因此基类的部分应当处于**栈的下面**，因此派生类指针的地址应该是高于基类指针的。也就是说二者是存在一个**偏移量**的。

因此在运行期间，会有这个偏移量会施加在派生类的指针身上用以取得正确的基类指针的值。

从这一个角度也可以说明，**对于单一对象而言，可能会有一个以上的地址**（刚刚不就是嘛，基类和派生类的指针都可以指向该对象，而这两个指针的地址并不相同）。

这个有意思的故事说完了，我们回归正题，来讨论一个转型所带来的问题：

```cpp
class Base
{
public:
    virtual void Resize()//虚函数，下面有缺省实现
    {
        //...
    }

};

class Derived : public Base
{
public:
    void Resize()//派生类需要重写基类的虚函数
    {
        //处于某种需要，我们要在第一行就调用 基类 中该函数的实现
        //然后，我们便想通过转型实现

        (static_cast<Base>(*this)).Resize();
    }
};
```

在这里我们试图将 *this 转型为 Base ，然后以此调用基类当中的 Resize 函数。

然而实际上并不是这样的。它**调用的并不是当前对象身上的函数**，而是转型动作所建立的一个「 *this 对象的 base class 成分」的暂时副本身上的 Resize 函数。

意思就是：转型所得到的 Base 类型的对象是原对象当中 Base 部分的**副本**而已。这很容易出事，因为如果 Resize 这个函数修改了某些成员变量，**并不会导致**当前对象身上的成员被改变，改变的**仅仅是副本**。

然后，这部分之后的 dynamic_cast 转型我看不懂。嘛，反正不要用就是了。话说这一条款的标题就是少用转型，这不是废话嘛.........



# 26.避免返回引用、指针来指向对象的内部成分



```cpp
class tp
{
public:

	tp()
		:rh(0), ch(0), close(false)
	{  }

	int& re_rh() const { return rh; }//这两个部分都加了引用
	int& re_ch() const { return ch; }

private:
	int rh, ch;
}; 
```

我们上面有说过，传引用的速度快过传值的速度，但并不能什么情况都传引用，特别是在返回值的地方

当我们调用这两个函数的时候，由于其返回值是一个**指向对象内部数据的引用**，因此我可以直接**通过这个引用来修改对象内部的数据**，这会导致**原本 private 权限下的变量变为 public 权限下的变量**，破坏了封装性

好，你也许看到之后会说，不就是会修改嘛，我加个 const 就行

```cpp
const int& re_rh() const { return rh; }
```

但这样子其实也会造成问题

假定我们有一个匿名对象，并用一个指针来进行接收

```cpp
int* a = &tp().re_ch();
```

然后你会发现，这个匿名对象在执行完这一行之后就会被自动销毁，那么这就会导致这个指针**指向的是一块未定义的内存**

因此为了避免出现这两种问题，最好的办法就是**不要在返回值的地方写上引用和指针**



# 27.绝不从新定义继承而来的 non-virtual 函数



前面我们说过，基类定义不同的函数就意味着不同的意思：

* 当基类定义一个 pure virtual 函数时，意味着派生类**只继承该函数的接口（声明）**（为pure virtual 函数添加缺省实现并没有什么意义）
* 当基类定义一个 virtual 函数时，意味着派生类**继承该函数接口和一份默认实现**
* 当基类定义一个 non-virtual 函数时，意味着派生类**继承该函数接口和一份强制实现**

基于这三点，我们需要在此说明的是：不用在派生类当中重写基类的 non-virtual 函数，这除了会发生名称遮掩这个问题外，还会有更严重的事情发生。我们考虑下面这个例子：

```cpp
class Base
{
public:
	void fun()
	{
		cout << "Base" << endl;
	}
};

class Derived : public Base
{
public:
	void fun()//派生类从新定义了基类的函数
	{
		cout << "Derived" << endl;
	}
};
```

然后，我们直到一个派生类对象可以由两个指针指向它（这两个指针的地址不同，但 C++ 允许这样），即：

```cpp
	Derived x;
	Base* pt = &x;
	Derived* pr = &x;

//当我们调用函数时，这么做会发生意外
	pt->fun();//Base::fun() 的调用
	pr->fun();//Derived::fun() 的调用
```

也就是说，当函数 `fun` 被调用时，任何一个对象（这里指的是 x ）可能即表现出基类的特性，也表现出派生类的特性。说人话就是，一个对象可以同时表现出基类和派生类的行为（我通过一个指针来调用对象当中的某个成员方法，但实际的结果会因为我指针的类型不同而不同）。这里的原因不在于对象自身，而在于**指向这个对象的指针的类型**。

这里的原因是： **non-virtual 函数都是静态绑定的，也就是说如果一个指针被声明为 pointer-to-Base ，那么通过该指针所调用的 non-virtual 函数就永远是在 Base 中定义的版本，就算该指针指向一个派生类对象也没用**。



# 28.了解隐式接口和编译期多态



对于一个对象，如果我们可以通过查看头文件就知道该对象的接口（如果该对象为变量，则接口为类型，而函数则为声明），那么这种接口称为**显示接口**

而**隐式接口则是那种无法在编译阶段就能确定的接口**（典型的就是动态分配的对象和 virtual 函数）

多态分为：**编译期多态**和**运行期多态**。我们说函数重载就是一种多态，再细一点，**函数重载是编译期多态**。因为在编译阶段就能够知道程序要调用哪个函数。

而运行期多态则在编译阶段无法确定需要调用哪个函数，具体调用得到运行阶段才能确定。典型例子是 virtual 函数和函数模板。

接下来，我们详细来讨论一下函数模板的隐式接口问题。我们有一个函数模板：

```cpp
template<typename T>
void fun(T w)
{
    if(w.size() > 10 && w != otherValue)
        ...
}
```

对于 w 的隐式接口 T ，它看起来需要有以下的约束：

* 它必须提供一个 size 函数接口并且该函数返回一个整数
* 它必须提供一个 `operator!=` 用来比较两个 T 对象（我们假设 otherValue 的类型为 T ）

实际上，这两个条件都不需要满足（函数模板真牛逼）。确实，T 必须支持 size 函数，但该函数可以从基类**继承而来**。再进一步，这个 size 函数可以**不需要返回一个 `int` 类型**，它可以返回一个类型 x ，只要 x 重载了 `operator>` 使得类型 x 可以与 `int` 相比较即可。

想到这里你格局还是小了，这里牛逼的地方在于：

size 函数**只需要返回一个类型为 x 的对象**，这个 x 可以是任何类型，只要类型 x 加上 `int` 能够调用 `operator>` 即可（即 x 不需要重载 `operator>` ）。

更进一步， `operator>` 所接受的参数可以不是 x ，可以是另外一个类型 y ，**只要存在一个构造函数能够将类型 x 隐式转换成类型 y 就行**。

不得不感叹，这东西真牛啊。想出这玩意的是个天才！！！！！

同样， T 并不需要提供 `operator!=` 的接口，`operator!=` 这个函数只需要接受一个类型为 x 的对象和一个类型为 y 的对象，只要 T 可以被隐式转换成 x 而 otherValue 可以被隐式转换成 y 就行。

因此，这种隐式接口的表达式仅仅**由一组有效表达式构成**。



# 29.了解 typename 的双重意义



提一个问题：

```cpp
template<typename T> class tp{};
template<class T> class tp{};
```

这当中的 `typename` 和 `class` 有何不同？

答案是，没有任何不同。是的，没有任何不同。在这种情况下二者完全一样。

然而，C++ 并不总将 `typename` 与 `class` 视为等价，具体看下面这个例子：

```cpp
template<typename C>
void fun(const C& container)
{
    C::const_iterator iter = container.begin();
    int value = *iter;
}
```

我们的想法是：定义一个迭代器 `iter` 并将它赋值为传进来的容器的起始位置，将此迭代器的值赋值给一个 `int` 类型的变量

这个函数当中，存在两个 `local` 变量：`iter` 和 `value` 。

`iter` 的类型为 `C::iterator` ，实际的类型必须由模板参数 C 来进行确定。因此我们说：**模板内如果出现的名称相依于某个模板参数的话，该名称称之为从属名称**。**如果从属名称在 class 内程嵌套状，则称为嵌套从属名称**（这里的类型 `C::iterator` 属于一个类当中的嵌套类型，而用此类型定义的名称称为从属名称，合起来就是嵌套从属名称）。

另外一个 `local` 变量 value 的类型是 `int` ，**不依赖于模板参数类型**，因此它是**非从属名称**。

嵌套从属名称会导致编译器解析困难，我们假定在函数中有如下代码：

```cpp
template<typename C>
void fun(const C& container)
{
    C::const_iterator* iter;
}
```

编译器在这里会有两种解释：

1. 认为 `C::const_iterator` 是一个嵌套从属类型，那么这里就是在声明一个指针。
2. 认为 `C::const_iterator` 是一个**类当中定义的 `static` 类型的变量**（这么想是合法的，因为 `static` 类型的变量可以直接通过类名来访问）而 `iter` 可以是一个**全局变量**，那么这里编译器会认为这是在将两个变量**做乘法**。

这里便出现了歧义，原因在于：在不能确定模板参数是什么的情况下，没有任何办法知道 `C::const_iterator` 是否是一个类型。

当然，C++ 有一个规则来解析这个问题：如果解析器在模板中遭遇一个嵌套从属名称，那么它将假设该名称不是类型，除非你告诉它。即**在缺省状态下，嵌套从属名称不是类型**。

因此在默认状态下，`C::iterator iter` **不是合法的代码**。**如果我们想告诉编译器嵌套从属名称是一个类型，只需要在前面加上 `typename` 即可**。

我们来看下面这个例子：模板函数接受一个容器和一个指向容器的迭代器

```cpp
template<typename C>//模板声明
void fun(const C& container,              //不允许加 typename
        typename C::const_iterator& iter);//必须要加 typename
```

这个例子说明了：只要在需要确定一个嵌套从属名称为类型的时候才允许使用 `typename` 。即 **`typename` 必须作为嵌套从属名称的前缀词**。

当然，「`typename` 必须作为嵌套从属名称的前缀词」是有例外的，这一例外规则是**「`typename` 不可以出现在 base class list 内的嵌套从属名称前面，也不允许出现在初始化列表的嵌套从属名称前面」**，即：

```cpp
template<typename T>
class Derived : public Base<T>::Nested //这里属于 base class list ，这里的嵌套从属名称前面不允许加 typename
{
public:
    explicit Derived(int x)
        : Base<T>::Nested(x) //构造函数的初始化列表当中的嵌套类型前也不允许加上 typename
        {
            typename Base<T>::Nested x;//嵌套从属名称前面必须要加上 typename
        }
    
}
```

其实这里也很容易理解，毕竟在 base class list （基类列表）和初始化列表当中的嵌套类型不可能会是一个类当中的静态成员变量。

我们最后再看一个 `typename` 例子，这将会发生在真实程序当中。

假设我们需要写一个模板函数，该函数接受一个迭代器，而我们打算为该迭代器指涉的对象做一份 `local` 复件 temp （也就是将这个指针所指向的对象做一份拷贝）。我们可以这么写：

```cpp
template<typename iterT>
void fun(iterT iter)
{
	typename std::iterator_traits<iterT>::value_type temp(*iter);
}
```

别被 `std::iterator_traits<iterT>::value_type` 给吓到了，它只不过是标准 traits class 的运用而已，相当于在说：「类型为 `iterT` 的对象所指之物的」类型。

也就是说，这里会将 temp 的类型声明为**「`iterT` 类型的迭代器所指向对象的类型」**（是指针指向对象的类型），之后再对 temp 进行赋值，就可以得到此迭代器指涉对象的一份拷贝（如果 `iterT` 的类型为 `vector<int>::iterator` ，那么 temp 的类型就是 `int` ；如果 `iterT` 的类型为 `stack<string>::iterator` ，那么 temp 的类型为 `string`）。

当然，这玩意是个嵌套从属类型，需要在前面加上 `typename` 。

当然，每次打那么长也会觉得很不舒服，因此我们可以使用 `typedef` 来简化：

```cpp
template<typename iterT>
void fun(iterT iter)
{
	typedef typename std::iterator_traits<iterT>::value_type value_type;
	value_type temp(*iter);
}
```

这么做也是合法的，**不过要记得加上 `typename`** 。



# 30.绝不从新定义继承而来的缺省参数值



这里我们只把理解说一下，具体的例子去看书

我们先来讨论静态类型和动态类型

对象的静态类型就是它在程序中**被声明时所采用的类型**，而动态类型则是「目前对象所指的类型」（这里默认对象是指针了）

我们在基类当中写一个 virtual 函数，然后这个函数存在一个默认值。如果派生类重写这个函数的时候修改了这个默认值，就会出现一些问题

我们通常会用基类指针来指涉派生类对象，由于 virtual 函数的默认参数是静态绑定的，而 virtual 函数则是动态绑定的，因此我在用基类指针调用派生类对象的 virtual 函数时，派生类的 virtual 函数的默认参数将会变为基类的默认参数。关于这一点我们是不希望看到的。

因此，我们最好**不要在派生类当中从新定义 virtual 函数的默认参数**。如果确实需要，我们可以将 virtual 函数放在 private 权限下，然后对外提供一个 public 的 non-virtual 函数接口。通过这个 non-virtual 函数调用 virtual 函数，并在 non-virtual 函数中写上默认参数，这样所有的派生类就都不会从新定义 virtual 函数的默认参数了。



# 31.学习处理模板化基类的名称



本例主要讨论当基类为类模板时候访问基类函数可能会出现的问题。具体看下面的例子：

```cpp
template<class T>
class Base
{
	void func()const
	{
		cout << "Chisato" << endl;
	}
};

template<class Ty>
class Derived : public Base<Ty>
{
	void fun()//与基类函数名称不一样，避免发生遮掩
	{
		func();
	}
};
```

这里我们有一个基类模板，并通过派生类来基础它，我们希望在派生类当中调用基类函数 `func` ，但这么做编译器会报错。

这里的报错原因是编译器不认识 `func` 这个标识符？？？你会很纳闷，什么玩意儿，这不是继承吗为什么会有这种情况？？？

模板这玩意在搞的时候总是会出奇奇怪怪的问题，习以为常了（

我们先看基类，对于模板参数 `T` ，我们可能会因为这个类无法满足某些需求而对一些特定的类型进行**全特化**的处理，在这个全特化的过程中，我们可能会删掉类当中的一些函数，也可能会添加。

明确这一点之后，我们回到派生类，由于无法确定模板参数 `Ty` ，因此我们也**无法确定基类是否会对一些特定的类进行全特化的处理**。C++ 知道这一点，并且它也知道如果进行了全特化，那个全特化的版本也有可能会不提供基类函数 `fun` ，因此 C++ 会直接在这里报错。

要解决这个问题有三种方法

第一个是在函数调用前加上 `this` 指针，即：

```cpp
template<class T>
class Derived : public Base<T>
{
	void fun()//与基类函数名称不一样，避免发生遮掩
	{
		this->func();
	}
};
```

第二种是通过 `using` 声明来让该名称在派生类当中可见，即：

```cpp
template<class T>
class Derived : public Base<T>
{
	using Base<T>::func;
	void fun()//与基类函数名称不一样，避免发生遮掩
	{
		func();
	}
};
```

第三种是直接用基类的名称来访问，不过如果该函数是 virtual 函数的话就会关闭其 virtual 绑定行为

```cpp
template<class T>
class Derived : public Base<T>
{
	void fun()//与基类函数名称不一样，避免发生遮掩
	{
		Base<T>::func();
	}
};
```



