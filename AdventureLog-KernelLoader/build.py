#!/usr/bin/env python3
"""
MOD 编译和打包脚本
编译所有 MOD 并打包成 ZIP 文件
"""

import os
import sys
import subprocess
import json
import shutil
import zipfile
from pathlib import Path
from typing import List, Dict, Set, Optional
import re
import glob

# MOD 列表
MODS = [
    "Accessorize",
    "InfiniteLegion",
    "InstantRespawn",
    "Overclock",
    "Seeker",
    # "ForgottenItem",
]

# 平台配置 - 根据 CMakeLists.txt 的命名规则
PLATFORMS = {
    "android": {
        "archs": ["arm", "arm64"],
        "system_name": "android",
        "suffix": ".so",
        "keep_lib_prefix": True,  # Android 保留 lib 前缀
    },
    "linux": {
        "archs": ["x86", "x64"],
        "system_name": "linux",
        "suffix": ".so",
        "keep_lib_prefix": True,  # Linux 保留 lib 前缀
    },
    "windows": {
        "archs": ["x86", "x64"],
        "system_name": "windows",
        "suffix": ".dll",
        "keep_lib_prefix": False,  # Windows 去掉 lib 前缀
    },
    "macos": {
        "archs": ["x64", "arm64"],
        "system_name": "darwin",
        "suffix": ".dylib",
        "keep_lib_prefix": True,  # macOS 保留 lib 前缀
    }
}

