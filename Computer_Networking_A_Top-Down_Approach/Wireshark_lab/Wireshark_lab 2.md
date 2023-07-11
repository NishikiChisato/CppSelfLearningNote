# Wireshark_lab 2

- [Wireshark\_lab 2](#wireshark_lab-2)
  - [Prepare](#prepare)
  - [Process](#process)
    - [1. The Basic HTTP GET/response interaction](#1-the-basic-http-getresponse-interaction)
    - [2. The HTTP CONDITIONAL GET/response interaction](#2-the-http-conditional-getresponse-interaction)
    - [3. Retrieving Long Documents](#3-retrieving-long-documents)
    - [4. HTML Documents with Embedded Objects](#4-html-documents-with-embedded-objects)
    - [5 HTTP Authentication](#5-http-authentication)
  - [Conclusion](#conclusion)


## Prepare

实验指导书下载地址：[WIRESHARK LABS](https://gaia.cs.umass.edu/kurose_ross/wireshark.php)

文档翻译可以考虑用：[DeepL](https://www.deepl.com/translator)

`Wireshark` 下载：[WIRESHARK](https://www.wireshark.org/download.html)

## Process

### 1. The Basic HTTP GET/response interaction

> 对应文件 `HTTP1.pcapng`

我们在浏览器输入 [http://gaia.cs.umass.edu/wireshark-labs/HTTP-wireshark-file1.html](http://gaia.cs.umass.edu/wireshark-labs/HTTP-wireshark-file1.html)，利用 `Wireshark` 抓包，得到如下结果（请求报文）：

![Wireshark_lab2.1](../img/Wireshark_lab2.1.png)

可以看到，在我们浏览器发出 `GET` 请求后，经过 $0.3$ 秒得到了 `200 OK` 的响应

这个实验需要我们回答以下问题：

1. Is your browser running HTTP version 1.0 or 1.1? What version of HTTP is the server running?
2. What languages (if any) does your browser indicate that it can accept to the server?
3. What is the IP address of your computer? Of the gaia.cs.umass.edu server?
4. What is the status code returned from the server to your browser?
5. When was the HTML file that you are retrieving last modified at the server?
6. How many bytes of content are being returned to your browser?
7. By inspecting the raw data in the packet content window, do you see any headers within the data that are not displayed in the packet-listing window? If so, name one.

具体结果如下（响应报文）：

![Wireshark_lab2.2](../img/Wireshark_lab2.2.png)

因此：

1. 浏览器 `HTTP` 版本为 `1.1`，服务器也同理
2. `HTTP` 请求报文中 `Accept-Language` 字段的值为：`zh-CN`
3. 在 `Transmission Control Protocol, TCP` 字段中，`Src` 为 `172.29.251.75`，`Dst` 为 `128.119.245.12`
4. `HTTP` 响应报文为 `304 Not Modified`，表示浏览器可以用本地缓存中的数据
5. 上次修改时间为 `Last-Modified: Sun, 09 Jul 2023 05:59:02 GMT`
6. 响应报文中 `content` 部分的大小为 `128 Byte`
7. 我没找到。。。。。

### 2. The HTTP CONDITIONAL GET/response interaction

> 对应文件 `HTTP2.pcapng`

我们需要将浏览器缓存清空掉后，连续输入两次 [http://gaia.cs.umass.edu/wireshark-labs/HTTP-wireshark-file2.html](http://gaia.cs.umass.edu/wireshark-labs/HTTP-wireshark-file2.html)，得到如下结果：

> `Edge` 清除缓存的方法：
> 
> 打开 Microsoft Edge。
> 
>在浏览器的右上角，选择设置等 (Alt+F)：
>
>选择设置，然后单击隐私与服务。
>
>在清除浏览数据下，选中选择要清除的数据。
>
>选中缓存的图像和文件以及 Cookie 和其他站点数据，然后选择立即清除（可以不用清除 `Cookie`）。
>
>重启浏览器。

![Wireshark_lab2.3](../img/Wireshark_lab2.3.png)

该实验需要我们回答以下问题：

8. Inspect the contents of the first HTTP GET request from your browser to the server. Do you see an “IF-MODIFIED-SINCE” line in the HTTP GET?
9. Inspect the contents of the server response. Did the server explicitly return the contents of the file? How can you tell?
10. Now inspect the contents of the second HTTP GET request from your browser to the server. Do you see an “IF-MODIFIED-SINCE:” line in the HTTP GET? If so, what information follows the “IF-MODIFIED-SINCE:” header?
11. What is the HTTP status code and phrase returned from the server in response to this second HTTP GET? Did the server explicitly return the contents of the file? Explain.

第一次 `HTTP` 的请求报文结果如下：

![Wireshark_lab2.4](../img/Wireshark_lab2.4.png)

并没有看到 `IF-MODIFIED-SINCE` 字段

第一次返回的结果如下：

![Wireshark_lab2.5](../img/Wireshark_lab2.5.png)

`HTTP` 的返回内容包含在 `content` 中，返回的是明文，并不是文件

第二次请求报文结果如下：

![Wireshark_lab2.6](../img/Wireshark_lab2.6.png)

我们可以看到有 `IF_MODIFIED_SINCE` 字段，与第一次返回的 `Last-Modified` 字段中的值一致

第二次返回的状态为 `304 Not Modified`，并没有实际返回内容，`content` 部分为空

### 3. Retrieving Long Documents

> 对应文件 `HTTP3.pcapng`

我们将缓存清空后，在浏览器输入 [http://gaia.cs.umass.edu/wireshark-labs/HTTP-wireshark-file3.html](http://gaia.cs.umass.edu/wireshark-labs/HTTP-wireshark-file3.html)，得到如下结果：

![Wireshark_lab2.7](../img/Wireshark_lab2.7.png)

我们需要回答以下问题：

12. How many HTTP GET request messages did your browser send? Which packet number in the trace contains the GET message for the Bill or Rights?
13. Which packet number in the trace contains the status code and phrase associated 
with the response to the HTTP GET request?
14. What is the status code and phrase in the response?
15. How many data-containing TCP segments were needed to carry the single HTTP response and the text of the Bill of Rights?

我们浏览器只发送了一个 `HTTP` 请求报文，哪怕这是大文件，包含 `Bill or Rights` 的截图如下：

![Wireshark_lab2.8](../img/Wireshark_lab2.8.png)

对应的分组号为 `1525`

包含 `status code` 和 `pharse` 的分组号为 `1525`，分别是 `200 OK`

![Wireshark_lab2.9](../img/Wireshark_lab2.9.png)

一个发送了 `4` 个 `TCP segments`，**一个 `TCP segments` 最多只能携带 `1448 Bytes`**

### 4. HTML Documents with Embedded Objects

> 对应文件：`HTTP4.pcapng`

清空缓存后在浏览器输入 [http://gaia.cs.umass.edu/wireshark-labs/HTTP-wireshark-file4.html](http://gaia.cs.umass.edu/wireshark-labs/HTTP-wireshark-file4.html)

这是一个 `base HTML` 文件，该文件中包含两张图片，**两张图片以 `URL` 的形式嵌入 `base HTML` 文件中**，我们得到如下结果

![Wireshark_lab2.10](../img/Wireshark_lab2.10.png)

我们需要回答以下问题：

16. How many HTTP GET request messages did your browser send? To which Internet addresses were these GET requests sent?
17. Can you tell whether your browser downloaded the two images serially, or whether they were downloaded from the two web sites in parallel? Explain.

我们浏览器一共发送了三个 `HTTP` 请求，**对于每一个 `URL` 都需要发送一个 `HTTP` 请求**。结合上面的结果，我们得出：无论单次请求的文件有多大，只要该文件中没有其他的 `URL`，那么只会发送一次 `HTTP` 请求；如果有多个 `URL`，则会发送多个请求

它们之间有时间差，因此是连续下载的而不是并行下载的

### 5 HTTP Authentication

> 对应文件 `HTTP5.pcapng`

访问网站 [http://gaia.cs.umass.edu/wireshark-labs/protected_pages/HTTP-wireshark-file5.html](http://gaia.cs.umass.edu/wireshark-labs/protected_pages/HTTP-wireshark-file5.html)

这个网站需要登陆验证，用户名与密码分别是：`wireshark-students`、`network`。我们主要观察需要验证时 `HTTP` 报文的情况

报文情况如下：

![Wireshark_lab2.12](../img/Wireshark_lab2.12.png)

可以看到我们浏览器首先发送一个 `GET` 请求，随后收到 `401 Unauthorized` 的报文。之后我们浏览器再次发送 `GET` 请求，此次请求头带有 `Authorization` 字段，用于表示用户名与密码，之后再次收到带有内容的响应报文，完成请求

18.  What is the server’s response (status code and phrase) in response to the initial HTTP GET message from your browser?
19.  When your browser’s sends the HTTP GET message for the second time, what new field is included in the HTTP GET message?

第一次 `HTTP` 响应报文返回的是 `401 Unauthorized`。当第二次发送 `GET` 请求报文时，多了字段 `Authorization: Basic d2lyZXNoYXJrLXN0dWRlbnRzOm5ldHdvcms=`

这里字符串 `d2lyZXNoYXJrLXN0dWRlbnRzOm5ldHdvcms=` 包含了我们输入的用户名与密码的加密 `encode`，对应的算法为 `Base 64`，我们可以对其进行解密 `decode`，便可以得到真实的用户名与密码

## Conclusion

我们得到以下几点结论：

* 无论单个 `HTTP` 传输的文件有多大，只会有一次 `HTTP` 请求报文和一次 `HTTP` 响应报文，文件过大会有多个 `TCP segment`
* 如果一个 `base HTML` 中有包含其他对象的 `URL`，那么需要多个 `HTTP` 请求报文去依次请求所有的对象
* 如果网站需要验证登录，那么会有两次请求报文与两次响应报文。第一次响应报文会返回 `401 Unauthorized`，第二次请求报文会对输入的用户名与密码进行加密，第二次响应报文为正常的 `200 OK`