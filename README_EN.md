# BlackDex

![](https://img.shields.io/badge/language-java-brightgreen.svg)

BlackDex is an Android unpack tool, it supports Android 5.0~12 and need not rely to any environment. BlackDex can run on any Android mobile phones or emulators, you can unpack APK File in several seconds.

## Description

This project supports almost all the Android system above 5.0, it has **high successful rate** to dump DexFile **quickly** and **conveniently**. Generally, it only takes a few seconds to unpack an installed app. For the APK File that is not installed on your device, BlackDex will spends some time on copying file, the length of time depend on file size.

## Principle

BlackDex unpacks APK File by DexFile cookie. In theory, it supports all version Android systems that use ART (Android Runtime). But it may vary from a few special devices. We do not have enough resources to execute a lot of testing, if you have problems with this project, please open an issue on the GitHub.

## Environment

- An Android mobile phone or emulator
- ~~Xposed~~
- ~~Frida~~
- ~~Magisk~~
- ~~Root~~
- ~~A custom ROM~~

## Special Instructions

This project distinguishes between 32-bit and 64-bit, it was compiled 2 different demos. If you can not find your target in installed packages list, please use another version.

BlackDex download：https://github.com/CodingGay/BlackDex/releases

## Preview

![xx](demonstration.gif)

## Looking forward?

- Confront DexCode->insns extraction

## References

- [VirtualApp](https://github.com/asLody/VirtualApp)
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
> http://www.apache.org/licenses/LICENSE-2.0
>
> Unless required by applicable law or agreed to in writing, software
> distributed under the License is distributed on an "AS IS" BASIS,
> WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
> See the License for the specific language governing permissions and
> limitations under the License.
> ```