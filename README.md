xLua常用库集成
=====

本库在原作者的工程基础上，把编译的库换成了以下库

且暂不支持jit编译，因为sproto库目前在jit下编译会报错。

## sproto

https://github.com/cloudwu/sproto

## LuaSocket

xLua默认集成库。

## LPeg

模式匹配库。


## 如何使用

* 到[自动编译](https://github.com/chexiongsheng/build_xlua_with_libs/actions)下载你希望使用的版本（lua53,lua54, luajit），解压覆盖Unity工程Assets下Plugins目录，注意是覆盖，别删除后拷
* 把LibsTestProj/Assets下的BuildInInit.cs和Resources目录放到Unity工程Assets下
* 库的初始化看实例：LibsTestProj/Assets/Helloworld/Helloworld.cs

## 注意

* 这只是常用库的扩展，首先你得安装了xLua
* 这几个库的使用请查阅相关文档
* 由于时间的关系，这不一定能做到和xLua版本总是同步，如果xLua报“wrong lib version ...”信息的话，请把https://github.com/Tencent/xLua/tree/master/build下的除CMakeLists.txt之外的文件目录，覆盖到https://github.com/MartinLong/build_xlua_with_libs/tree/master/build目录。
* 由于C#代码拷贝就可用，所以Plugins没发生变动（Plugins版本更新频率很低，很可能几个月更新一次），这个工程不会同步C#最新代码。自行到https://github.com/Tencent/xLua更新最新代码即可，只要运行没报“wrong lib version ...”的话，表示是C#代码和Plugins是版本匹配的。