class ModPacker:
    def __init__(self, source_dir: Path):
        self.source_dir = source_dir
        self.build_dir = source_dir / "build"
        self.output_dir = self.build_dir / "mods"
        self.mods = MODS

    def ensure_build_dirs(self):
        """确保构建目录存在"""
        self.build_dir.mkdir(parents=True, exist_ok=True)

    def run_command(self, cmd: List[str], cwd: Path = None) -> bool:
        """运行命令并检查结果"""
        print(f"\n执行: {' '.join(cmd)}")
        try:
            result = subprocess.run(
                cmd,
                cwd=cwd or self.source_dir,
                check=False,
                capture_output=True,
                text=True
            )
            if result.returncode != 0:
                print(f"❌ 命令执行失败: {result.stderr}")
                return False
            if result.stdout:
                print(result.stdout)
            return True
        except Exception as e:
            print(f"❌ 执行命令时出错: {e}")
            return False

    def find_library(self, mod: str, platform: str, arch: str) -> Optional[Path]:
        """查找库文件"""
        system_name = PLATFORMS[platform]["system_name"]
        suffix = PLATFORMS[platform]["suffix"]

        # 构建可能的库文件名
        possible_names = [
            f"lib{mod}.{system_name}.{arch}{suffix}",
            f"{mod}.{system_name}.{arch}{suffix}",
            f"lib{mod}{suffix}",
            f"{mod}{suffix}"
        ]

        # 先在 build 目录下查找
        for name in possible_names:
            lib_path = self.build_dir / name
            if lib_path.exists():
                return lib_path

        # 在 build 的子目录中查找
        for subdir in self.build_dir.glob("*-release"):
            for name in possible_names:
                lib_path = subdir / name
                if lib_path.exists():
                    return lib_path
                # 也在 mod 子目录中查找
                mod_lib_path = subdir / mod / name
                if mod_lib_path.exists():
                    return mod_lib_path

        # 使用通配符查找
        try:
            for pattern in [
                f"*{mod}*{system_name}*{arch}{suffix}",
                f"*{mod}*{suffix}"
            ]:
                # 在 build 目录下查找
                matches = list(self.build_dir.glob(pattern))
                if matches:
                    return matches[0]

                # 在子目录中查找
                for subdir in self.build_dir.glob("*-release"):
                    matches = list(subdir.glob(pattern))
                    if matches:
                        return matches[0]
                    mod_dir = subdir / mod
                    if mod_dir.exists():
                        matches = list(mod_dir.glob(pattern))
                        if matches:
                            return matches[0]
        except:
            pass

        return None

    def get_zip_filename(self, lib_path: Path, platform: str) -> str:
        """获取在 ZIP 中的文件名"""
        filename = lib_path.name

        # Windows 平台去掉 lib 前缀
        if platform == "windows" and filename.startswith("lib"):
            filename = filename[3:]  # 去掉 "lib" 前缀

        return filename

    def collect_libraries(self, mod: str) -> Dict[str, dict]:
        """收集某个 MOD 的所有库文件"""
        libraries = {}

        print(f"\n查找 {mod} 的库文件:")
        for platform, config in PLATFORMS.items():
            for arch in config["archs"]:
                lib_path = self.find_library(mod, platform, arch)
                if lib_path and lib_path.exists():
                    # 获取在 ZIP 中的文件名
                    zip_filename = self.get_zip_filename(lib_path, platform)

                    libraries[f"{platform}-{arch}"] = {
                        "path": lib_path,
                        "platform": platform,
                        "arch": arch,
                        "zip_filename": zip_filename
                    }
                    print(f"  ✓ {platform}/{arch}: {lib_path}")
                    print(f"    -> {zip_filename}")
                else:
                    print(f"  ✗ {platform}/{arch}: 未找到")

        return libraries

    def create_mod_zip(self, mod: str) -> bool:
        """为单个 MOD 创建 ZIP 包"""
        print(f"\n{'='*60}")
        print(f"打包 MOD: {mod}")
        print(f"{'='*60}")

        mod_dir = self.source_dir / mod

        # 检查必需的 JSON 文件
        json_files = ["Info.json", "Manifest.json"]
        # 添加 mod 特定的 JSON 文件（如果存在）
        mod_json = f"{mod}.json"
        if (mod_dir / mod_json).exists():
            json_files.append(mod_json)

        missing_files = [f for f in json_files if not (mod_dir / f).exists()]

        if missing_files:
            print(f"❌ 缺少必需文件: {', '.join(missing_files)}")
            print(f"目录 {mod_dir} 中的文件:")
            for f in mod_dir.iterdir():
                print(f"  {f.name}")
            return False

        # 收集库文件
        libraries = self.collect_libraries(mod)
        if not libraries:
            print(f"❌ 未找到任何库文件")
            return False

        # 创建输出目录
        self.output_dir.mkdir(parents=True, exist_ok=True)

        # 创建 ZIP 文件
        zip_path = self.output_dir / f"{mod}.zip"

        with zipfile.ZipFile(zip_path, 'w', zipfile.ZIP_DEFLATED) as zipf:
            # 添加 JSON 文件到根目录
            print("\n添加 JSON 文件:")
            for json_file in json_files:
                src_path = mod_dir / json_file
                if src_path.exists():
                    arcname = json_file
                    zipf.write(src_path, arcname)
                    print(f"  ✓ {json_file} -> {arcname}")

            # 添加库文件到 Resources/lib/ 目录
            print("\n添加库文件到 Resources/lib/:")
            for platform_key, lib_info in libraries.items():
                lib_path = lib_info["path"]
                zip_filename = lib_info["zip_filename"]

                arcname = f"Resources/lib/{zip_filename}"
                zipf.write(lib_path, arcname)
                print(f"  ✓ {lib_path.name} -> {arcname}")

            # 添加 mod.png (如果存在)
            png_path = mod_dir / "mod.png"
            if png_path.exists():
                zipf.write(png_path, "mod.png")
                print(f"  ✓ mod.png -> mod.png")

        print(f"\n✅ 打包完成: {zip_path}")
        print(f"   文件大小: {zip_path.stat().st_size / 1024:.2f} KB")
        return True

    def pack_all_mods(self) -> bool:
        """打包所有 MOD"""
        print("=" * 60)
        print("开始打包 MOD...")
        print("=" * 60)

        success_count = 0
        fail_count = 0

        for mod in self.mods:
            if self.create_mod_zip(mod):
                success_count += 1
            else:
                fail_count += 1

        print("\n" + "=" * 60)
        print(f"打包完成: 成功 {success_count}, 失败 {fail_count}")
        print("=" * 60)

        return fail_count == 0

    def clean(self):
        """清理输出目录"""
        if self.output_dir.exists():
            print(f"清理: {self.output_dir}")
            shutil.rmtree(self.output_dir)
        print("✅ 清理完成")

    def list_libraries(self, mod: str = None):
        """列出所有库文件（调试用）"""
        print("=" * 60)
        print("库文件列表")
        print("=" * 60)

        mods_to_check = [mod] if mod else self.mods

        for mod_name in mods_to_check:
            print(f"\n{mod_name}:")
            for platform, config in PLATFORMS.items():
                for arch in config["archs"]:
                    lib_path = self.find_library(mod_name, platform, arch)
                    if lib_path and lib_path.exists():
                        zip_filename = self.get_zip_filename(lib_path, platform)
                        print(f"  ✓ {platform}/{arch}: {lib_path}")
                        print(f"    -> Resources/lib/{zip_filename}")
                    else:
                        print(f"  ✗ {platform}/{arch}: 未找到")

    def run(self, jobs: int = 4):
        """运行完整流程"""
        print("=" * 60)
        print("MOD 编译打包工具")
        print("=" * 60)
        print(f"源码目录: {self.source_dir}")
        print(f"构建目录: {self.build_dir}")
        print(f"输出目录: {self.output_dir}")
        print(f"并行任务数: {jobs}")
        print("=" * 60)

        # 打包
        if not self.pack_all_mods():
            print("❌ 打包失败")
            sys.exit(1)

        print("\n🎉 所有操作完成!")

def main():
    import argparse

    parser = argparse.ArgumentParser(description="MOD 编译打包工具")
    parser.add_argument(
        "-j", "--jobs",
        type=int,
        default=4,
        help="并行编译任务数 (默认: 4)"
    )
    parser.add_argument(
        "--clean",
        action="store_true",
        help="清理输出目录"
    )
    parser.add_argument(
        "--mod",
        type=str,
        help="只打包指定的 MOD (可选)"
    )
    parser.add_argument(
        "--list",
        action="store_true",
        help="列出所有库文件（不编译）"
    )
    parser.add_argument(
        "--dir",
        type=str,
        default=".",
        help="源码目录 (默认: 当前目录)"
    )

    args = parser.parse_args()

    source_dir = Path(args.dir).resolve()
    if not source_dir.exists():
        print(f"❌ 目录不存在: {source_dir}")
        sys.exit(1)

    packer = ModPacker(source_dir)

    if args.clean:
        packer.clean()
        return

    if args.list:
        packer.list_libraries(args.mod)
        return

    # 如果指定了 mod，只打包这一个
    if args.mod:
        packer.mods = [args.mod]

    packer.run(jobs=args.jobs)

if __name__ == "__main__":
    main()
