# Wireshark_lab 2

- [Wireshark\_lab 2](#wireshark_lab-2)
  - [准备](#准备)
  - [过程](#过程)
    - [1. The Basic HTTP GET/response interaction](#1-the-basic-http-getresponse-interaction)
    - [2. The HTTP CONDITIONAL GET/response interaction](#2-the-http-conditional-getresponse-interaction)


## 准备

实验指导书下载地址：[WIRESHARK LABS](https://gaia.cs.umass.edu/kurose_ross/wireshark.php)

文档翻译可以考虑用：[DeepL](https://www.deepl.com/translator)

`Wireshark` 下载：[WIRESHARK](https://www.wireshark.org/download.html)

## 过程

### 1. The Basic HTTP GET/response interaction

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