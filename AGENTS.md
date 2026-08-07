# xv6-labs-2022 实验工作流

---

## 一、总则（提取自 AGENTS.md）

1. **任务**：带用户完成 xv6-2022 系列实验（Lab1–Lab10）。
2. **html 优先**：所有实验指导必须先阅读对应 lab 目录下的 **html 文件**（例如
   `Lab_ Multithreading.html`）；如果在该 lab 目录下**没有找到** html 文件，必须
   先向用户报告，不得继续。
3. **依据来源**：所有回答和建议都要先基于该 html 指导文件和对应 lab 的源代码
   （`xv6_for_LabN/kernel/`、`xv6_for_LabN/user/`）。
4. **语言规则**：
   - 用户向我**提问**时，回答一律使用**中文**；
   - 需要我**撰写文档**（README、报告等）时，一律使用**英文**；
   - 代码文件中的注释一律使用**英文**。

---

## 二、标准工作流（每个 lab 依次执行）

每个 lab 按照以下 5 个阶段依次推进，**不要跳步、不要提前进入下一阶段**。

### 阶段 0：准备与查找 html

- 确认用户要开始的 lab 编号 N，定位 `LabN_*/` 目录。
- 在该目录下查找官方指导文件（html）：
  - 顶层可能是 `Lab_ xxx.html`，也可能在 `Lab_ xxx_files/` 子目录；用
    `list_files` 递归查找。
- **找到 html**：完整读取全文，重点提取：
  - 实验背景（The problem / The solution）；
  - 任务要求（`div.required`）与完成标准（用户程序/测试名）；
  - 官方 hints；
  - 官方 plan of attack（若给出）；
  - 需要回答的 question（若有，提醒写入 `answers-*.txt`）；
  - grading 与 提交要求（`time.txt`、`make grade`、`make handin`）；
  - Optional challenge exercises。
- **没有找到 html**：立即向用户报告（例如"LabN 目录下未找到 html 指导文件"），
  等待用户指示后再继续。

### 阶段 1：解读代码

- 阅读 html 中明确提到或与任务相关的源代码：
  - kernel 侧：`vm.c`、`kalloc.c`、`proc.c`、`trap.c`、`riscv.h`、`defs.h`、
    `memlayout.h`、`param.h` 等；
  - user 侧：测试程序（如 `cowtest.c`、`alarmtest.c`）、`user.h`、`usys.pl`、
    `Makefile` 的 `UPROGS`。
- 分析"现状 vs 目标"的差距（例如默认 `fork()` 全量复制导致内存不足）。
- 用**中文**向用户讲解：
  1. 实验背景与任务目标；
  2. 关键机制/原理（配合表格、ASCII 图）；
  3. 涉及的关键代码位置。

### 阶段 2：计划方案

- 基于 html 的官方方案（plan of attack），给出**详细实施计划**。
- 计划格式建议：
  - 修改文件总览表（文件 | 改动内容）；
  - 每个文件给出**完整代码**与**设计理由**（重点部分必须解释"为什么这样设计"，
    例如：为什么父子 PTE_W 都要清、为什么先复制后释放、为什么用某个位/结构）；
  - 引用指导文件或权威资料作为依据（如 RISC-V 手册、xv6 book、hints）。
- 确认用户认可方案后，再进入实施阶段（若当前处于 PLAN MODE，提醒用户切换到
  Act 模式）。

### 阶段 3：撰写代码

- 按计划**逐文件、逐处**修改：
  - **同一文件的多处修改必须分多次进行**（一次 `replace_in_file`/`write_to_file`
    只做一处编辑），避免出错；
  - 每次编辑后基于返回的最新文件内容继续下一次编辑；
  - 代码注释一律**英文**；
  - 保留原有代码风格（xv6 的 K&R 缩进、`void\nfunctionname()` 写法等）。
- 全部修改完成后：
  1. 编译（`make qemu` 或 `make clean && make qemu`），确认无 `-Wall -Werror`
     错误；
  2. 运行官方测试：`make grade`（或 `./grade-lab-xxx`）；
  3. 若 html 要求创建 `time.txt`（单个整数，实验耗时）等文件，一并创建；
  4. 测试失败则迭代修复，直到通过。

### 阶段 4：更新 README

