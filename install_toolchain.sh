#!/bin/bash
# =============================================================================
# ARM GCC Toolchain 安装脚本 for STM32H7R3 工程
# 优先使用 apt 安装，失败时回退到手动下载
# 支持: Ubuntu/Debian
# =============================================================================

set -e  # 遇到错误立即退出

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 配置（手动下载用）
TOOLCHAIN_VERSION="13.3.rel1"
TOOLCHAIN_NAME="arm-gnu-toolchain-${TOOLCHAIN_VERSION}-x86_64-arm-none-eabi"
TOOLCHAIN_URL="https://developer.arm.com/-/media/Files/downloads/gnu/${TOOLCHAIN_VERSION}/binrel/${TOOLCHAIN_NAME}.tar.xz"
INSTALL_DIR="$HOME/opt"
TOOLCHAIN_DIR="${INSTALL_DIR}/${TOOLCHAIN_NAME}"

# 打印信息函数
info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# 检查命令是否存在
check_command() {
    command -v "$1" &> /dev/null
}

# 检查 ARM GCC 是否已安装
check_arm_gcc() {
    if check_command arm-none-eabi-gcc; then
        local version=$(arm-none-eabi-gcc --version | head -n1)
        success "ARM GCC 已安装: $version"
        return 0
    fi
    return 1
}

# 使用 apt 安装 ARM 工具链
install_with_apt() {
    info "尝试使用 apt 安装 ARM GCC 工具链..."

    if ! check_command apt; then
        warning "未找到 apt 命令，跳过 apt 安装方式"
        return 1
    fi

    if ! check_command sudo; then
        warning "未找到 sudo 命令，尝试直接执行 apt..."
        # 尝试直接执行（如果是 root 用户）
        if [ "$EUID" -eq 0 ]; then
            apt update
            apt install -y gcc-arm-none-eabi binutils-arm-none-eabi gdb-multiarch cmake build-essential
        else
            error "需要 sudo 权限来安装软件包"
            return 1
        fi
    else
        info "更新软件包列表..."
        sudo apt update

        info "安装 ARM GCC 工具链..."
        sudo apt install -y gcc-arm-none-eabi binutils-arm-none-eabi gdb-multiarch cmake build-essential
    fi

    if check_arm_gcc; then
        success "ARM GCC 工具链通过 apt 安装成功!"
        return 0
    else
        warning "apt 安装后仍未找到 ARM GCC，尝试手动下载..."
        return 1
    fi
}

# 手动下载并安装 ARM GCC 工具链（apt 失败时的回退方案）
install_manual() {
    info "开始手动下载 ARM GCC 工具链..."
    info "版本: ${TOOLCHAIN_VERSION}"
    info "下载地址: ${TOOLCHAIN_URL}"

    # 创建安装目录
    mkdir -p "${INSTALL_DIR}"
    cd "${INSTALL_DIR}"

    # 检查是否已存在
    if [ -d "${TOOLCHAIN_DIR}" ]; then
        warning "工具链目录已存在: ${TOOLCHAIN_DIR}"
        read -p "是否重新安装? (y/N): " confirm
        if [[ ! $confirm =~ ^[Yy]$ ]]; then
            info "使用已存在的工具链"
            # 设置环境变量（当前会话）
            export PATH="${TOOLCHAIN_DIR}/bin:$PATH"
            return 0
        fi
        rm -rf "${TOOLCHAIN_DIR}"
    fi

    # 下载工具链
    info "正在下载工具链，请稍候..."
    if check_command wget; then
        wget -c --show-progress "${TOOLCHAIN_URL}" -O "${TOOLCHAIN_NAME}.tar.xz" || {
            error "wget 下载失败，尝试使用 curl..."
            curl -L "${TOOLCHAIN_URL}" -o "${TOOLCHAIN_NAME}.tar.xz"
        }
    elif check_command curl; then
        curl -L "${TOOLCHAIN_URL}" -o "${TOOLCHAIN_NAME}.tar.xz"
    else
        error "需要 wget 或 curl 来下载工具链"
        exit 1
    fi

    # 解压
    info "正在解压工具链..."
    tar -xf "${TOOLCHAIN_NAME}.tar.xz"

    # 清理下载文件
    rm -f "${TOOLCHAIN_NAME}.tar.xz"

    # 设置环境变量（当前会话）
    export PATH="${TOOLCHAIN_DIR}/bin:$PATH"

    success "工具链安装完成: ${TOOLCHAIN_DIR}"

    # 添加到 shell 配置文件
    setup_manual_env
}

# 设置手动安装的环境变量
setup_manual_env() {
    local bin_path="${TOOLCHAIN_DIR}/bin"
    local shell_rc=""

    # 检测当前 shell
    if [ -n "$ZSH_VERSION" ]; then
        shell_rc="$HOME/.zshrc"
    elif [ -n "$BASH_VERSION" ]; then
        shell_rc="$HOME/.bashrc"
    else
        shell_rc="$HOME/.bashrc"
    fi

    # 检查是否已添加
    if grep -q "${bin_path}" "$shell_rc" 2>/dev/null; then
        warning "环境变量已在 ${shell_rc} 中设置"
    else
        info "添加环境变量到 ${shell_rc}"
        echo "" >> "$shell_rc"
        echo "# ARM GCC Toolchain for STM32" >> "$shell_rc"
        echo "export PATH=\"${bin_path}:\$PATH\"" >> "$shell_rc"
        success "环境变量已添加到 ${shell_rc}"
        info "重新打开终端或运行: source ${shell_rc}"
    fi
}

