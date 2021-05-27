# BlackDex

**[English Version](README_EN.md)**

![](https://img.shields.io/badge/language-java-brightgreen.svg)

BlackDex是一个运行在Android手机上的脱壳工具，支持5.0～12，无需依赖任何环境任何手机都可以使用，包括模拟器。只需几秒，即可对已安装包括未安装的APK进行脱壳。

## 项目声明
由于 [黑盒BlackBox](https://github.com/nnjun/BlackBox) 已被抄家，请关注本项目或者其他未来项目吧。

## 脱壳说明
本项目针对一代壳，摆脱对以往脱壳环境的困扰，几乎支持5.0以上的任何系统。并且拥有 **快速**、**方便**、**成功率高** 的优点。一般只需要几秒钟即可完成对已安装包括未安装应用脱壳。**未安装应用**脱壳时间主要花费在复制文件IO消耗上，由应用大小决定速度。已安装应用一般在数秒内即可完成脱壳。

**不要问如何修复**</br>
**不要问如何修复**</br>
**不要问如何修复**</br>

## 脱壳原理
通过DexFile cookie进行脱壳，理论兼容art开始的所有版本。可能少数因设备而异，绝大部分是支持的。资源有限无法大量测试，遇到问题请提issues.

## 环境要求
- 一台普通手机
- ~~Xposed~~
- ~~Frida~~
- ~~Magisk~~
- ~~Root~~
- ~~定制系统~~

## 架构特别说明
本项目区分32位与64位，目前是2个不同的app，如在Demo已安装列表内无法找到需要开启的app说明不支持，请使用另一个版本。

BlackDex下载：https://github.com/CodingGay/BlackDex/releases

## 演示
![xx](demonstration.gif)

## 展望未来？
- 对抗抽取壳

## 感谢
- [VirtualApp](https://github.com/asLody/VirtualApp)  (小声bb：虽然被干了但是还是感谢)
- [VirtualAPK](https://github.com/didi/VirtualAPK)
- [FreeReflection](https://github.com/tiann/FreeReflection)
- [Dreamland](https://github.com/canyie/Dreamland)
### License

> ```
> Copyright 2021 Milk
>
> Licensed under the Apache License, Version 2.0 (the "License");
> you may not use this file except in compliance with the License.
> You may obtain a copy of the License at
>
>    http://www.apache.org/licenses/LICENSE-2.0
>
> Unless required by applicable law or agreed to in writing, software
> distributed under the License is distributed on an "AS IS" BASIS,
> WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
> See the License for the specific language governing permissions and
> limitations under the License.
> ```
