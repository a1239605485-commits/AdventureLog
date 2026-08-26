# 冒险日志 AdventureLog

这是面向 **新 KernelLoader** 的 Android ARM64 模组工程。

## 已记录事件

- 普通怪与 Boss 击杀（NPC 类型 ID）
- 物品拾取（物品类型 ID、数量、稀有度；稀有度大于 0 会标记为 `RareItem`）
- 玩家死亡、复活
- 探索区域（按 1200×720 世界坐标划分的扇区）
- 游戏天数（每次由夜晚进入白天时递增）

日志文件为模组私有目录中的 `AdventureLog.txt`；同时也会出现在 TEFManager 运行日志里，标签为 `AdventureLog`。

## 生成手机安装包

把整个工程上传到 GitHub 后，打开 **Actions**，运行 `Build AdventureLog Android ARM64`。成功后下载产物 `AdventureLog-AndroidARM64-v1.0.0-test`，里面的
`冒险日志-AdventureLog-KernelLoader-AndroidARM64-v1.0.0-test.zip` 才是可由 TEFManager 导入的安装包。

源码包没有伪装成 `.so`：必须由该工作流使用 Android NDK r28 真实编译后再安装。