# 验证安装
verify_installation() {
    info "验证安装..."

    if check_arm_gcc; then
        success "ARM GCC 工具链可用!"
        info "工具链路径: $(which arm-none-eabi-gcc)"
        info "版本信息:"
        arm-none-eabi-gcc --version | head -n2
        return 0
    else
        error "ARM GCC 工具链验证失败"
        return 1
    fi
}

# 编译工程
build_project() {
    info "开始编译 STM32 工程..."

    local project_root="$(dirname "$0")"
    cd "${project_root}"
    project_root=$(pwd)  # 获取绝对路径

    local build_dir="${project_root}/build"

    # 创建构建目录
    mkdir -p "${build_dir}"
    cd "${build_dir}"

    info "CMake 配置..."
    cmake .. -DCMAKE_TOOLCHAIN_FILE="${project_root}/cmake/gcc-arm-none-eabi.cmake"

    info "开始编译..."
    make -j$(nproc) 2>&1

    if [ $? -eq 0 ]; then
        success "编译成功!"
        echo ""
        info "输出文件:"
        ls -lh "${build_dir}"/*.elf "${build_dir}"/*.bin 2>/dev/null || true
        ls -lh "${build_dir}"/*.hex 2>/dev/null || true

        # 显示文件大小
        if [ -f "${build_dir}/atk_h7r3.elf" ]; then
            echo ""
            info "ELF 文件信息:"
            arm-none-eabi-size "${build_dir}/atk_h7r3.elf"
        fi
    else
        error "编译失败!"
        return 1
    fi
}

# 清理构建
clean_build() {
    local project_root="$(dirname "$0")"
    cd "${project_root}"
    if [ -d "build" ]; then
        info "清理构建目录..."
        rm -rf build
        success "构建目录已清理"
    else
        warning "构建目录不存在"
    fi
}

# 清理下载文件
cleanup() {
    if [ -f "${INSTALL_DIR}/${TOOLCHAIN_NAME}.tar.xz" ]; then
        rm -f "${INSTALL_DIR}/${TOOLCHAIN_NAME}.tar.xz"
    fi
}

# 显示帮助信息
show_help() {
    cat << EOF
ARM GCC 工具链安装和编译脚本 (apt 优先)

用法: $0 [选项]

选项:
    -h, --help      显示此帮助信息
    -i, --install   仅安装工具链（不编译）
    -b, --build     仅编译工程（假设工具链已安装）
    -c, --clean     清理构建目录
    -v, --verify    验证工具链安装
    -m, --manual    强制使用手动下载方式（跳过 apt）

示例:
    $0              # 完整流程：优先使用 apt 安装工具链并编译
    $0 -i           # 仅安装工具链
    $0 -b           # 仅编译工程
    $0 -c           # 清理构建目录
    $0 -m           # 强制手动下载方式安装

EOF
}

# 主函数
main() {
    local MODE="full"
    local USE_MANUAL=false

    # 解析参数
    while [[ $# -gt 0 ]]; do
        case "$1" in
            -h|--help)
                show_help
                exit 0
                ;;
            -i|--install)
                MODE="install"
                ;;
            -b|--build)
                MODE="build"
                ;;
            -c|--clean)
                clean_build
                exit 0
                ;;
            -v|--verify)
                verify_installation
                exit $?
                ;;
            -m|--manual)
                USE_MANUAL=true
                ;;
            "")
                ;;
            *)
                error "未知选项: $1"
                show_help
                exit 1
                ;;
        esac
        shift
    done

    echo "=========================================="
    echo "  STM32H7R3 工程编译脚本"
    echo "=========================================="
    echo ""

    # 检查操作系统
    if [ -f /etc/os-release ]; then
        source /etc/os-release
        info "检测到操作系统: ${PRETTY_NAME:-$NAME}"
    fi

    # 根据模式执行
    case "$MODE" in
        install|full)
            # 检查是否已安装
            if ! check_arm_gcc; then
                if [ "$USE_MANUAL" = true ]; then
                    install_manual
                else
                    # 优先使用 apt
                    if ! install_with_apt; then
                        warning "apt 安装失败，切换到手动下载方式..."
                        install_manual
                    fi
                fi
                verify_installation || exit 1
            fi
            ;;
    esac

    # 编译工程
    if [ "$MODE" == "full" ] || [ "$MODE" == "build" ]; then
        build_project || exit 1
    fi

    echo ""
    echo "=========================================="
    success "全部完成!"
    echo "=========================================="

    # 如果手动安装，提示用户
    if [ "$MODE" == "full" ] && [ -d "$TOOLCHAIN_DIR" ] && [ "$USE_MANUAL" = true ]; then
        echo ""
        info "提示: 运行以下命令以在当前终端使用工具链:"
        echo "    export PATH=\"${TOOLCHAIN_DIR}/bin:\$PATH\""
        echo ""
        info "或者重新打开终端以使环境变量永久生效"
    fi
}

# 设置清理钩子
trap cleanup EXIT

# 运行主函数
main "$@"
