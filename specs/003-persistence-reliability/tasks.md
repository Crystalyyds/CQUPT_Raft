# 003-persistence-reliability Tasks

## Phase 1 History

- [ ] T001 阅读 storage / snapshot / node 相关 AGENTS.md
- [ ] T002 梳理 meta.bin 写入和恢复路径 `moved-to-Phase-3`
- [ ] T003 梳理 log segment append / truncate / recovery 路径
- [ ] T004 梳理 snapshot save / publish / load 路径
- [ ] T005 标记 fsync / directory fsync 缺口 `meta.bin / hard state portion moved-to-Phase-3`
- [ ] T006 标记 publish ordering 风险 `meta.bin / hard state portion moved-to-Phase-3`
- [ ] T007 编写 durability contract
- [ ] T008 规划后续测试场景
- [ ] T009 生成 Phase 1 报告

## Phase 2 Checklist

- [x] T201 明确 segment log append durability 边界
- [x] T202 明确 segment log truncate durability 边界
- [x] T203 明确 `log/` directory publish 边界
- [x] T204 规划 `WriteSegments` 受影响点
- [x] T205 规划 `ReplaceDirectory` 受影响点
- [x] T206 设计 segment log append / truncate / recovery 测试

## Phase 3 Checklist

- [ ] T301 承接 T002，明确 `meta.bin` 写入与恢复路径
- [ ] T302 承接 T005，明确 hard state file `fsync` / directory `fsync` 需求
- [ ] T303 承接 T006，明确 `meta.bin` publish ordering 风险
- [ ] T304 规划 `WriteMeta` 受影响点
- [ ] T305 规划 `ReplaceFile` 受影响点
- [ ] T306 规划 `PersistStateLocked` 与 hard state durable publish 边界