- **先阅读**该 lab 的 html 指导文件和当前 `README.md`，再按
  [第三节 README 风格规范](#三readme-风格规范用户期望)重写/更新
  `LabN_*/README.md`。
- **更新根目录 `README.md`** 的进度表格：
  - 当前 lab 行：状态改为 ✅；
  - 工作图标 👨🏻💻 移到下一个 lab 行；
  - 其余未开始 lab 保持 😴。
- 若需要统一历史文档风格（如前面 Lab 的 README 与最新风格不一致），可依次
  阅读各 lab 的 html 并统一更新，但需用户明确要求。

---

## 三、README 风格规范（用户期望）

> 以下为 Lab1–Lab5 README 沉淀出的统一风格。所有 Lab README 必须**全英文**。

### 1. 标题

```markdown
# Lab N: <官方实验名>
```

例如 `# Lab 5: Copy-on-Write Fork for xv6`、`# Lab 1: Xv6 and Unix utilities`。

### 2. Overview

```markdown
## Overview

2–4 句实验简介（要完成什么、关键思想是什么）。

For details, hints, and grading criteria, please refer to the official MIT lab page:
https://pdos.csail.mit.edu/6.828/2022/labs/<lab>.html
```

可选小节（视 lab 需要添加，顺序固定）：

```markdown
### Boot xv6            # 仅 Lab1 需要：make qemu、常见操作
### Recommended reading before coding   # html 的 prereq：xv6 book 章节 + 相关文件列表
### Reference links     # xv6 book / RISC-V 手册 / 相关文章
```

若该 lab 有需要先讲清的核心机制（如 Lab4 的 trap 路径、Lab5 的 RSW 位与
store page fault），可在 Overview 后加：

```markdown
### Key mechanisms used
- 原理讲解（配合表格、简洁 ASCII 图）
- 关键概念，例如 scause 取值表、PTE 标志位表
```

### 3. Exercises

每个练习一个 `###`，标题为 `### <编号>. <名称> (<难度>)`，难度用小写英文
（easy / moderate / hard）。

练习内部按以下小节组织（按需选用，顺序一致）：

```markdown
**UNIX interfaces used**

| Interface | Kind | Description |
| --- | --- | --- |
| `xxx()` | system call / library / kernel helper | 一句话说明用途 |

**How the pieces fit together**

ASCII 图展示 user space ↔ kernel 的数据/调用流程（可选）。

**Approach** 或 **Implementation steps**

1. 按步骤描述实现；涉及文件时用相对链接：
   [`kernel/vm.c`](./xv6_for_LabN/kernel/vm.c)
2. 关键代码片段用代码块。

**Key points**   （或针对易错点加粗说明，重点部分给出设计理由）

- 为什么这样设计
- 易错点

**Expected output**（若 html 给出官方输出示例，必须包含）

```
$ <程序名>
...
```

若有官方 question，用引用块：

> Which other xv6 system call(s) could be made faster using this shared page?

并给出解答。

最后给出实现文件链接：

```markdown
Solution: [`user/xxx.c`](./xv6_for_LabN/user/xxx.c)
```

### 4. Testing

格式示例（注意不要嵌套代码块；这里的 `sh` 标记仅为说明）：

```markdown
## Testing

In the `xv6_for_LabN` directory:

    make grade          # run all grading tests
```

- 若 lab 有 written answers，说明其位置（`answers-*.txt`）。
- 提醒 `time.txt`（单个整数，实验耗时）需 `git add` / `git commit`。
- 如已实测，可附官方评分输出与分数（如 `Score: 110/110`）。

### 5. Optional challenge exercises

```markdown
## Optional challenge exercises

From the lab description (not graded):

- 第 1 个挑战（难度）
- 第 2 个挑战（难度）
- ...
```

### 格式约定

- **全英文**撰写；术语可用 `code` 格式。
- 文件引用一律用相对路径：`./xv6_for_LabN/kernel/xxx.c`。
- 使用 Markdown 表格、代码块、ASCII 图，风格与现有 Lab1–Lab5 README 保持一致。
- 若一段内容会大幅改变文件结构（如 Lab1 重写、Lab3/Lab4 补章节），可用
  `write_to_file`；小改动用多次 `replace_in_file`（一次一处）。

---

## 四、注意事项清单

- [ ] 动手前先读 html（找不到则报告用户）。
- [ ] 回答用中文；README/文档用英文；代码注释用英文。
- [ ] 代码修改一次一处，不批量编辑同一文件。
- [ ] 有 question 的 lab，提醒用户把答案写入 `answers-*.txt`。
- [ ] 完成代码后运行 `make grade` 并报告分值。
- [ ] 创建/更新 `time.txt`（html 要求）。
- [ ] 重写本 lab README.md（按第三节风格）。
- [ ] 更新根 README.md 进度表格（✅ 与 👨🏻💻 移动）。