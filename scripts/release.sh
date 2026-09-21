#!/bin/bash
# MeasureKit 一键发版：更新版本号 → 提交 → 打标签 → 推送
# 用法：./scripts/release.sh <版本号> [说明]
#   ./scripts/release.sh 0.1.1            # 修订版：修 bug
#   ./scripts/release.sh 0.2.0 "新增拍照测量"  # 次版本：新功能
#   ./scripts/release.sh 1.0.0            # 主版本：大版本
set -euo pipefail

cd "$(dirname "$0")/.."

VERSION="${1:-}"
NOTE="${2:-}"
if [[ ! "$VERSION" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
    echo "错误：版本号必须是 X.Y.Z 格式，如 0.1.1 / 0.2.0 / 1.0.0" >&2
    exit 1
fi

# 同步更新根 CMakeLists.txt 中的项目版本号
sed -i -E "s/^project\(measurekit VERSION [0-9]+\.[0-9]+\.[0-9]+/project(measurekit VERSION ${VERSION}/" CMakeLists.txt

git add CMakeLists.txt
if ! git diff --cached --quiet; then
    git commit -m "chore: 发布 v${VERSION}${NOTE:+（${NOTE}）}"
fi

git tag -a "v${VERSION}" -m "v${VERSION}${NOTE:+：${NOTE}}"
git push
git push origin "v${VERSION}"

echo "✅ v${VERSION} 发布完成：代码与标签已推送"
