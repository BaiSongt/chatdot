# ChatDot

ChatDot 是一个基于 Qt 框架开发的跨平台聊天应用程序，支持多种 AI 模型对话功能。

## 功能特点

- 🌐 支持多种 AI 模型后端
  - OpenAI API
  - DeepSeek API
  - Ollama 本地部署
  - 本地模型加载
- 🎨 现代化界面设计
- 🔄 支持多轮对话
- 🌍 多语言国际化支持
- 📸 图片处理功能
- 📝 完整的日志系统

## 系统架构

项目采用 MVVM(Model-View-ViewModel) 架构模式，实现了界面和业务逻辑的分离。

### 核心组件

```
src/
├── views/              # 视图层
│   ├── mainwindow      # 主窗口
│   └── settingsdialog  # 设置对话框
├── viewmodels/         # 视图模型层
│   ├── chatviewmodel   # 聊天视图模型
│   └── settingsviewmodel# 设置视图模型
├── models/             # 模型层
│   ├── chatmodel      # 聊天数据模型
│   ├── settingsmodel  # 设置数据模型
│   └── imagemodel     # 图片数据模型
└── services/          # 服务层
    ├── llmservice     # LLM服务基类
    ├── apiservice     # API服务
    ├── ollamaservice  # Ollama服务
    ├── localmodelservice# 本地模型服务
    └── logger         # 日志服务
```

### 技术栈

- 开发框架：Qt 6
- 编程语言：C++17
- 构建工具：CMake
- 依赖管理：vcpkg
- 单元测试：Google Test
- 代码风格：ClangFormat

## 开发环境配置

### 系统要求

- CMake 3.15+
- Qt 6.2+
- C++17 兼容的编译器
- vcpkg 包管理器

### 构建步骤

```bash
# 克隆仓库
git clone https://github.com/yourusername/chatdot.git
cd chatdot

# 创建构建目录
mkdir build
cd build

# 配置项目
cmake .. -G "MinGW Makefiles"

# 编译
cmake --build .
```

## 使用说明

1. 启动程序
2. 在设置中配置 AI 模型参数
3. 开始对话

## 项目规划

- [ ] 支持更多 AI 模型
- [ ] 优化对话上下文管理
- [ ] 添加语音交互功能
- [ ] 支持插件系统
- [ ] 优化性能和内存使用

## 贡献指南

欢迎提交 Pull Request 和 Issue！

1. Fork 本仓库
2. 创建特性分支
3. 提交变更
4. 推送到分支
5. 创建 Pull Request

## 许可证

本项目采用 MIT 许可证 - 查看 LICENSE 文件了解详情

## 联系方式

- 项目主页：[GitHub](https://github.com/BaiSongt/chatdot)
- 问题反馈：[Issues](https://github.com/BaiSongt/chatdot/issues)

---

*README 最后更新时间: 2025-05-29*
